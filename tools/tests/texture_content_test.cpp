#include "gpu/texture_content.h"

#include <array>
#include <cstdio>
#include <stdexcept>
#include <vector>

using gpu::texture_cache::ContentHash;
using gpu::texture_cache::FirstFullScanFrame;
using gpu::texture_cache::GuestContentHash;
using gpu::texture_cache::kFullScanInterval;
using gpu::texture_cache::PhysicalRangeValid;
using gpu::texture_cache::SampledContentHash;
using gpu::texture_cache::SampledGuestContentHash;

static size_t checks = 0;
static void Check(bool condition, const char* message)
{
    ++checks;
    if (!condition) throw std::runtime_error(message);
}

static void TestShortTails()
{
    for (const size_t bytes : {size_t(0), size_t(1), size_t(2), size_t(3), size_t(7), size_t(8),
            size_t(9), size_t(15), size_t(16), size_t(17), size_t(127), size_t(128), size_t(129)})
    {
        std::vector<uint8_t> data(bytes);
        const auto original = ContentHash(data.data(), data.size());
        Check(original == ContentHash(data.data(), data.size()), "unchanged content retains hash");
        for (size_t i = 0; i < bytes; ++i)
        {
            data[i] = 1;
            Check(ContentHash(data.data(), data.size()) != original, "every short-buffer byte participates, including tail");
            data[i] = 0;
        }
    }
}

static void TestUnsampledChanges()
{
    for (const size_t bytes : {size_t(8191), size_t(8192), size_t(8193), size_t(65536), size_t(65539)})
    {
        std::vector<uint8_t> data(bytes);
        const auto original = ContentHash(data.data(), data.size());
        for (const size_t offset : {size_t(0), size_t(511), size_t(512), size_t(700), bytes / 2, bytes - 1})
        {
            data[offset] = 1;
            Check(ContentHash(data.data(), data.size()) != original, "detect head, sample gap, middle and tail changes");
            data[offset] = 0;
        }
    }

    // The old guestBytes cap must not hide a valid stored extent's large tail.
    std::vector<uint8_t> large((64u << 20) + 1);
    const auto original = ContentHash(large.data(), large.size());
    large.back() = 1;
    Check(ContentHash(large.data(), large.size()) != original, "hash stored bytes after the old 64 MiB cap");
}

static void TestMipChanges()
{
    std::array<uint8_t, 17> base{};
    std::vector<uint8_t> mips(65539);
    const auto original = GuestContentHash(base.data(), base.size(), mips.data(), mips.size());
    Check(GuestContentHash(base.data(), base.size(), nullptr, 0) == ContentHash(base.data(), base.size()),
        "base-only textures do not read a mip source");
    for (const size_t offset : {size_t(700), mips.size() - 1})
    {
        mips[offset] = 1;
        Check(GuestContentHash(base.data(), base.size(), mips.data(), mips.size()) != original,
            "mip gap and tail changes invalidate content");
        mips[offset] = 0;
    }
    base.back() = 1;
    Check(GuestContentHash(base.data(), base.size(), mips.data(), mips.size()) != original,
        "base tail change invalidates texture with mip chain");
}

static void TestSampledWindows()
{
    for (const size_t bytes : {size_t(1), size_t(7), size_t(17), size_t(8191), size_t(8192)})
    {
        std::vector<uint8_t> data(bytes);
        const auto original = SampledContentHash(data.data(), bytes);
        for (size_t i = 0; i < bytes; i += bytes > 64 ? 61 : 1)
        {
            data[i] = 1;
            Check(SampledContentHash(data.data(), bytes) != original, "small extents are sampled completely");
            data[i] = 0;
        }
        data.back() = 1;
        Check(SampledContentHash(data.data(), bytes) != original, "small extent tail byte is sampled");
    }
    std::vector<uint8_t> data(65539);
    const auto sampled = SampledContentHash(data.data(), data.size());
    const auto full = ContentHash(data.data(), data.size());
    for (const size_t offset : {size_t(0), size_t(511), size_t(512), data.size() - 512, data.size() - 1})
    {
        data[offset] = 1;
        Check(SampledContentHash(data.data(), data.size()) != sampled, "sampled hash covers head, windows and tail");
        data[offset] = 0;
    }
    // 700 lies between the first two 64-byte windows: only the full scan sees it.
    data[700] = 1;
    Check(SampledContentHash(data.data(), data.size()) == sampled, "gap write is outside the sampled windows");
    Check(ContentHash(data.data(), data.size()) != full, "full scan detects the gap write");
    Check(SampledGuestContentHash(data.data(), 17, data.data(), data.size()) ==
        SampledGuestContentHash(data.data(), 17, data.data(), data.size()), "sampled guest hash is stable");
}

static void TestFullScanSchedule()
{
    std::array<size_t, kFullScanInterval> phases{};
    for (uint32_t page = 0; page < 1024; ++page)
    {
        const uint32_t address = 0x10000000u + page * 0x1000u;
        const uint64_t first = FirstFullScanFrame(100, address);
        Check(first > 100 && first <= 100 + kFullScanInterval, "first full scan falls within one interval");
        ++phases[first - 101];
    }
    for (const size_t count : phases)
        Check(count >= 32, "textures uploaded together spread their full scans across the interval");
}

static void TestPhysicalExtents()
{
    Check(PhysicalRangeValid(0, 0x20000000ull), "complete physical mapping extent");
    Check(PhysicalRangeValid(0xA0001000u, 0x1FFFF000ull), "physical address alias and exact mapping end");
    Check(PhysicalRangeValid(0x1FFFFFFFu, 1), "last mapped byte");
    Check(!PhysicalRangeValid(0x1FFFFFFFu, 2), "reject extent past physical mapping");
    Check(!PhysicalRangeValid(0, 0x20000001ull), "reject oversized physical extent");
    Check(!PhysicalRangeValid(0, UINT64_MAX), "reject arithmetic overflow sized extent");
}

int main()
{
    try
    {
        TestShortTails();
        TestUnsampledChanges();
        TestMipChanges();
        TestSampledWindows();
        TestFullScanSchedule();
        TestPhysicalExtents();
        std::printf("texture content: %zu checks passed\n", checks);
        return 0;
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "texture content failure: %s\n", error.what());
        return 1;
    }
}

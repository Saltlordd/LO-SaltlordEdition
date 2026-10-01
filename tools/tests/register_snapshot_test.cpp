#include <gpu/register_snapshot.h>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {
struct BigEndianWord {
    std::array<uint8_t, 4> bytes{};
    mutable unsigned reads = 0;
    explicit operator uint32_t() const {
        ++reads;
        return uint32_t(bytes[0]) << 24 | uint32_t(bytes[1]) << 16 |
            uint32_t(bytes[2]) << 8 | bytes[3];
    }
    void Set(uint32_t value) {
        bytes = {uint8_t(value >> 24), uint8_t(value >> 16), uint8_t(value >> 8), uint8_t(value)};
    }
};
void Check(bool condition) { if (!condition) { std::fputs("register snapshot mismatch\n", stderr); std::exit(1); } }
}
int main() {
    std::vector<uint32_t> registers(0x5003);
    std::vector<BigEndianWord> mmio(registers.size());
    for (size_t i = 0; i < registers.size(); ++i) {
        registers[i] = i % 3 ? uint32_t(i * 2654435761u) : 0;
        mmio[i].Set(uint32_t(i * 2246822519u) ^ 0x80ff0100u);
    }
    unsigned checked = 0;
    // Full VS/PS banks, mixed zero/nonzero values, both ends and an invalid start.
    for (uint32_t first : {0u, 0x4000u, 0x4400u, 0x4fffu, 0x5003u, 0xffffffffu}) {
        for (size_t count : {size_t(0), size_t(1), size_t(1024), size_t(2048)}) {
            std::vector<uint32_t> output(count + 2, 0xdeadbeef);
            for (auto& word : mmio) word.reads = 0;
            gpu::CopyRegisterSnapshot(std::span<const uint32_t>(registers), first,
                std::span<uint32_t>(output).subspan(1, count), first < mmio.size() ? mmio.data() + first : nullptr);
            Check(output.front() == 0xdeadbeef && output.back() == 0xdeadbeef);
            for (size_t i = 0; i < count; ++i) {
                const uint64_t index = uint64_t(first) + i;
                uint32_t expected = 0;
                if (index < registers.size()) {
                    Check(mmio[index].reads == (registers[index] == 0 ? 1u : 0u));
                    expected = registers[index] ? registers[index] : uint32_t(mmio[index]);
                }
                Check(output[i + 1] == expected); ++checked;
            }
        }
    }
    // Direct MMIO stores must be observed afresh where the register bank is zero.
    std::array<uint32_t, 3> output{};
    for (uint32_t value : {0u, 0xffffffffu, 0x80000000u, 0x12345678u}) {
        registers[0] = 0; mmio[0].Set(value);
        registers[1] = value; mmio[1].Set(~value);
        gpu::CopyRegisterSnapshot(std::span<const uint32_t>(registers), 0, std::span<uint32_t>(output), mmio.data());
        Check(output[0] == value && output[1] == (value ? value : ~value)); ++checked;
    }
    // A bypass MMIO write leaves the command generation alone. The tracked
    // zero-register word still changes the snapshot and rejects the old GPU upload.
    registers[0] = 0; registers[1] = 0x12345678; registers[2] = 0;
    mmio[0].Set(0x01020304); mmio[1].Set(0xffffffff); mmio[2].Set(0);
    gpu::CopyRegisterSnapshot(std::span<const uint32_t>(registers), 0,
        std::span<uint32_t>(output), mmio.data());
    std::array<uint16_t, 3> offsets{};
    size_t fallbackCount = gpu::CollectRegisterFallbackOffsets(
        std::span<const uint32_t>(registers).first(3), std::span<uint16_t>(offsets));
    Check(fallbackCount == 2 && offsets[0] == 0 && offsets[1] == 2);
    uint64_t version = 1;
    const uint64_t uploadedVersion = version, uploadedOffset = 64;
    Check(gpu::CanReuseUploadedConstants(uploadedOffset, uploadedVersion, version));
    mmio[0].Set(0x55667788); mmio[1].Set(0xaaaaaaaa);
    Check(gpu::RefreshRegisterSnapshotFallbacks(
        std::span<const uint16_t>(offsets).first(fallbackCount),
        std::span<uint32_t>(output), mmio.data(), version));
    Check(output[0] == 0x55667788 && output[1] == registers[1] && version == 2);
    Check(!gpu::CanReuseUploadedConstants(uploadedOffset, uploadedVersion, version));
    Check(!gpu::RefreshRegisterSnapshotFallbacks(
        std::span<const uint16_t>(offsets).first(fallbackCount),
        std::span<uint32_t>(output), mmio.data(), version) && version == 2);
    ++checked;
    std::printf("register snapshot: %u values, MMIO fallback and upload reuse checks passed\n", checked);
}

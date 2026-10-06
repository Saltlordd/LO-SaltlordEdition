#include "gpu/bc_decode.h"

#include <array>
#include <cstdio>
#include <stdexcept>
#include <vector>

using namespace gpu::bc_decode;

static size_t checks = 0;
static void Check(bool condition, const char* message)
{
    ++checks;
    if (!condition) throw std::runtime_error(message);
}

using Rgba = std::array<uint8_t, 4>;
static Rgba At(const std::vector<uint8_t>& image, size_t pitch, uint32_t x, uint32_t y)
{
    const uint8_t* p = image.data() + size_t(y) * pitch + size_t(x) * 4;
    return {p[0], p[1], p[2], p[3]};
}

static void TestBc1()
{
    std::vector<uint8_t> out(16 * 4);
    // Solid red, both endpoints equal (3-colour mode, index 0).
    const uint8_t red[8] = {0x00, 0xF8, 0x00, 0xF8, 0, 0, 0, 0};
    DecodeBc1Block(red, out.data(), 16);
    for (uint32_t i = 0; i < 16; ++i)
        Check(At(out, 16, i % 4, i / 4) == Rgba{255, 0, 0, 255}, "BC1 solid red");

    // Four-colour mode: c0 white > c1 black; row y uses index y.
    const uint8_t ramp[8] = {0xFF, 0xFF, 0x00, 0x00, 0x00, 0x55, 0xAA, 0xFF};
    DecodeBc1Block(ramp, out.data(), 16);
    Check(At(out, 16, 0, 0) == Rgba{255, 255, 255, 255}, "BC1 index 0");
    Check(At(out, 16, 3, 1) == Rgba{0, 0, 0, 255}, "BC1 index 1");
    Check(At(out, 16, 1, 2) == Rgba{170, 170, 170, 255}, "BC1 index 2 = 2/3 c0 + 1/3 c1");
    Check(At(out, 16, 2, 3) == Rgba{85, 85, 85, 255}, "BC1 index 3 = 1/3 c0 + 2/3 c1");

    // Three-colour mode: c0 black <= c1 white; index 2 midpoint, index 3 transparent black.
    const uint8_t punch[8] = {0x00, 0x00, 0xFF, 0xFF, 0x00, 0x55, 0xAA, 0xFF};
    DecodeBc1Block(punch, out.data(), 16);
    Check(At(out, 16, 0, 0) == Rgba{0, 0, 0, 255}, "BC1 3-colour index 0");
    Check(At(out, 16, 0, 1) == Rgba{255, 255, 255, 255}, "BC1 3-colour index 1");
    Check(At(out, 16, 0, 2) == Rgba{128, 128, 128, 255}, "BC1 3-colour midpoint");
    Check(At(out, 16, 0, 3) == Rgba{0, 0, 0, 0}, "BC1 transparent black");
}

static void TestBc2()
{
    std::vector<uint8_t> out(16 * 4);
    uint8_t block[16]{};
    for (uint32_t k = 0; k < 8; ++k) block[k] = uint8_t((2 * k) | ((2 * k + 1) << 4));
    // Solid green colour block; BC2 never uses the 3-colour mode even when c0 <= c1.
    block[8] = 0xE0; block[9] = 0x07; block[10] = 0xE0; block[11] = 0x07;
    DecodeBc2Block(block, out.data(), 16);
    for (uint32_t i = 0; i < 16; ++i)
        Check(At(out, 16, i % 4, i / 4) == Rgba{0, 255, 0, uint8_t(i * 17)}, "BC2 explicit 4-bit alpha");
    block[12] = 0xFF; // index 3 for row 0: 1/3 c0 + 2/3 c1, opaque colour
    DecodeBc2Block(block, out.data(), 16);
    Check(At(out, 16, 0, 0)[1] == 255, "BC2 index 3 is a colour, not transparent");
}

static void TestBc3()
{
    std::vector<uint8_t> out(16 * 4);
    uint8_t block[16]{};
    block[0] = 255; block[1] = 0; // 8-alpha mode
    uint64_t bits = 0;
    for (uint32_t i = 0; i < 16; ++i) bits |= uint64_t(i % 8) << (3 * i);
    for (uint32_t i = 0; i < 6; ++i) block[2 + i] = uint8_t(bits >> (8 * i));
    block[8] = 0x1F; block[9] = 0x00; block[10] = 0x1F; block[11] = 0x00; // solid blue
    DecodeBc3Block(block, out.data(), 16);
    const uint8_t expected[8] = {255, 0, 219, 182, 146, 109, 73, 36};
    for (uint32_t i = 0; i < 16; ++i)
        Check(At(out, 16, i % 4, i / 4) == Rgba{0, 0, 255, expected[i % 8]}, "BC3 8-alpha ramp");

    block[0] = 0; block[1] = 255; // 6-alpha mode with explicit 0 and 255
    DecodeBc3Block(block, out.data(), 16);
    const uint8_t expected6[8] = {0, 255, 51, 102, 153, 204, 0, 255};
    for (uint32_t i = 0; i < 16; ++i)
        Check(At(out, 16, i % 4, i / 4)[3] == expected6[i % 8], "BC3 6-alpha ramp");
}

static void TestImageLayout()
{
    // 2x2 blocks of BC1 at a padded source pitch into a padded RGBA8 pitch.
    const size_t srcPitch = 32, dstPitch = 64;
    std::vector<uint8_t> src(srcPitch * 2, 0xEE);
    const uint8_t colors[4][2] = {{0x00, 0xF8}, {0xE0, 0x07}, {0x1F, 0x00}, {0xFF, 0xFF}};
    for (uint32_t b = 0; b < 4; ++b) {
        uint8_t* block = src.data() + (b / 2) * srcPitch + (b % 2) * 8;
        block[0] = block[2] = colors[b][0];
        block[1] = block[3] = colors[b][1];
        block[4] = block[5] = block[6] = block[7] = 0;
    }
    std::vector<uint8_t> dst(dstPitch * 8, 0x11);
    Check(DecodeImage(18, src.data(), srcPitch, 2, 2, dst.data(), dstPitch), "BC1 image decodes");
    Check(At(dst, dstPitch, 3, 3) == Rgba{255, 0, 0, 255}, "block (0,0) red");
    Check(At(dst, dstPitch, 4, 0) == Rgba{0, 255, 0, 255}, "block (1,0) green");
    Check(At(dst, dstPitch, 0, 4) == Rgba{0, 0, 255, 255}, "block (0,1) blue");
    Check(At(dst, dstPitch, 7, 7) == Rgba{255, 255, 255, 255}, "block (1,1) white");
    Check(dst[32] == 0x11 && dst[dstPitch * 7 + 63] == 0x11, "row padding untouched");
    Check(!DecodeImage(6, src.data(), srcPitch, 2, 2, dst.data(), dstPitch), "non-BC format rejected");
    Check(IsBcFormat(18) && IsBcFormat(19) && IsBcFormat(20) && !IsBcFormat(6), "BC format set");
}

int main()
{
    try {
        TestBc1();
        TestBc2();
        TestBc3();
        TestImageLayout();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "FAIL: %s\n", e.what());
        return 1;
    }
    std::printf("bc_decode_test: %zu checks passed\n", checks);
    return 0;
}

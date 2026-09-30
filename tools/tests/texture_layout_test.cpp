#include <gpu/texture_layout.h>
#include <cstdio>

int main()
{
    struct Case { unsigned width, height, mip, blockWidth, blockHeight, x, y; };
    // Xenia packed-tail placements, including the title's BC1 white texture
    // and tall BC3 gradient. Large base levels must remain at the origin.
    constexpr Case cases[] = {
        {4, 4, 0, 4, 4, 4, 0}, {4, 64, 0, 4, 4, 4, 0},
        {64, 4, 0, 4, 4, 0, 4}, {16, 16, 0, 1, 1, 16, 0},
        {32, 32, 0, 4, 4, 0, 0}, {256, 256, 0, 1, 1, 0, 0},
        {32, 32, 1, 4, 4, 4, 0}, {32, 32, 2, 4, 4, 2, 0},
        {32, 32, 3, 4, 4, 1, 0}, {32, 32, 4, 4, 4, 0, 2},
        {512, 256, 4, 4, 4, 0, 4}, {512, 256, 7, 4, 4, 4, 0},
        {1, 1, 0, 1, 1, 16, 0}, {17, 16, 0, 1, 1, 0, 16},
    };
    for (const auto& c : cases)
    {
        const auto offset = gpu::PackedMipOffset2D(c.width, c.height, c.mip, c.blockWidth, c.blockHeight);
        if (offset.x != c.x || offset.y != c.y)
        {
            std::printf("FAIL: %ux%u mip %u -> %u,%u (expected %u,%u)\n", c.width, c.height, c.mip, offset.x, offset.y, c.x, c.y);
            return 1;
        }
    }
    std::puts("PASS: packed mip block origins (small, rectangular, compressed and later levels)");

    struct MipCase { unsigned width, height, level; bool packed; unsigned blockWidth, blockHeight, bytesPerBlock, offset, pitch, x, y; };
    // Levels 1+ from mip_address: power-of-two storage, 32-block pitch/rows,
    // 4 KiB subresources, packed tails shared by later levels.
    constexpr MipCase mips[] = {
        {256, 256, 1, true, 4, 4, 8, 0, 32, 0, 0}, {256, 256, 3, true, 4, 4, 8, 16384, 32, 0, 0},
        {256, 256, 4, true, 4, 4, 8, 24576, 32, 4, 0}, {256, 256, 5, true, 4, 4, 8, 24576, 32, 2, 0},
        {64, 64, 2, false, 1, 1, 4, 4096, 32, 0, 0}, {1024, 1024, 1, false, 1, 1, 4, 0, 512, 0, 0},
        {16, 16, 1, true, 1, 1, 4, 0, 32, 8, 0}, {200, 100, 2, false, 1, 1, 4, 32768, 64, 0, 0},
    };
    for (const auto& c : mips)
    {
        const auto mip = gpu::GuestMip2D(c.width, c.height, c.level, c.packed, c.blockWidth, c.blockHeight, c.bytesPerBlock);
        if (mip.storageOffset != c.offset || mip.pitchBlocks != c.pitch || mip.origin.x != c.x || mip.origin.y != c.y)
        {
            std::printf("FAIL: %ux%u level %u -> offset %u pitch %u origin %u,%u\n", c.width, c.height, c.level,
                mip.storageOffset, mip.pitchBlocks, mip.origin.x, mip.origin.y);
            return 1;
        }
    }
    std::puts("PASS: guest mip storage offsets, pitches and packed origins");
    return 0;
}

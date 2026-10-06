#pragma once

#include <cstddef>
#include <cstdint>

// CPU decoders for BC1/BC2/BC3 (Xenos DXT1, DXT2/3, DXT4/5) into RGBA8, used
// when the host device cannot sample block-compressed images (Mali GPUs
// expose no textureCompressionBC, #214). Input blocks are little endian, as
// left by the renderer's untile + endian swap.
namespace gpu::bc_decode {

inline void Rgb565(uint32_t c, uint8_t out[3]) {
    const uint32_t r = (c >> 11) & 31, g = (c >> 5) & 63, b = c & 31;
    out[0] = uint8_t((r << 3) | (r >> 2));
    out[1] = uint8_t((g << 2) | (g >> 4));
    out[2] = uint8_t((b << 3) | (b >> 2));
}

// Colour part of a block (8 bytes). BC1 uses the 3-colour + transparent
// black mode when c0 <= c1; BC2/BC3 colour blocks always use four colours.
// Writes RGB and, for BC1, alpha.
inline void DecodeColor(const uint8_t* b, bool bc1, uint8_t* dst, size_t dstPitch) {
    const uint32_t c0 = uint32_t(b[0]) | uint32_t(b[1]) << 8;
    const uint32_t c1 = uint32_t(b[2]) | uint32_t(b[3]) << 8;
    uint8_t colors[4][4]{};
    Rgb565(c0, colors[0]);
    Rgb565(c1, colors[1]);
    colors[0][3] = colors[1][3] = colors[2][3] = colors[3][3] = 255;
    if (!bc1 || c0 > c1) {
        for (unsigned c = 0; c < 3; ++c) {
            colors[2][c] = uint8_t((2 * colors[0][c] + colors[1][c] + 1) / 3);
            colors[3][c] = uint8_t((colors[0][c] + 2 * colors[1][c] + 1) / 3);
        }
    } else {
        for (unsigned c = 0; c < 3; ++c)
            colors[2][c] = uint8_t((colors[0][c] + colors[1][c] + 1) / 2);
        colors[3][0] = colors[3][1] = colors[3][2] = colors[3][3] = 0;
    }
    const uint32_t bits = uint32_t(b[4]) | uint32_t(b[5]) << 8 | uint32_t(b[6]) << 16 | uint32_t(b[7]) << 24;
    for (unsigned i = 0; i < 16; ++i) {
        uint8_t* p = dst + size_t(i / 4) * dstPitch + (i % 4) * 4;
        const uint8_t* color = colors[(bits >> (2 * i)) & 3];
        p[0] = color[0]; p[1] = color[1]; p[2] = color[2];
        if (bc1) p[3] = color[3];
    }
}

// dst points at the block's top-left texel; dstPitch is the RGBA8 row pitch in bytes.
inline void DecodeBc1Block(const uint8_t* b, uint8_t* dst, size_t dstPitch) {
    DecodeColor(b, true, dst, dstPitch);
}

inline void DecodeBc2Block(const uint8_t* b, uint8_t* dst, size_t dstPitch) {
    DecodeColor(b + 8, false, dst, dstPitch);
    for (unsigned i = 0; i < 16; ++i) {
        const uint8_t nibble = (b[i / 2] >> ((i & 1) * 4)) & 15;
        dst[size_t(i / 4) * dstPitch + (i % 4) * 4 + 3] = uint8_t(nibble * 17);
    }
}

inline void DecodeBc3Block(const uint8_t* b, uint8_t* dst, size_t dstPitch) {
    DecodeColor(b + 8, false, dst, dstPitch);
    uint8_t alphas[8] = {b[0], b[1]};
    if (alphas[0] > alphas[1]) {
        for (unsigned i = 1; i <= 6; ++i)
            alphas[i + 1] = uint8_t(((7 - i) * alphas[0] + i * alphas[1] + 3) / 7);
    } else {
        for (unsigned i = 1; i <= 4; ++i)
            alphas[i + 1] = uint8_t(((5 - i) * alphas[0] + i * alphas[1] + 2) / 5);
        alphas[6] = 0;
        alphas[7] = 255;
    }
    uint64_t bits = 0;
    for (unsigned i = 0; i < 6; ++i) bits |= uint64_t(b[i + 2]) << (i * 8);
    for (unsigned i = 0; i < 16; ++i)
        dst[size_t(i / 4) * dstPitch + (i % 4) * 4 + 3] = alphas[(bits >> (3 * i)) & 7];
}

// Xenos fetch formats 18 (DXT1), 19 (DXT2/3), 20 (DXT4/5).
inline bool IsBcFormat(uint32_t xenosFormat) {
    return xenosFormat == 18 || xenosFormat == 19 || xenosFormat == 20;
}

inline uint32_t BytesPerBlock(uint32_t xenosFormat) {
    return xenosFormat == 18 ? 8u : 16u;
}

// Decodes blocksX x blocksY blocks read at srcPitch into an RGBA8 image of
// (blocksX*4) x (blocksY*4) texels written at dstPitch.
inline bool DecodeImage(uint32_t xenosFormat, const uint8_t* src, size_t srcPitch,
    uint32_t blocksX, uint32_t blocksY, uint8_t* dst, size_t dstPitch) {
    if (!IsBcFormat(xenosFormat) || !src || !dst) return false;
    const uint32_t bpb = BytesPerBlock(xenosFormat);
    for (uint32_t by = 0; by < blocksY; ++by)
        for (uint32_t bx = 0; bx < blocksX; ++bx) {
            const uint8_t* block = src + size_t(by) * srcPitch + size_t(bx) * bpb;
            uint8_t* out = dst + size_t(by) * 4 * dstPitch + size_t(bx) * 16;
            if (xenosFormat == 18) DecodeBc1Block(block, out, dstPitch);
            else if (xenosFormat == 19) DecodeBc2Block(block, out, dstPitch);
            else DecodeBc3Block(block, out, dstPitch);
        }
    return true;
}

} // namespace gpu::bc_decode

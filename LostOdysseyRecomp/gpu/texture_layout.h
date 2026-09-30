#pragma once

#include <algorithm>
#include <bit>
#include <cstdint>

namespace gpu
{
    struct TextureBlockOffset
    {
        uint32_t x = 0;
        uint32_t y = 0;
    };

    // Follows Xenia texture_util.cc GetPackedMipOffset (Copyright 2022 Ben
    // Vanik, BSD 3-Clause; see thirdparty/xenia-LICENSE.txt). Dimensions are
    // those of the original texture, before selecting the mip level.
    constexpr TextureBlockOffset PackedMipOffset2D(uint32_t width, uint32_t height,
        uint32_t mip, uint32_t blockWidth, uint32_t blockHeight)
    {
        const uint32_t logWidth = std::bit_width(width - 1);
        const uint32_t logHeight = std::bit_width(height - 1);
        const uint32_t logMin = std::min(logWidth, logHeight);
        if (logMin > 4 + mip) return {};
        const uint32_t firstPackedMip = logMin > 4 ? logMin - 4 : 0;
        const uint32_t packedMip = mip - firstPackedMip;
        TextureBlockOffset offset;
        if (packedMip < 3)
        {
            if (logWidth > logHeight) offset.y = 16u >> packedMip;
            else offset.x = 16u >> packedMip;
        }
        else
        {
            if (logWidth > logHeight)
                offset.x = (1u << (logWidth - firstPackedMip)) >> (packedMip - 2);
            else
                offset.y = (1u << (logHeight - firstPackedMip)) >> (packedMip - 2);
        }
        offset.x /= blockWidth;
        offset.y /= blockHeight;
        return offset;
    }

    // First mip level stored inside the packed tail (Xenia GetPackedMipLevel).
    constexpr uint32_t PackedMipLevel(uint32_t width, uint32_t height)
    {
        const uint32_t logMin = std::min(std::bit_width(width - 1), std::bit_width(height - 1));
        return logMin > 4 ? logMin - 4 : 0;
    }

    struct GuestMipLevel
    {
        uint32_t storageOffset = 0; // bytes from mip_address
        uint32_t storageBytes = 0;  // the whole stored level, 4 KiB aligned
        uint32_t pitchBlocks = 0;
        TextureBlockOffset origin;
    };

    // Tiled 2D levels 1+ start at mip_address. Each stored level uses the
    // power-of-two size of the base, 32-block aligned pitch and height and a
    // 4 KiB aligned subresource. Levels past the packed level share its tail;
    // a level-0 tail holds every mip at mip_address offset zero (Xenia
    // texture_util.cc GetGuestTextureLayout, BSD 3-Clause).
    constexpr GuestMipLevel GuestMip2D(uint32_t width, uint32_t height, uint32_t level, bool packedMips,
        uint32_t blockWidth, uint32_t blockHeight, uint32_t bytesPerBlock)
    {
        auto levelSize = [&](uint32_t storage, uint32_t& pitch) {
            const uint32_t w = std::max(1u, std::bit_ceil(width) >> storage);
            const uint32_t h = std::max(1u, std::bit_ceil(height) >> storage);
            pitch = ((w + blockWidth - 1) / blockWidth + 31) & ~31u;
            const uint32_t rows = ((h + blockHeight - 1) / blockHeight + 31) & ~31u;
            return (pitch * rows * bytesPerBlock + 4095u) & ~4095u;
        };
        GuestMipLevel result;
        const uint32_t storage = packedMips ? std::min(level, PackedMipLevel(width, height)) : level;
        if (packedMips) result.origin = PackedMipOffset2D(width, height, level, blockWidth, blockHeight);
        uint32_t pitch = 0;
        for (uint32_t s = 1; s < storage; ++s) result.storageOffset += levelSize(s, pitch);
        result.storageBytes = levelSize(storage, result.pitchBlocks);
        return result;
    }
}

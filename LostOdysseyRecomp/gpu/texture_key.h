#pragma once

#include <cstddef>
#include <cstdint>

namespace gpu::texture_cache
{
    struct Key
    {
        uint32_t address, format, width, height, flags; // flags: tiled | endian<<1 | pitch<<3
        uint32_t mipAddress = 0; // effective source of uploaded levels 1+, zero without a mip chain

        bool operator==(const Key& other) const
        {
            return address == other.address && format == other.format &&
                width == other.width && height == other.height && flags == other.flags &&
                mipAddress == other.mipAddress;
        }
    };

    struct KeyHash
    {
        size_t operator()(const Key& key) const
        {
            uint32_t hash = key.address * 1000003u ^ key.format * 8191u ^
                key.width * 131u ^ key.height * 17u ^ key.flags ^ key.mipAddress * 0x9E3779B9u;

            // Guest addresses are 4 KiB aligned. Mix their high bits into the
            // low bits used by unordered_map's power-of-two bucket mask.
            hash ^= hash >> 16;
            hash *= 0x7FEB352Du;
            hash ^= hash >> 15;
            hash *= 0x846CA68Bu;
            hash ^= hash >> 16;
            return hash;
        }
    };
}

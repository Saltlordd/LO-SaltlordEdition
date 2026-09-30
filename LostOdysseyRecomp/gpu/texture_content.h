#pragma once

#include <cstddef>
#include <cstdint>
#include <xxhash.h>

namespace gpu::texture_cache
{
    // Phys aliases addresses into the 512 MiB guest physical mapping. Validate
    // the entire extent before decoding or hashing; a span may not wrap it.
    constexpr bool PhysicalRangeValid(uint32_t address, uint64_t bytes)
    {
        constexpr uint64_t physicalBytes = 0x20000000ull;
        return bytes <= physicalBytes - (address & 0x1FFFFFFFu);
    }

    inline uint64_t ContentHash(const void* data, size_t bytes)
    {
        // XXH3 reads every byte, including short tails. Its vectorized scan
        // avoids an extra texture-sized snapshot allocation on every upload.
        return XXH3_64bits(data, bytes);
    }

    inline uint64_t GuestContentHash(const void* base, size_t baseBytes,
        const void* mips, size_t mipBytes)
    {
        const uint64_t hash = ContentHash(base, baseBytes);
        return mipBytes ? hash ^ ContentHash(mips, mipBytes) * 0x9E3779B97F4A7C15ull : hash;
    }
}

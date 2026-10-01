#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>

namespace gpu {
// Match ReadRegister's zero-value MMIO fallback. Callers run on the command
// processor thread and keep the same register order; this is not a cache.
template<class MmioValue>
inline void CopyRegisterSnapshot(std::span<const uint32_t> registers,
    uint32_t first, std::span<uint32_t> output, const MmioValue* mmio)
{
    const size_t count = first < registers.size()
        ? std::min(output.size(), registers.size() - first) : 0;
    const auto* source = count ? registers.data() + first : nullptr;
    for (size_t i = 0; i < count; ++i)
        output[i] = source[i] ? source[i] : uint32_t(mmio[i]);
    std::fill(output.begin() + count, output.end(), 0);
}

// A generation change rebuilds this list. Between command writes, only these
// zero-register words can change through the guest MMIO image.
inline size_t CollectRegisterFallbackOffsets(std::span<const uint32_t> registers,
    std::span<uint16_t> offsets)
{
    size_t count = 0;
    for (size_t i = 0; i < registers.size(); ++i)
        if (registers[i] == 0 && count < offsets.size())
            offsets[count++] = static_cast<uint16_t>(i);
    return count;
}

template<class MmioValue>
inline bool RefreshRegisterSnapshotFallbacks(std::span<const uint16_t> offsets,
    std::span<uint32_t> snapshot, const MmioValue* mmio, uint64_t& snapshotVersion)
{
    bool changed = false;
    for (uint16_t offset : offsets) {
        if (offset >= snapshot.size()) continue;
        const uint32_t value = uint32_t(mmio[offset]);
        if (snapshot[offset] != value) {
            snapshot[offset] = value;
            changed = true;
        }
    }
    if (changed) ++snapshotVersion;
    return changed;
}

inline bool CanReuseUploadedConstants(uint64_t uploadedOffset, uint64_t uploadedVersion,
    uint64_t snapshotVersion)
{
    return snapshotVersion != 0 && uploadedOffset != UINT64_MAX && uploadedVersion == snapshotVersion;
}
}

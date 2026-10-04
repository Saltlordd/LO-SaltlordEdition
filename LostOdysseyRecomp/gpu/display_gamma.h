#pragma once
#include <os/logger.h>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <mutex>

// Xenos display gamma ramp (DC_LUT_*). D3D's SetGammaRamp writes a 256-entry
// 10-bit table, adjusted for the display type VdGetCurrentDisplayGamma reports,
// and the scanout applies it to the front buffer. Presentation applies the same
// table (#179). Only the 256-entry mode is decoded, as in Xenia; Lost Odyssey
// does not use the piecewise-linear mode.
namespace gpu::display_gamma
{
constexpr uint32_t RegisterFirst = 0x1920, RegisterLast = 0x1937;
constexpr uint32_t RegRwMode = 0x1921, RegRwIndex = 0x1922, RegSeqColor = 0x1923, RegPwlData = 0x1924,
                   Reg30Color = 0x1925, RegWriteEnableMask = 0x1927;

struct Ramp
{
    // Normalized red, green and blue output per 8-bit input.
    std::array<float, 3 * 256> values{};
    bool identity = true;
};

namespace detail
{
// Packed like DC_LUT_30_COLOR: blue in bits 0-9, green 10-19, red 20-29.
inline uint32_t IdentityEntry(uint32_t i)
{
    const uint32_t v = (i * 1023 + 127) / 255;
    return v | (v << 10) | (v << 20);
}
struct State
{
    std::mutex mutex;
    std::array<uint32_t, 256> table = [] {
        std::array<uint32_t, 256> t{};
        for (uint32_t i = 0; i < 256; ++i) t[i] = IdentityEntry(i);
        return t;
    }();
    uint32_t rwMode = 0, rwIndex = 0, component = 0, writeMask = 7;
    std::atomic<uint32_t> generation{0};
    uint32_t logged = 0;
    bool pwlWarned = false;
};
inline State& Get()
{
    static State state;
    return state;
}
inline bool IsIdentity(const std::array<uint32_t, 256>& table)
{
    for (uint32_t i = 0; i < 256; ++i)
        for (uint32_t shift : {0u, 10u, 20u})
            if (std::abs(int((table[i] >> shift) & 1023) - int((IdentityEntry(i) >> shift) & 1023)) > 1) return false;
    return true;
}
inline void LogCompleted(State& s)
{
    if (s.logged >= 8) return;
    ++s.logged;
    auto out = [&](uint32_t i) { return ((s.table[i] >> 20) & 1023) * 255.0f / 1023.0f; };
    LOG_INFO("display gamma ramp: identity={} 0->{:.1f} 16->{:.1f} 54->{:.1f} 128->{:.1f} 192->{:.1f} 235->{:.1f} 255->{:.1f}",
        IsIdentity(s.table), out(0), out(16), out(54), out(128), out(192), out(235), out(255));
}
} // namespace detail

// GPU worker thread: every register write in RegisterFirst..RegisterLast.
inline void OnRegisterWrite(uint32_t index, uint32_t value)
{
    auto& s = detail::Get();
    std::lock_guard lock(s.mutex);
    switch (index)
    {
    case RegRwMode: s.rwMode = value; break;
    case RegWriteEnableMask: s.writeMask = value & 7; break;
    case RegRwIndex: s.rwIndex = value & 0xFF; s.component = 0; break;
    case Reg30Color:
    {
        if (s.rwMode & 1) break;
        uint32_t& entry = s.table[s.rwIndex];
        // Write-enable bits: 0 blue, 1 green, 2 red, matching the packing.
        for (uint32_t channel = 0; channel < 3; ++channel)
            if (s.writeMask & (1u << channel))
                entry = (entry & ~(1023u << (10 * channel))) | (((value >> (10 * channel)) & 1023) << (10 * channel));
        s.generation.fetch_add(1, std::memory_order_release);
        if (s.rwIndex == 255) detail::LogCompleted(s);
        s.rwIndex = (s.rwIndex + 1) & 0xFF;
        break;
    }
    case RegSeqColor:
    {
        if (s.rwMode & 1) break;
        // Components arrive red, green, blue; bits 0-5 are hardwired to zero.
        const uint32_t channel = 2 - s.component;
        if (s.writeMask & (1u << channel))
        {
            uint32_t& entry = s.table[s.rwIndex];
            entry = (entry & ~(1023u << (10 * channel))) | (((value >> 6) & 1023) << (10 * channel));
            s.generation.fetch_add(1, std::memory_order_release);
        }
        if (++s.component == 3)
        {
            s.component = 0;
            if (s.rwIndex == 255) detail::LogCompleted(s);
            s.rwIndex = (s.rwIndex + 1) & 0xFF;
        }
        break;
    }
    case RegPwlData:
        if (!s.pwlWarned)
        {
            s.pwlWarned = true;
            LOG_WARNING("display gamma ramp: piecewise-linear mode is not supported; keeping the 256-entry table");
        }
        break;
    default: break;
    }
}

inline uint32_t Generation() { return detail::Get().generation.load(std::memory_order_acquire); }

// Presentation thread: copies the current table.
inline Ramp Snapshot()
{
    auto& s = detail::Get();
    std::array<uint32_t, 256> table;
    {
        std::lock_guard lock(s.mutex);
        table = s.table;
    }
    Ramp ramp;
    ramp.identity = detail::IsIdentity(table);
    for (uint32_t i = 0; i < 256; ++i)
    {
        ramp.values[3 * i + 0] = ((table[i] >> 20) & 1023) / 1023.0f;
        ramp.values[3 * i + 1] = ((table[i] >> 10) & 1023) / 1023.0f;
        ramp.values[3 * i + 2] = (table[i] & 1023) / 1023.0f;
    }
    return ramp;
}
} // namespace gpu::display_gamma

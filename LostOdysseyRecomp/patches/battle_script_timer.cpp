#include <stdafx.h>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <mutex>
#include <unordered_map>

extern "C" PPC_FUNC(__imp__sub_8238AE08);
extern "C" PPC_FUNC(__imp__sub_82A9DBB8);
extern "C" PPC_FUNC(__imp__sub_82A9E3B8);

namespace
{
struct ScriptClock
{
    uint32_t script = 0;
    double fraction = 0;
};

std::mutex clockMutex;
std::unordered_map<uint32_t, ScriptClock> clocks;

bool Enabled()
{
    // Restore the original truncation for controlled comparisons.
    static const bool enabled = [] {
        const char* value = std::getenv("LO_BATTLE_SCRIPT_TIMER");
        return !value || std::strcmp(value, "0") != 0;
    }();
    return enabled;
}

void Reset(uint32_t manager)
{
    std::lock_guard lock(clockMutex);
    clocks.erase(manager);
}
}

PPC_FUNC(sub_8238AE08)
{
    // sub_8238A640 obtains a millisecond delta from sub_8238AC40 and converts
    // it to seconds. sub_8238ACC8 saves that delta in f31, then writes
    // int(float(delta * 60)) to its script's +16 before calling this function.
    // At 90/120 FPS the result is normally zero. Timed wait opcode
    // sub_82A9BF40 subtracts it from the remaining wait, so only slower frames
    // advance the wait. Retain the fraction between actual script updates.
    //
    // This call site is after the guest's inactive/first-update early returns
    // and before any consumers. Leave all registers and the update frequency
    // intact; only replace the already-written integer step.
    if (Enabled() && uint32_t(ctx.lr) == 0x8238AD64)
    {
        const uint32_t manager = ctx.r3.u32;
        const uint32_t script = PPC_LOAD_U32(manager + 44);
        if (script)
        {
            const double delta = ctx.f31.f64;
            const double ticks = static_cast<float>(delta * 60.0);
            std::lock_guard lock(clockMutex);
            if (!std::isfinite(delta) || delta < 0 ||
                !std::isfinite(ticks) || ticks > std::numeric_limits<int32_t>::max())
            {
                // Preserve the guest's conversion for discontinuous/invalid
                // time, and do not carry a fraction across that discontinuity.
                clocks.erase(manager);
            }
            else
            {
                auto& clock = clocks[manager];
                if (clock.script != script) clock = {script, 0};
                const double accumulated = clock.fraction + ticks;
                const auto whole = static_cast<int32_t>(accumulated);
                clock.fraction = accumulated - whole;
                PPC_STORE_U32(script + 16, static_cast<uint32_t>(whole));
            }
        }
    }
    __imp__sub_8238AE08(ctx, base);
}

PPC_FUNC(sub_82A9DBB8)
{
    // Script initialization can reuse both addresses from a previous battle.
    if (Enabled()) Reset(ctx.r3.u32);
    __imp__sub_82A9DBB8(ctx, base);
}

PPC_FUNC(sub_82A9E3B8)
{
    const uint32_t manager = ctx.r3.u32;
    __imp__sub_82A9E3B8(ctx, base);
    // An inactive script can refuse release; retain its clock in that case.
    if (Enabled() && !PPC_LOAD_U32(manager + 44)) Reset(manager);
}

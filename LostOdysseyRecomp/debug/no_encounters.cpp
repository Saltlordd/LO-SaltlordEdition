#include <stdafx.h>
#include <os/logger.h>
#include "no_encounters.h"
#include <settings/config.h>
#include <atomic>

extern "C" PPC_FUNC(__imp__sub_829E3268);

namespace
{
    std::atomic<bool>& Requested()
    {
        static std::atomic<bool> requested{settings::GetConfig().noRandomEncounters};
        return requested;
    }
}

bool debug_menu::NoEncountersEnabled()
{
    return Requested().load(std::memory_order_relaxed);
}

void debug_menu::SetNoEncountersEnabled(bool enabled)
{
    Requested().store(enabled, std::memory_order_relaxed);
    if (!settings::SaveNoRandomEncounters(enabled))
        LOG_WARNING("debug menu: failed to persist no random encounters setting");
    LOG_INFO("debug menu: no random encounters {}", enabled);
}

// The walking encounter update sub_829E3048 accumulates the distance walked
// and asks this draw (field player vtable +1636) whether to fight; on 1 it
// requests the battle through 0x828278A0. Keep the draw and its counter/
// threshold updates, and only turn its result into "no battle" at that call
// site; an early return from sub_829E3048 would also skip the distance update.
PPC_FUNC(sub_829E3268)
{
    constexpr uint32_t WalkingEncounterReturn = 0x829E31F8;
    const uint32_t caller = static_cast<uint32_t>(ctx.lr);
    __imp__sub_829E3268(ctx, base);
    if (caller == WalkingEncounterReturn && ctx.r3.u32 == 1 &&
        Requested().load(std::memory_order_relaxed))
        ctx.r3.u64 = 0;
}

#pragma once

#include <cstdint>

namespace debug_menu
{
    bool SaveAnywhereEnabled();
    bool SetSaveAnywhereEnabled(bool enabled);
    // Issue #74: re-arm the event script's RB party switch after a mid-split load.
    void RequestPartySwitch();
    void PartySwitchTick(uint8_t* base);
}

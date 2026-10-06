#pragma once
#include <cstdint>

namespace debug_menu
{
    void Toggle();
    void Update();
    bool RequestVictory();
    void CancelVictory();
    const wchar_t* Status();
    // Session number of the battle running now (from 1), 0 outside battles. Any thread.
    uint32_t CurrentBattle();
}

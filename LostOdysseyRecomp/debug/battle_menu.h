#pragma once
#include <cstdint>

namespace debug_menu
{
    uint32_t CurrentBattle();
    void Toggle();
    void Update();
    bool RequestVictory();
    void CancelVictory();
    const wchar_t* Status();
}

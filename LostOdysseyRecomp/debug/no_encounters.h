#pragma once

namespace debug_menu
{
    // Field random encounters never start a battle; scripted battles still run.
    bool NoEncountersEnabled();
    void SetNoEncountersEnabled(bool enabled);
}

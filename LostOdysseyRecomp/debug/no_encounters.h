#pragma once

namespace debug_menu
{
    // Field random encounters never start a battle; scripted battles still run.
    bool NoEncountersEnabled();
    void SetNoEncountersEnabled(bool enabled);
    // Session-only: every walking step in an area with random encounters starts
    // a battle. Turning either switch on turns the other off.
    bool EncounterEveryStepEnabled();
    void SetEncounterEveryStepEnabled(bool enabled);
}

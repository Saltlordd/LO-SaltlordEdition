#pragma once

#include <cstdint>

// Issue #114: hold a random encounter while a field event has taken player
// control. Decision only; the hooks live in encounter_defer.cpp.
namespace encounter_defer
{
// Loader manager 0x832631F0 as sub_8231F5E0 reads it.
struct LoaderState
{
    uint8_t stage = 0;      // +0, the loader returns at once when >= 4
    uint8_t current = 6;    // +1, slot being processed; >= 6 means none
    int32_t phase[6] = {};  // +3168 + 88 * slot; slot 0 map change, slot 1 battle
    uint32_t battleId = 0;  // slot 1 +16 (+3272), the encounter id
};

// The battle request the walking encounter queued, seen by the hooks.
struct RandomRequest
{
    bool active = false;
    uint32_t id = 0;
    uint32_t heldPicks = 0;
};

// sub_8231F5E0 starts slot 1 (phase 1 -> 2, then the field unloads) on this call.
constexpr bool PicksBattle(const LoaderState& s)
{
    return s.stage < 4 && s.current >= 6 && s.phase[0] <= 0 && s.phase[1] > 0;
}

// Longest wait for the event to give control back. The box's item message
// clears its wait (VM field 0x1010) after about 2.6 s; once it has, a battle
// no longer strands the script, which then only waits for a button press.
constexpr uint64_t MaxHoldMs = 15000;

// Skip this loader pick. The first pick after the request is always skipped:
// the field script that accepts a touch in the request's frame may run after
// the loader in that frame, so its player lock is visible only on the next
// pick. After that, wait while the event keeps player control, up to
// MaxHoldMs. Only the walking encounter's own phase 1 request is held, and
// only while no other request is queued, so the order of requests never changes.
constexpr bool HoldPick(const LoaderState& s, const RandomRequest& r, bool playerControl, uint64_t heldMs)
{
    if (!PicksBattle(s) || s.phase[1] != 1 || !r.active || r.id != s.battleId)
        return false;
    for (int slot = 2; slot < 6; ++slot)
        if (s.phase[slot] > 0)
            return false;
    return r.heldPicks == 0 || (!playerControl && heldMs < MaxHoldMs);
}

// Another battle request arrives while the random one is being held: drop the
// random one so the scripted battle is not refused as "already requested".
constexpr bool DropForScriptedRequest(int32_t battlePhase, uint32_t battleId, const RandomRequest& r)
{
    return r.active && r.heldPicks > 0 && battlePhase == 1 && r.id == battleId;
}
}

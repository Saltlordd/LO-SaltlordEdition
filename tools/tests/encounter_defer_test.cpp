// Decision table for the #114 encounter hold (patches/encounter_defer.h).
#include <patches/encounter_defer.h>
#include <cstdio>
#include <cstdlib>

using namespace encounter_defer;

namespace
{
int failures = 0;
int checks = 0;

void Expect(bool condition, const char* what)
{
    ++checks;
    if (!condition)
    {
        ++failures;
        std::printf("FAIL: %s\n", what);
    }
}

// Idle loader with only the walking encounter queued in slot 1.
LoaderState Queued(uint32_t id = 4)
{
    LoaderState s;
    s.phase[1] = 1;
    s.battleId = id;
    return s;
}

RandomRequest Random(uint32_t id = 4, uint32_t held = 0) { return {true, id, held}; }
}

int main()
{
    // The game's own pick condition (sub_8231F5E0).
    Expect(PicksBattle(Queued()), "idle loader picks queued battle");
    LoaderState s = Queued();
    s.stage = 4;
    Expect(!PicksBattle(s), "stage >= 4 returns early");
    s = Queued();
    s.current = 1;
    Expect(!PicksBattle(s), "busy loader does not pick");
    s = Queued();
    s.phase[0] = 1;
    Expect(!PicksBattle(s), "map change in slot 0 goes first");
    s = Queued();
    s.phase[1] = 0;
    Expect(!PicksBattle(s), "empty slot 1");
    s.phase[1] = -1;
    Expect(!PicksBattle(s), "negative phase is inactive");

    // First pick after the request is always held (request and touch share a frame).
    Expect(HoldPick(Queued(), Random(), true, 0), "first pick held even with control");
    Expect(HoldPick(Queued(), Random(), false, 0), "first pick held without control");
    // Afterwards only while the field event keeps control.
    Expect(!HoldPick(Queued(), Random(4, 1), true, 0), "control back: battle starts");
    Expect(HoldPick(Queued(), Random(4, 1), false, 0), "event holds control: wait");
    Expect(HoldPick(Queued(), Random(4, 500), false, MaxHoldMs - 1), "long event: still wait");
    Expect(!HoldPick(Queued(), Random(4, 500), false, MaxHoldMs), "wait is capped");
    Expect(HoldPick(Queued(), Random(4, 0), true, MaxHoldMs), "first pick held regardless of time");

    // Only the walking encounter's own phase 1 request.
    Expect(!HoldPick(Queued(), RandomRequest{}, false, 0), "scripted battle never held");
    Expect(!HoldPick(Queued(), Random(3, 1), false, 0), "different request in slot 1");
    s = Queued();
    s.phase[1] = 2;
    Expect(!HoldPick(s, Random(4, 1), false, 0), "loading request is not touched");
    s.phase[1] = 0;
    Expect(!HoldPick(s, Random(4, 1), false, 0), "cancelled request");

    // Never reorders requests: a map change or any other queued load is not held back.
    s = Queued();
    s.phase[0] = 1;
    Expect(!HoldPick(s, Random(4, 1), false, 0), "slot 0 pending");
    for (int slot = 2; slot < 6; ++slot)
    {
        s = Queued();
        s.phase[slot] = 1;
        Expect(!HoldPick(s, Random(4, 1), false, 0), "slot 2..5 pending releases the hold");
        Expect(!HoldPick(s, Random(4, 0), true, 0), "slot 2..5 pending, first pick");
    }
    s = Queued();
    s.current = 2;
    Expect(!HoldPick(s, Random(4, 1), false, 0), "loader busy: original runs");

    // A scripted battle arriving during a hold replaces the held encounter.
    Expect(DropForScriptedRequest(1, 4, Random(4, 1)), "held encounter dropped");
    Expect(!DropForScriptedRequest(1, 4, Random(4, 0)), "not yet held: original busy result");
    Expect(!DropForScriptedRequest(2, 4, Random(4, 3)), "already loading");
    Expect(!DropForScriptedRequest(1, 7, Random(4, 3)), "slot holds another request");
    Expect(!DropForScriptedRequest(1, 4, RandomRequest{}), "no random request");

    std::printf("encounter defer: %d/%d checks passed\n", checks - failures, checks);
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}

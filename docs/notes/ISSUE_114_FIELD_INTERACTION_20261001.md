# Issue #114: encounter while breaking a container

Updated 2026-10-06 (UTC). [#114](https://github.com/freefrank/LostOdysseyRecomp/issues/114):
a random encounter starts in the same moment Kaim breaks a box in Highlands of Wohl -
Edge of Wasteland (3) (`u13_0_scrw`, map 3). After the battle Kaim cannot move and the
menu does not open.

The first fix (v0.7.35, `659932eb`) made the C1 touch query reject while a battle was
queued. C1 is a general "A touches B" query, and the Lunar Palace platforms poll it every
frame, so it broke them after battles (#171); v0.8.6 removed it. The current fix leaves
C1 alone and holds the queued encounter instead.

## What happens

Scripts: `u13_0_scrw` records 4 and 5 are the two boxes (objects 4/5, touch volumes
10/11); records 6 and 7 run the item message. Walking into a box breaks it; no button
is needed.

| Step | Code | State |
|---|---|---|
| Walking encounter | `0x829E3048` → `0x828278A0` (return address `0x829E3220`) | Loader slot 1 (`0x83263EA8`, manager `0x832631F0` +3256) phase 1, encounter id at +16 |
| Same frame, box script | C1 at pc `0x3A` accepts: its predicate `0x829E54F0` only checks committed battles | |
| | `DD 01` (pc `0x45`) → `0x82A4B468` → `0x82A15868(controller, 0, 0)` | controller +0x5C8 bits `0x0C000000` cleared: player has no control, no menu |
| | pc `0x54` `3A` | destroyed bit (`0x1C0A` / `0x1C0B`) set |
| | pc `0xC2` `05 1010`, pc `0xC5` `AB` | VM field `0x1010` = 1, item message ("Wind Bomb obtained") |
| | pc `0xCB`..`0xD6` | waits while `0x1010` = 1; records 6/7 clear it at their pc `0x1AB` when the message ends |
| | pc `0xDE`..`0xF5`, `0xF8` `33 01` | waits for A, then gives control back |
| Next loader pick | `0x8231F5E0` (from `0x8231F010` ← `0x8231EE68` ← engine tick `0x82290B60`) | slot 1 phase 2; the field unloads about 0.1 s later |

After the battle the field VM resumes the box script where it was (pc `0xCB`), but the
item message is gone, so `0x1010` stays 1 and the script never reaches `33 01`. The new
controller comes up with the control bit clear: that is the softlock.

Diagnostic runs on psvita (2026-10-05, before the decision to stop game runs) showed this
directly: a normal break goes `DD` → `0x1010` = 1 → cleared at pc `0x1AB` 2.6 s later →
A → `33 01` (controller `0xa60c418` → `0x260c418` → `0xa60c418`). With an encounter forced
in the frame of the touch, the request and pc `0x45`..`0xCB` ran in the same frame; after
the battle the box script stepped `0xD6` → `0xCB`, a VM dump showed `0x1010` = 1, there was
no message, and the controller read `0x270c418`. After a request the walking update is
not called again, so the player cannot reach a box later than that frame.

## Fix

`LostOdysseyRecomp/patches/encounter_defer.cpp`:

- `0x828278A0`: remembers whether the request that took slot 1 came from the walking
  encounter (return address `0x829E3220`) and its id. Script battles (`0x82A4CC70`,
  `0x82A4CE30` and others) are never held. If a script requests a battle while a random
  encounter is being held, the held one is cleared with the game's own `0x82826E80`
  (as `0x82826F40` does) so the script's battle is not refused as "already requested".
- `0x82828698` (the other writer of slot 1): if it rewrites the slot, tracking stops.
- `0x8231F5E0`: when it would start that random encounter (idle loader, no map change in
  slot 0, no other request in slots 2-5), the call is skipped. The first pick after the
  request is always skipped once, because the touch may be accepted after the loader in
  that frame. After that the pick waits while controller +0x5C8 bit `0x08000000` is clear,
  for at most 15 s. When the event gives control back, the battle starts as usual. Request
  order never changes: anything else queued releases the hold.

The cap covers one open question: the prompt's A check (`D0`, `0x82A63FC0`) asks the
player controller through its vtable +972, and whether that answers while the controller
sits after an encounter request was not established from the code. If it does not, the
battle starts after 15 s; by then records 6/7 have cleared `0x1010` (about 2.6 s), so the
box script only waits for A, which works again after the battle.

The decision is a pure function in `encounter_defer.h`; `LoEncounterDeferTest` checks it
(32 cases). No game run was done with the fix.

## Why it cannot affect C1 (#171)

No hook touches C1 (`0x82A4FA38`), its predicate `0x829E54F0`, the VM or any script
state, so every C1 poll returns the game's own answer. A hold only changes when a random
battle leaves the field: one frame later than before in every case, and longer only while
the controller's control bit is clear, which a random encounter cannot start (walking
needs control). The Lunar Palace polls keep running with real answers during a hold.

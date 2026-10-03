# Issue #114: field interaction during a pending battle request

Date: 2026-10-01 (UTC). Issue [#114](https://github.com/freefrank/LostOdysseyRecomp/issues/114)
was OPEN when checked at 20:47 UTC. The reporter describes a rare encounter during
container destruction; there is no reliable reproduction available for this investigation.

The original game code leaves a gap between queuing a battle and marking it pending
for touch interactions. A local guard now covers that gap while the request is active.
This supports the original-game-bug hypothesis, but does not establish matching
console behavior or the cause of the reported permanent softlock.

## Static evidence

MapID 3 maps to `u13_0_scrw`. Records 4/5 describe breakable objects 2001/2002;
the screenshot coordinates have not been mapped to either object ID.

| Stage | Original function | Relevant behavior |
|---|---|---|
| Walking | `0x82A1D260` / `0x82A168C8` | A successful touch-state change aborts the current walking action before its random-encounter check. |
| Encounter request | `0x829E3048` → `0x828278A0` | Queues the battle resource record at `0x83263EA8`, without setting committed battle/scene state. |
| Battle predicate | `0x829E54F0` | Checks BattleId at GI `+0x348`, or scene 3 at World `+0x1D8`/`+0x1DC`; it does not check the queued request. |
| Touch entry | C1, `0x82A4FA38` | Calls the predicate through GameInfo vtable `+0x34C`, with return LR `0x82A4FBA4`. |

The request uses a signed active-phase test: phase 1 is queued and phase 2 is loading.
While either is active and the committed state still says no battle, C1 can accept
the interaction under the original predicate.

Both box records reach C1 at PC `0x3A`. Acceptance advances to PC `0x45` (DD1,
menu close), followed by the persistent destroyed-flag write. Rejection reads a
little-endian u16 operand and jumps to `0x10F`, skipping that work. The records differ
in their touch-object operand (10/11), but have the same rejection target.

The later item-event, reference `0x1010`, UI and button-A waits have not yielded a
proven permanent non-recovery path. Their state is absent from the available log.

## Implemented guard

`LostOdysseyRecomp/patches/field_interaction.cpp` strongly overrides `0x829E54F0`.
It saves incoming LR and calls the original implementation, changing a zero result
to one only for LR `0x82A4FBA4` with a positive signed phase at `0x83263EA8`.
C1 then follows its original rejection branch. All users of this C1 call site are
covered; item-event updates and VM scheduling retain the original predicate behavior.
The wrapper writes no guest memory and leaves encounter requests intact.

`0x8231F010` / `0x8231F5E0` select the resource record and advance phase 1 to 2.
Normal completion in `0x8231FBF8` and explicit cancellation pass through
`0x82826F40`, which applies `0x82826E80` to clear the request records.
The complete ordering between request clearing and formal battle-pending state
has not been established; the guard covers the positive-phase interval only.

After cancellation, touch animation has an independent exit: `0x82A29B80` enters
the Touch animation, and `0x82A21C78` transitions to PlayerWalking when it completes.
PlayerTouching.EndState calls `0x82A18A00` to clear touch state. This static path
assumes valid objects and normal animation completion; it is not a gameplay test.

## Checks

```powershell
python tools/diagnostics/issue114/verify_guard.py --compiler "C:/Program Files/LLVM/bin/clang++.exe"
```

Result: `field interaction: 264 checks passed`. The fixture extracts the actual
C1 handler, operand reader `0x8229E0C8` and original predicate from three explicit
PPC chunks, and compiles them together with the guard.

Coverage includes phases -1/0/1/2, matching/non-matching touch, committed BattleId,
current/next scene 3, both original results at non-C1 callers, rejection PC,
register/stack preservation, unchanged menu/persistent flags during C1, and
restored admission conditions after request clearing. Calls are manually dispatched;
the fixture does not model the resource state machine, controller input or later
DD/end opcodes. Restored admission conditions do not prove controller recovery.

The guard also compiled with the project's formal headers and existing clang-cl
include/define settings, without PCH. The 2288-byte object at
`out/issue114/guard-test/field_interaction.obj` exports a strong symbol for
`sub_829E54F0`. CMake's runtime source glob includes `patches/*.cpp`.
No complete application link or game-runtime test was performed.

## Evidence boundary

The available v0.7.20 log contains 272450 bytes / 3037 lines, with SHA-256
`58DAE6ABC3645FE72F5BB87A08F40095C7114C8F92436208778AD23A49C7E2F6`.
It records completed battle phases without the script/menu/controller state needed
to identify the softlock mechanism. Local evidence is retained under ignored
`out/issue114/`; private game assets and extracted function bodies are not part of
this document.

Implementation and focused checks were complete. Full-game reproduction, gameplay
validation, player acceptance and publication remain pending; issue #114 stays open.

## 2026-10-03: guard removed (caused #171)

The guard was removed after it was traced as the cause of
[#171](https://github.com/freefrank/LostOdysseyRecomp/issues/171): in the Lunar
Palace (`ev7_0_scrw`, disc 4) the two platforms that start with pillars on them
rose by themselves after a random battle, leaving the pillars floating.

C1 is a general "object A touches object B" query (A and B come from the
script operands; B or any of its parents must be in A's contact list), not only
a player interaction. Platform object scripts (records 35/36, persistent bits
`0x1E2F`/`0x1E30`) poll `C1 0x30, <pillar>` every frame and treat "no pillar
touching" as a removal, which raises the platform and sets its bit. A random
encounter queues the request about 0.1 s before the field unloads; during that
window the field VM still runs, so the guard turned every C1 poll into a
rejection. The platform script stored its pending raise in its frame (`L2 = 1`),
the frame survived the battle, and the raise finished about 1.5 s after the
field returned. The original predicate only reports a battle once it is
committed, when field scripts are no longer running, so the original game never
returns a false negative to these polls.

Checks with the #171 reporter's save (v0.8.0-equivalent build plus diagnostics,
120 FPS): the platform stays down with no encounters; one normal battle inside
the room raises it every time; the same build without
`patches/field_interaction.cpp` keeps it down after a battle. A battle-request
log showed the request at phase 1 from 67.90 s, the field unloading at
68.01 s, and the request already cleared when the field returned.

Narrowing the guard to the player operand (`C1 1, …`, which the containers use)
was considered and rejected: ev7 also polls `C1 1/2/3, <platform>` for the
platform rides, so any per-frame C1 poll can still read a false result in that
window. With the guard removed, the original #114 window (a touch accepted while
an encounter is queued) can occur again; a future fix should defer the
encounter while a touch-started script runs instead of changing C1's result.

# Cutscene tour and 45 reviewed jitter pairs — 2026-10-04

After #212 the maintainer asked for a tour of every real-time cutscene to look for shadow and jitter problems. This note records how the tour ran, what it found, and the 45 VS/PS pairs mapped from it.

## How the tour ran

- **Index.** `LO.fpi` lists 195 real-time event packages (`bin/xenon/event/rt_*`): disc 1 has 65, disc 2 32, disc 3 60, disc 4 38. OCR of the 36-page Scenario Jump list from the event-debug map (`z0g_9_scrw`) mapped 178 of them to a page and row. The other 17 were not toured.
- **Hosts.** Disc 1 ran on the PC (RTX 5080, D3D12, DLSS Balanced + TAA + AO). Disc 3 ran on psvita (Radeon 8060S, the Windows build under Proton, FSR + TAA + AO). Discs 2 and 4 ran on the MacBook (M1 Max, Metal, MetalFX + TAA + AO).
- **Build.** A diagnostic build (not committed) added map jump, disc request and a file-triggered F1 capture. It also logged, per 60-frame window, every mapped scene draw rejected with Camera/Depth/ShadowDepth mismatch or a near-full incompatible viewport.
- **Per cutscene.** The driver navigated the menu by OCR and the arrow cursor and played up to 240 s. It took a screenshot every ~2 s, and the maintainer reviewed numbered contact sheets.

## Findings

- **No visible shadow defect.** The maintainer reviewed every sheet and saw no shadow problem.
- **`702c` mismatches, flagged for review.** Eight cutscenes logged one mapped draw of VS `702c` per frame rejected with `CameraMismatch`. They are RT_270_1A/2A/3A and RT_261B on the Mac and RT_006B on the PC, plus three single-frame transients. Nothing visible came of them, and they are left as they are.
- **47 unmapped pairs.** The cutscenes logged 47 runtime suspect pairs. Two are the held slot-3 depth writers (`4353`, `83b4`). The camera for those shows up at c230/c233, not at their oPos slot, so they stay held.
- **RT_099B crash.** RT_099B crashes on both the PC and the Mac after the cutscene, while loading the world map (`xx2_0_scrw`). The fault is a guest read of 0x54 in `sub_82987D20`. It is most likely state that Scenario Jump does not set up; no player has reported it.

## The 45 pairs

`triage_suspect.py` and three parallel reviews of the HLSL (oPos slot, outputs that carry the camera matrix, PS use of clip X/Y) found every pair safe:

- **Clip W or no clip copy (10 pairs).** Eight depth writers and materials read only clip W, or have no clip copy: `1c00/3073` (slot 8), `4cca/e3a4`, `66fe/0fed`, `bd4c/f7ff`, `e275/0715` (slot 7), `a8d2/00af` (slot 4), and `ac32/dfdf`, `eb5f/38b9` (slot 0). Two more are sky pairs with their VS's existing policies: `bda4/6426` (the depth pass under the `2496/42b1` light) and `db23/1693`.
- **Per-light passes (30 pairs).** Each samples tex0, the light attenuation, at ScreenPosition and multiplies it into the light, as `e810/c44e` from #212 does. Eleven are further `e810` partners, including `fe31`, which no longer needs the constant-sample gate. Eighteen use other vertex formats at slot 7, and `cabb/b1d4` is at slot 8 (its c7 is a UV transform).
- **Cutscene characters (5 pairs).** `2580/957d`, `2580/8724`, `6261/aab1`, `6261/0aa1` and the depth writer `6960/c468` blend bones in branches. After the blend they multiply by c233–c236 outside every branch, like `3148`/`118a`. Their only clip copy is `o3` or `o0`.

Exact pairs only: `ScreenLightPairs` (slot 7), a new `ReviewedPairs` table carrying each pair's slot, and two `SkyMaterialPairs`. No VS is mapped as a whole, so unreviewed PS partners stay held.

## Validation

- `LoTemporalJitterTest --captured-cutscene-tour` runs on the first logged banks of all 45 pairs (`tools/tests/tour_cutscene_20261004_capture.h`). It checks the slot, that jitter changes only the x/y of the camera rows, and that the PS bank stays exact. 41 pairs carry a mapped depth companion with the same camera, and those get the identical jittered matrix. The case also checks rejection on a changed camera. Full test: 6,136,035 checks.
- **Reruns with the mappings.** Every touched cutscene that played logged zero suspects for the mapped pairs: four on the PC, and on psvita and the Mac (counts in the PR). Only the held slot-3 pairs remain. With the same build and the new pairs disabled, RT_028A logged 6 suspects and RT_036B 1.
- **Not shown.** An offline A/B with 8-frame bursts could not show a difference. The two runs captured different shots at the same timestamps, and animation dominates the frame-to-frame change. The pairs were half a pixel off; no visible defect was reported before or after.

## Follow-up: RT_183B lights

The psvita rerun with the 45 pairs logged two new pairs in RT_183B: `3fc1/6383` and `c559/6c26`. Both draw over the `66fe` depth writer's geometry with the same camera, so they were left half a pixel behind it once `66fe` was jittered. They are per-light passes like the other `ScreenLightPairs`. The VS writes the slot-7 clip to `o4`, and the PS samples tex0 at `i4.xy/i4.w` and multiplies it into the light. `i2` is a sign flag, not a clip copy. Both are now `ScreenLightPairs`, with fixture entries in the tour capture. A second psvita rerun of RT_183B logged no suspects.

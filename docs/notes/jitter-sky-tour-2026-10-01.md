# Sky pairs and a depth VS from the map tour — 2026-10-01

Status: mapped in `temporal_scene.h`, checked by `LoTemporalJitterTest --captured-tour-sky`. Not rechecked in the game, and no player has reported these spots yet.

## How they were found

The renderer logs a "temporal suspect" line for each unmapped scene-camera VS/PS pair that writes the main scene depth under jitter or repeats a jittered depth draw's geometry. Until now these lines arrived only from players who happened to stand in the right place. On 2026-10-01 a diagnostic build visited the game's maps one after another with TAA on:

- Of the 316 native map definitions it reached 264. The others did not resolve on any disc, or had no player to jump from.
- Each map got about 7 s, with the camera turning.
- The run collected 36 suspect pairs. The tour driver and its uncommitted map-jump bridge are kept outside the repository.

`tools/capture_analysis/triage_suspect.py` then reviewed every pair against an HLSL dump of the 28,484 known shaders (`LO_SHADER_HLSL_DIR` at startup preparation):

- the `oPos` matrix and the outputs that carry it;
- PS reads of the clip copy's X/Y, and clip-derived texture coordinates;
- the logged depth companion.

The pairs below passed the review; the rest stay held for the reasons the triage printed.

## Sky pairs: the #67 VS with six more PS

All six are `depth_writer_after_jittered_geometry` suspects of VS `bda41a11626a545c` over the jittered `b030ab4e17a20783` depth with the same world and camera, the #67/#121/#130 class:

- The VS's c7-c10 reach only `oPos` and the `o1` copy (HLSL 435-441).
- Each PS reads only `i1.w`.
- Each PS samples only at mesh UVs; the fetch lines are listed in the triage output.
- The only statement the clip review could not parse is the translator's host debug view (`xeFlags`, `i15`).

| PS | Seen at |
|---|---|
| `e086f5f676c72482` | Numara Palace - Facade, Ghost Town, Armored Vehicle, Ipsilon Mountains - Mountain Hut |
| `e2b89a553d00ef47` | Saman - Main Street, Port of Saman |
| `72bcd05d7ab61ce1` | Experimental Staff Marine Division, Ice Canyon - Glacier Fang, The White Boa - Queen's Room |
| `bbbac6693e441760` | Uhra - Amphitheater of the Sky, Grand Staff - Central Connector (also in the user's v0.7.25 logs) |
| `40496f0784d54689` | The White Boa - Main Deck |
| `1693d368b809e65d` | Aurora-Bound Train - Engine Car |

Each is an exact `SkyMaterialPairs` entry with the #67 motion fallback. The 2026-09-30 telemetry sweep had listed e086, 72bc, 1693, e2b8 and a9e9 as unreviewed partners of this VS; the tour gave runtime evidence for four of them and found 4049 and bbba. `bda4/642665c9452ccafb` (seen on map `gh7_0_scrw`) samples at clip-derived coordinates and stays held.

## Depth VS `b9b8056050a4c194`

This VS draws to the main scene depth with the scene camera at slot 4 (Uhra - Army Sewers, Eastern Gohtza Railroad Track). Its HLSL is the mapped alpha-tested depth VS `8d3c80b318235b22` line for line, except for the vertex stride (12 dwords instead of 10). It joins `8d3c` in the slot-4 group, so its depth now gets the same jitter as the materials drawn over it.

## Validation

- `LoTemporalJitterTest --captured-tour-sky`: 6 × 33,029 checks with the logged banks, max jitter error 0.0001–0.002 px against an old separation of about 0.488 px. It also checks that `b9b8` and `8d3c` map to slot 4.
- The case fails against the previous `temporal_scene.h`; the full run passes (4,298,844 checks).
- Not rechecked in the game.

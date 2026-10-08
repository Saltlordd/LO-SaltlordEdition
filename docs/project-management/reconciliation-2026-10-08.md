# Project reconciliation — 2026-10-08

The maintainer closed three Issues as completed on 2026-10-07 and set their Project Status to Done by hand. The next plan reported three Status conflicts (remote `Done`, manifest `Todo` / `Awaiting validation` / `In Progress`). Following the README, the manifest takes the remote value; nothing on the Project was overridden.

| Item | Issue | Change | Evidence boundary |
| --- | --- | --- | --- |
| `issue-167-cutscene-audio-desync` | [#167](https://github.com/freefrank/LostOdysseyRecomp/issues/167), closed 2026-10-07 19:28:19 UTC | Status Done; Delivery `Not started` → `Research complete`; Evidence updated | A reporter said the drift was not extremely bad and the Japanese voice lip sync is wrong by design; the maintainer asked for a new Issue if the sync is worse than it should be. No runtime change. |
| `issue-214-mali-black-screen` | [#214](https://github.com/freefrank/LostOdysseyRecomp/issues/214), closed 2026-10-07 19:27:08 UTC | Status Done; Evidence updated (Delivery Released, Release v0.8.37 unchanged) | PR #241's BC decode fallback; no Mali device was run and no reporter confirmation is recorded. |
| `issue-172-fsr-scaling-performance` | [#172](https://github.com/freefrank/LostOdysseyRecomp/issues/172), closed 2026-10-07 22:59:34 UTC | Status Done; Evidence updated (Delivery Released, Release v0.8.44 unchanged) | Adds PR #305 (merged 2026-10-08, not yet released); the reporter was asked on 2026-10-08 to rerun after the next release. No reporter confirmation is recorded. |

Release keeps the first release that carried the delivery; the unreleased #305 is in the #172 Evidence only.

## Apply and readback

- Before: `{"created": 0, "updated": 0, "unchanged": 291, "conflicts": 3, "operations": 0}`. `sync-state.json` Status for the three items was set to the remote `Done`.
- Plan after the manifest edit: 3 items, 4 operations (#167 Delivery and three Evidence fields), 0 conflicts. Applied.
- Plan after apply: `{"created": 0, "updated": 0, "unchanged": 291, "conflicts": 0, "operations": 0}`.
- Readback with `gh project item-list 3 --owner freefrank`: #167 Done / Research complete / no Release (Evidence 546 characters); #172 Done / Released / v0.8.44 (762); #214 Done / Released / v0.8.37 (768).

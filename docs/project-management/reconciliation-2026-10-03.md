# Project #3 reconciliation — 2026-10-03 (PM4 removal)

Project: [Lost Odyssey Recomp Roadmap](https://github.com/users/freefrank/projects/3)

The Project and the roadmaps still listed "Remove the PM4 translator" as a v0.8.0 item that had not started, although the native front-end work had been built and closed on a branch. On 2026-10-03 the maintainer moved full PM4 removal out of v0.8.0. This pass records that decision, corrects the native migration records it depends on, and copies three Issue closures that were already on the Project into the manifest. Only Project items changed on GitHub: no Issue, pull request, comment, release or branch was touched. The 2026-10-02 [v0.7.35 reconciliation](reconciliation-2026-10-02.md) still applies to everything not listed here.

## Result

The Project still holds 249 items. Four items were updated through `tools/project_management/sync.py --apply` (10 operations: 2 draft bodies, 1 draft body with fields, and field values), and the `Release` field of `remove-pm4-translator` was cleared with one `clearProjectV2ItemFieldValue` call, because the sync tool leaves null fields alone. The read-back of the seven affected items found 0 mismatches against the manifest. A read-only plan afterwards returned 248 unchanged, 0 conflicts, 0 operations.

## Updated items

| Key | Before (Status / Delivery / Release) | After | Basis |
| --- | --- | --- | --- |
| `remove-pm4-translator` | Todo / Not started / v0.8.0 | Paused / Deferred / none; body and Evidence | Maintainer decision on 2026-10-03. The body now records the work done (PRs #83, #84, #89, #98), the negative CPU result and the remaining PM4 dependencies. |
| `native-migration-architecture-boundaries` | In Progress / In progress / none | Done / Implemented / none; body and Evidence | All three criteria were already checked; draft PR #83 was closed on 2026-09-30 and its map fed PRs #84 and #89. |
| `native-renderer-pm4-bypass-prototype` | Done / Implemented / none | fields unchanged; body | The four criteria are now checked, matching the existing runtime checkpoint (PR #84, CmdProc CPU +23.7%, go gate failed). |
| `native-renderer-conditional-coverage-expansion` | Paused / Deferred / none | fields unchanged; Evidence | Adds the native front-end result (PR #89) to the reason for the pause. |

## Manifest-only alignment

| Key | Manifest before | Project and manifest now | Basis |
| --- | --- | --- | --- |
| `issue-114-container-enemy-softlock` | Awaiting validation | Done | Issue #114 closed as completed at 2026-10-02T19:48:35Z. |
| `issue-121-old-sorceress-mansion-sky-flicker` | Awaiting validation | Done | Issue #121 closed as completed at 2026-10-02T15:47:04Z after the reporter wrote that the flicker is gone. |
| `issue-116-linux-nvidia-dlss` | Awaiting validation | Done | Issue #116 closed as completed at 2026-10-03T01:59:19Z. |

The Project had already moved these three to `Done` when the Issues were closed; the first plan of this pass reported them as conflicts and wrote nothing. `sync-state.json` also lacked the title and body hash of the two native drafts above, which made their body updates report conflicts. The first plan showed both bodies equal to the manifest, so the stored title and hash were filled in from the committed manifest before applying.

## Platform and #118 follow-up

A second pass the same night updated three more items through `sync.py --apply` (9 operations, 0 conflicts); the read-back found 0 mismatches and the plan afterwards returned 248 unchanged, 0 conflicts, 0 operations.

| Key | Before (Status / Delivery / Release) | After | Basis |
| --- | --- | --- | --- |
| `experimental-android-platform` | Todo / Not started / v0.8.0 | In Progress / In progress / v0.8.0; body and Evidence | PR #150 merged the Android port on 2026-10-02 and PR #152 the shared SPIR-V pack. Three of four criteria checked; 16 KB pages, audio and controller hot-plug stay open. |
| `linux-aarch64-platform` | Todo / Not started / v0.8.0 | In Progress / In progress / v0.8.0; body and Evidence | PR #60 (dj5927) merged the cross-build path on 2026-10-02. No criterion is complete: the build half of the first exists, but not dependency distribution, CI or a hardware run. |
| `issue-118-sun-occlusion-lens-flare` | Awaiting validation / Released / v0.7.35 | fields unchanged; Evidence | The reporter wrote that v0.7.35 still flickered on an RX 6600; PR #149 is on `main`, unreleased. |

Both roadmaps were brought up to date with the same reads: #114, #116 and #121 closed, #118 open with the PR #149 fix pending, #103 closed, #148 and #151 opened on 2026-10-02 (11 open Issues at about 02:27 UTC on 2026-10-03), and items 9 and 11 of the v0.8.0 plan describe the merged Linux AArch64 and Android work. #103, #148 and #151 have no Project items, and none were created.

## Evidence boundaries

Read on 2026-10-03: the commits on `fix/native-frontend-rework` (local and git.zkx.ca; the branch is not on GitHub), the PR #84 and PR #89 states and the PR #89 closing comment, the PR #83 state, the PR #98 merge, and the Issue states and closure times above. The 70% native draw share, the per-swap count and the 4K120 ABBA numbers are quoted from the PR #89 closing comment and were not measured again. No build, game run or test was run for this pass.

## Records

[items.json](items.json) holds the updated bodies and fields; [sync-state.json](sync-state.json) holds the applied values, the two filled-in draft hashes, and no `Release` for `remove-pm4-translator`. Both [roadmaps](../ROADMAP.md) mark item 8 of the v0.8.0 plan as moved out and list PM4 removal under the later backlog.

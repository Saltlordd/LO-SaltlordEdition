# Project #3 reconciliation — 2026-10-02 (v0.7.35)

Project: [Lost Odyssey Recomp Roadmap](https://github.com/users/freefrank/projects/3)

This reconciliation records the v0.7.35 transition. [v0.7.35](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.7.35) was published at 2026-10-02T09:13:35Z from the tag commit `95f2c89`. All 246 Project items were read, including every draft body and the `Status`, `Delivery`, `Release`, `Source key` and `Evidence` fields. Twelve items were updated and three were created through the GraphQL API, then read back. Only Project items changed on GitHub: no Issue, pull request, comment, release or remote repository state was touched. Publication is not acceptance: the maintainer has not accepted v0.7.35 and no reporter confirmation is recorded. The 2026-09-30 [baseline reconciliation](reconciliation-2026-09-30.md) still applies to everything not listed here.

## Result

The Project now holds 249 items (209 drafts and 40 linked Issues; it held 246). No write failed. [items.json](items.json) and [sync-state.json](sync-state.json) hold 248 source-keyed records; the 249th Project item is the retained keyless Issue #17 link, which is `Done`.

| Status | Before | After |
| --- | ---: | ---: |
| Done | 192 | 193 |
| In Progress | 14 | 16 |
| Awaiting validation | 5 | 7 |
| Paused | 16 | 14 |
| Todo | 19 | 19 |

The writes ran from 09:38:53 to 09:39:36 UTC (53 field values, 5 draft bodies, 3 new drafts). The readback of all 249 items found 0 mismatches against the plan: the other 234 existing items, including #17, were unchanged in field values, titles, draft bodies and Issue state, and the saved views were unchanged. The public Roadmap board shows a `v0.7.35` lane with 11 items (2 Done, 2 In Progress, 7 Awaiting validation).

One follow-up write at 09:52:03 UTC corrected the body of `shader-delivery-v080`: it cited run 126's 75.6 MB AppImage, and now cites the published sizes (Windows ZIP 77,109,842 B, AppImage 76,270,072 B, Flatpak 54,113,384 B; v0.7.25: 254,531,925 B, 253,770,232 B, 231,397,328 B). The readback was repeated with 0 mismatches.

`python -B tools/project_management/sync.py` (read-only plan, report written outside the repository) against the updated manifest returned 248 unchanged, 0 conflicts, 0 operations. Before the manifest update the same plan reported 28 conflicts across the 12 changed keys and overwrote nothing, which is the intended behavior after a direct Project write.

## Updated items (12)

| Key | Before (Status / Delivery / Release) | After | Basis |
| --- | --- | --- | --- |
| `issue-116-linux-nvidia-dlss` ([#116](https://github.com/freefrank/LostOdysseyRecomp/issues/116)) | Awaiting validation / Implemented / none | Awaiting validation / Released / v0.7.35 | PR #120. The reporter's 06:30 UTC log is v0.7.25 with `LO_DLSS_RUNTIME_PATH`, not the packaged path. |
| `issue-118-sun-occlusion-lens-flare` ([#118](https://github.com/freefrank/LostOdysseyRecomp/issues/118)) | Awaiting validation / Validated / none | Awaiting validation / Released / v0.7.35 | PR #122. Checked on an RTX 5080 only; AMD and Intel unchecked. |
| `issue-121-old-sorceress-mansion-sky-flicker` ([#121](https://github.com/freefrank/LostOdysseyRecomp/issues/121)) | Awaiting validation / Implemented / none | Awaiting validation / Released / v0.7.35 | PR #128. Not rechecked in the game; Evidence also names the f6814 cutscene half of the PR. |
| `issue-114-container-enemy-softlock` ([#114](https://github.com/freefrank/LostOdysseyRecomp/issues/114)) | Awaiting validation / Implemented / none | Awaiting validation / Released / v0.7.35 | PR #135 guard. Gameplay validation is pending and the maintainer said the bug may come from the original game. |
| `issue-117-battle-camera-flicker` ([#117](https://github.com/freefrank/LostOdysseyRecomp/issues/117)) | Done / Validated / none | Done / Released / v0.7.35 | PR #132; closed as completed 2026-10-02T04:20:42Z; no reporter confirmation. |
| `taa-jitter-fixes-20261001` | Awaiting validation / Implemented / none | Awaiting validation / Released / v0.7.35 | PRs #128 (cutscene half), #130, #133, #134, #136. Only the PR #136 battle mapping was checked in the game. |
| `flicker-triage-tooling-20261001` | Done / Implemented / none | fields unchanged; Evidence and body | PRs #131, #133, #136, in the v0.7.35 source. Developer tooling: no `Release`, per the tooling records. |
| `shader-delivery-v080` | In Progress / In progress / v0.8.0 | fields unchanged; Evidence and body | PRs #141, #142, #144 shipped; local cache cleanup is not started and stays in v0.8.0. |
| `hdr-output-tonemapping` | Paused / Deferred / none | In Progress / Released / v0.7.35 | PR #145, experimental. The save-state half of #90 is a separate record. |
| `save-state-support` | Paused / Deferred / none | fields unchanged; Evidence | The HDR half of #90 shipped; the maintainer kept the issue open for save states on 2026-10-02. |
| `native-macos-platform` | Paused / Deferred / v0.8.0 | In Progress / Released / v0.7.35 | Commit `a8d1c27` and the first macOS disk image; the source-build path was in v0.7.25. |
| `temporal-phased-p4-dlss-fg-2x` | In Progress / In progress / v0.8.0 | fields unchanged; Evidence | PR #119 shipped (Vulkan DLSS 2× to 6×; Vulkan FSR and MetalFX frame generation only in source builds). |

## Created items (3)

| Key | Fields | Basis |
| --- | --- | --- |
| `release-v0-7-35` | Release, Done / Released / v0.7.35, 2026-10-02 | Publication record: tag, Gitea run 135, the four assets with sizes and SHA-256, and the work included. Done means published, not accepted. |
| `tall-ultrawide-layout-20261002` | Feature, Awaiting validation / Released / v0.7.35 | PR #146. Checked on Windows Vulkan in Uhra only. |
| `portable-shader-pack-contract-20261002` | Bug, Awaiting validation / Released / v0.7.35 | PR #127, with the packs-not-bundled change of PR #144. |

## Judgment calls

Each can be reverted on its own; the previous values are in the `95f2c89` versions of `items.json` and `sync-state.json`.

- **Release item.** The Project has no release item for v0.7.25, v0.7.20, v0.7.15, v0.7.10, v0.7.9, v0.7.2 or v0.7.0: release items stop at v0.5.6, plus `flatpak-release` for v0.7.1, and later releases are recorded through the `Release` field. A `release-v0-7-35` record was still created because the Project does use `Release` items. The other v0.7.x releases were not backfilled.
- **macOS.** `native-macos-platform` moved from the v0.8.0 lane to v0.7.35, as `flatpak-release` did for a package delivered ahead of the v0.8.0 plan. Set `Release` back to v0.8.0 if the remaining macOS validation should stay in that plan.
- **HDR.** `hdr-output-tonemapping` is In Progress / Released, as shipped-with-remainder records such as `issue-40-controller-mod-support` are. Its unchecked criteria (scene and exposure contract, UI and movie behavior) stay open.
- **Plan items.** `shader-delivery-v080` and `temporal-phased-p4-dlss-fg-2x` keep Release v0.8.0 and their In Progress state; the shipped parts are recorded in Evidence. PR #119 appears only in P4's Evidence; its body was not changed.
- **Pack contract.** Awaiting validation rather than Done: no first launch on the packaged v0.7.35 is recorded.
- **Split PRs.** PR #128's cutscene half is recorded under `taa-jitter-fixes-20261001` and its mansion-sky half under #121.
- **Docs and CI PRs.** #129, #137, #139, #140 and #143 have no Project items and none were created; the #137 deferral is already recorded in `character-shadow-flow-2026-10-01`.

## Evidence boundaries

Read directly from GitHub on 2026-10-02: the release (published 09:13:35Z, latest, not a prerelease), its four assets with sizes and GitHub digests, the annotated tag `v0.7.35` (08:59:50Z) pointing at `95f2c89`, the merge state of the 22 pull requests the release includes and the first-parent history from v0.7.25, the v0.7.25 asset sizes, the Issue states, and the maintainer's 09:16 UTC v0.7.35 notices on #90, #116, #118 and #121.

Supplied with the request and not re-verified here: the outcome of Gitea Actions run 135 (five jobs succeeded; its Linux job's shader-pack contract check passed; Gitea was outside the authorized scope); the #116 reporter's log details (RTX 4070 SUPER, DLAA active with `LO_DLSS_RUNTIME_PATH` on v0.7.25; the attachment was not downloaded); that the macOS image was built on the maintainer's M1 Max from the tag and uploaded by hand; and the maintainer's statement that an in-game check of the #114 guard was done in a Codex session, which has no repository record.

Not established: reporter confirmation for #114, #116, #117, #118 and #121, and maintainer acceptance of v0.7.35. Validation limits stay as recorded in each item: HDR was confirmed on-device by the maintainer without a recorded platform, backend or display, and Linux and Mac HDR displays are untried; the tall layout was checked only on Windows Vulkan in Uhra; macOS ran only on one M1 Max with macOS 26.6.2; the Linux shader-pack download path was not run.

## Left unchanged

- The live Roadmap view is a Board grouped by Release with Status columns (last changed 2026-09-09), not the timeline layout recorded on 2026-09-08. [project.json](project.json) now records the live configuration; the sync tool reads only its repository binding.
- The `save-state-support` body contains literal `` `n `` sequences instead of line breaks, in the Project and in the manifest. It was not repaired here.
- Three differences between `sync-state.json` and the live Project predate this pass and are tolerated because the manifest desires null there: the `rx9060xt-kaim-shadow-report` Evidence and the `dlc-import-support` and `native-object-motion` start dates.
- Shipped work that the Project never recorded was not backfilled: #74 is `Done` / `Implemented` without a `Release` although PR #106 shipped in v0.7.25, and #102, #105 and PR #109 have no items.
- [import-audit.md](import-audit.md) and the frozen table in [backlog-coverage.md](backlog-coverage.md) keep their 2026-09-08 values; the latter now carries a note about superseded rows such as `hdr-output-tonemapping`.

## Records

The local manifest was updated in [items.json](items.json) (12 records changed, 3 added; `source_refs` extended for the four drafts whose Sources changed) and [sync-state.json](sync-state.json) (14 records changed, 3 added). Two of the 14 are not Project changes: the stored body hashes of `pc-backend-release-readiness` and `4k-runtime-performance-diagnosis` were stale although their bodies matched the Project, and were realigned with no content change. [project.json](project.json) records the live Board view without a schema change, and [README.md](README.md) and [backlog-coverage.md](backlog-coverage.md) point to this record. These local changes are uncommitted.

The plan, write log and full before and after snapshots are kept outside the repository, in the session scratchpad `C:\Users\FREEFR~1\AppData\Local\Temp\claude\C--Users-freefrank-ownCloud-Git-LostOdysseyRecomp\5d9461ec-799e-4ee0-a033-b940f2f45137\scratchpad` (temporary and not committed): `plan_out2.txt` (every field and body diff of the 09:38 batch; the 09:52 correction is the one described under Result), `apply_log_v0735.jsonl` (each write, including the correction), `before_pages.jsonl` and `pre_apply_pages.jsonl` (state before the writes), `after_pages.jsonl` (state after), `views_before.json` and `views_after.json`, `sync_sim/out/report-patched.json` (the plan against the updated manifest) and `sync_ctrl/out/report-ctrl.json` (the 28-conflict control run).

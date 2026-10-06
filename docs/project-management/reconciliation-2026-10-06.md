# Project reconciliation — 2026-10-06

Receipt for the maintainer's v0.9.0 lane decisions of 2026-10-06 and four stale manifest records. Only Project items and the two roadmaps changed; no Issue, pull request, comment, build or game run was touched.

## Decisions applied

- **Character shadows** (`character-shadow-flow-2026-10-01`): the maintainer reported the shadows fixed. The shadow-projection fixes of v0.8.10 to v0.8.30 (PRs #186, #197 and #218) shipped in the meantime; no capture was taken and no separate fix was made. Paused / Deferred / v0.9.0 → Done / Released / v0.8.30. Both roadmaps mark the v0.9.0 bullet `[x]` with the same note.
- **Linux AArch64** (`linux-aarch64-platform`): moved past v1.0.0. No ARM64 package is planned before then; the cross-build path of PR #60 stays in the source. In Progress / In progress / v0.9.0 → Paused / Deferred / After v1.0.0 (the Release field is free text). Both roadmaps record the move on the v0.8.0 item 9 and list the packaging in the later backlog.

## Stale records corrected

The first plan reported four Status conflicts where the Project already held the maintainer's newer value; the manifest was updated to match and the other fields brought in line:

| Key | Was (manifest) | Now (manifest and Project) |
| --- | --- | --- |
| `issue-49-physical-pixel-window-coords` | Todo / Not started | Done / Deferred (closed 2026-10-05; PR #230 in v0.8.30 delay-loads d3d12/dxgi) |
| `issue-30-dof-toggle-slider` | Todo / Not started | Done / Released / v0.8.30 (PR #228) |
| `issue-85-minimap-toggle` | Paused / Deferred | Done / Released / v0.8.30 (PR #229, README) |
| `issue-114-container-enemy-softlock` | Paused | Awaiting validation / Implemented / Next release (PR #243, merged 2026-10-06; decision-table test only, no in-game run) |

## Plan and readback

First plan: 2 updated, 4 conflicts. After the manifest corrections: 6 updated, 254 unchanged, 0 conflicts, 21 operations. `--apply` succeeded; the Project read back the six items with the values above, and a new plan showed 260 unchanged and 0 operations.

## v0.8.37 release

Receipt for the v0.8.37 release record (published 2026-10-06T07:46:23Z, tag commit `bfa6c824`, not accepted). Manifest changes only; the Project was not written, no Issue, pull request or comment was touched, and no game was run. The live Issue and release state was read with `gh` at about 07:49 UTC.

| Key | Change |
| --- | --- |
| `release-v0-8-37` (new) | Kind Release, Done / Released / v0.8.37, 2026-10-06. Publication facts, the Gitea run (318, API id 710), the five assets with sizes and SHA-256 values, the macOS image checks, the work included and the open acceptance items. There is no v0.8.30 release record in the manifest (the latest earlier one is `release-v0-8-6`), so the shape follows that one |
| `issue-114-container-enemy-softlock` | Awaiting validation stays; Delivery Implemented → Released; Release Next release → v0.8.37. PR #243 has a decision-table test only. Live state: the Issue is **closed** (the maintainer closed it at 2026-10-06T07:11:17Z, after the 07:01 UTC comment that the fix ships in the next release), not open as the request assumed; the manifest keeps Awaiting validation because no reporter confirmation and no in-game run exist |
| `issue-54-japanese-cutscene-language` | Done stays; Delivery Awaiting validation → Released; Release v0.6.3 → v0.8.37. The 2026-10-06 cause (an upper-case voice code from the host language hook) and PR #242 are in the evidence and in a dated body section; the 2026-09-26 text is kept |
| `issue-214-mali-black-screen` (new) | Bug, Awaiting validation / Released / v0.8.37. The Issue is open (closed by the PR #241 merge at 05:38:59Z, reopened by the maintainer at 06:36:06Z); no Mali device ran the fix |
| `issue-199-android-fullscreen` (new) | Feature, Done / Released / v0.8.37 (closed by the PR #240 merge at 05:49:55Z); no device with a display cutout was checked |
| `issue-220-cgi-voice-language` (new) | Bug, Done / Released / v0.8.37 (closed by the PR #242 merge at 06:46:54Z); checked from a trace, not by listening |

No record exists yet for #237 (closed with PR #238, shipped in v0.8.37) or the earlier v0.8.7 to v0.8.30 releases; they are not added here.

Plan, `python -B tools/project_management/sync.py` (plan only, `--apply` not run): 4 created, 2 updated, 258 unchanged, 0 conflicts, 43 operations. The two updates are `issue-114-container-enemy-softlock` and `issue-54-japanese-cutscene-language`; the four creations are the release record and the three Issues above. The apply and the readback are for the maintainer's session.

Apply notes: the first `--apply` stopped at the #114 field batch with `Column value must be a valid value for text column`; the record's Evidence had 1,105 characters, above the Project text-field limit (1,024). The Delivery and Release values of that batch had already been written. The Evidence was shortened to 725 characters and the apply completed (4 created, 1 updated). The following plan showed one conflict: `issue-214-mali-black-screen` Status was `In Progress` on the Project (set by the Project's own workflow when the maintainer reopened #214 at 06:36 UTC) while the manifest wanted `Awaiting validation`; the tracked value was aligned to the remote and the desired value applied. A final plan showed 264 unchanged and 0 operations.

## v0.8.39 release

Receipt for the v0.8.39 release record (published 2026-10-06T08:48:26Z, tag commit `fd7d82ce`, not accepted). Manifest changes only; the Project was not written, no Issue, pull request or comment was touched, and no game was run. The live release state and asset digests were read with `gh release view v0.8.39` and `git ls-remote`; the PR bodies of #248 and #249 were read with `gh pr view`.

| Key | Change |
| --- | --- |
| `release-v0-8-39` (new) | Kind Release, Done / Released / v0.8.39, 2026-10-06. Publication facts, the Gitea run (335, API id 727), the five assets with sizes and SHA-256 values, the macOS image checks, the two PRs included and the open acceptance items. The shape follows `release-v0-8-37` |
| `shader-delivery-v080` | Status Todo → Done; Delivery In progress → Released; Release v0.9.0 → v0.8.39. The last step (cache cleanup, PR #248, merged 07:50:09Z) is checked in the body, a dated "Released" section was added and the Evidence carries the measurement (Proton Direct3D 12 only) and "not accepted" |
| `pipeline-first-use-stalls` (new) | Feature, Graphics performance, In Progress / In progress / v0.9.0. P0 and P1 (PR #249, merged 08:24:01Z) shipped in v0.8.39 with the psvita and M1 Max numbers; P2 to P4 are open. No item for this plan existed in the manifest, so it was created. Delivery is "In progress", not "Released", because three of five phases are not started |

No Issue is linked to PR #248 or #249, so no Issue record changed. #237 and the v0.8.7 to v0.8.30 release records are still not in the manifest.

Plan, `python -B tools/project_management/sync.py` (plan only, `--apply` not run): 2 created, 1 updated, 263 unchanged, 0 conflicts, 24 operations. The two creations are `release-v0-8-39` and `pipeline-first-use-stalls`; the update is `shader-delivery-v080`. The apply and the readback are for the maintainer's session.

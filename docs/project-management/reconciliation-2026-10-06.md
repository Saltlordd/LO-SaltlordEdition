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

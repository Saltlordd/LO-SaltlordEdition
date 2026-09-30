# Project #3 reconciliation — 2026-09-30

Project: [Lost Odyssey Recomp Roadmap](https://github.com/users/freefrank/projects/3)

This reconciliation read the initial 233 Project items, including every draft body and the `Status`, `Delivery`, `Source key`, `Release`, and `Evidence` fields. It applied reviewed Project records, fields and draft text, then read back all 237 resulting items at 2026-09-30T07:28:19Z. It did not change Issues, pull requests, comments, repository access, commits, or releases.

## Result

The Project contains 237 items after the reconciliation. The synchronized local manifest contains 236 source-keyed records: 187 `Done`, 14 `In Progress`, 16 `Paused`, and 19 `Todo`. The remaining Project item is an older source-keyless Issue #17 link, `[Feature] Ultrawide (21:9)`, which duplicates the separately tracked `ultrawide-fov-layout` record. It is retained as an external duplicate rather than adding a second local record for the same Issue URL.

The remote Project field totals are 188 `Done`, 14 `In Progress`, 16 `Paused`, and 19 `Todo`. The one-item difference is that retained source-keyless `Done` link.

The final readback ran `python -B tools/project_management/sync.py` without `--apply` and returned 236 unchanged manifest records, with zero planned updates, creates, or conflicts.

## Applied corrections

Twenty-six Project records changed. Four were added: linked records for open Issues #74, #85, and #88, plus the source-linked save-state backlog record for Issue #90. Fourteen existing records changed workflow status:

| Transition | Records | Basis |
| --- | ---: | --- |
| `Todo` → `Done` | 3 | Released D3D12 `.lospd` shader pack, D3D12 FSR FG, and D3D12 DLSS FG. |
| `Todo` → `In Progress` | 1 | Capability-gated diagnostic dynamic MFG path is implemented; validation remains open. |
| `Todo` → `Paused` | 9 | Deferred frame-generation backlog, the v1.0 HUDless/UI handoff, the known Streamline exception, four conditional native-migration follow-ups, HDR, and macOS. |
| `In Progress` → `Done` | 1 | Bounded PM4-bypass prototype completed with a negative performance result; expansion is not authorized. |

The remaining eight records received current evidence, source-key, or draft-body corrections without a final status transition. They include P0 (validated Gate 1 with remaining lifecycle scope), P3 and P4 (merged `81fe304` / PR #72 with bounded runtime evidence and remaining validation), the four linked historical Issues that were missing a `Source key`, and the F1 archive-size record. Its existing maintainer `Done` status was retained because its historical draft evidence alone did not provide current contrary code or runtime evidence.

Open Issue #74 is `In Progress` / `Implemented`: the maintainer reproduced the split-party Save Anywhere failure and added warning commit `2e1c547` after the v0.7.20 tag. The warning is not in that published package and does not restore affected saves or prevent the unsafe save. Open Issues #85, #88, #90, and #96 are retained as `Paused` / `Deferred` backlog or platform work. Their comments are recorded as scope evidence; an interest request, a future intent, or an external fork is not recorded as current main-repository implementation. HDR and macOS reuse their existing records; save states is a separate deferred draft because Issue #90 also covers HDR.

The former local `Todo`/`In Progress` records that a prior Project readback already marked `Done` were synchronized locally without changing their remote fields. Their `Delivery` values remain as maintained in the Project. In particular, `resource-only-pso-preparation` and `shader-translation-failures` keep maintainer `Done` while their older Delivery evidence is not promoted to release or acceptance proof.

## Evidence boundaries

The released D3D12 shader pack is `shaders/portable_dx12.lospd` in [v0.7.10](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.7.10). Windows D3D12 DLSS/FSR FG shipped in [v0.7.9](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.7.9); the maintainer accepted the v0.7.15 feature set, within its recorded limits. Dynamic MFG is recorded as implemented diagnostic work, with API, hardware, pacing, visual-quality, physical-display, and failure-injection coverage still open.

P0 is still `In Progress` despite the accepted Gate 1 because physical-display classification, failure injection, and settings-restart work remains in its scope. The `PRESENT-AFTER-WRITE` SDK exception is separately `Paused` / `Deferred`; it was not erased or treated as visual acceptance. P3/P4 remain `In Progress` for their lifecycle and provider-combination validation.

No completion was inferred solely from a closed Issue, a build, a published artifact, or a checkbox. The Project's existing maintainer `Done` decisions were preserved where the current record supplied no contrary current evidence.

## Reconciliation records

The exact before and after Project readbacks, reviewed proposed changes, application reports, and final no-op plan are retained under `out/docs-normalization-20260930/project/`. The local manifest is updated in [items.json](items.json), and [sync-state.json](sync-state.json) now records the final field values and draft content hashes. These local changes are uncommitted.

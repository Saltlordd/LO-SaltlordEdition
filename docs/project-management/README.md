# Project synchronization records

The [Lost Odyssey Recomp Roadmap](https://github.com/users/freefrank/projects/3) is a public maintainer Project. The [2026-09-30 reconciliation](reconciliation-2026-09-30.md) records the full readback, applied corrections, current item totals and retained external duplicate. The initial 145-item import and the later 218-record local checkpoint are historical counts. `items.json` and `sync-state.json` now reflect the reviewed managed items; matching item counts alone never prove field agreement. The English and Chinese roadmaps are maintained together. Follow this document and applicable workspace/session `AGENTS.md` instructions for on-demand maintenance.

## Files

| File | Purpose |
| --- | --- |
| [project.json](project.json) | Verified repository binding, field IDs/options and saved Roadmap configuration |
| [items.json](items.json) | Reviewed work items, stable source keys, source references and desired field values |
| [sync-state.json](sync-state.json) | Remote item IDs and last synchronized values used to detect conflicting edits |
| [import-audit.md](import-audit.md) | Initial import scope, coverage and actual readback result |
| [reconciliation-2026-09-30.md](reconciliation-2026-09-30.md) | Current normalization receipt, applied transitions and remaining evidence boundaries |

Update the affected manifest records from live Issues, accepted requirements, commits and recorded validation. Preserve source keys so wording changes update existing items. Reconcile the matching passages in both roadmaps; the script does not interpret prose or decide acceptance. `docs_sync` maintains the implementation and release narrative in [STATUS](../STATUS.md) and related documents.

## Run from the repository root

```powershell
python -B tools/project_management/sync.py
python -B tools/project_management/sync.py --apply
```

Python 3.10+ and a signed-in `gh` CLI with Projects access are required. The first command reads GitHub and writes a local plan to `out/project-management/sync-report.json`. The second applies reviewed changes to this Project. It links existing Issues and creates/updates Project drafts and mapped fields; it does not modify Issue bodies, Issue state or comments.

`Status` tracks work progress. `Delivery` distinguishes implementation, validation, release and deferred work. Explicitly deferred work uses `Paused` / `Deferred`; unscheduled research is not an active implementation. Preserve maintainer `Done` decisions unless new contrary evidence exists, and describe remaining coverage separately. Old incomplete Delivery fields do not justify reopening a completed record or inventing a release. Draft bodies retain detailed evidence; linked Issues use the `Evidence` field for the current acceptance boundary without rewriting the report. Historical dates record the cited work or report checkpoints; unknown future dates remain unset.

Keep `sync-state.json` with the manifest. Remote text or field changes that disagree with the last synchronized value produce conflicts and a nonzero exit instead of being overwritten. Investigate missing/deleted/archived items and uncertain writes before retrying. Null desired fields are left alone, preserving manual scheduling. The tool never deletes unlisted items.

After applying, read back the affected item contents and fields, then run the plan again to confirm no unintended operations remain. A successful API call alone is insufficient verification. This bookkeeping does not require rebuilding or rerunning the game.

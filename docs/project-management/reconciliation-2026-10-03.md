# Project #3 reconciliation — 2026-10-03 (PM4 removal)

Project: [Lost Odyssey Recomp Roadmap](https://github.com/users/freefrank/projects/3)

The Project and the roadmaps still listed "Remove the PM4 translator" as a v0.8.0 item that had not started, although the native front-end work had been built and closed on a branch. On 2026-10-03 the maintainer moved full PM4 removal out of v0.8.0. This pass records that decision, corrects the native migration records it depends on, and copies three Issue closures that were already on the Project into the manifest. Only Project items changed on GitHub: no Issue, pull request, comment, release or branch was touched. The 2026-10-02 [v0.7.35 reconciliation](reconciliation-2026-10-02.md) still applies to everything not listed here. The final section, [v0.8.0 and v0.8.5 releases](#v080-and-v085-releases), records the two releases published later the same day.

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

## P3 to the backlog, P4 accepted

On the maintainer's decisions the same night, P3's remainder moved to the backlog and all P4 validation was recorded as passed. Two items were updated through `sync.py --apply` (6 and then 4 operations, 0 conflicts) and the `Release` of P3 was cleared with `clearProjectV2ItemFieldValue`; the read-backs found 0 mismatches and the plan afterwards returned 248 unchanged, 0 conflicts, 0 operations.

| Key | Before (Status / Delivery / Release) | After | Basis |
| --- | --- | --- | --- |
| `temporal-phased-p3-fg-infrastructure` | In Progress / In progress / v0.8.0 | Paused / Deferred / none; body and Evidence | The foundation (PR #72) is merged and used by the shipped frame generation. Its synchronization hazard is inside Streamline and stays under `streamline-vulkan-present-after-write`; resize, mode-switch and exit validation moved to P4. |
| `temporal-phased-p4-dlss-fg-2x` | In Progress / In progress / v0.8.0 | Done / Validated / v0.8.0; body and Evidence | Maintainer acceptance of all P4 validation on 2026-10-03, including the lifecycle validation moved from P3. All criteria are checked on that acceptance; no new run is attached. |

## Platform decisions

Maintainer decisions on 2026-10-03, recorded in the `Evidence` field only (3 operations through `sync.py --apply`, read back with 0 mismatches):

- `native-macos-platform`: macOS packages stay ad-hoc signed; Developer ID signing and notarization will not be done. [MACOS_RELEASE](../MACOS_RELEASE.md#signed-release-workflow) keeps the helper for reference.
- `linux-aarch64-platform`: ships as is, without an ARM64 hardware validation gate.
- `experimental-android-platform`: the maintainer will add Android CI later. In a later pass the maintainer deferred Android out of v0.8.0 because of an upstream Qualcomm bug: the item moved from In Progress / In progress / v0.8.0 to Paused / Deferred with `Release` cleared (`clearProjectV2ItemFieldValue`), and its body records the deferral. The bug details are not recorded in the repository.

## Shader cache cleanup deferred

The maintainer deferred the local shader-cache cleanup out of v0.8.0. `shader-delivery-v080` moved from In Progress / In progress / v0.8.0 to Paused / Deferred with `Release` cleared; its body and Evidence record the deferral, and the parts shipped in v0.7.35 are unchanged.

## Evidence boundaries

Read on 2026-10-03: the commits on `fix/native-frontend-rework` (local and git.zkx.ca; the branch is not on GitHub), the PR #84 and PR #89 states and the PR #89 closing comment, the PR #83 state, the PR #98 merge, and the Issue states and closure times above. The 70% native draw share, the per-swap count and the 4K120 ABBA numbers are quoted from the PR #89 closing comment and were not measured again. No build, game run or test was run for this pass.

## Records

[items.json](items.json) holds the updated bodies and fields; [sync-state.json](sync-state.json) holds the applied values, the two filled-in draft hashes, and no `Release` for `remove-pm4-translator`. Both [roadmaps](../ROADMAP.md) mark item 8 of the v0.8.0 plan as moved out and list PM4 removal under the later backlog.

## v0.8.0 and v0.8.5 releases

Project: [Lost Odyssey Recomp Roadmap](https://github.com/users/freefrank/projects/3)

After the platform and shader-cache decisions above, two releases were published on 2026-10-03: [v0.8.0](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.8.0) at 06:41:51Z (tag commit `eddbb7d`) and [v0.8.5](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.8.5) at 18:45:17Z (tag commit `30a76ac`, latest). The Project was read in full and reconciled at about 19:15 UTC. Only Project items changed on GitHub: no Issue, pull request, comment, label, release or branch was touched, and three Issues were linked to the Project (`gh project item-add`, which does not edit the Issue). Publication is not acceptance: neither release is accepted, and no reporter confirmation is recorded for a named v0.8.5 build.

### Result

The Project held 250 items when this pass started and holds 256 now. The 250th was Issue #112, which was not among the 249 items counted on 2026-10-02 (248 manifest records + #17); when and by whom it was added is not recorded here. It had no `Source key` and only `Status` Done, and is now in the manifest. [items.json](items.json) and [sync-state.json](sync-state.json) hold 255 source-keyed records; the 256th item is the retained keyless Issue #17 link.

The first plan returned 1 conflict (`issue-118-sun-occlusion-lens-flare`: Project Done, manifest Awaiting validation, because the maintainer closed the Issue). The manifest was corrected first, as in the earlier manifest-only alignments, and the second plan returned 6 created, 10 updated, 239 unchanged, 0 conflicts. `tools/project_management/sync.py --apply` then ran 84 operations (72 field values, 6 draft texts, 3 draft creations, 3 Issue links) without a failure. A read-back of all 256 items compared every desired field value, draft title and draft body (2,255 comparisons) against the Project with 0 mismatches (the one difference it reports, `ultrawide-fov-layout`, is the retained duplicate link of #17 described in the 2026-09-30 record). The other 239 existing items, including their draft bodies, were unchanged from the pre-apply snapshot, and the three saved views were unchanged. A plan afterwards returned 255 unchanged, 0 conflicts, 0 operations. `sync.py` wrote `sync-state.json` with CRLF line endings on Windows; they were converted back to LF.

| Status | Before (250) | After (256) |
| --- | ---: | ---: |
| Done | 200 | 205 |
| In Progress | 13 | 15 |
| Awaiting validation | 3 | 5 |
| Paused | 18 | 17 |
| Todo | 16 | 14 |

### Created items (6)

| Key | Fields | Basis |
| --- | --- | --- |
| `release-v0-8-0` | Release, Done / Released / v0.8.0, 2026-10-03 | Publication record: tag, Gitea run 184, the five assets with sizes and SHA-256 (GitHub API), the work included. States that the Android APK was attached by hand at 08:03Z after publication and that the battle dialogue fix is not in it. Done means published, not accepted. |
| `release-v0-8-5` | Release, Done / Released / v0.8.5, 2026-10-03 | Publication record: tag, Gitea run 212 (first with an Android job), the five assets with sizes and SHA-256; the Android APK is CI-built, the macOS image was built on the Mac and uploaded by hand. |
| `issue-148-battle-dialogue-pauses` ([#148](https://github.com/freefrank/LostOdysseyRecomp/issues/148)) | Bug, Done / Released / v0.8.5 | Closed by the maintainer at 02:45Z with "solved in 9f52d78", before the fix merged (`4022256`, 16:22Z) and before any release; it shipped only in v0.8.5. The reporter's 06:14Z comment is about the cause, not a test. |
| `issue-173-first-forest-battle-hang` ([#173](https://github.com/freefrank/LostOdysseyRecomp/issues/173)) | Bug, Done / Released / v0.8.5 | The maintainer identified it from the reporter's video as the #148 bug at 16:29Z and closed it at 19:03:09Z asking for a report if the fix does not work. The Issue was still open when this pass began and was closed while it was running. The reporter attached no log and has not replied. |
| `issue-175-save-not-found-after-power-loss` ([#175](https://github.com/freefrank/LostOdysseyRecomp/issues/175)) | Bug, Done / Released / v0.8.5 | PRs #178 (renamed folders, not the cause) and #181 (list from `save.bin`, sync saves to disk). Closed by the maintainer at 18:23Z, 22 minutes before publication. Validated locally with the reporter's own `save.bin`; not reporter-accepted. See the judgment calls. |
| `debug-no-random-encounters-20261003` | Feature, Awaiting validation / Released / v0.8.5 | PR #180, requested while investigating #171. Checked by the PR author in one area at 120 FPS; no player confirmation. |

Creations: 3 drafts (two release records and the switch) and 3 Issue links.

### Updated items (10)

| Key | Before (Status / Delivery / Release) | After | Basis |
| --- | --- | --- | --- |
| `issue-118-sun-occlusion-lens-flare` | Done (Project) / Released / v0.7.35; manifest Awaiting validation | Done / Released / v0.8.0; Evidence | Closed as completed 08:12Z. PR #149 shipped in v0.8.0; the reporter (RX 6600) wrote at 08:40Z that v0.8.0 works, the one reporter confirmation recorded for a v0.8.0 or v0.8.5 change. |
| `issue-112-native-gtao-and-shadow-resolution` ([#112](https://github.com/freefrank/LostOdysseyRecomp/issues/112)) | Done / none / none (existing keyless item) | Done / Released / v0.8.0; Kind, Area, Evidence, `Source key` | Adopted, not created. The maintainer answered "Done!" at 02:25Z and closed it at 06:54Z. |
| `higher-resolution-shadows` | Todo / Not started / none | Awaiting validation / Released / v0.8.0; body and Evidence | PR #155 (shadow resolution 1x, 2x, 4x). Frame rates were not benchmarked and the VRAM cost is not recorded, so the second criterion stays unchecked. |
| `ssao-and-reshade-depth` | Todo / Not started / none | In Progress / Released / v0.8.0; body and Evidence | PR #155 shipped experimental SSAO and GTAO (off by default). Nothing is recorded for the ReShade depth half, and all three criteria stay unchecked. |
| `hdr-output-tonemapping` | In Progress / Released / v0.7.35 | fields unchanged; body and Evidence | PRs #159 and #162 (v0.8.0): HDR with FXAA, SMAA, TAA, DLSS, FSR, MetalFX and Vulkan DLSS frame generation. The maintainer's statement on #90 is recorded as a statement, with no platform or display. |
| `save-state-support` | Paused / Deferred / none | fields unchanged; body and Evidence | #90 closed as completed at 06:50Z because save states are postponed. The body held literal `` `n `` sequences; it was rewritten with real line breaks. |
| `experimental-android-platform` | Paused / Deferred / none | In Progress / Released / v0.8.5; body and Evidence | The APK was attached to v0.8.0 by hand and is CI-built in v0.8.5 (PRs #168, #169, #170, #177). The earlier "Deferred" section stays as history. |
| `native-macos-platform` | In Progress / Released / v0.7.35 | fields unchanged; body and Evidence | Disk images in v0.8.0 and v0.8.5; macOS 15 minimum since PR #147 (the body said 14.0); the v0.8.5 image's integrity checks. |
| `linux-aarch64-platform` | In Progress / In progress / v0.8.0 | fields unchanged; Evidence | v0.8.0 and v0.8.5 were published without an ARM64 package; the cross-build path is in the source of both. |
| `temporal-phased-p4-dlss-fg-2x` | Done / Validated / v0.8.0 | Done / Released / v0.8.0; Evidence | v0.8.0 is published. Maintainer acceptance of 2026-10-03 stays in the Evidence, like `issue-117-battle-camera-flicker` (Validated to Released on 2026-10-02). |

### Judgment calls

Each can be reverted on its own; the previous values are in the committed `b3e231f4` versions of `items.json` and `sync-state.json`.

- **#175 acceptance boundary.** Status is Done because the maintainer closed the Issue (the #114, #116 and #121 precedent). The reporter wrote "Its all working now" at 18:48:01Z, three minutes after v0.8.5 was published and two minutes before the maintainer's 18:50Z announcement asking the reporter to confirm that Continue finds the save on v0.8.5; the comment does not name a build and no later answer is recorded. So the record says validated locally with the reporter's save and reporter-reported working, not reporter-accepted on v0.8.5. [STATUS](../STATUS.md) and the [roadmap](../ROADMAP.md) still say there is no report that a published build recovered the save; that sentence (docs_sync's) predates or overlooks the 18:48Z comment.
- **#148 and #173 as two items.** Same root cause and one fix, but two reporters with different acceptance boundaries (#148's reporter has only commented on the cause; #173's has not replied), so they are separate linked Issue items and the fix is described in `issue-148-battle-dialogue-pauses`. No separate draft was made for the fix.
- **Android.** Moved from Paused / Deferred to In Progress / Released because an APK is now published, with `Release` v0.8.5 rather than v0.8.0: the v0.8.0 APK was attached by hand after publication and the maintainer had deferred Android out of v0.8.0, while v0.8.5 is the first release whose Android APK is part of the release process. The deferral reason (an upstream Qualcomm bug) is still not recorded in the repository; the v0.8.5 changelog only says the device driver leaves the highlighted menu text invisible and that Turnip draws it, and this record does not claim that is the same bug.
- **HDR Release.** `hdr-output-tonemapping` keeps `Release` v0.7.35, the first release carrying it (as `native-macos-platform` does); v0.8.0 is in the Evidence and body. Setting it to v0.8.0 is the alternative.
- **Shadows and ambient occlusion.** The linked #112 item is Done because the maintainer closed it, while the two roadmap drafts are Awaiting validation / In Progress: their own criteria (performance, VRAM, AO with AA or upscaling, ReShade depth) are not met or recorded. `ssao-and-reshade-depth` is a Research item and shows Released because the experimental passes shipped.
- **P4.** Delivery moved to Released and `Release` stayed v0.8.0, the plan milestone (the 2026-10-02 pass made the same choice for plan items). P4 was first shipped in v0.7.35 (PR #119).
- **Items left in the v0.8.0 lane although v0.8.0 is published.** `temporal-phased-p0-common-contracts-probe` (In Progress / Validated: physical-display, failure-injection and settings-restart validation open), `linux-aarch64-platform` (In Progress / In progress: no package, CI or hardware run) and `frame-generation-research` (Paused / Deferred) were not changed. Whether to move them to the backlog, a later release or leave them is for the maintainer.
- **Not itemized.** Shipped work without an Issue or roadmap item stays in the release bodies only: PR #152 (one SPIR-V pack), #97 with Issue #103 (8BitDo), #124, #125, #147, and the CI, test and documentation PRs. Issue #171 (the Great Ancient Ruins platform; the maintainer found the platform already raised in the reporter's save, not a logic fault in the room, and is repairing the save) and the other open reports #167, #172, #174, #176 and #179, plus #104 and #151, have no Project items; they are open reports without a fix, not shipped work.

### Evidence boundaries

Read directly from GitHub on 2026-10-03: both releases (publication times, assets, sizes and SHA-256 digests), the tags `v0.8.0` and `v0.8.5`, the merge times of PRs #168, #169, #170, #177, #178, #180 and #181 and the PRs merged between v0.7.35 and v0.8.0, the state and comments of Issues #90, #103, #112, #118, #148, #171, #173 and #175, and the whole Project before and after. Commit ancestry (`9f52d78` is not in `v0.8.0`, is in `v0.8.5`) was read from the repository.

Cited from the repository and not re-read: the outcome of Gitea runs 184 and 212 and the `shader-packs` check (from [STATUS](../STATUS.md#v085-published--2026-10-03)); the validation numbers of PRs #155, #159, #162, #149, #180 and #181 (from the PR bodies and STATUS); the M1 Max image checks; and that the v0.8.0 APK was built in WSL and reached the title screen on the tablet (from PR #170). No build, game run or test was run for this pass, and nothing was installed from the published packages.

Not established: maintainer acceptance of v0.8.0 or v0.8.5; reporter confirmation of #148, #173 or #175 on a named v0.8.5 build; any run of the published Windows, Linux or Android v0.8.x packages.

### Left unchanged

- The Roadmap board, Backlog and History views were not changed (`updatedAt` 2026-09-09, 2026-09-08, 2026-09-08).
- `shader-delivery-v080`, `frame-generation-research`, `temporal-phased-p0-common-contracts-probe` and every item not listed above kept their Project values and bodies.
- Issue #17 stays the keyless duplicate link recorded on 2026-09-30.
- [STATUS](../STATUS.md), the roadmaps' narrative, [backlog-coverage.md](backlog-coverage.md), [history-coverage.md](history-coverage.md) and [import-audit.md](import-audit.md) were not rewritten. The roadmaps' item 11 (Android "deferred out of v0.8.0"), the open-Issue list (now 13 open; #173 closed at 19:03Z) and the #175 sentence above are docs_sync's to update. Only the two PM-owned "Project was not re-read" markers in [ROADMAP.md](../ROADMAP.md) and [ROADMAP.zh-CN.md](../ROADMAP.zh-CN.md) were replaced with a pointer to this section.

### Records

[items.json](items.json) holds the 7 added records and the 9 changed ones (255 in total); [sync-state.json](sync-state.json) holds the applied values and Project item IDs. The plans, apply report and before and after snapshots were kept outside the repository, in the session scratchpad (`plan0.json`, `plan1.json`, `apply1.json`, `before_items.json`, `after_items.json`), and are not committed.

### Release lane follow-up

Later on 2026-10-03 the maintainer moved the three items still in the published v0.8.0 lane to v0.9.0: `temporal-phased-p0-common-contracts-probe`, `linux-aarch64-platform` and `frame-generation-research` (Release field only; Status and Delivery unchanged). The plan showed 3 updates and 0 conflicts; after `--apply` the Project read back v0.9.0 for all three and a new plan showed 0 operations. `shader-delivery-v080` stays Paused without a Release value.

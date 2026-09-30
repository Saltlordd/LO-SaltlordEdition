# Project status

[Documentation](README.md) · [Roadmap](ROADMAP.md) / [路线图](ROADMAP.zh-CN.md) · [Changelog](../CHANGELOG.md)

## Current source and release — 2026-09-30

GitHub release and Issue states were read on 2026-09-30 at 06:49 UTC. Project items were subsequently reconciled and read back; the [reconciliation record](project-management/reconciliation-2026-09-30.md) gives the final scope and totals. These are checked snapshots. Historical build results retain the executable, scene and platform limits of their original records.

| Area | Verified state | Evidence and limits |
|---|---|---|
| Published release | **v0.7.20**, published 2026-09-30T04:46:36Z; tag commit `dabe072a0db4be81114385e5ef5bb5f48326436a` | [Release](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.7.20); Windows ZIP, Linux AppImage and standalone Flatpak assets listed below. Publication does not establish full-game or cross-GPU acceptance. |
| Source after the release | `2e1c547` adds the F1 Save Anywhere warning for #74; the local checkout subsequently reached `84f569a` with a temporary-build cleanup tool | [Warning commit](https://github.com/freefrank/LostOdysseyRecomp/commit/2e1c5477447b5d3df361867a2c4395d03e99cdb4); the warning is after the v0.7.20 tag and does not restore the missing party-switch state. |
| Last recorded feature acceptance | The maintainer accepted the feature set shipped in **v0.7.15** on 2026-09-29 | [Historical evidence](archive/STATUS-2026-09-29.md). This does not automatically accept later changes or add full-game, cross-GPU or physical-display coverage. |
| Gate 1 | **Accepted as passed** by the maintainer on 2026-09-27 | [Host repair record](notes/gate1-host-repair-20260927.md). The known SDK `PRESENT-AFTER-WRITE` report remains backlog; physical-display classification, failure injection and settings-restart coverage remain separate. |
| Current planning | The [roadmap](ROADMAP.md) owns remaining work; GitHub owns live Issue and Project state | The [Project reconciliation](project-management/reconciliation-2026-09-30.md) records applied status, evidence and backlog corrections. The [workflow](project-management/README.md) explains local manifest ownership and conflict handling. |

### Published v0.7.20 assets

The GitHub API listed these three assets. Sizes are metadata readback; this documentation task did not download, hash, rebuild or run them.

| Asset | Bytes |
|---|---:|
| `LostOdysseyRecomp-windows-x64-v0.7.20.zip` | 254,480,480 |
| `LostOdysseyRecomp-linux-x64-v0.7.20.AppImage` | 253,716,984 |
| `LostOdysseyRecomp-linux-x64-v0.7.20.flatpak` | 231,367,664 |

The optional `portable_dx12.lospd` was distributed with earlier releases including [v0.7.10](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.7.10). It is not a fourth asset on the checked v0.7.20 release. Vulkan packs are bundled in application packages; see the [pack reference](PORTABLE_SHADER_PACK.md).

## Delivered behavior and validation scope

| Area | Implementation / delivery | Recorded validation boundary |
|---|---|---|
| v0.7.20 changes | 6× FG request cap, #82 shutdown fix, guest texture mip chains (#87), command-processor reductions and D3D12 redundant-state filtering | [Changelog](../CHANGELOG.md#v0720--2026-09-30) and [pre-release evidence](archive/STATUS-2026-09-29.md). The FG cap has focused CPU checks. Shutdown evidence covers the recorded RTX 5080 D3D12 exclusive-fullscreen FG route; other exit paths were not equivalently tested. |
| Performance | Command-processor and D3D12 filtering improvements shipped in v0.7.20 | Fixed Uhra-plaza D3D12 comparisons on RTX 5080 / Ryzen 7 9800X3D. Capped and uncapped runs are separate; Vulkan, Linux runtime, battles and moving routes cannot inherit these gains. Exact numbers and controlled conditions remain in the [historical ledger](archive/STATUS-2026-09-29.md). |
| SR / frame generation | DLSS/DLAA and FSR SR are integrated; Windows D3D12 DLSS/FSR FG has in-game same-process switching; Vulkan FG has bounded integration evidence | [Reusable FG integration](notes/reusable-fg-game-integration.md), [Gate 1 record](notes/gate1-host-repair-20260927.md), [remaining P0–P4 work](ROADMAP.md#v080-plans). SDK submission/present counters do not prove physical-display FPS or whole-game image quality. |
| Native high refresh / VRR | 90/120 FPS targets and FreeSync / G-SYNC Compatible output pacing shipped in v0.7.15 and are included in its acceptance | [Native FPS](notes/native-90-120fps.md), [VRR](notes/vrr-freesync-gsync-compatible.md). Same-scene user evidence does not establish every Ring, audio, cutscene or gameplay timing path. |
| Modding | Mod API v1, LOTEX1/PNG tooling, native-menu atlas/font replacements, PlayStation prompts and Wiki source are delivered | [Modding](wiki/Modding.md). Arbitrary guest texture/model consumers and real MO2 integration remain outside the delivered API scope; #40 remains open. |
| Linux packaging | Linux x64 runtime, importer/updater, AppImage and standalone Flatpak are delivered; bounded native AMD 8060S and Flatpak user evidence is recorded | [Native FSR/8060S evidence](notes/fsr-repair-2026-09-24.zh-CN.md), [Flatpak release evidence](archive/STATUS-2026-09-29.md#v071-published--v071-已发布), [AppImage ABI baseline](../packaging/linux/APPIMAGE_COMPATIBILITY.md). Steam Deck hardware, FUSE/AppImageHub acceptance, Flathub publication and full playthrough are separate. |
| Shaders / cache / import / storage | Portable Vulkan/DX12 packs, bounded parsing and cache fallback, supported disc/DLC import, native saves and updater rollback are documented | [Installation](INSTALLING.md), [pack reference](PORTABLE_SHADER_PACK.md), [subsystem notes](notes/README.md). A pack hit, importer check or synthetic fixture is not complete resource/gameplay certification. |

## Issues and acceptance

<a id="live-issue-reconciliation"></a>

The checked open Issue set is **#30, #40, #48, #49, #74, #85, #88, #90 and #96**. Current descriptions, priorities and implementation gaps belong in the [active roadmap](ROADMAP.md#active-work), not in dated handoffs.

- [#74](https://github.com/freefrank/LostOdysseyRecomp/issues/74): Save Anywhere can lose party-switch permission after loading a split-party save. The source warning does not repair a saved file; a save from before the split is still needed for the proposed guard. See the [investigation](notes/issue-74-save-anywhere-party-split.md).
- [#82](https://github.com/freefrank/LostOdysseyRecomp/issues/82) and [#87](https://github.com/freefrank/LostOdysseyRecomp/issues/87): closed on 2026-09-30; their fixes are included in v0.7.20. Tracker closure does not add undocumented reporter or broad gameplay acceptance.
- Audio reports [#54](https://github.com/freefrank/LostOdysseyRecomp/issues/54) and [#55](https://github.com/freefrank/LostOdysseyRecomp/issues/55): closed on 2026-09-26. PR #66's diagnostics alone do not prove a fix for every missing line or language; retained feedback remains in the [audio follow-up](notes/issue-54-55-audio-followup.md).

Known limitations such as the SDK synchronization exception remain explicit backlog. Earlier failed Gate 1 runs remain failed historical runs, even though the maintainer later accepted the gate. New reproduction evidence can reopen a defect; an old unchecked checkbox alone cannot.

## Historical evidence

The previous status document had accumulated over 1,100 lines of release receipts, old current-state headings, validation counts and local experiments. It is preserved in [STATUS through 2026-09-29](archive/STATUS-2026-09-29.md), with its relative links adjusted. The earlier [2026-09-10 snapshot](archive/STATUS-2026-09-10.md) is also retained. These snapshots preserve failures, accepted scene results and private evidence paths without becoming today's execution plan.

Use the [notes index](notes/README.md) for technical investigations, [archive index](archive/README.md) for superseded ledgers, and [documentation review](audits/documentation-2026-09-30.md) for the normalization record. This cleanup performed document and evidence reconciliation; it added no runtime, hardware or player validation.

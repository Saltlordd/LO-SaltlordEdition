# Project status

[Documentation](README.md) · [Roadmap](ROADMAP.md) / [路线图](ROADMAP.zh-CN.md) · [Changelog](../CHANGELOG.md)

## macOS integration branch — 2026-10-01

The integration combines MikeRavenelle's `arm64-macos` work from commit
`b7951cd` with `main` commit `ca0d8fa`; the local integration validation uses
commit `e7b45b6`. It remains experimental and has no public Mac release.
The changes cover Apple Silicon memory and threading, Metal/MetalFX, packaging
and test workflow support. The integration also includes the independent shader
predicate, constant-cache, DLC hashing and updater identity fixes recorded in
the current branch.

Recorded local checks include a complete Windows clang-cl runtime build and link
with D3D12/Vulkan/DLSS/FSR/FG enabled, nine focused Windows test targets, and a
complete ARM64 Mac runtime build and link with eight focused Mac test targets.
The Windows production-build receipt is `fffbd56`; the final Mac validation
source tree is `fe7131f` (the later changes select pinned zstd on Mac and adjust
focused test staging/diagnostics without changing Windows runtime behavior).
The Mac build also passes the pinned-zstd portable-shader-pack checks after
matching zstd to the deployment target. On an M1 Max with 32 GiB running macOS
26.6.2, cold `--prepare-shaders-only` compiled 28,484 shaders with zero failures
and exited successfully in about 319 seconds. The 300-second new-game/first-battle
script passed in 302.2 seconds including shutdown (`passed=true`,
`unfinished_steps=0`, zero error lines, no crash reports). Four 1280×720 captures
show the opening, battle menu, attack and return to the menu. Local evidence is
retained under `out/macos-integration/mac-validation/` (`result.json`,
`runtime.log` and the captures).
This is not long-play, broad-scene, image-quality or performance acceptance;
the mixed cutscene/battle frame window is not a standalone benchmark. The
branch remains an experimental source integration. The local ad-hoc package was
signature-checked; an installed app reused the shader cache with 28,504 valid,
zero missing/invalid shaders and zero DXC attempts. This is cache-reuse evidence
and adds no new gameplay coverage.

## Current source and release — 2026-09-30

GitHub release and Issue states were read on 2026-09-30 at 06:49 UTC. Project items were subsequently reconciled and read back; the [reconciliation record](project-management/reconciliation-2026-09-30.md) gives the final scope and totals. The latest release, the open Issue list and the merged source after the tag were re-read with `gh` on 2026-10-01 at 05:16 UTC, after PR #110 merged. These are checked snapshots. Historical build results retain the executable, scene and platform limits of their original records.

| Area | Verified state | Evidence and limits |
|---|---|---|
| Published release | **v0.7.20**, published 2026-09-30T04:46:36Z; tag commit `dabe072a0db4be81114385e5ef5bb5f48326436a` | [Release](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.7.20); Windows ZIP, Linux AppImage and standalone Flatpak assets listed below. Publication does not establish full-game or cross-GPU acceptance. |
| Source after the release | `main` at `abbcfb2` (PR #110 merged 2026-10-01T05:12:02Z) holds merged, **unreleased** changes after the v0.7.20 tag: the F1 Force RB Party Switch button and Save Anywhere split-party guard for #74 (PR #106), the Legacy of the Eastern Tribe sky jitter fix for #102 plus a runtime flicker-suspect log (PR #107), legacy-updater manifest hashes in future Windows ZIPs (`43f4317`, #105), the project-review runtime hardening (PR #109), a test-fixture update (PR #108) and the move of pull request checks to Gitea (PR #110) | [Changelog, Unreleased](../CHANGELOG.md#unreleased) and the [review fix record](notes/PROJECT_REVIEW_FIXES_20260930.md). None of this is in a published package. Merging is not release or reporter acceptance; reporter confirmation for #102 is recorded as pending and #74 stays open. |
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
| Unreleased runtime hardening (PR #109) | 13 confirmed findings from the 2026-09-30 project review (4 P1, 9 P2) are fixed on `main`: file query, read, resize and directory-entry APIs, guest allocation sizing, GPU command-list validation, texture identity and content revalidation, audio and GPU callback publication, guest-main exit, the shared settings path and the Linux Flatpak update notice | [Fix record](notes/PROJECT_REVIEW_FIXES_20260930.md), [test commands](../tools/tests/README.md#project-review-regressions-2026-09-30). Validated: Windows clang-cl full runtime build, WSL ASan/UBSan suite 14/14, Windows header tests, same-input new-game script parity with `main`, the frozen Uhra 4K save loading and rendering as on `main`, and uncapped 4K alternating runs (`main` 137.4 FPS, branch 140.9 FPS average over two sessions). Not exercised: real audio-device unregistering, Flatpak installation, save writing and long gameplay. No player acceptance beyond the merge; not released. |
| Pull request checks | The FG contract, FG game integration compile, reusable FG and review-regression workflows run as Gitea Actions at `git.zkx.ca` (Linux on a docker runner, Windows on `win-t640`); the GitHub copies are manual-only and each Gitea result is reported to the GitHub commit as `gitea/<workflow>`. Release packaging, the Mod API and Wiki workflow and issue triage stay on GitHub Actions | [Pull request checks on Gitea](notes/ci-gitea.md) records the first green run (2026-09-30), timings and runner setup. Check outcomes are build and fixture results, not gameplay acceptance. |
| Performance | Command-processor and D3D12 filtering improvements shipped in v0.7.20 | Fixed Uhra-plaza D3D12 comparisons on RTX 5080 / Ryzen 7 9800X3D. Capped and uncapped runs are separate; Vulkan, Linux runtime, battles and moving routes cannot inherit these gains. Exact numbers and controlled conditions remain in the [historical ledger](archive/STATUS-2026-09-29.md). |
| SR / frame generation | DLSS/DLAA and FSR SR are integrated; Windows D3D12 DLSS/FSR FG has in-game same-process switching; Vulkan FG has bounded integration evidence | [Reusable FG integration](notes/reusable-fg-game-integration.md), [Gate 1 record](notes/gate1-host-repair-20260927.md), [remaining P0–P4 work](ROADMAP.md#v080-plans). SDK submission/present counters do not prove physical-display FPS or whole-game image quality. |
| Native high refresh / VRR | 90/120 FPS targets and FreeSync / G-SYNC Compatible output pacing shipped in v0.7.15 and are included in its acceptance | [Native FPS](notes/native-90-120fps.md), [VRR](notes/vrr-freesync-gsync-compatible.md). Same-scene user evidence does not establish every Ring, audio, cutscene or gameplay timing path. |
| Modding | Mod API v1, LOTEX1/PNG tooling, native-menu atlas/font replacements, PlayStation prompts and Wiki source are delivered | [Modding](wiki/Modding.md). Arbitrary guest texture/model consumers and real MO2 integration remain outside the delivered API scope; #40 remains open. |
| Linux packaging | Linux x64 runtime, importer/updater, AppImage and standalone Flatpak are delivered; bounded native AMD 8060S and Flatpak user evidence is recorded | [Native FSR/8060S evidence](notes/fsr-repair-2026-09-24.zh-CN.md), [Flatpak release evidence](archive/STATUS-2026-09-29.md#v071-published--v071-已发布), [AppImage ABI baseline](../packaging/linux/APPIMAGE_COMPATIBILITY.md). Steam Deck hardware, FUSE/AppImageHub acceptance, Flathub publication and full playthrough are separate. |
| Shaders / cache / import / storage | Portable Vulkan/DX12 packs, bounded parsing and cache fallback, supported disc/DLC import, native saves and updater rollback are documented | [Installation](INSTALLING.md), [pack reference](PORTABLE_SHADER_PACK.md), [subsystem notes](notes/README.md). A pack hit, importer check or synthetic fixture is not complete resource/gameplay certification. |

## Issues and acceptance

<a id="live-issue-reconciliation"></a>

The checked open Issue set (read 2026-10-01 05:16 UTC) is **#30, #40, #48, #49, #74, #85, #88, #90, #96, #103 and #104**. [#103](https://github.com/freefrank/LostOdysseyRecomp/issues/103) (embedded SDL controller database) and [#104](https://github.com/freefrank/LostOdysseyRecomp/issues/104) (faster menu animation) were opened on 2026-09-30, after the Project reconciliation, which therefore does not include them. Current descriptions, priorities and implementation gaps belong in the [active roadmap](ROADMAP.md#active-work), not in dated handoffs.

- [#74](https://github.com/freefrank/LostOdysseyRecomp/issues/74): open. Save Anywhere can lose party-switch permission after loading a split-party save. Merged but unreleased (PR #106): an F1 Force RB Party Switch button re-arms switching, and Save Anywhere keeps the game's own Save permission while the party is split. It was checked only in Astral Square with the reporter's save; other areas and native save points during a split were not tested, and no reporter confirmation is recorded. See the [investigation](notes/issue-74-save-anywhere-party-split.md).
- [#102](https://github.com/freefrank/LostOdysseyRecomp/issues/102) and [#105](https://github.com/freefrank/LostOdysseyRecomp/issues/105): closed on 2026-09-30. The #102 sky jitter fix (PR #107) and the legacy-updater manifest hashes referencing #105 (`43f4317`) are on `main` but unreleased; closure is not reporter confirmation, which the changelog records as pending for #102.
- [#82](https://github.com/freefrank/LostOdysseyRecomp/issues/82) and [#87](https://github.com/freefrank/LostOdysseyRecomp/issues/87): closed on 2026-09-30; their fixes are included in v0.7.20. Tracker closure does not add undocumented reporter or broad gameplay acceptance.
- Audio reports [#54](https://github.com/freefrank/LostOdysseyRecomp/issues/54) and [#55](https://github.com/freefrank/LostOdysseyRecomp/issues/55): closed on 2026-09-26. PR #66's diagnostics alone do not prove a fix for every missing line or language; retained feedback remains in the [audio follow-up](notes/issue-54-55-audio-followup.md).

Known limitations such as the SDK synchronization exception remain explicit backlog. Earlier failed Gate 1 runs remain failed historical runs, even though the maintainer later accepted the gate. New reproduction evidence can reopen a defect; an old unchecked checkbox alone cannot.

## Historical evidence

The previous status document had accumulated over 1,100 lines of release receipts, old current-state headings, validation counts and local experiments. It is preserved in [STATUS through 2026-09-29](archive/STATUS-2026-09-29.md), with its relative links adjusted. The earlier [2026-09-10 snapshot](archive/STATUS-2026-09-10.md) is also retained. These snapshots preserve failures, accepted scene results and private evidence paths without becoming today's execution plan.

Use the [notes index](notes/README.md) for technical investigations, [archive index](archive/README.md) for superseded ledgers, and [documentation review](audits/documentation-2026-09-30.md) for the normalization record. This cleanup performed document and evidence reconciliation; it added no runtime, hardware or player validation.

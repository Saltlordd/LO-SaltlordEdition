# Portable shader packs

This is the distribution and tool reference for Vulkan `.lospv` and D3D12
`.lospd` packs. Vulkan packs are bundled in application packages. The DX12 pack
is an optional standalone asset; a release need not include a newly uploaded
copy. Installation uses `shaders/portable_vk.lospv` or
`shaders/portable_dx12.lospd` beside the executable. See [installation](INSTALLING.md)
and [current release status](STATUS.md) for the player-facing path.

Release measurements below are dated evidence. The initial implementation was
based on `menu@257f3866e9f9f5d3e65550c86dce453290cf7ee4`; its delivery archive and
unverified boundaries are retained in the final historical sections.

## Runtime contract and release check (after v0.7.25)

v0.7.25 shipped the Vulkan pack pinned for v0.7.10, although its runtime had moved from shader translator version 24 to 26 (the macOS merge changed predicate-push translation). The game rejected the pack (`portable shader pack rejected; local cache fallback: portable shader contract mismatch`) and compiled all 28,549 shaders on first launch: 180 s with 15 workers on a 16-thread CPU. Nothing in the release compared the pack with the runtime, and `verify-runtime` could not do it in general. The runtime hashed its guest image after `XexLoader` had written host-assigned import addresses into it, so the tool relied on one audited contract pair, which the next translator change made useless.

The contract now hashes the first `0x185C60` bytes of the image as parsed, before import binding (`XexLoader::UnboundIdentityPrefix()`). That prefix equals the start of `tools/xexdump` output, so `RuntimeContract`, shared by the renderer, its export and `LoShaderPackTool`, gives the same value offline. `verify-runtime` now checks SPIR-V and DXIL packs exactly, and the audited exception is gone from it and from `merge`. Packs made with the old definition are rejected. The local startup bundle still keys on the bound image, so local caches stay valid.

Release CI fetches the pinned pack before building and passes it as `LO_PORTABLE_SHADER_PACK`. The runtime build then runs `verify-runtime` against `LostOdysseyRecompLib/private/image_disc1.bin` and stages the pack only if it matches, so a stale pack fails the release build. `tools/release/sync_shader_pack.py` runs the same check before it pushes, when the image is present.

To refresh the pack after a translator, option or discovery change:

1. Build the runtime from the release source and run it with `--prepare-shaders-only` and `LO_SHADER_EXPORT_PACK=<path>.lospv`. A complete local `startup_vk12_v1.bundle` made by that source is streamed without DXC; without one, the run compiles every shader.
2. Run `LoShaderPackTool verify-runtime <pack> LostOdysseyRecompLib/private/image_disc1.bin`.
3. Push the pack with `python tools/release/sync_shader_pack.py --pack <pack>` and pin the printed build-inputs commit for `out/shader-input` in `.gitea/workflows/release.yml` and `.github/workflows/release.yml`.

The pack refreshed on 2026-10-01 was exported in 18 s from the startup bundle that v0.7.25 had just built (no guest shader DXC calls). It has 28,549 records and 27,793 unique binaries, is 180,052,919 bytes, SHA-256 `f5eadb4fcc27a40bf4d76bae6bf83224bfb730fab8f49581ba3254c2d2f17c25`, contract `4e123a08e148937e41b1b7dc636608377bd15a0043fb506a43a75cb6930ea6ae`, and is pinned as build-inputs commit `5fae27a5a3c05262e7b64631f141ebcd19d5f656`. `verify-runtime` passed. A `--prepare-shaders-only` run with an empty cache reported a pack hit for all 28,549 records. The release build check failed with the v0.7.25 pack and passed with this one. The optional DX12 pack has not been regenerated for the new contract.

## v0.7.10 shader-pack release

The v0.7.10 shader-pack work added backend-specific selection and a DX12
`.lospd` format alongside the existing Vulkan `.lospv` path. The new Vulkan
pack contains 28,546 records and is 180,198,461 bytes; it covers 8,186
installed v24 `.spv` cache entries, 62 source microcodes and 205 direct DLC
candidates. The new DX pack contains 28,546 records and is 80,222,382 bytes;
`LoShaderPackTool verify` passed for it. The CPU contract passed 74 checks and
the development game build passed.

These packs are published with v0.7.10. A Windows D3D12
`--prepare-shaders-only` run selected the DX12 backend and loaded the default-path
DX pack, reporting `28546 records`, `25057 unique binaries` and `80222382 file
bytes`; guest startup was intentionally skipped. The standalone DX asset is
published separately. The runtime-hit check used `--prepare-shaders-only` and did not
start the guest or validate GPU draws, image quality or full-game coverage.

## v0.6.15 release

The v0.6.15 GitHub Release was published on 2026-09-24T19:47:35Z from source `6eef30d257f2e14ce30a546217574a0dc74fad69` via Release CI [36044604844](https://github.com/freefrank/LostOdysseyRecomp/actions/runs/36044604844). The updated portable Vulkan shader pack contains 28,527 shaders (178,332,830 bytes, SHA-256 `b486c87d121968bcec67fae6bc1aa7926378455281bc8b2a221409b8c06c6e2b`), consolidating 45 newly compiled microcodes from recent gameplay caches with the 28,482 baseline records.

Fixed hash and FidelityFX license gates passed in Release CI. The pack is bundled directly into the official Windows and Linux application packages; users do not need a standalone Vulkan bundle, and the standalone ZIP and sidecar were removed from public release assets. The DX12 bundle statement above supersedes the historical note that a DX12 bundle was not yet implemented.

## v0.6.1 release

The v0.6.1 GitHub Release was published on 2026-09-18. [Release CI
35374267882](https://github.com/freefrank/LostOdysseyRecomp/actions/runs/35374267882)
passed the Windows and Linux Release jobs and focused regressions. The standalone pack is
available from the [v0.6.1 release](https://github.com/freefrank/LostOdysseyRecomp/releases/tag/v0.6.1),
with SHA-256
`387a23b9328b8136847d48b37b574fddd600a526eb837807fbb08b758c6de4d9`.
It is byte-identical to the v0.6.0 pack and contains 28,482 records under the v0.6.1
asset name; this is release provenance rather than a claim of new shader coverage.

At v0.6.1 the source-side verifier permitted the audited exception for the raw image
contract `d5a2fab10441a46444b6b41ffcb4f1ba562bea75668a7b445fd43688aec67507`
mapping to runtime contract
`f6fd1179b50f6ff9b63d6be84c662d1337af6b7dfa78865a9a6c025509c9b77f`; other
images retain direct contract verification. This is required because `XexLoader`
patches import thunks before runtime shader preparation. The current local
verification covered a real 28,482-record pack with all payloads and runtime
compatibility, and `portable_shader_release_test.py` passed. Previous runtime
logs also recorded pack hits in two runs, but they came from a 0.5.14 dirty
source and are retained as supporting evidence only. The exception was removed
after v0.7.25, when the contract moved to the image before import binding (see
the runtime contract section above).

The uploaded pack matches the audited contract through the release verifier and
the published asset's sidecar and GitHub digest match. Native Linux GPU, Steam
Deck, AppImage update transactions and full-game shader coverage remain unverified.

## Implemented

- A separate, read-only `.lospv` distribution artifact for successful guest SPIR-V
  and all fields of `TranslatedShader` needed for rendering. HLSL, translation
  diagnostics and negative compiler records are not shipped.
- Exact SPIR-V binaries are deduplicated by SHA-256, while every `(stage, guest
  hash)` keeps its own metadata. There is no semantic/approximate shader merging.
- Zstandard compression in independent roughly 1 MiB blocks. A single unusually
  large shader can occupy a block up to 16 MiB. The writer streams blocks; the
  reader opens a small index and retains only its most recently decoded block.
  Neither needs to retain the whole decompressed binary corpus.
- Runtime module creation is lazy. Already-created shaders still take the normal
  single-map-lookup hot path. A valid portable pack bypasses startup guest shader
  discovery/translation/DXC. Missing shaders still use the existing local path.
- Before existing PSO preparation takes pointers into the shader maps, all shaders
  needed by those PSO recipes are loaded first. No map insertions during that
  pointer-collection phase. Existing PSO worker scheduling is otherwise unchanged.
- Local success caches, negative caches and schema-2 startup bundles keep their
  current compiler/path identity rules. No cache-version rollback or global
  weakening of `cache::IdentityKey` / `RuntimeIdentity`.
- Portable compatibility keeps schema, translator options, variant and layout
  markers needed to reject an incompatible pack. It does not bind startup to
  install paths, local DXC DLL/SO hashes or a full content digest.
- The reader checks the pack format, bounded Zstd frames, sizes, offsets and
  decompression boundaries. It loads records on demand and does not scan every
  SPIR-V payload at startup. A malformed record disables the pack and leaves
  the local fallback retryable, without inserting an invalid shader entry.
- Windows ZIP and Linux AppImage staging copy only `shaders/portable_vk.lospv`,
  check that it is a nonempty file, and retain the Zstandard license. The current
  Python staging helper does not run the native verifier. They do not
  scoop up `cache/shaders`, debug output, HLSL, both backends, or old cache versions.
  The DX12 `.lospd` asset is packaged and published separately, with the dimensions
  and bounded runtime-hit evidence recorded above; it is not copied into these
  application payloads.

## Building from the repository

The implementation is integrated into the repository. Follow the [build guide](BUILDING.md);
the original `apply_portable_shader_pack.py` delivery script is not a current
checkout prerequisite. Focused pack tools can be built independently as below.

## Build and CPU tests

The pack library needs static Zstandard. CMake uses an installed static zstd CMake
package, or fetches upstream v1.5.7 pinned to commit
`f8745da6ff1ad1e7bab384bd1f9d742439278e99`. For offline builds, install a static
package or set `FETCHCONTENT_SOURCE_DIR_LO_PACK_ZSTD` to the pinned source tree.
`LO_PACK_FETCH_ZSTD=OFF` makes a missing package a configuration error.

No game assets are required for these targets:

```sh
cmake -S tools/shader_pack -B out/portable-pack -DCMAKE_BUILD_TYPE=Release
cmake --build out/portable-pack --config Release --target LoShaderPackTool LoPortableShaderPackTest LoPortableShaderPackIntegrationTest
ctest --test-dir out/portable-pack -C Release --output-on-failure
```

The original `tools/tests/shader_prebuild_regression.py` remains a fake-DXC/GPU
orchestration test. Its fixture is updated to include the real new integration
methods/library, plus the settings/skip API that menu had already introduced.
It now also needs zstd development headers/library (`-lzstd`). Existing checks
are not deleted or rewritten to assert away a regression.

## Export without recompiling already-valid shaders

Use a runtime containing the pack implementation, keep the same game root / local cache / DXC
pair, select Vulkan, and enable normal shader preparation. In PowerShell:

```powershell
$env:LO_SHADER_EXPORT_PACK = "$PWD\shaders\portable_vk.lospv"
# Launch the rebuilt runtime with the normal game arguments.
```

An existing *compatible, complete* startup bundle is validated and streamed into
this exporter. That path does not translate or invoke DXC for its guest shader
records. HLSL is reconstructed one record at a time only to count discarded text,
then discarded again. If the local bundle is missing or incompatible, normal
preparation runs once and the exporter consumes its successes instead.

Export is explicit, and `.lospv` is required as its suffix to avoid overwriting a
local `.bundle`. Deterministic failures are counted but omitted. Transient
compiler/infrastructure failures, cancellation, incomplete discovery and device
module failures discard the temporary export. The destination is atomically
replaced only after completion. No partially completed export is published.

Remove `LO_SHADER_EXPORT_PACK` after export, otherwise every subsequent launch
continues to run the developer export path:

```powershell
Remove-Item Env:LO_SHADER_EXPORT_PACK
```

An export log ends with `portable shader pack published` and exact byte counts.
The file is deliberately not called `startup_vk12_v1.bundle`; old clients cannot
read it. Ship it only with this patched client.

## Measure, verify and stage

```sh
LoShaderPackTool inspect shaders/portable_vk.lospv
LoShaderPackTool verify shaders/portable_vk.lospv
LoShaderPackTool verify-runtime shaders/portable_vk.lospv LostOdysseyRecompLib/private/image_disc1.bin
```

`inspect` reads structure/index. `verify` also visits all payload blocks and checks
SPIR-V framing. JSON reports raw bytes, deduplicated bytes, compressed bytes,
index bytes, omitted source/diagnostic bytes and final file size. Omitted HLSL
is the sum of reconstructed input strings; the old bundle shares its prelude,
so this is NOT a measurement of bytes saved from that old file. Use `file_bytes`
for the actual distribution size. `inspect` and `verify` report
`runtime_compatibility_verified: false`: they never trust an artifact's own header.
`verify-runtime` computes the runtime contract for the pack's format from a
`tools/xexdump` image and reports `true` only when the pack matches it.

To stage automatically during normal builds, add this CMake cache option to the
existing build configuration:

```text
-DLO_PORTABLE_SHADER_PACK=/absolute/path/to/portable_vk.lospv
```

This opt-in CMake path builds `LoShaderPackTool`, runs `verify-runtime` against
the configured private image, and copies the file to
`<executable-directory>/shaders/portable_vk.lospv`. Linux install uses `bin/shaders`.
Release CI uses this path, so a pack that does not match fails the release build.
The separate Python ZIP/AppImage staging helper then checks presence and size and
copies the file.
Use the explicit tool commands above when changed pack inputs require inspection.

For a standalone downloaded pack, put it under `shaders` beside the final game
executable, or set `LO_SHADER_PACK_PATH` to an absolute file path. Lookup is based
on the executable directory, not the process working directory. No unpacking to
hundreds of megabytes in the user's writable cache is necessary.

`LO_NO_PORTABLE_SHADER_PACK=1` is the explicit pack bypass. Full scan, HLSL dump,
failure retry and export also bypass it. Ordinary `skip_shader_prebuild` and
`LO_NO_SHADER_PREPARE` suppress bulk prebuild, but still allow this index-only
pack path and on-demand cache use. The pack contains no negative records.

## Merge new shader caches into baseline packs

`LoShaderPackTool` provides a `merge` subcommand to import newly observed shader caches into an existing portable pack baseline without recompiling unchanged records:

```sh
LoShaderPackTool merge <baseline.lospv> <decrypted-image.bin> <manifest.tsv> <output-dir>
```

- **Manifest format**: A tab-separated file with header `action\tstage\thash\tsource\tprovenance`. Each row defines:
  - `action`: `include` or `exclude`.
  - `stage`: `vs` or `ps`.
  - `hash`: 16-character lowercase hexadecimal guest shader hash.
  - `source`: Relative path from the manifest file to the raw microcode binary; use `-` for excluded entries.
  - `provenance`: Origin description text.
- **Compilation & Preservation**: New microcodes are compiled using the current shader translator and DXC. `Contains` checks whether a candidate key already exists; `Writer::Import` preserves baseline metadata, omissions, and byte payloads without recompilation.
- **Safety & Atomicity**: The baseline pack and input sources are opened read-only. Merged packs and reports are assembled in an exclusive temporary directory before publishing to `<output-dir>`, and the tool rejects existing output destinations to prevent accidental overwrite. The tool CLI has no hardcoded count limit.
- **DXC requirement**: Compilation requires SPIR-V code generation support. Use the repository-bundled DXC binaries rather than generic system DXC installations that may lack SPIR-V emission.

### Development merge verification

A local test merge produced `out/merged-shaders/portable_vk.lospv` (178,332,830 bytes, SHA-256 `b486c87d121968bcec67fae6bc1aa7926378455281bc8b2a221409b8c06c6e2b`). Starting from the 28,482 baseline shaders, 45 raw microcodes gathered from recent gameplay testing were recompiled and merged, reaching 28,527 total shaders (0 skipped, 1 excluded: `vs_8f6ce5a4f714294a` due to missing supplementary source metadata; original cache entry retained). Verification via `LoShaderPackTool verify-runtime` confirmed `all_payloads_verified: true` and `runtime_compatibility_verified: true`. Detailed logs are recorded in `out/merged-shaders/merge-execution.log`, `verification.log`, and `merge-report.json`. On 2026-09-24, this merged 28,527-shader pack was packaged directly into the official v0.6.15 Windows and Linux application archives via Release CI [36044604844](https://github.com/freefrank/LostOdysseyRecomp/actions/runs/36044604844); standalone bundles were omitted from publication.

## Initial implementation evidence and limits (historical)

The following paragraphs record the original source-only delivery before the
v0.6.1, v0.6.15 and v0.7.10 measurements above. Their absence-of-evidence claims
and proposed checks do not override those later results or require retesting an
unchanged release.

No complete real startup bundle or SPIR-V corpus was provided in this session.
Therefore **no real final pack size or game-data compression ratio is measured**.
800–900 MB may be a plausible raw corpus size; it is not an established download
size. The synthetic compression test is a functional test, not a predictor of
Lost Odyssey's compression ratio.

The implementation has native CPU checks for exact byte/metadata roundtrips,
negative-record omission, exact-binary deduplication, lazy reads, relocation,
contract invalidation, malformed/corrupt payloads, interrupted publication and
concurrent reads. The exact production `.inl` is separately compiled against
explicit fake GPU/DXC services. See the delivery validation report for commands
and outcomes.

Not established here: full runtime linking with the private generated game
inputs; actual DXC execution; actual driver module/pipeline creation; Windows,
Steam Deck or ARM execution; full-game shader coverage; FPS, load-time or peak
process-memory improvement. Core parsing checks are not full SPIR-V semantic
validation. Checksums detect corruption, not authorship: use trusted release
provenance/checksums and validate real shaders/drivers before publishing.

Not changed in this first implementation: shader translation/optimization,
specialization-constant conversion, PSO threading policy, builtin host shader
compilation, or removal of DXC from the application. Unlike reblue, this initial
codec uses block Zstd without smol-v. Exporting from a valid local bundle is the
lowest-risk release-generation path; an autonomous all-assets CI shader compiler
is not added here.

Before release, verify one exported pack on Windows Vulkan and Linux/Deck under
different paths, check that covered shaders do not invoke guest DXC, test an
uncovered shader's fallback, and compare real scene images and frame-time traces.

### Original patch-delivery procedure

The original delivery archive contained an application script and source payload,
without game shaders, an executable, game data or an exported pack. Its Python
3.10+ commands were:

```sh
python apply_portable_shader_pack.py --repo /path/to/LostOdysseyRecomp --check
python apply_portable_shader_pack.py --repo /path/to/LostOdysseyRecomp --apply
```

That historical script checked Git blob identities after CRLF normalization and
replacement anchors, and made `.lo-portable-backup` backups. It did not reset,
commit or push. `--allow-compatible-edits` permitted reviewed compatible edits;
`--diff candidate.patch` generated a patch against the local checkout. These are
archive instructions, not commands supplied by the current repository.

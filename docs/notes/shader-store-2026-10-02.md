# One-file shader store and the 2026-10-02 packs

Status: implemented on `feat/shader-packs-20261002`. Checked with `LoShaderStoreTest` and three runtime runs on a copy of a real cache; not yet run through a play session.

## Why

Before this change, every shader compiled on the player's PC was its own file in `cache/shaders` (`vs_<hash>_v26_b1_f1_<identity>.spv`). The maintainer's install held 39,083 files there: 28,573 for the current translator, about 10,500 left over from translator versions 23 and 24, and a 963 MB startup bundle with the same binaries again.

The file name carried the translator version. Any translator change therefore threw every compiled shader away, whether or not its generated HLSL changed. When the shipped portable pack did not match the runtime either, which happened in v0.7.25, the next start compiled all 28,549 shaders: 177 s with 15 workers on a 16-thread CPU.

## Design

`LostOdysseyRecomp/gpu/shader/shader_store.h` keeps the compiled shaders in one append-only file per binary format: `cache/shaders/shaders_spirv.lostore` or `shaders_dxil.lostore`.

- **Key.** A record is found by the guest shader hash and a SHA-256 of the exact compiler input: the translated HLSL, which embeds the common prelude, plus entry, profile, compile options and the DXC identity. The translator version is not part of the key, so a translator change recompiles only the shaders whose HLSL actually changed. Metal's `-O1` options give it separate records.
- **Integrity.** Each record carries an FNV-1a 64 checksum of its binary and is checked again with the existing container framing check before use.
- **Crashes.** A torn last record, left by a crash during an append, is dropped and the file truncated on the next open.
- **Two instances.** The first process opens the store for writing (`_SH_DENYWR` on Windows, `flock` elsewhere). A second instance reads it but does not append.
- **Compaction.** After a complete preparation pass, records that no prepared shader used are dropped once they waste at least 64 MiB and a quarter of the file.

The renderer reads and writes the store at the two places that used per-shader files: startup preparation and the on-demand compile at draw time. Both already translate the shader before compiling, so the lookup now happens after translation. If the store cannot be opened, they fall back to per-shader files.

## Moving existing caches

- **First start.** When this format's per-shader files exist and no store does, the first start runs one preparation pass even on a startup-bundle hit. Valid files are copied into the store without DXC, then deleted.
- **Old translator versions.** Per-shader files of an older translator version are deleted on any start, including a portable-pack hit, since nothing can read them again.
- **Other backends.** Files of another backend at the current version stay until that backend moves them.
- **Failure diagnostics.** `.failed` files are kept; they are already keyed by HLSL.

## Measurements (Vulkan, maintainer's cache copy, 2026-10-02)

| Run | Result |
|---|---|
| First start with 28,687 per-shader files | 28,687 moved, 0 DXC, 28,687 files deleted, 22.5 s preparation |
| Next start | startup-bundle hit, 2.4 s |
| Start without the startup bundle (as after a translator change) | 28,687 store hits, 0 DXC, 1.8 s preparation |

Afterwards `cache/shaders` held four entries (`builtin`, `shaders_spirv.lostore` at 858 MB, `source`, `startup_vk12_v1.bundle`) instead of 28,700. `LoShaderStoreTest` has 58 checks and passes with GCC and clang under ASan/UBSan and with clang-cl on Windows.

## Packs exported on 2026-10-02

The same branch adds `LO_SHADER_EXPORT_METAL=1`. Together with `LO_SHADER_EXPORT_PACK`, it makes a Vulkan run export the Metal contract, SPIR-V compiled at `-O1`. `LoShaderPackTool verify-runtime <pack> <image> --metal` checks that contract.

New sources came from the learned `cache/shaders/source` folders of the maintainer's install, the test folder and the map-tour and test runs, from the tour's microcode dump, and from 26 shader programs players uploaded through the opt-in feedback. 138 shaders were not in the 28,549-record pack.

| Pack | Contract | Records | Size |
|---|---|---|---|
| Vulkan `portable_vk.lospv` | `4e123a08…` | 28,687 | 180,527,457 bytes |
| DX12 `portable_dx12.lospd` | `58054972…` | 28,687 | 80,462,294 bytes |
| Metal `portable_metal.lospv` | `6fe8486e…` | 28,687 | 180,540,137 bytes |

- `verify-runtime` passed for all three against the private disc 1 image.
- The Vulkan pack keeps every earlier record and adds exactly the 138, including the map tour's runtime variants 2441, f8b1, cbad and f964.
- DXC produced the same SPIR-V at `-O1` and `-O3`, so the Metal pack has the same binaries as the Vulkan pack under its own contract.
- No Mac has loaded the Metal pack yet.
- None of the three is published yet.

## Not done

- The `builtin/` folder (506 host-shader files) still uses one file per shader.
- The startup bundle now duplicates the store. Removing it needs a warm-start measurement from the store alone.

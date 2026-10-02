# Shadow resolution and screen-space ambient occlusion — development checkpoint

This note describes unreleased source work. Shadow resolution defaults to native guest resolution (1×), and ambient occlusion (AO) defaults to Off. The Graphics menu offers shadow resolution 1×, 2× or 4× and AO Off, SSAO or GTAO; `settings.ini` stores them as `shadow_resolution` and `ambient_occlusion`. The choices appear in all five interface languages.

## Rendering scope

Shadow scaling applies only to identified ShadowDepthRT/ShadowDepthZ targets. The multiplier applies to each texture axis, so 2× uses four times the texels and 4× uses sixteen times the texels of 1×. It does not upscale every depth surface or replace the game's shadow algorithm.

SSAO and GTAO are experimental screen-space passes. They use current-frame resolved depth and color, reconstruct normals from depth, and composite visibility late over gamma-encoded scene color (and a matching extended-gamma HDR sidecar when available). They cannot separate ambient from direct light or replace native material lighting. The current 40-guest-unit radius is not calibrated to metres or tuned across scenes. If the projection, depth or matching scene inputs are unreliable, AO skips that scene. The GTAO horizon calculation is adapted from Intel XeGTAO; see its [MIT license](../../thirdparty/licenses/XeGTAO.txt).

`LO_AO_TRACE` enables bounded AO scene and input logging; the first few AO applications are logged without it. `LO_AO_DEBUG=ao`, `normals` or `depth` displays the corresponding diagnostic view. These are developer diagnostics, not additional Graphics settings.

## Validation and limits

The Windows clang-cl/Ninja RelWithDebInfo runtime build passed with DLSS and FSR disabled. It rebuilt the changed runtime sources with `LO_BUILD_RECOMP_LIB=OFF`, reusing the existing PPC static library from the same-HEAD main checkout; it was not a from-scratch PPC/code-generation build. Focused CPU checks passed for shadow scaling (3,337), render resolution (12,053), frame planning (105) and depth clear. Direct3D 12 and Vulkan GPU fixtures found zero per-pixel differences in the expected local depth clears at shadow 1×, 2× and 4×. Synthetic AO GPU fixtures passed on both backends; menu and config persistence checks passed.

On an NVIDIA RTX 5080, a 60-second Vulkan Uhra scene run with GTAO and 4× shadows logged AO mode 2 applied with depth and color from the same frame, a depth-only raster at 4×, and a shadow-depth resolve from 864 to 3456 pixels. A separate 60-second Direct3D 12 Uhra run with SSAO, 2× shadows and SMAA selected logged AO mode 1 applied with matching same-frame inputs and a depth-only shadow raster at 2×. A 55-second Vulkan Uhra run with GTAO, 1× shadows and TAA selected logged AO mode 2 applied and a 1×1 shadow viewport scale. The TAA selection does not independently prove per-frame TAA execution or history reuse.

None of these three runs exited early or logged an error, AO bypass, device loss or crash. Reviewed 3D and UI screenshots looked normal, and the checked baseline hashes were unchanged. The timed runner then stopped each game process deliberately; exit code -1 is not a natural game exit. These were moving scenes, not frozen visual comparisons or performance benchmarks. Local, unpublished records are in the `shadow-ao-7601/runs/final-gtao4`, `final-ssao2-d3d12` and `final-gtao1-taa` worktree folders.

Merge checks on 2026-10-02 (this branch on top of main `fe89910`+, shared SPIR-V packs):

- Default settings (1× shadows, AO Off), frozen Uhra save, 4K, uncapped, RTX 5080: screenshots of this branch and of main differ by about 1% of pixels (animated characters and foliage), the same as two main runs against each other, on both D3D12 and Vulkan. Frame rates were in the same range (D3D12 212 vs 204, Vulkan 194/218 vs 200/208 FPS), but another process was compiling during some runs, so these are not a benchmark.
- #118 world-map save, 1600x900: D3D12 and Vulkan with GTAO and 4× shadows rendered correctly; GTAO darkens crevices slightly.
- M1 Max, Metal, `new-game-battle`: default and GTAO with 4× shadows both passed; the GTAO run logged `AO applied mode=2` with no errors and a normal battle frame.
- Lenovo TB321FU (Adreno 750), Android: the Graphics page lists both settings; with GTAO and 4× shadows the opening battle logged a 4× shadow raster and `AO applied mode=2` and rendered normally.
- `LoShadowResolutionTest` now also runs in review_regressions.
- Independently of these settings, `DropResolved` moves a dropped resolved surface's texture and fetch views to the GPU slot's retired list instead of destroying them while the frame may still use them.

The AO and shadow paths are connected to anti-aliasing, super resolution, frame generation and HDR composition, but those combinations have not been comprehensively exercised. The recorded build had DLSS and FSR disabled. These Uhra checks cover one scene on each backend and do not establish quality, performance, full-map coverage or behavior on other platforms.

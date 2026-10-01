# MetalFX Temporal: design

The measurements and phase labels in this historical design note come from the
MikeRavenelle fork at [`b7951cd`](https://github.com/MikeRavenelle/LostOdysseyRecomp/tree/b7951cd5aa0830b5342aeb3efa835f878e227796).
They are not M1 Max re-tests for this integration. See [development status](STATUS.md)
for the current build and validation boundary.

Status, 2026-09-29: phase 1 done. Upscaler setting 3 (`upscaler=3`, quality from
`fsr_quality`) runs the scene-copy promotion on Metal with MetalFX Spatial as a
stand-in provider. Two Metal-specific requirements surfaced:

- The provider opens and closes the isolated command list itself (as NGX and FSR
  do).
- plume's resources are untracked, so the prefix, isolated and continuation
  command buffers can overlap on the GPU. `EncodeMetalQueueSignal` /
  `EncodeMetalQueueWait` (plume patch) order them with a queue event; without
  this the composite read the scratch target before MetalFX wrote it (black scene).

Phase 2 done: `plume::EncodeMetalFxTemporalScale` records `MTLFXTemporalScaler`
with colour (RGBA8), depth (R32F, reversed), motion (RG16F) and the Halton jitter.
Colour now matches native rendering (the spatial stand-in's harsh look is gone).

Measured in Numara (M1 Pro, 120 FPS target, 720p -> 2560x1440, median after arrival):

| Path | GPU ms/frame | FPS |
|---|---|---|
| Native 1440p | 30.2 | 33 |
| MetalFX Temporal, object motion replay | 26.5 | 37 |
| MetalFX Temporal, camera/depth motion (default) | 21.9 | 45 |
| Plain 720p (no upscaling) | 17.4 | 54 |

The MetalFX dispatch costs ~3 ms. Replaying the scene for object motion vectors
costs ~6 ms more: this game is vertex/tiling-bound on Apple GPUs, so a second pass
over the geometry is expensive. MetalFX therefore uses the existing SR hybrid
camera/depth motion (`sr_hybrid_motion_gpu.h`, now enabled on Metal) by default;
`LO_METALFX_OBJECT_MV=1` restores object vectors. Moving characters get camera
motion only, and MetalFX's disocclusion handling covers them.

Conventions confirmed on screen: motion vectors in render pixels with scale 1
(`LO_METALFX_MV_SCALE` overrides), jitter passed with FSR's sign (the opposite
sign visibly blurs static detail; `LO_METALFX_JITTER_SIGN` overrides).

Phase 3 done: on macOS the Anti-aliasing / Upscaling row's fifth choice is
"MetalFX Temporal", the quality row (FSR quality IDs) is labelled MetalFX, and the
status line reports MetalFX results. Failures reuse the SR fallbacks: a frame
without eligible inputs renders normally; a scaler failure disables the request
for that plan (logged with its step).

Remaining (phase 4): battle and cutscene scenarios, 1080p -> 2160p measurement on
an external display, and a look at moving characters during fast camera turns
with camera-only motion.

## Goal

Add MetalFX Temporal as a third super-resolution provider next to DLSS and FSR, so
the Metal backend can render the 3D scene at a low internal resolution and upscale
it with temporal accumulation before the game draws its UI.

MetalFX Spatial (already shipped, `ScalingMetalFx`) upscales the finished frame,
UI included, and has no history. Temporal reconstructs detail from jittered frames
and upscales only the scene; the UI is drawn at output resolution afterwards.

## Expected gain (measured)

Numara, M1 Pro, 120 FPS target, median after arriving in the city:

| Internal resolution | GPU ms/frame | Frame ms | FPS |
|---|---|---|---|
| 720p | 17.4 | 18.5 | 54 |
| 1440p | 30.2 | 30.6 | 33 |

That is ~13 ms of resolution-independent GPU work (vertex/tiling, structural
passes) plus ~4.6 ms per megapixel. Rendering 720p and upscaling to 1440p should
cost 17.4 ms plus the MetalFX dispatch (to be measured; expected 1-2 ms at 1440p
output), about 50 FPS against 33 FPS at native 1440p, with close-to-1440p
detail. At 30 FPS it lowers GPU load and power (laptop battery and heat).

## How upscaling works today (DLSS/FSR)

The game renders the 3D scene, then copies it into another target and draws the
UI on top. The renderer detects that full-scene copy and *promotes* it:

1. The renderer collects temporal inputs during the scene: color, depth, motion
   vectors (`motion_replay_gpu.h`), a motion-invalidity mask, the camera and
   Halton jitter (`temporal_jitter.h`). These arrive as
   `temporal::TemporalFrameInputs` (`temporal_frame_inputs.h`).
2. `frame_plan.h` picks the consumer (`DlssSr` or `FsrSr`) and the render extent
   from a per-provider sizing query (`upscaling::SizingCache`).
3. At the scene copy, `PrepareSceneCopyDestination` allocates output-size
   promoted, scratch and composite targets. `RecordSceneCopyDlssUsing`
   (`renderer.cpp`) closes the current command list (prefix), records the
   provider into a separate isolated list (`srIsolated`), and continues in a
   third list (`srContinuation`). The composite pass writes the upscaled RGB into
   the promoted target while keeping the alpha from the game's own copy.
4. The UI then renders at output resolution into the promoted target.

Provider dispatch goes through `gpu::TemporalUpscaler`
(`temporal_upscaler.h`): `QuerySizing`, `Prepare`, `RecordIsolated`, and
lifetime hooks (`OnSubmitted`, `OnDiscarded`, `ReleaseCompleted`, shutdown).
Only Vulkan and D3D12 overloads exist.

### What is gated off on macOS today

- `video.cpp` creates no `dlss::Controller` or `TemporalUpscaler` on macOS.
- `renderer.cpp` creates `srIsolated`/`srContinuation` only when a DLSS
  controller exists, and the promotion call site requires `dlssController`.
- `BackendDeviceSnapshot::Available` accepts only Vulkan and D3D12.
- The menu hides DLSS/FSR on macOS.
- Casts inside `RecordSceneCopyDlssUsing` assume `VulkanDevice`,
  `VulkanCommandList` and `VulkanTexture` unless the backend is D3D12.

Already working on Metal: the motion-vector replay (camera TAA runs at 30 FPS in
Numara with MV ready every frame), `temporal::HistoryOwner`, jitter, and the
scene-copy promotion shaders (they compile through the shared HLSL to SPIR-V to
MSL path).

## Design

### 1. Provider identity (all platforms, append-only)

- `upscaling::Upscaler::MetalFx = 3`, and `TemporalConsumer::MetalFxSr = 5`.
  These are persisted and wire-format enums: append, never renumber. Update
  `KnownUpscaler`, `KnownTemporalConsumer`, `IsSrConsumer`,
  `ProviderForConsumer`, `MatchesSrProvider`, `SameEffectiveQuality`,
  `ValidSrRequest`, `RouteConsumer`, `dlss_status_log.h` names.
- Quality: reuse `FsrQuality` (Quality 1.5x, Balanced 1.7x, Performance 2.0x,
  NativeAA 1.0x). MetalFX has no fixed modes; the ratio decides the render size
  within the scaler's `supportedInputContentMinScale/MaxScale` range.
- `BackendDeviceSnapshot` gains `metalFxAvailable`; `Available()` accepts the
  Metal backend only for `Upscaler::MetalFx`.

### 2. plume (Metal patch, `tools/patches/plume-macos.patch`)

Next to the existing `EncodeMetalFxSpatialScale`, add:

```cpp
struct MetalFxTemporalDesc {
    RenderTexture *color, *depth, *motion, *output;
    RenderTexture *reactiveMask;          // optional
    uint32_t renderWidth, renderHeight, outputWidth, outputHeight;
    float jitterX, jitterY;               // pixels, [-0.5, 0.5]
    float motionScaleX, motionScaleY;     // converts MV texels to pixels
    bool depthReversed, reset;
};
bool SupportsMetalFxTemporal(RenderDevice*);
bool EncodeMetalFxTemporalScale(RenderCommandList*, const MetalFxTemporalDesc&);
```

- Caches one `MTLFXTemporalScaler` keyed by formats and sizes; a key change
  recreates it (the caller treats that as a history reset).
- Ends the active encoder and encodes into the list's `MTLCommandBuffer`, as the
  spatial path does. plume's untracked resources need its existing fence
  handling around the encode (same as spatial).
- `MTLFXTemporalScalerDescriptor` requires the input/output formats and usages
  up front: color, depth and motion need `ShaderRead`; output needs
  `RenderTarget` (and `ShaderWrite` on older OS versions), private storage.

### 3. Host upscaler backend (runtime)

`gpu/metalfx_upscaler.{h,cpp}`, macOS only, mirroring `fsr::Controller`:

- `QuerySizing`: computed locally from the quality ratios and the scaler's
  supported scale range; publishes `SizingState::Ready` without GPU work.
- `Prepare`: validates formats and extents, creates or reuses the scaler.
- `RecordIsolated(plume::RenderCommandList&, ...)`:
  1. Input conversion pass (render extent) when the guest formats are not
     accepted by MetalFX: color to RGBA16F (or RGBA8 for SDR), depth to a
     depth or R32F format, motion to RG16F in pixels. The FSR path does the same
     conversion (`linearColor`, `canonicalDepth`) and is the reference.
  2. `EncodeMetalFxTemporalScale` with jitter from `inputs.jitter.pixelX/Y`,
     `reset = inputs.resetHistory`, depth reversed per `inputs.depthConvention`.
  3. The motion-invalidity mask becomes the reactive mask where the OS provides
     one (reactive mask support is newer than the base scaler). Otherwise
     invalid regions rely on MetalFX's own disocclusion handling.
- Lifetime: MetalFX has no external use tokens, so `OnSubmitted`/`OnDiscarded`
  only release per-frame leases; `ReleaseCompleted` frees conversion targets
  retired on a device-epoch or extent change.

`TemporalUpscaler` gains a `plume::RenderDevice&`/`RenderCommandList&` overload
set, compiled on macOS, that routes `Upscaler::MetalFx` to this controller. The
DLSS controller reference becomes optional on macOS (no NGX there).

### 4. Renderer wiring

- Create `srIsolated`/`srContinuation` when a `TemporalUpscaler` exists, not only
  a DLSS controller. On Metal, `EndGpuCommands`/`BeginGpuCommands` already map
  onto plume command lists, so the prefix/isolated/continuation split carries over.
- The promotion call site requires `temporalUpscaler` instead of
  `dlssController`.
- `RecordSceneCopyDlssUsing`: add a Metal branch that calls the generic
  overloads. Barriers: inputs `SHADER_READ`; the scratch output uses the layout
  plume's Metal backend needs for a MetalFX write (render target).
- `CreateSceneCopyScratch`: output format matches the source (RGBA16F/RGBA8),
  with `RENDER_TARGET | STORAGE` flags.
- The composite pass and UI path stay unchanged.

### 5. Settings and menu

- The upscaler setting gains a MetalFX choice on macOS; DLSS/FSR stay hidden
  there. Quality reuses the FSR quality row.
- Interaction with MetalFX Spatial (`scaling_quality = 2`): Temporal already
  outputs at drawable size, so the final present scaling becomes a no-op. Keep
  both selectable; document that Temporal replaces Spatial for the scene.
- Anti-aliasing: an SR consumer forces the scene AA off (`effectiveAA = 0`), as
  DLSS/FSR do today.

## Risks and open questions

1. **Scene-copy promotion has never run on Metal.** It is the largest unknown:
   the three-list split, texture layouts in plume's Metal backend, and the alpha
   composite. Phase 1 de-risks it with a trivial provider.
2. **Motion vector units and sign.** FSR takes them with scale `{1, 1}` (pixels,
   pointing to the previous frame). MetalFX expects the same convention with an
   explicit `motionVectorScale`; confirm on screen with a static camera pan.
3. **Depth format.** MetalFX's accepted depth formats must be checked against
   what `canonicalDepth` produces; a conversion pass is cheap if needed.
4. **Jitter source.** Jitter is only applied to classified scene viewports
   (`IsJitterViewport`); anything drawn without jitter must still converge.
5. **Colour encoding.** SDR scenes should go through as display-encoded RGBA8;
   verify whether MetalFX needs linear input for good results (FSR converts to
   linear first).
6. **OS support.** MetalFX Temporal needs macOS 13 and a supported GPU (all Apple
   Silicon). Check `supportsDevice:` at startup; the option is hidden when false.

## Phases

1. **Promotion on Metal with a stand-in provider.** Enable the promotion path on
   Metal with a provider that bilinear-scales color into the scratch target.
   Proves the split, layouts and composite. Scenario: Numara, screenshot compare
   against native output resolution.
2. **plume `EncodeMetalFxTemporalScale`** plus the Metal upscaler backend and
   input conversion. Scenario: Numara with TAA-style camera motion; check ghosting
   and shimmer in screenshots, `LO_RENDER_TIMING` GPU time.
3. **Settings, menu, sizing,** capability publishing, and failure fallbacks (frame
   fallback to spatial, disable-request on scaler creation failure).
4. **Validation:** full scenario suite, new `numara-metalfx-temporal` and a
   battle scenario, 720p->1440p and 1080p->2160p measurements, docs.

Nothing here changes XenonRecomp or the guest code. The provider interface stays
backend-neutral, so a Linux AArch64 Vulkan build keeps FSR through the existing
Vulkan path.

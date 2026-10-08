# Lost Odyssey Android — Phase 12 surface, aspect and performance test

Phase 11 phone evidence: the user reached the opening battle and navigated menus with touch controls. The supplied appended log records a complete Disc 1 import, native guest launch, 1602 successful guest presentations during the two-minute observation and 152 successful shader compiles. Screenshot/user report establishes useful graphics and input. Performance remained slow. Game audio submission is logged, but audible quality was not reported. The log contains 2322 same-dimension swapchain-resize reports; that is a performance lead, not a proven sole bottleneck.

## Changes

Android Vulkan needsResize now compares the window request with the same fixed/clamped surface extent used by resize. SDL drawable dimensions can differ from the authoritative Vulkan currentExtent, otherwise provoking repeated identical rebuilds with device-idle waits. A shared tested extent resolver is used for creation and comparison. Surface-out-of-date, empty swapchains, present-mode changes, zero extents and real size changes retain recovery. Duplicate Android same-size window events no longer force rebuilds when needsResize reports no change. Desktop event behavior is retained.

Android scene output now uses the whole drawable, including the Fold inner display's taller aspect. The existing camera hooks extend projection adjustment to the vertical column for taller output, so the scene exposes more vertical view rather than stretching shapes. Main-camera detection and original frustum-generation order remain. Primary HUD canvas matrices and scissor edges fit both axes and restore guest state after flush. Movies fit both axes in the native safe region; vertical black bars are cleared using the renderer's selected frame plan. Native 16:9 footage remains letterboxed. Wider displays retain horizontal expansion. This is experimental: host math checks cannot establish camera correctness, culling, movie composition, every UI path or fold transitions on the phone. Desktop scene output keeps its prior 16:9 lower aspect limit.

Every ten seconds the log records interval successful presentations/second, frontbuffer-call count, average elapsed host frontbuffer-presentation-path milliseconds, and successful swapchain-resize count. Counters are atomic. Host timing includes waits, is not a GPU timestamp, and does not by itself distinguish CPU work from GPU synchronization. Existing shader work counters remain. No full-speed promise is made.

Same bounded two-minute test, touch controls, package, signing key, separate :bootcheck process, import verification and installed-disc reuse are retained. No ISO or game data is bundled or modified.

## Checks

- Surface extent test passed: SDL/fixed-extent mismatch, stable extent, orientation change, variable clamp, and zero surface.
- Existing aspect-layout suite: 20630 checks passed, covering projection scale/shape fitting, canvas composition/scissors and safe-area maths.
- Android output-region test passed for tall full-height, wide and zero drawable. Desktop output-region fixture passed, retaining taller-screen letterboxing.
- Video, renderer, camera hooks, frame plan and readiness units compiled with NDK r29/API 28/ARM64. Updated plume Vulkan archive member compiled; complete runtime relinked. Final video event filtering recompiled/relinked before packaging. Changed objects retain debug information.
- Java/DEX packaging, signature verification and 16 KiB ZIP alignment passed. All four native ELF libraries have LOAD alignment at least 16 KiB, packaged bytes match checked stripped inputs, signing certificate matches restored original Runtime Check key. Native APK contains the Phase 12 marker and interval metrics.
- All 62857 generated PPC definitions remain. All 226 strong import-function exports exactly match Phase 6, retaining previously verified required hooks. Symbol coverage is not gameplay correctness.
- Package io.github.freefrank.lostodyssey.runtimecheck; versionCode 7, versionName phase12-surface-aspect-metrics, Activity process :bootcheck, ARM64, min API28/target35.
- APK bytes 114459708; SHA-256 02d2c1d1c8647f5941eb846668ef59b3b0ffe34801bfda317e71e96724e83e59.

## Phone test

Install over the existing app without uninstalling. Keep the same graphics settings as the last run. Continue to reuse the installed Disc 1, then Continue after executable setup to begin the observation. On the inner display check whether the 3D view fills the height without distorted characters, HUD stays readable and movies remain proportioned. Share the final log and a screenshot. Report steady speed versus pauses and any camera/UI/culling artifacts. Changed aspect can change rendering workload, so this is not a controlled isolation of the resize fix alone. Cover/inner transitions and audible game output are still unverified.

## Recovery

Apply the cumulative patch to upstream 66f033b76a7ad44ca8d3491adb3a83a933dd6c1e. Initialize submodules and apply plume-lostodyssey.patch BEFORE the updated plume-android.patch; apply the XenonRecomp patch. Restore private generated guest headers/sources from Phase 2 and pinned DXC/FFmpeg from Phase 6 Source. Use NDK r29/SDK35/Java17.

For fast recovery restore COMPLETE Phase 6 Private Compiled Checkpoint, then overlay Phase 12 Private Checkpoint. It includes the matching unstripped runtime, all ten changed runtime objects relative to Phase 6 (the earlier eight plus hor_plus/frame_plan) and updated libplume.a. Earlier overlays are unnecessary. Preserve base dependency archives, relink helper/link argv, SDL/DXC and original signing key. Compile helpers contain this workspace's absolute paths and must be adjusted after recovery. Private checkpoints/generated game code/keys stay private; public source excludes them.

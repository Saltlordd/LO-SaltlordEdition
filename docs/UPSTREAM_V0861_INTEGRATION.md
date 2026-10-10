# v0.1.1 upstream integration candidate

Base: public beta 89317cf6e858baed3e543bd9e96f106f4f26ffbf.
Reference: upstream v0.8.61, 5d1958b443cfea78068b26c703092db009259692.
Separate branch: saltlord-upstream-v0861-candidate. Public beta unchanged.

## Integrated
- First upstream CPU optimisations: dirty ALU blocks, register snapshots, geometry preparation, index upload reuse, texture checks and selected dispatcher/hash improvements.
- Live, persistent Motion blur and Dynamic shadows switches in Android Graphics. Original guest functions remain the enabled path.
- Selected Vulkan barrier and render-pass fixes, viewport/scissor caching, resolve-copy reuse, presentation target caching and same-size FXAA direct output.
- Occlusion owner tracking prerequisite and adaptive query policy, expanded query capacity and corrected awaited-query bookkeeping.
- Android controls, Flex layouts, setup, saves, controller profiles and Fast Forward preserved.

Apply tools/patches/plume-v0861.patch after the existing plume-lostodyssey.patch and plume-android.patch on the pinned Plume checkout. The additional patch was checked against that exact combination. SDL retains the existing Android patch.

## Deferred
This is a selective backport, not a complete v0.8.61 rebase. Remaining sync-epoch/frame-plan and register-run changes, device-address vertex fetching and low-32-bit shader arithmetic require missing prerequisites. Temporal jitter fixes, newer AO/HDR/upscaling contracts and SDL3 migration are also deferred. NO_END_DEP experimental synchronization was not enabled.

## Actual validation
Fresh ARM64 native runtime build: 1,162 actions, including recovered generated PPC source, SDL, FFmpeg and Plume. Fresh DXC build: 1,266 actions after 188 host table-generator actions. Fresh adrenotools hooks, Java compilation, DEX and APK packaging completed. Previous native app libraries were not repackaged.

Host tests passed: register snapshots, geometry preparation, vertex cache, texture content, captured shader identity, dispatcher waits, occlusion bookkeeping, production graphics-effect dispatch with mocked imports, SavedGraphics persistence fixtures, touch cheats/Fast Forward, Android output/aspect/surface lifecycle and pipeline cache tests, and four PPC generator tests. Link and diff checks passed.

APK v3 signing and 16 KiB ZIP alignment verified. All nine packaged native ELF libraries have load alignment of at least 16 KiB. Signing certificate matches the recovered earlier signed APK; update installation against the installed public beta still requires phone testing. No signing secrets are included in this document or source patch.

No device gameplay, Vulkan execution, thermal/performance comparison, native shader translation execution or visual regression testing was performed here. Generated PPC translation units were rebuilt from the recovered checkpoint, not regenerated from a game executable.

## Phone release gates
1. Install over beta without uninstalling; confirm saves, settings and profiles survive.
2. Cold launch, shader preparation/restart, Android Back/reopen and log export/cancel/retry.
3. Repeat demanding cutscene and battle on a cold cache and again warm; export non-empty diagnostic reports for both.
4. Compare 720p/30, 720p/Dynamic 60/SMAA and 1080p/30/SMAA; watch rendering, shadows, geometry and occlusion for regressions.
5. Toggle Motion blur and Dynamic shadows live, then cold reopen and confirm persistence.
6. Cover/inner transition, both Flex layouts and editors, controller profiles, Fast Forward, Options pause and performance monitor.
7. Save anywhere plus backup/restore using a separate test backup.

Do not publish this test artifact as the public release until these checks pass. Prepare final release branding/version after acceptance; candidate currently identifies itself as 0.1.1-test, version code 52.

This document records Test 1. See [Test 2 integration](UPSTREAM_TEST2_INTEGRATION.md) for the next candidate.

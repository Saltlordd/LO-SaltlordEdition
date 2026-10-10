# Android v0.1.1 Test 2 integration

This is a selective Android backport of upstream v0.8.61 (`5d1958b`) onto the Saltlord public beta and Test 1 checkpoint. Latest upstream `690e91b` was also reviewed: the newer Windows DXGI fixes and desktop upscaler supersampling are not Android performance fixes and are excluded here.

## Added after Test 1

- CPU trim #314: block-wise register comparisons, synchronisation epochs, epoch-aware vertex validation, frame-plan and catalog generations, framebuffer and tile-owner caches, and bounded request polling. Adapted to the existing Android renderer without importing unavailable gamma/shadow-resolution contracts.
- Android-safe 24-byte shader push addresses using pairs of 32-bit words; vertex buffer device-address fetching with bounded scalar/vector loads; lower-word constant-bank arithmetic. Upload buffers are guarded against crossing a 4 GiB window and invalid device addresses fail safely.
- Shader cache version 27 and a distinct Vulkan contract identity invalidate shaders compiled with the earlier layout.
- The complete reviewed temporal header and capture fixture chain, including boat, sky, battle, cutscene and multi-camera/depth/shadow-projection corrections.
- The framebuffer key type follow-up fix.

Test 1 motion-blur and dynamic-shadow switches, adaptive occlusion, Vulkan recording fixes, Android setup wizard, touch controls, Flex layouts, saves, controller profiles and fast-forward remain included.

## Validation so far

Host checks passed: register snapshots (18,443), bounded vertex cache (3,668,957), temporal scene ordering, arena policy (35), shader identity (74), occlusion bookkeeping, captured shader fixtures (2,082), kernel wait handles (45), temporal jitter fixtures (6,270,523), graphics-effect dispatch and 82 production Android input-adapter checks. The pinned host DXC compiled the production BDA prelude to valid Vulkan 1.2 SPIR-V (5,416 bytes), retaining all three address pairs, all fetch widths and constant-bank loads.

These are host/build checks, not device rendering or performance validation. Native Android compilation and signed packaging are pending in this source checkpoint. The original signing key is private; package versionCode 53 / versionName 0.1.1-test2. No public release is changed.

## Scope and phone testing

SDL3 input/lifecycle migration, newer AO/HDR/upscaling contracts and desktop settings/menu changes remain separate work. Experimental NO_END_DEP stays disabled. This candidate does not claim complete upstream parity or guaranteed FPS.

Install as an update without uninstalling. Expect shader preparation followed by restart because the shader contract changed. Compare the same scene/settings after preparation, then check geometry, shadows, water/boats, battles and cutscenes. Also exercise fold/screen/Flex switching, controllers, fast-forward, Options pause and save/layout backups. Hardware, driver and thermal conditions affect performance.

Based on freefrank's LostOdysseyRecomp and its contributors; integration was AI-assisted. Public identity: Saltlord.

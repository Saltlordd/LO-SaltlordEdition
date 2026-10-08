# Lost Odyssey Android Phase 7

The Phase 6 phone log confirmed runtime loading, guest memory allocation, DXC vertex/pixel SPIR-V compilation and GameSir input. Phase 7 adds basic touch controls, Disc 1 ISO access and production Vulkan renderer startup. **It does not launch Lost Odyssey. Phase 7 execution needs phone testing.**

## Phone test

1. Install LostOdyssey-Android-Touch-Renderer-Test.apk over Lost Odyssey Runtime Check. Same package/signing key; versionCode 2.
2. Tap Continue. For 20 seconds use both touch sticks, D-pad, A/B/X/Y, LB/RB, LT/RT, L3/R3, Start and Back. Try a stick and a button simultaneously. A controller is optional for this test.
3. Choose the local Disc 1 ISO when prompted. This reads its directory and executable; no whole-disc copy or installation occurs. Choose the ISO, rather than default.xex. Skip is available.
4. Wait for the production renderer check, then Share log. A blank background is expected: frame presentation is not tested. If the app closes before results, reopen and share the existing log from the first dialog.

## Changes

- A fixed translucent Android Canvas overlay tracks finger IDs, supports simultaneous controls, radial stick clamping, D-pad diagonals and binary triggers. Controls scale with the surface. No editor, controller auto-hide, presets or haptics are added.
- JNI publishes a complete mutex-protected touch snapshot. Production GetState merges buttons, maximum triggers and stronger per-axis stick values before existing menu filtering. Host input includes touch buttons/triggers for existing chords. Pause, focus loss, resize, touch cancellation and shutdown clear state.
- The Android file picker grants a read-only descriptor to the SDL native thread. An Android-only production IsoImageReader overload validates a regular seekable file and opens /proc/self/fd. It reuses XDVDFS scanning and bounds checks; desktop path/link policy remains unchanged. Providers that prohibit descriptor reopening or supply streams report failure.
- The asset check lists files, verifies the exact previously supplied Disc 1 XEX SHA-256 and reads up to eight other file samples. This tests selected source access, rather than complete asset integrity, installation, extraction or persistent mounts. The descriptor closes afterward.
- The temporary test window is destroyed before production gpu::video::Init, including presentation pipelines and renderer resources, runs on the same SDL thread. No headless/no-renderer bypass is set. Startup success/failure is logged, events run briefly, then production shutdown releases resources before HID/SDL shutdown. No guest threads run.

## Validation

- NDK r29/API 28 ARM64 compilation of the three changed native units and full runtime relink succeeded using matching saved Phase 6 objects.
- Host touch tests passed for physical/touch merging, neutral release and 100,000 concurrent snapshot writes.
- Production ISO reader host tests passed for standard, padded and chunk-boundary layouts, descriptor ownership after original closure, and rejection of pipes/invalid descriptors. These exercise the Android-specific branch on Linux, rather than a phone provider.
- Java compilation, DEX packaging, APK signature verification and 16 KiB ZIP alignment passed. Packaged native bytes match the checked libraries; all four ELF LOAD segments have at least 16 KiB alignment.
- Exported touch JNI entry and SDL_main, 62,857 PPC definitions and 222 strong kernel hooks are present. Symbol coverage does not prove guest execution accuracy.
- Implementation source commit: 90bae5de8da21059fbb8cb0532f6154ace017601
- Package io.github.freefrank.lostodyssey.runtimecheck; versionCode 2; phase7-touch-assets-renderer; min API 28, target API 35, ARM64.
- APK bytes 114459708; SHA-256 bef3d6514c0d7c6fd8dd2428a83ba28adcf1ae7e9984055cdca2443c30fd4b7c.

## Remaining work

No disc installation, guest boot, game shader translation, presented frame, game audio or gameplay is exercised. Renderer startup, provider access, touch behavior, repeat launches and Android background/surface transitions need the new phone log. Full asset installation/access during guest execution and a first real frame remain subsequent stages. Gameplay use of the touch controls remains unverified until guest boot works. Fold/cover/other-phone control sizing is also untested.

## Rebuild and recovery

Apply android-phase7-combined.patch to baseline 66f033b76a7ad44ca8d3491adb3a83a933dd6c1e. Initialize pinned submodules and apply the supplied XenonRecomp/plume patches. Restore private generated PPC sources from the Phase 2 checkpoint. Phase 6 Source.zip retains the pinned DXC/FFmpeg source snapshots; helpers may fetch pinned sources for clean builds. Use NDK r29, SDK 35 and Java 17 with configure_android_runtime.py, build_android_dxc.py and build_android_runtime_check.py.

For fast recovery extract the COMPLETE Phase 6 Private Compiled Checkpoint, then overlay Phase 7 Private Checkpoint into it. This replaces the three changed objects and runtime library; untouched archives, SDL and DXC come from Phase 6. Run existing relink.py with NDK r29, then the APK packager. The relink was exercised here. Preserve the Phase 6 debug keystore for updates. Keep generated code, compiled checkpoints and keys private.

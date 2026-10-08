# Lost Odyssey Android Phase 8: direct ISO access and crash diagnostics

Phase 7 phone testing confirmed touch sticks/triggers and buttons 0xf3f7 (D-pad Right was not observed). Selecting Disc 1 failed because its granted descriptor could not be reopened through /proc/self/fd. Renderer startup also crashed after shader-cache initialization when ISO selection was skipped, confirming a separate failure. Vulkan device and swapchain creation succeeded; complete renderer startup did not.

This update addresses descriptor reopening and adds renderer diagnostics. **The renderer crash is not yet fixed or pinpointed. No gameplay is exercised.**

## Phone test

1. Install LostOdyssey-Android-ISO-Fix-Test.apk over Lost Odyssey Runtime Check (same signing key/package; versionCode 3).
2. Continue through the touch test. Please explicitly try D-pad Right as well as both sticks and triggers.
3. Select the same local Disc 1 ISO. The new dialog shows the ISO access result and allows Share log before graphics starts. No whole-disc copy is performed.
4. Tap Continue to run renderer startup. If it crashes, reopen the app and Share log from the first dialog before continuing. The log is appended across launches.

## Changes

- Android IsoImageReader duplicates the granted descriptor using F_DUPFD_CLOEXEC and uses positional pread with partial-read/EINTR handling and 64-bit offset checks. It does not reopen /proc/self/fd or request additional storage permissions. The duplicate is closed on destruction and failed construction. Original descriptor offsets remain unchanged. Local regular seekable files remain required.
- Both constructor consumers were recompiled after the Android reader layout change: import_game.cpp and the diagnostic, together with import_image.cpp. Desktop path sources still use ifstream; partition scanning shares the same bounded raw-reader helper.
- ISO results have a separate touch/share dialog before renderer startup, so successful asset access can be recorded independently.
- Android diagnostic shader tracing logs builtin/cache and actual compiler begin/end boundaries. Renderer initialization logs boundaries around blit, scene-copy, transfer shader and known shader/pipeline preparation. Normal desktop builds do not use this tracing.
- Diagnostic-only signal handlers use the existing emergency append sink to record signal, ARM64 PC, fault address and runtime/compiler module bases. Formatting uses a fixed stack buffer, no logger mutex or allocation. The previous handler is restored and the signal is raised again to preserve original crash handling. Native crash capture remains phone-untested and cannot guarantee a report for every failure.
- Restored the Android plume patch context intended to apply after upstream plume-lostodyssey.patch. The compile used that upstream patch plus the Android SDL changes, matching the saved dependency's interface.

## Validation

- NDK r29/API 28 ARM64 compile and full shared-runtime relink passed for all five changed native units, retaining the other matching Phase 6/7 objects. The touch input object remains the Phase 7 version.
- Host ISO tests passed standard, padded and chunk-boundary partition scans; duplicate ownership after original closure; pipe/invalid-descriptor rejection; a sparse read beyond 5 GB; and preservation of the source descriptor's seek position. These test the Android branch on Linux, not Samsung's file provider.
- Existing host touch merging/release/concurrent snapshot test passed.
- Java/DEX packaging, APK signature verification and 16 KiB ZIP alignment passed. All four native ELF LOAD segments have at least 16 KiB alignment. Packaged native bytes match the verified stripped libraries.
- All 62,857 PPC definitions and 222 strong kernel hooks remain present; this does not prove guest correctness.
- Implementation commit: 58f5556039a791986bbc0b889743fbcbe9ed37af
- Package io.github.freefrank.lostodyssey.runtimecheck; versionCode 3; phase8-direct-iso-crash-log; min API 28, target API 35, ARM64 only.
- APK bytes: 114426940; SHA-256: ee616dff51ca1d536c222b6a6c987f84df0e16e1fa264822333fbade39e027b0.

## Limits and recovery

Phase 8 is not phone-verified. ISO directory access, exact Disc 1 executable hashing and up to eight sampled asset reads are checked, not full asset integrity or installation. Renderer crash localization requires the next phone log; there is no guest boot, game shader translation, presented frame, game audio or gameplay.

For a clean build apply the combined patch to upstream 66f033b76a7ad44ca8d3491adb3a83a933dd6c1e, initialize pinned submodules, apply upstream plume-lostodyssey.patch THEN plume-android.patch and the XenonRecomp patch. Restore private generated PPC sources from Phase 2. Use pinned compiler/decoder sources retained in Phase 6 Source.zip or the source-fetching build helpers. Configure/build with NDK r29, SDK 35 and Java 17, then package with build_android_runtime_check.py.

For fast recovery overlay the Phase 8 Private Checkpoint into the COMPLETE Phase 6 Private Compiled Checkpoint. It contains the updated runtime and all six replacement objects (the Phase 7 HID object plus the five Phase 8 units); it does not require the separate Phase 7 overlay. Retain untouched archives, SDL, DXC, relink.py/link-argv.json and the debug key from Phase 6. Relink with NDK r29, then package. Preserve the unstripped matching runtime for resolving logged addresses. Keep generated code, compiled checkpoints and signing keys private.

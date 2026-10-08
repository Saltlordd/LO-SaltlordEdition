# Lost Odyssey Android Phase 9 — executable setup and touch feedback

## Result

A signed ARM64 startup-test APK was built. Phase 9 is not phone-verified and does not execute the game or present game frames.

The latest phone log confirms Phase 8 Disc 1 directory access, eight sampled reads and the exact executable SHA-256. It then crashes at PrepareKnownShaders. The crash PC minus runtime base is 0x1dedfe4; the matching unstripped Phase 8 library resolves it to xenos::resources::Sha256Incremental::Block. The diagnostic had never loaded the executable before this code hashed a span of guest memory. This evidence supports the missing-executable setup as the crash cause; the next phone run must confirm the fix.

## Changes

- After verifying the selected Disc 1 executable, save only its 6,623,232 bytes to app-private cache. Initialize production guest allocators and the filesystem root, then use production XexLoader::Load including decrypt/decompress, guest copy and variable import binding. Require the expected base 0x82000000, size 20,709,376 and entry 0x827ca440. No guest thread starts; no timestamp thread starts.
- Guard renderer game shader preparation before constructing a span when there is no loaded executable or its identity prefix is unavailable. Normal loaded-game shader preparation retains its behavior.
- The bounded diagnostic defers game shader and pipeline prebuilding because the complete game assets are not installed. Built-in renderer shader compilation/module creation remains enabled. A shader-prebuild skip is not evidence of game shader compatibility.
- Draw moving knobs on both analog sticks. Existing pointer capture, clamped analog movement and independent finger tracking are retained. Draw four separate D-pad arrow pads with directional feedback; the directional touch region still supports sliding and diagonals. The previous log proved directional bits were observed, not that the static-circle controls were usable or visually explained.
- Increase package versionCode to 4, versionName phase9-xex-renderer-touch. Preserve the existing Runtime Check package and signing key for an update install.

## Verification

- Two changed native units cross-compiled using NDK r29 / API 28 / ARM64, with debug information retained for future crash symbolization. Relink of the actual runtime succeeded with existing dependencies and generated guest-code archive; no undefined-symbol substitute added.
- Host Java harness exercises the production TouchGamepadView with minimal Android API stubs: centered stick, drag outside bounds, full-scale clamping, both sticks with reordered pointer IDs, partial finger release, cancellation, D-pad right and diagonal, drawing two knobs/four arrows, and resize release. Passed. This does not test Android event delivery, visual rendering on the phone or guest input.
- Existing C++ touch merging/release/concurrent snapshot test passed.
- Java compilation, DEX packaging, APK signature verification and 16 KiB ZIP alignment passed. Packaged native bytes match checked stripped libraries; all four native libraries have ELF LOAD alignment at least 16 KiB.
- All 62,857 generated PPC definitions remain present. The 226 strong import function exports exactly match the Phase 6 base (including the previously verified 222 required kernel hooks). Symbol presence does not establish guest correctness.
- APK size: 114,443,324 bytes. SHA-256: f1ea4baa2a2a99dc2d856df4e2b412893f436f0f584eca7a1551a69e2399e367.

## Phone test

Install over Lost Odyssey Runtime Check without uninstalling. Tap Continue at the first dialog; during the input window drag both sticks and press the arrows/buttons. Select Disc 1 ISO. Tap Continue on the asset/executable results dialog to run the renderer check. Share the final log. If it crashes, reopen and Share log from the first dialog before continuing.

No full disc installation, persistent ISO mount, first guest boot, game shader translation, game frame or game audio is claimed. If this passes, connect the full game assets before attempting a first guest boot.

## Recovery

Apply the combined source patch to upstream baseline 66f033b76a7ad44ca8d3491adb3a83a933dd6c1e. Initialize pinned submodules; apply plume-lostodyssey.patch before plume-android.patch and apply XenonRecomp-lostodyssey.patch. Restore generated PPC sources from the private Phase 2 checkpoint. Phase 6 Source retains pinned DXC/FFmpeg sources. Use NDK r29, SDK 35 and Java 17.

For fast relinking, extract the COMPLETE Phase 6 Private Compiled Checkpoint, then overlay Phase 9 Private Checkpoint. Phase 9 includes the updated runtime and all six changed objects relative to Phase 6, so neither Phase 7 nor Phase 8 overlay is required. Preserve the base SDL/DXC/dependency archives, relink.py/link-argv.json and signing key. Keep generated code, compiled checkpoints and keys private. The source archive contains all changed public files and the combined patch, but excludes generated guest code, game bytes and signing keys. The supplied manual compile helper contains this run's absolute workspace paths and must be adjusted after recovery.

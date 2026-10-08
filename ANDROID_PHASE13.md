# Lost Odyssey Android — Phase 13 low-graphics launch baseline

The user requested a fixed lowest-settings start so later tests can raise individual graphical options from a consistent baseline. This build implements that launch policy on top of Phase 12. It is a focused settings change; it does not claim to fix the remaining resize loop, full-height rendering, battle transition stalls or playable speed.

## Behavior

Before SDL renderer or guest threads start, the Android first-boot check reads the existing configuration and applies a low-graphics copy through PreviewConfig:

- Fixed supported 720p scene height; 1280x720 configured output preference.
- Anti-aliasing off (including legacy FXAA mirror), anisotropic filtering off.
- Bilinear spatial scaling; upscaler off; FSR sharpening off.
- Frame generation off, dynamic target disabled.
- Normal 30fps game target; variable-refresh option off.

The actual phone surface is still supplied by Android; configured dimensions do not resize the physical display. 720p is the lowest supported scene-resolution menu preset, not a guarantee every internal game pass runs at that size. Xbox game assets remain unchanged.

Apply once per fresh test process, rather than continuously overwriting the menu. Live settings changes remain available. A relaunch reapplies the baseline even if the user saved higher graphics settings. This uses in-memory preview and does not save the low preset over the existing settings file. Language, save-related preferences and other unrelated configuration fields are retained. The log records the resulting GetConfig values, so the next phone log can confirm actual startup settings.

Package remains io.github.freefrank.lostodyssey.runtimecheck with the original signing key, versionCode 8, versionName phase13-low-graphics-baseline, separate :bootcheck Activity process, ARM64/minAPI28/target35. Imported Disc 1 reuse, touch controls, smooth-audio path and the two-minute observation/metrics are retained.

## Evidence and validation

- Host baseline test passed with high settings input: low graphics values selected; language/save preference retained; live edits possible; subsequent baseline application resets edited graphics.
- Changed readiness translation unit cross-compiled with NDK r29/API28/ARM64, then complete Phase 12 runtime relinked. Debug information retained for changed units.
- Java/DEX packaging, APK signature and 16KiB ZIP alignment checks passed. Native ELF LOAD alignment >=16KiB, packaged bytes match verified stripped inputs, certificate matches restored original key.
- All 62857 generated PPC definitions and all 226 strong import-function exports retained; exports match Phase 6. Presence does not prove guest correctness.
- APK contains Phase 13 launch marker and applied-baseline log string. Compiled manifest versionCode8 checked.
- APK bytes 114459708; SHA-256 84f4eadbf647a08a8a2a110583ea112bb4cf53332f84544254b4c5806efc59ad.

Phone validation of this settings policy is pending. Phase 12 phone report confirmed improved but unplayable performance and smooth audio, with no visible aspect change. Its log still reports repeated resize events and a 16:9 scene. Those fixes are not marked successful.

## Test

Install over the existing app without uninstalling. Continue using the installed Disc 1. Leave graphical settings untouched for the first observation, then share the final log. To test one option, change that option during a run; a fresh launch returns graphics to the same baseline. Scene output may remain letterboxed until the remaining aspect issue is corrected.

## Recovery

Apply the combined source patch to upstream 66f033b76a7ad44ca8d3491adb3a83a933dd6c1e. Initialize pinned submodules. Apply plume-lostodyssey.patch before plume-android.patch and the XenonRecomp patch. Restore private generated sources/headers from Phase 2 and pinned DXC/FFmpeg sources from Phase 6 Source. Use NDKr29/SDK35/Java17.

For fast relink, restore COMPLETE Phase 6 Private Compiled Checkpoint and overlay Phase 13 Private Checkpoint. It includes the runtime, all ten changed objects relative to Phase 6 and the Phase 12 plume archive. No earlier overlay needed. Preserve base dependencies, SDL/DXC, relink helper/link argv and original signing key. Helpers include this task's absolute paths and must be adjusted after recovery. Keep compiled checkpoints, generated game code and keys private.

Additional source investigation found another 16:9 lower-aspect limit in render_resolution.h beyond ResolveOutputRegion. That is a lead for a separate aspect correction; no change to that code is included here. Resize trigger diagnostics also remain to be narrowed down on the actual device.

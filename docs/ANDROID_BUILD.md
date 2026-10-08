# Android source and build status

The Android adaptation was developed against upstream commit `66f033b76a7ad44ca8d3491adb3a83a933dd6c1e`. This preparation branch starts from that tested baseline; newer upstream commits have not yet been integrated or validated.

## Requirements

NDK r29 (`29.0.14206865`), Android SDK/platform/build-tools 35, Java 17+, Python 3, CMake and Ninja. Initialise pinned upstream submodules. Native dependencies additionally require the Android DXC/FFmpeg builds and libadrenotools. Read the configuration helpers and dependency patches under tools/patches before compiling.

The runtime requires generated guest code produced from a supported executable supplied by the builder, following upstream's recompilation process. Game data, generated guest code, precompiled private checkpoints and signing keys are deliberately excluded.

## Existing entry points

- `tools/configure_android_runtime.py`: configure/build ARM64 native runtime.
- `tools/configure_android_ffmpeg.py`: Android FFmpeg configuration.
- `tools/build_android_drivers.py`: driver loader dependencies.
- `tools/build_android_runtime_check.py`: Java/resources/DEX and APK packaging from completed native dependency builds. Run each helper with `--help` for explicit path arguments.

Packaging now includes all production Java classes, retains uncompressed WAV assets, and identifies the candidate as version 0.1.0-beta / code 51. Its local debug key is generated if absent; such an independently generated key cannot update the maintainer-signed installed APK.

## Validation boundary

The latest phone-tested APK was build 50, before the restart-only completion change. This branch removes Continue anyway and updates public packaging/documentation. Build 51 passed complete Java/resources/DEX compilation, original-certificate signing, 16 KiB ZIP alignment and byte retention checks for all nine native libraries. The restart-only dialog still needs phone confirmation. The corrected Android runtime has now compiled and linked with NDK r29 (ARM64/API 28), including all 256 generated PPC translation units, SDL2, Plume and FFmpeg. No previously compiled native objects were reused. This check used recovered private generated PPC inputs; regeneration from an original game dump, rebuilding DXC, and phone-testing the newly linked runtime are separate checks. The distributed build 51 APK still retains the previously tested native libraries. Historical ANDROID_PHASE*.md files describe earlier development checkpoints, not current installation instructions.

Do not enable a release workflow that embeds private generated code, game files or signing keys. A maintained reproducible build recipe and secure release signing configuration are still being prepared; no successful clean public-source build is claimed here.

# Android source and build status

The Android adaptation retains its tested upstream baseline and selectively backports newer changes. See [Test 1 integration](UPSTREAM_V0861_INTEGRATION.md) and [Test 2 integration](UPSTREAM_TEST2_INTEGRATION.md) for the exact scope and validation boundaries.

## Requirements

NDK r29 (`29.0.14206865`), Android SDK/platform/build-tools 35, Java 17+, Python 3, CMake and Ninja. Initialise pinned upstream submodules. Native dependencies additionally require the Android DXC/FFmpeg builds and libadrenotools. Read the configuration helpers and dependency patches under tools/patches before compiling.

The runtime requires generated guest code produced from a supported executable supplied by the builder, following upstream's recompilation process. Game data, generated guest code, precompiled private checkpoints and signing keys are deliberately excluded.

## Existing entry points

- `tools/configure_android_runtime.py`: configure/build ARM64 native runtime.
- `tools/configure_android_ffmpeg.py`: Android FFmpeg configuration.
- `tools/build_android_drivers.py`: driver loader dependencies.
- `tools/build_android_runtime_check.py`: Java/resources/DEX and APK packaging from completed native dependency builds. Run each helper with `--help` for explicit path arguments.

Packaging includes the production Java classes and uncompressed WAV assets. The private Test 2 candidate uses version 0.1.1-test2 / code 53. Restore the original private signing key and supply its password privately to preserve update compatibility; never substitute a new key for a maintainer update.

## Validation boundary

The latest phone-tested APK was build 50, before the restart-only completion change. This branch removes Continue anyway and updates public packaging/documentation. Build 51 passed complete Java/resources/DEX compilation, original-certificate signing, 16 KiB ZIP alignment and byte retention checks for all nine native libraries. The restart-only dialog still needs phone confirmation. The corrected Android runtime has now compiled and linked with NDK r29 (ARM64/API 28), including all 256 generated PPC translation units, SDL2, Plume and FFmpeg. No previously compiled native objects were reused. This check used recovered private generated PPC inputs; regeneration from an original game dump, rebuilding DXC, and phone-testing the newly linked runtime are separate checks. The distributed build 51 APK still retains the previously tested native libraries. Historical ANDROID_PHASE*.md files describe earlier development checkpoints, not current installation instructions.

Do not enable a release workflow that embeds private generated code, game files or signing keys. A maintained reproducible build recipe and secure release signing configuration are still being prepared; no successful clean public-source build is claimed here.

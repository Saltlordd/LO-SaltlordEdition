# Android ARM64 Phase 1 — platform probe

Baseline: `freefrank/LostOdysseyRecomp` commit `66f033b76a7ad44ca8d3491adb3a83a933dd6c1e` (source version 0.7.25), inspected 2026-10-02.

This is an asset-free diagnostic application. It compiles the **production guest-address-space allocator**, the pinned SDL Android backend and the existing patched plume Vulkan backend. It does **not** link `LostOdysseyRecompLib`, execute generated PPC code, load an XEX, initialize the game renderer or decode audio. It is not yet a supported Android game-runtime target. There are no fake implementations of those omitted systems.

## Toolchain and supported target

- ABI: `arm64-v8a` only; other Android architectures fail configuration.
- Android NDK r27d: `27.3.13750724`, NDK Clang, `c++_shared`.
- Minimum API 28 (Android 9): deliberately a modern 64-bit baseline. Actual Vulkan 1.2 driver support is separately required; API 28 alone does not guarantee that. `memfd_create` uses the existing syscall path, not a higher-API Bionic wrapper.
- Compile/target SDK 35; SDK Build Tools 35.0.0; current platform-tools for ADB.
- CMake >=3.28 and Ninja. The tested direct build used CMake 4.4.3 and Ninja 1.13.2. The optional Gradle project pins SDK CMake 3.31.6.
- Python 3 and JDK 17 (`java`, `javac`, `keytool`). The build script can also invoke the JDK compiler module when `javac` is not exposed on PATH.
- Android Studio is optional. No particular IDE version is required by the tested direct-SDK build. For IDE import, use a Studio installation compatible with the pinned Android Gradle Plugin 8.7.3, Gradle 8.9 and JDK 17. Gradle packaging could not be validated here because plugin resolution failed.

NDK flexible-page-size support and 16 KiB ELF/ZIP alignment are enabled. **This makes the APK loadable on 16 KiB-page systems; it does not solve the guest-memory E-alias constraint.**

## Source preparation

Apply the supplied root patch to a clean checkout of the baseline commit. It includes this document, the Android shell, build script and Android-specific plume patch. Initialize the pinned dependencies, then apply patches in this order:

```sh
git submodule update --init --recursive
git -C thirdparty/plume apply --check ../../tools/patches/plume-lostodyssey.patch
git -C thirdparty/plume apply ../../tools/patches/plume-lostodyssey.patch
git -C thirdparty/plume apply --check ../../tools/patches/plume-android.patch
git -C thirdparty/plume apply ../../tools/patches/plume-android.patch
```

Do not repeat patches on a modified dependency. Check an existing Android patch with `git -C thirdparty/plume apply --reverse --check ../../tools/patches/plume-android.patch`. XenonRecomp and macOS patches are unnecessary for the diagnostic build. No submodule commit pointers were changed.

## Tested build and APK output

Install the SDK packages (accept the SDK licenses in your normal development setup):

```sh
sdkmanager 'platform-tools' 'platforms;android-35' 'build-tools;35.0.0' 'ndk;27.3.13750724'
python3 tools/build_android_probe.py --sdk /absolute/path/to/android-sdk
```

If CMake/Ninja are not on PATH, pass `--cmake /path/to/cmake --ninja /path/to/ninja`. This script invokes the NDK build, compiles SDLActivity and the tiny subclass, converts Java to DEX, links the manifest with aapt2, packages the three ARM64 libraries, aligns the APK and signs it with a local development certificate. It needs no Gradle plugin downloads. The debug keystore is generated under ignored `out/android-probe/`; it is not included in the deliverable.

Output: `out/android-probe/LostOdyssey-Android-Phase1-Probe-debug.apk`.

Native-only configuration:

```sh
cmake -S . -B out/android-probe -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=/absolute/path/to/android-sdk/ndk/27.3.13750724/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-28 \
  -DANDROID_STL=c++_shared -DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON \
  -DLO_ANDROID_DIAGNOSTICS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build out/android-probe --target lo_android_probe -j 4
```

The optional IDE project is `android/`; with Gradle 8.9 installed, run `gradle :app:assembleDebug` there. Its expected output is `android/app/build/outputs/apk/debug/app-debug.apk`. This route remains unverified, unlike the direct script.

## Install and collect evidence

```sh
adb install -r out/android-probe/LostOdyssey-Android-Phase1-Probe-debug.apk
adb shell am start -n io.github.freefrank.lostodyssey.probe/io.github.freefrank.lostodyssey.ProbeActivity
adb logcat -d -s LostOdysseyRecomp:I SDL:I AndroidRuntime:E
adb shell run-as io.github.freefrank.lostodyssey.probe cat files/state/logs/android-phase1.log
adb shell getconf PAGE_SIZE
```

No game files are needed. After the independent memory and graphics probes, a small Android results dialog offers **Share log** and **Close**. Share log opens the system share sheet with `android-phase1.log` attached as a read-only file; it needs no PC or storage permission. The app retains the dialog while sharing. Close exits the probe. Version 2 uses the same signing certificate and package as version 1, so it can be installed over the original APK. Earlier startup failures may exit before the dialog: read logcat. The file log is overwritten on each successful storage initialization. Save it before another launch. The SDL/Android Java side's fatal loading errors are in logcat.

The probe records native entry, storage roots, host page size, exact guest-memory failure metadata or A/C/E alias coherence, SDL/controller enumeration, Vulkan loader, instance, physical/logical device, GPU feature hints, Android surface, swapchain and game-path presence. Plume's stderr/API errors are captured in the app-private file and its structured callback is routed to logcat. Rendering/acquire/submit/present is **not exercised**; a created swapchain is not proof of game-renderer compatibility.

## App-private paths

The Activity passes `getFilesDir()` and `getCacheDir()` through SDL arguments. The probe uses absolute paths and never treats `/proc/self/exe` (Android's app_process) as the game installation directory.

| Purpose | App-private relative location |
| --- | --- |
| Configuration | `files/config/` |
| Ordinary game saves | `files/data/save/userNN/` |
| Profiles | `files/data/profile/` |
| Diagnostics | `files/state/logs/android-phase1.log` |
| Shader cache, reserved for later runtime integration | `cache/shaders/` |
| Default development Disc 1 folder | `files/game/disc1/` |
| Optional absolute game-path override | `files/game-path.txt` |

These directories are established by the probe; the game save/cache implementations are not linked or tested. The shared path abstraction has an Android branch and rejects uninitialized/relative roots. Existing desktop path behavior is retained.

For later development, stream your own extracted Disc 1 directory into private storage. In Bash/WSL (binary pipes):

```sh
tar -C /absolute/path/to/Disc1 -cf - . | adb exec-in run-as io.github.freefrank.lostodyssey.probe sh -c 'mkdir -p files/game/disc1 && cd files/game/disc1 && tar -xf -'
```

Only `default.xex` presence is checked in this probe. Copying data will not enable gameplay. An optional one-line `game-path.txt` must contain an absolute path readable by the application. Android SAF importing, permissions for arbitrary shared storage, four-disc management and game validation remain later work. Do not bundle game data in source or APKs.

## Guest-memory feasibility

The real allocator reserves 4 GiB at host `0x100000000`, creates sparse `memfd` backing of `0xC0001000` bytes and establishes virtual/A/C/E views. `MAP_FIXED` is used only inside the reservation owned by the allocator; the initial reservation does not blindly overwrite existing memory. On a 4 KiB-page Android kernel, this code compiles against Bionic. The APK must establish that the device permits the reservation and syscall under its actual sandbox.

The E view has backing offset `0xA0001000`, exactly 4 KiB beyond A. That offset is not aligned to 16 KiB host pages. Rounding it, using private copies or silently omitting the view changes guest semantics. Android now fails before mapping with `EINVAL`, naming view 3, address, size and offset, and logs the host page size. Graphics probing continues independently.

Upstream macOS explicitly leaves E inaccessible and installs a fault probe. Its limited gameplay evidence therefore does not prove full alias compatibility on Android. This pass does not adopt that compromise. A future 16 KiB implementation needs guest-access evidence or an explicit translation strategy. The Fold's actual page size was not measured here.

## Audited integration boundary

| Area and inspected source | Finding / remaining full-runtime work |
| --- | --- |
| Root CMake, `cmake/LoPlatform.cmake` | Added Android-before-Linux identity and ARM64 restriction. Diagnostic branch is opt-in and returns before desktop runtime dependencies. |
| `os/platform.h`, `stdafx.h` | Android has its own 0/1 flag and shares genuine POSIX headers. Existing AArch64/SIMDe guest-code path is preserved. |
| `LostOdysseyRecompLib/CMakeLists.txt`, `tools/ppc_codegen.py` | Full configuration actually stops at missing generated PPC sources. `ppc_func_mapping.cpp`, generated context/config headers and private image/symbol inputs are absent. Generate from the user's own game using the upstream patched tools; do not fabricate mappings. |
| `kernel/guest_address_space.cpp`, layout and macOS backend | Real allocator compiled; 4 KiB path requires device execution. 16 KiB E-offset incompatibility diagnosed explicitly. |
| `main.cpp`, `kernel/memory.cpp` | Full memory allocation runs during static initialization. Full desktop entry is not linked by probe; needs a proper SDL shared-library entry and Android initialization ordering. |
| `thirdparty/CMakeLists.txt`, SDL Android project | Full build currently links static SDL; Android shell uses shared SDL and pinned SDLActivity instead. No custom lifecycle/controller framework. |
| plume CMake/types/Vulkan | Existing Android WSI reused through SDL. Two-file patch extends CMake eligibility and gives SDL_Window precedence over ANativeWindow when SDL integration is enabled. No new renderer. |
| `gpu/video.cpp`, `gpu/renderer.cpp` | Existing non-Windows SDL/plume calls are structurally reusable; game shaders, mobile feature requirements and presentation have not been validated. Surface creation alone does not validate them. |
| `gpu/shader/dxc_compiler.cpp`, runtime DXC staging | Desktop Linux stages x86_64 `libdxcompiler.so`; macOS uses a dylib. Neither is an Android/Bionic ARM64 compiler. Android-compatible DXC or a verified sufficiently complete offline shader path is still required. No dummy shader compilation. |
| `cmake/portable_shader_pack.cmake`, `portable_shader_pack.cpp` | Pack staging currently executes a target-built tool, which cannot be run on the build host when cross-compiling; separate host tooling needed. Default lookup also uses the executable directory. A pack does not automatically cover every runtime/presentation shader. |
| `thirdparty/ffmpeg.cmake`, macOS ARM64 config, pinned FFmpeg configure | ARM64/NEON sources exist. `android-aarch64/config.h` is absent; the pinned configure supports `--target-os=android`. Android config and decoder build are not validated in this probe; no copied Darwin config or audio stub. |
| `os/user_paths.h`, `kernel/io/file_system.cpp`, shader caches | Android app roots added. Full filesystem/cache paths still need wiring/validation; probe-created cache/saves directories are not game-runtime proof. |
| `updater/*`, `settings/restart.h`, `main.cpp` | Desktop updater, curl, process restart and apply-mode sources are excluded entirely from this APK. Full-runtime source selection/guards must exclude them explicitly; raw `__linux__` checks remain in that unported path. No APK self-updater. |
| `os/host_scheduling.*`, `os/guest_code_thread.*`, `os/main_thread.*`, `os/thread_name.h` | Standard POSIX threading mostly reusable; macOS-specific QoS stays macOS-only. Android guest-thread stack depth, SDL ownership, background/surface loss and renderer exit policy need real validation. Desktop `_Exit` behavior must be considered before adopting the full entry. |
| `os/crash_handler.cpp`, `os/watchpoint.cpp`, startup diagnostics | Desktop diagnostics were inspected; probe uses native exceptions, direct logcat and private file logs. No claim that desktop crash/watchpoint facilities were ported. |
| Upscaling/FG options | Not included in the probe. Android game target should reject unsupported vendor options instead of staging desktop libraries. |

## FFmpeg configuration recipe for later integration

The pinned fork is `https://github.com/xenia-project/FFmpeg.git` at `15ece0882e8d5875051ff5b73c5a8326f7cee9f5`. From a separate out-of-tree configure directory, use its `configure` with the NDK tool paths below. This recipe is source-grounded but **not executed/validated in this pass**:

```sh
lo_toolchain=/absolute/path/to/android-sdk/ndk/27.3.13750724/toolchains/llvm/prebuilt/linux-x86_64
/absolute/path/to/pinned-FFmpeg/configure \
  --enable-cross-compile --arch=aarch64 --target-os=android \
  --cc="$lo_toolchain/bin/aarch64-linux-android28-clang" \
  --cxx="$lo_toolchain/bin/aarch64-linux-android28-clang++" \
  --ar="$lo_toolchain/bin/llvm-ar" --ranlib="$lo_toolchain/bin/llvm-ranlib" \
  --nm="$lo_toolchain/bin/llvm-nm" --strip="$lo_toolchain/bin/llvm-strip" \
  --sysroot="$lo_toolchain/sysroot" --extra-cflags=-fPIC \
  --disable-everything --disable-programs --disable-all --disable-autodetect \
  --enable-avcodec --enable-avutil --enable-decoder=xmaframes
```

Inspect generated features/assembly and compile the existing source lists before installing the resulting `config.h` as `thirdparty/ffmpeg-config/android-aarch64/config.h`. Adjust the NDK host directory for macOS/Windows. Never substitute the macOS config.

## Validation limits and next step

Verified: actual NDK ARM64 shared-library build; Java/DEX/manifest/native packaging; APK signature and 16 KiB ZIP alignment; ARM64 ELF headers and exported SDL entry; upstream Linux memory-alias and path tests; Android path-policy tests on the host; Android logger syntax with NDK Clang; Android x86 rejection; Linux platform configure; standalone plume patch application and byte comparison.

Device feedback: the original build reported memory PASS and swapchain PASS in the user screenshot; full raw logs are pending. Not verified here: version 2 installation/share-sheet flow, game rendering/audio, or Windows/macOS builds. `adb devices` was empty. Full Android runtime configure failed at the genuine missing-generated-code boundary. Gradle packaging failed at plugin resolution; direct SDK packaging succeeded.

Next smallest task: install the diagnostic APK on the target Fold, collect its result dialog and complete file/logcat output, and determine actual page-size/memory/driver behavior. Do not broaden into touch controls, foldable behavior or gameplay until that evidence is reviewed.

## Device feedback and log-sharing update

The user supplied a screenshot from the original build showing guest memory PASS and Vulkan swapchain PASS on the target Fold. The full log is still pending. Version 2 adds the Share log button through a native-to-Java result-dialog call and a provider restricted to one cache snapshot. It grants read access only to the selected receiving app. Native compilation, Java/DEX packaging and APK verification were checked; the share-sheet flow still needs device confirmation.


## Saltlord beta save transfer (Phase50)

Options → Saves & backups exposes ordinary save ZIP export/import and the existing experimental Save Anywhere backing setting. Restore saves and overlay layouts are grouped on wizard page 4 of 7, before graphics. Transfers use Android document pickers; no storage or network permission is added. Transfers run only before guest gameplay, from the launcher or initial setup. The ZIP layout is `save/userNN/save.bin` with `.lo-content` and optional `.lo-thumbnail.png`. Extracted Xenia `userNN` ZIPs can omit `.lo-content`; the importer synthesises the same 308-byte, big-endian descriptor as upstream's pinned `tools/import_xenia_saves.py` and maps `__thumbnail.png`. Xbox container files need conversion first. Input validation is structural, not full game-state validation.

Restore stages files, previews colliding slots, copies untouched slots, exports existing saves to `files/save-backups/before-last-restore.zip`, then uses atomic directory renames. `.save-restore-old` means rollback on next startup; `.save-restore-committed` marks a completed replacement pending cleanup. Never run a transfer while the guest has save files open. The automatic backup is app-private; users must export it before uninstalling. Native Save Anywhere keeps the upstream split-party guard and changes only the saved preference, with failure returned to Java. It enables System → Save, not state snapshots. Compatibility with Xbox 360 containers and unusual reload locations is unverified.

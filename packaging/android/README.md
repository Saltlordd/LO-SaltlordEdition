# Android ARM64 development builds

This directory builds the Android development probe and experimental full
runtime for the Lost Odyssey Recomp Android port. The probe is a diagnostic APK
that reports host memory-page behavior and Vulkan device limits, formats and
heaps, and exercises one clear/present frame per check run. The runtime is an
arm64 development APK path and is not a published Android release.

The targets use **arm64-v8a** with API 26+, compile/target SDK
35, Android Gradle Plugin 8.9.3, Gradle 8.11.1 and NDK 28.2.13676358. The
source CMake entry accepts `Android` for the probe and for the explicitly
opt-in `LO_BUILD_ANDROID_RUNTIME=ON` runtime path. The runtime remains
experimental and requires host-generated PPC sources plus the Android FFmpeg,
DXC and staged native-library inputs described below.

## Prerequisites

Install these SDK packages with `sdkmanager` (or select the same versions in
Android Studio):

```sh
sdkmanager "platform-tools" "platforms;android-35" \
  "build-tools;35.0.0" "cmake;3.22.1" \
  "ndk;28.2.13676358"
```

Set `ANDROID_HOME` or `ANDROID_SDK_ROOT` to the SDK directory. The Gradle
wrapper downloads the pinned Gradle 8.11.1 distribution and verifies its
SHA-256 checksum from `gradle-wrapper.properties`. Use JDK 17 for the Android
Gradle Plugin 8.9.3 build.

## Build and lint

From this directory, run:

```sh
./gradlew assembleDebug lintDebug
```

The probe and runtime are separate Gradle modules. Build the probe with
`:app:assembleDebug`; build the runtime shell with `:runtime:assembleDebug`
after staging its native libraries with `tools/android/build-runtime.sh`.
The runtime Gradle task is a packaging step. The current runtime APK has passed
the debug build, lint, v2 signature and 16 KB zip-alignment checks and has been
installed with ADB. On the development tablet, the four-disc resources are in
the app's external files directory with readable permissions; the runtime has
loaded the XEX, created the Vulkan device and swapchain, and entered real DXC
shader preparation. The [Android DXC build note](../../docs/notes/android-dxc-build-2026-10-02.md)
records the native compiler staging details. The current development APK has loaded the XEX, played the opening video,
reached the first battle and completed two touch-driven attacks with visible
damage on the development tablet. One initial-battle shader preparation pause
of about 40 seconds was observed. Longer play, audio beyond native queue
evidence, other GPUs, 16 KB devices and physical-controller validation remain
open; these checks do not establish complete-game support.

On Windows PowerShell use:

```powershell
.\gradlew.bat assembleDebug lintDebug
```

The debug APK is written to
`app/build/outputs/apk/debug/app-debug.apk`. The current host build has passed
`assembleDebug` and `lintDebug`; APK signature and 16 KB zip alignment checks
also pass. These checks prove packaging and static toolchain output only; they
do not establish device compatibility or full-game support.

The build copies the pinned SDL Java sources into Gradle's generated sources
and applies a narrow local shim: it guards malformed USB broadcast intents and
uses AndroidX `ContextCompat.registerReceiver` with
`RECEIVER_NOT_EXPORTED`. The lint baseline contains exactly 26 upstream SDL
`MissingPermission` findings (Bluetooth, audio and vibration paths). It does
not suppress application or receiver errors introduced by the probe.

## Install and collect a report

With an ARM64 Android device connected and visible to ADB:

```sh
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb shell am start -n io.github.freefrank.lostodyssey.probe/.ProbeActivity
adb exec-out run-as io.github.freefrank.lostodyssey.probe \
  cat files/probe-report.txt
adb logcat -d -s LOAndroidProbe
```

The probe Activity provides **Run checks**, **Test audio** and **Copy report**
controls. The native report is stored at
`files/probe-report.txt`; native diagnostics use the `LOAndroidProbe` logcat
tag. Capture the device model, SDK, ABI, page size, Vulkan features and limits
alongside the report. No game files or storage permissions are required.

The probe's memory check is deliberately limited: it uses a fixed 4 GiB
virtual-address reservation with small aliases. The current experiment marks
the **E alias** path unsupported on 16 KiB hosts; its A/C checks can still run.
This is not evidence for full guest mapping, protection, or runtime
compatibility. Vulkan coverage stops at capability discovery and one
clear/present frame per check run; shaders, pipelines and game rendering remain
future work.

## Android ARM64 recompiled library

The library build separates host code generation from Android target
compilation. XenonRecomp and its tools run on the Windows host; the generated
PPC sources are then compiled by the Android NDK into the PIC static target
`LostOdysseyRecompLib`. The repository's generated `LostOdysseyRecompLib/ppc`
sources are private build inputs and must not be added to a commit.

Use a WSL system CMake **3.28 or newer** for this slice. The Gradle project's
CMake 3.22.1 requirement is for the probe APK and is separate from this
cross-build. From the repository root, with `ANDROID_NDK_ROOT` pointing to the
pinned NDK directory:

```sh
cmake --version
cmake -S . -B out/build/android-ppc -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-26 \
  -DLO_BUILD_RUNTIME=OFF -DLO_BUILD_GPU=OFF \
  -DLO_BUILD_TOOLS=OFF -DLO_BUILD_RECOMP_LIB=ON \
  -DCMAKE_BUILD_TYPE=Release
cmake --build out/build/android-ppc --target LostOdysseyRecompLib -j 4
```

The Release build passed with NDK 28.2: the archive contains 247 AArch64 ELF
objects (246 generated files plus the function mapping), all compiled with
PIC. This remains a separate library-only target. The opt-in full runtime
target links `libmain.so` for the Android shell after staging the Android
FFmpeg, DXC and other native libraries; the development build has passed the
runtime library link and focused host checks. It does not yet establish APK
installation, resource loading or a playable game flow.

## Experimental full runtime

From the repository root, generate PPC sources on the host and stage the
Android native dependencies before invoking the runtime CMake path:

```sh
tools/android/build-runtime.sh
```

The Gradle shell can then be packaged from this directory:

```sh
./gradlew :runtime:assembleDebug
```

The runtime uses app-owned external files for game data and keeps physical SDL
controller input. Its source touch-controller implementation provides a
`Controller settings` dialog with **Show touch controls**, **Control size**
(60–140%), **Opacity** (25–100%), **Apply**, **Cancel** and **Edit layout**.
The editor provides **SAVE**, **CANCEL**, **RESET** and **HIDE**/**SHOW**;
controls can be dragged independently, and hidden controls remain selectable
in the editor. The pure layout model checks and the configurable editor flow
were verified on the development tablet; broader multitouch and physical
controller coverage remain pending. The
BDA vertex-fetch path avoids requiring the complete
1 GiB vertex arena as one storage-buffer descriptor on devices with a smaller
reported range. Host HLSL remains unchanged for the desktop path.

Android online updating, desktop-style automatic restart and automatic tar
capture packaging are not supported by this development target. A successful
native link or Gradle package is not gameplay acceptance; install the APK and
record resource loading, shader compilation, input, audio, lifecycle and a
bounded game-flow test separately.

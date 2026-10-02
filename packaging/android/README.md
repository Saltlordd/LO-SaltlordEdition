# Android ARM64 probe

This directory builds the Android development probe for the Lost Odyssey Recomp
Android port. It is a diagnostic APK, not a playable Android release: the
probe reports host memory-page behavior and Vulkan device limits, formats and
heaps, and exercises one clear/present frame per check run. It does not load game files,
compile game shaders, or run the full game.

The current prototype targets **arm64-v8a** with API 26+, compile/target SDK
35, Android Gradle Plugin 8.9.3, Gradle 8.11.1 and NDK 28.2.13676358. The
source CMake entry accepts `Android` only for the probe path and explicitly
refuses to build the full game for Android until its platform work is complete.

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

The Activity provides **Run checks**, **Test audio** and **Copy report**
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

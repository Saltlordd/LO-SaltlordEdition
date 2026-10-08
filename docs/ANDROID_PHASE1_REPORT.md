# Lost Odyssey Android Phase 1 report

2026-10-02 · upstream baseline `66f033b76a7ad44ca8d3491adb3a83a933dd6c1e` · source 0.7.25

**Result: a signed ARM64 diagnostic APK was produced. The full Lost Odyssey runtime is not yet ported or running on Android.**

| Requested report | Result |
| --- | --- |
| What works | NDK Clang builds and links the real guest allocator, pinned SDL Android backend and existing plume Vulkan backend. Java shell, DEX and APK packaging succeed. Signature, alignment and native entry checks pass. Linux alias/path regressions pass. |
| What changed | Android platform/ISA detection, POSIX sharing, app-private path initialization, native logcat support, precise memory diagnostics and 16 KiB-page rejection; tiny SDLActivity subclass with Share log and read-only log export; asset-free CMake probe; SDK-only build script and optional Gradle project; two-file plume SDL integration patch; build/audit documentation. |
| APK status | `out/android-probe/LostOdyssey-Android-Phase1-Probe-debug.apk`, signed debug APK, arm64-v8a only, min API 28, target API 35. Intended to be installable; no installation test was possible here. |
| Runtime status | The original APK produced a user-supplied screenshot showing guest memory PASS and Vulkan swapchain PASS on the target Fold. The complete device log is still pending. The probe does not execute the game or present rendered frames. Version 2 adds a phone-only Share log button; that new share flow has not yet been device-tested. |
| First full-build blocker | Actual full Android CMake configuration stops at `LostOdysseyRecompLib/CMakeLists.txt:8`: `No recompiled sources in LostOdysseyRecompLib/ppc; run XenonRecomp first`. Generated game code/context/function mappings and private extraction inputs are absent. No fake library was supplied. |
| Architectural blocker | Full coherent E alias uses a 4 KiB backing offset. On a 16 KiB-page kernel it cannot be expressed with ordinary mmap aliases. The allocator logs this and fails safely; graphics probes can still run independently. Target phone page size is unknown. |
| Other known integration gaps | Full runtime shared-library entry/lifecycle and Android paths; desktop updater/curl/restart exclusion in full source list; Android FFmpeg config/build; Android/Bionic ARM64 DXC or verified offline shader strategy; host-vs-target shader tooling; mobile GPU feature/presentation/game-shader validation; guest-thread stacks and Android background/surface-loss behavior. Detailed source findings are in ANDROID_DEVELOPMENT.md. |
| Temporary compromises | Separate, clearly named platform probe; game code, XEX loader, game renderer, audio, updater, advanced graphics and importer are not linked. No broad fake subsystem implementations, no copied desktop FFmpeg config, no E-alias approximation. The result dialog is diagnostic only. |
| Next recommended phase | Run this APK on the Fold and review memory/page-size/Vulkan logs. Then choose the smallest demonstrated blocker. No next-phase implementation was started. |

The optional Gradle route failed at Android plugin dependency resolution in this environment. The tested SDK-tool packaging script produced the APK without that dependency. Windows/macOS behavior is structurally retained; full desktop rebuilds were not possible without the same private/generated game inputs. Linux targeted regressions passed.

No copyrighted game data, generated game code, private keys or user data are included. See ANDROID_DEVELOPMENT.md for build/install/log commands, private-storage paths and later FFmpeg configuration instructions.

Version 2: `phase1-probe-sharelog`, versionCode 2, same package/signing certificate as the original APK. Install it over version 1; open it, tap Share log and attach the file through the Android share sheet.

# Lost Odyssey Android toolchain and import integration — Phase 5

The full native ARM64 runtime now compiles and links as an Android PIE executable. This clears the Phase 4 compile/link blockers. It is not an APK, and it has not run the game on the phone.

## Changes

- Tested the pinned Android NDK r29 (29.0.14206865), Clang 21 and its libc++. The existing std::atomic_ref, std::jthread and floating-point std::from_chars code compiles intact. The SDK archive matched Google's published SHA-1: 87e2bb7e9be5d6a1c6cdf5ec40dd4e0c6d07c30b (783,549,481 bytes).
- Added an Android runtime configure-time feature check. The old r27d runtime configuration now fails early with an actionable message. Diagnostic-only APKs can still use r27d.
- Changed DLC JSON parsing to pass char iterators over the same bytes. This fixes the legacy parser's reliance on the removed char_traits<unsigned char> specialization without modifying the third-party dependency.
- Implemented the 14 missing Android import bindings with IPv4 POSIX transport, real manual-reset kernel events, guest-heap DNS results and release tracking. The socket backend translates guest big-endian fields, uses owned handles, translates errno into Winsock errors and stores last-error state per thread.
- Android WSA startup negotiates the supported version, fills WSADATA while preserving its vendor pointer, tracks startup references and closes sockets at final cleanup. Existing Android socket/close hooks now use this transport.
- Xbox Live/XNet secure transport reports unsupported operation (10045). Title network identity reports NONE (1), not pending. No successful Xbox Live connection is simulated. DNS completes synchronously in this first implementation, publishes its result/status and then signals the supplied event.
- The backend deliberately supports a limited IPv4 subset. Unsupported families, protocols, options and flags return errors. IPv6, secure Xbox UDP/VDP, full Winsock extensions and asynchronous DNS are not implemented.
- Desktop networking branches and the existing desktop import-generator scan remain unchanged. Android-only additional hooks live in an included .inc file so the desktop generator does not mistake them for active desktop definitions.
- Added configure_android_runtime.py, reusable transport tests and audit prerequisite generation.

## Validation

The complete LostOdysseyRecomp executable linked with all generated game code and GPU/audio/runtime dependencies, without generated Android import stubs. Inspection of demangled defined symbols confirmed all 222 Disc 1 imports and all 62,857 generated PPC functions. All ELF LOAD segments have alignment 0x4000 (16 KiB), machine AArch64, interpreter /system/bin/linker64. The executable depends on Android system libraries and the matching r29 libc++_shared.so.

Host regressions passed:

- Actual TCP/UDP loopback connect, bind, send/receive, getsockname, scalar options and ioctl operations.
- Guest byte order, numeric DNS resolution, unsupported inputs, UDP truncation, stale handles, per-thread error state, WSA version/reference management and cleanup.
- Guest critical sections: big-endian fields, recursion, try-enter, handoff and 100,000 contended recursive acquisitions.
- Shader preparation queue: bounded workers, partial/failed thread launch, cancellation/exception wakeup and persistence of failed compilation.
- Existing importer suite: synthetic extracted DLC and STFS failures, four-disc transactional installation, rollback, locks, cancellation and mixed-edition rejection.
- Old-toolchain configure rejection and new-toolchain acceptance.

These tests exercise host transport and existing synchronization logic. The new event/DNS kernel bindings compiled and linked but have not been exercised through guest code on Android. Symbol coverage does not prove the accuracy of every existing upstream HLE implementation.

## Rebuild

Initialize the upstream submodules and apply the existing XenonRecomp and plume patch sets. Restore your private generated Disc 1 code/XEX image checkpoint; these are intentionally excluded from the source bundle. Install NDK 29.0.14206865 (or explicitly pass its extracted directory).

    python3 tools/configure_android_runtime.py --ndk /path/to/android-ndk-r29 --build

This builds the native executable, not an APK. The isolated compiler audit can be run after configuring:

    python3 tools/audit_android_runtime.py --build-dir out/android-runtime-r29

For asset-free host network tests, configure with LO_BUILD_RUNTIME=OFF, LO_BUILD_RECOMP_LIB=OFF and LO_BUILD_ANDROID_NETWORK_TEST=ON, build LoAndroidNetworkTest, then run CTest.

## Next stage

Integrate the linked code into an Android SDL shared-library lifecycle, adapt startup paths and provide access to the game's full extracted assets. The current phone diagnostic contains only the copied default.xex; it cannot supply the game data. The runtime dynamically loads DXC, while the repository's Linux DXC binary is x86-64, so an Android ARM64 shader-compiler build or a suitable prebuilt shader-cache route still needs to be established. Android transport will require INTERNET permission in a runtime APK.

No new APK is produced by this checkpoint. The Phase 3 XEX Test remains the latest phone-tested build; loading, memory/Vulkan and audible PCM output passed there. No generated guest code, game renderer, actual game audio or gameplay has been executed on the phone.

## Reference sources

- NDK archive/version/checksum: https://github.com/android/ndk/wiki/Unsupported-Downloads#r29
- Xbox ABI reference (structures, arguments and manual-reset events): https://github.com/xenia-project/xenia/blob/master/src/xenia/kernel/xam/xam_net.cc
- Winsock receive/error semantics: https://learn.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-recv

The new backend is an independent implementation using those ABI definitions; it does not copy Xenia's transport implementation.

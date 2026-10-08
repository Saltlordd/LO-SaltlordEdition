# Lost Odyssey Android runtime integration — Phase 4 checkpoint

## Phone baseline confirmed

The uploaded android-phase1.log (2).txt confirms the exact Disc 1 XEX fingerprint, production decrypt/decompress and the 20,709,376-byte guest-memory image all passed on the Fold 8 Ultra. Memory alias coherence, Vulkan swapchain creation and audio smoke tests passed. The user confirmed hearing the tone. No game code or game rendering was exercised.

## Runtime build audit

Using the current NDK r27d / API 28 ARM64 GPU runtime configuration, the precompiled header and 80 of 84 runtime source files compile. Four files fail:

| Source | Blocking feature |
| --- | --- |
| gpu/renderer.cpp | std::jthread; std::atomic_ref |
| kernel/imports.cpp | std::atomic_ref, including guest critical-section ownership |
| gpu/video.cpp | Floating-point std::from_chars |
| settings/menu.cpp | Floating-point std::from_chars |

These are limitations of the currently configured C++ standard library. They are not evidence of a phone hardware limitation. Compilation was audited independently, without linking or generating import stubs; it does not prove dependency linkage, runtime behavior or game execution.

## Missing imports

Of the 222 imports in the supplied Disc 1 symbol dump, 208 have existing explicit source definitions. Definition coverage does not establish correct implementation. The following 14 have no source definition:

- __imp__NetDll_WSACreateEvent
- __imp__NetDll_XNetDnsLookup
- __imp__NetDll_XNetDnsRelease
- __imp__NetDll_bind
- __imp__NetDll_connect
- __imp__NetDll_getsockname
- __imp__NetDll_getsockopt
- __imp__NetDll_inet_addr
- __imp__NetDll_ioctlsocket
- __imp__NetDll_recv
- __imp__NetDll_recvfrom
- __imp__NetDll_send
- __imp__NetDll_sendto
- __imp__NetDll_setsockopt

The existing desktop import generator would create implementations that log once and return zero for these calls. Android builds now exclude this generator so the missing definitions remain linker errors. The desktop branch retains the original generator. This change does not implement networking or prove that these imports are needed during offline startup.

## Reproduce

Configure the full Android GPU runtime with CMAKE_EXPORT_COMPILE_COMMANDS=ON and the existing toolchain settings. Then run:

    python3 tools/audit_android_runtime.py --build-dir out/android-full-runtime

The audit returns nonzero when compilation or import-definition coverage is incomplete. Its results.json and individual compiler logs are included in the source checkpoint. It builds isolated objects, selects the actual runtime target by its output path, and skips Ninja module response maps because these translation units contain no C++ modules.

## Next work and stopping point

Before connecting an executable game runtime to SDLActivity, resolve the C++ library features with a compatible Android toolchain or narrow, tested platform implementations that preserve threading and atomic semantics. The missing imports then require actual implementations or an explicitly justified game-specific handling policy. Do not substitute success-returning placeholders. Android shared-library lifecycle integration, full dependency linking, game assets access and actual renderer presentation remain unverified.

This stage stops at genuine compile/link blockers as requested by the original bring-up brief. No new APK was produced; the XEX Test remains the latest verified phone diagnostic.

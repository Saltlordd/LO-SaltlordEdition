# Android source validation — 8 October 2026

The initial published source snapshot mixed SDL3-era upstream native files with the tested SDL2 Android integration. This correction restores the complete Android native source snapshot, retains the newer Java UI/report fixes, and restores the Android Vulkan pipeline-cache patch.

Build fixes: shared SDL2 dependency, pinned nlohmann JSON dependency, C++20 for the Android pipeline-cache implementation, SDL Vulkan declarations, and hidden internal FFmpeg tables required by its ARM64 assembly inside a shared library.

Validation: NDK r29 / Clang 21, ARM64 Android API 28, Release build; all 256 generated PPC units and native runtime/dependencies compiled and linked successfully with no-undefined enabled. ELF load segments have 16 KiB alignment and Android JNI entry points are exported.

Private generated PPC input was recovered from an earlier user-derived codegen checkpoint. No compiled checkpoint objects were used. This does not validate regenerating those inputs from a game dump or rebuilding DXC. Generated game inputs and signing keys remain excluded from Git.

The existing build 51 APK is unchanged. The freshly linked runtime has not been installed or phone-tested; this source correction must not be described as a newly phone-tested APK.

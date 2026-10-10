# LO: Saltlord Edition — Test7 investigation and device test

## Finding
Test6 did not preserve the selected custom Vulkan loader. gpu/video.cpp called volkInitializeCustom, then the rebuilt Plume VulkanInterface constructor called volkInitialize unconditionally, replacing the dispatch with the system loader. The UI/report selection therefore did not prove the active driver.

The older Test5 v34 log reports driver_raw=109060195 (0x06802063), with about 11.85 GB reported device memory. All three Test6 sessions report driver_raw=2150932499 (0x8034a013), with about 15.81 GB. This matches the earlier system-driver observations. The recovered Plume patch chain lacked the custom-loader preservation fix; Test6 package verification checked signing/configuration but missed this runtime handoff. This is an integration regression, not evidence that the CPU optimizations themselves are slower.

## Test6 log analysis
Only the latest three mobile-baseline sessions were analyzed as Test6; older appended sessions were excluded.

| Observation | First run | Second run | Third run |
| --- | --- | --- | --- |
| Cached shader modules | 28,512 valid; startup bundle rebuilt | 28,512 restored | 28,512 restored |
| DXC shader compilations | 0 | 0 | 0 |
| Pipeline recipes prebuilt | 0 | 199 | 199 |
| Logged pipeline-creation stalls >=33ms | 109; summed wall time 7,558ms | 0 | 1; 122ms |
| Android thermal pressure | 2 | 2 | 2 |

The warm runs still contain scene intervals around 50–53 host presentations/sec, interspersed with near 60. These are host presentation observations, not a GPU bottleneck measurement. The save test and menu/exit intervals are not treated as clean benchmarks; the second run includes Save Anywhere. Third-run final 4-second interval around 17/sec is also not treated as sustained scene performance. Prior Test5 warm runs showed thermal pressure 1, another comparison difference. Thermal pressure is not chipset temperature or direct clock evidence.

The short first-launch preparation was cache validation/startup-bundle reconstruction, not missing shader compilation. Later launches reused the bundle. Driver-cache snapshots of roughly 2–3ms are not sufficient to explain sustained slowdowns. The report does not establish simultaneous use of two shader caches or prove cache corruption. Uninstalling would discard warm caches and confound this comparison, so it is not the next test.

## Test7 changes
- Android Plume initialization preserves an already-installed Vulkan entry point. Desktop initialization is unchanged; explicit system fallback still initializes the system loader.
- Native startup verifies the selected custom entry point survived interface creation, logs the result, and rejects an unexpected replacement rather than pretending the custom driver is active. It also queries and logs Vulkan driver ID/name/info on the actual device.
- Android CMake configuration refuses a Plume tree missing the loader-preservation patch.
- Retains Test6 reviewed CPU trims, Vulkan/BDA/shader arithmetic and temporal backports, low-workload launch baseline and motion-blur/dynamic-shadow settings.
- Save diagnostic report buttons on the wizard first/preparation pages and the compilation display, aligned to the left. Export works without an existing runtime log, using device/setup/exit metadata.
- Optional detailed renderer CPU/GPU timing switch in Graphics, applied on next launch; off by default. Uses existing queue-query diagnostics without an extra GPU wait, but logging/instrumentation still changes workload. These are overlapping CPU wall components and renderer GPU queue batches, not full-frame/display timing.
- User-initiated reports include bounded portable pipeline state/hash recipes when available. No shader binaries, driver cache binaries or saves are included. This enables collecting warmup profiles; no bundled full-game warmup profile has been implemented or validated yet.

## Phone test
1. Install Test7 over the existing app; back up saves but do not uninstall or clear caches.
2. Keep Turnip Gen8 V34 selected. Let the phone cool before comparing; use the same screen/route, 720p, Dynamic 60, AA Off and Basic scaling. Keep motion blur and shadow choices identical.
3. Expect possible pipeline preparation because the now-correct driver will reject incompatible system-driver cache data. Existing compatible SPIR-V shaders and learned pipeline recipes remain usable; allow any requested restart.
4. Do three consecutive runs of the same save and route. Avoid saving/opening menus during the timed comparison. Save Anywhere already worked in the Test6 test but is a separate workload.
5. Export the report. If performance is still poor, enable Graphics → Collect detailed performance timings, restart and do one extra same-route run, export, then disable and restart. Keep instrumented runs separate from normal FPS comparisons. We expect selected custom loader entry point preserved=true and the earlier v34 driver version. Device results are required before declaring the performance regression fixed.

## Precompilation direction
SPIR-V shader preparation is one stage; Vulkan later compiles/links shaders into device graphics pipelines using render state. Learned recipes let us perform this work at startup. We can extend coverage using real collected state/hash warmup profiles and compile those locally for the chosen driver. The Test7 report export provides collection data for that work. It cannot infer every game state from shader hashes alone, and does not promise zero first-encounter hitches. Graphics pipeline libraries are a separate possible driver-dependent investigation, not included here. More texture/asset preload is a distinct memory/I/O problem; blind full-game preload is not performed.

## Build and verification
Original signing identity and package ID retained. No public beta release changed. Native Android build/link, production Java/D8 packaging, original certificate, APK integrity and ARM64 16KiB ELF/ZIP alignment passed. No INTERNET permission. See Test7-verification.json for checksum and checks. Native/device performance remains unverified until the phone test. Host loader policy regression test passes custom preservation, fresh system initialization, failure and retry; bounded warmup metadata export test passes roundtrip, size limit and missing file. Production Java/D8 compilation and pinned patch application checked. Existing Test6 tests are historical validation, not comprehensive new device/game testing.

Based on freefrank's LostOdysseyRecomp and contributors. Integration/development is AI-assisted. Public identity: Saltlord.

## Recovery build recipe
Workspace: /workspace/scratch/ce0bac179dd2/native-test7
Branch: saltlord-test7
Base: 89317cf6e858baed3e543bd9e96f106f4f26ffbf
Apply cumulative Test7-source.patch to the base; recursively initialize pinned submodules. Apply tools/patches/sdl-android-preparation.patch in SDL and, in Plume, plume-lostodyssey.patch, plume-android.patch, plume-v0861.patch, then plume-android-loader.patch.
Private generated PPC inputs are recovered from the Phase2 Codegen checkpoint; FFmpeg is pinned to 15ece0882e8d5875051ff5b73c5a8326f7cee9f5. Generated code and signing secrets are excluded from the source checkpoint.

```bash
/root/.local/bin/cmake -S native-test7 -B native-test7/out/android-runtime-r29 -G Ninja -DCMAKE_TOOLCHAIN_FILE=/tmp/saltlord-toolchain7/android-ndk-r29/build/cmake/android.toolchain.cmake -DCMAKE_MAKE_PROGRAM=/root/.local/bin/ninja -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-28 -DANDROID_STL=c++_shared -DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DFETCHCONTENT_SOURCE_DIR_LO_FFMPEG=/workspace/scratch/ce0bac179dd2/native7-private/ffmpeg-source
/root/.local/bin/cmake --build native-test7/out/android-runtime-r29 --target LostOdysseyRecomp -j6
```

Java is compiled against Android35 with the pinned SDL Java source, then D8 min API28. Package retains Test6 assets/resources and unchanged DXC/driver-hook libraries, replacing freshly built runtime/SDL, NDK C++ runtime and Java DEX. versionCode58 / 0.1.1-test7; zipalign -P16; original signer supplied privately. Package verification uses Test6 as the certificate baseline. This is not a fully reproducible end-to-end build claim.

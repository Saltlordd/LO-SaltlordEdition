# LO: Saltlord Edition — 0.1.1 Test6

Private performance/stability candidate, not a published release. Install as an update; back up saves first. No uninstall or cache deletion is required.

## Included
- Fresh Android ARM64 Release native runtime, generated PPC compilation and SDL2 build using NDK r29.
- Reviewed upstream CPU trims #303/#314: register blocks, synchronization epochs, validation/lookup caches and bounded request polling.
- Vulkan recording/occlusion backports, BDA vertex fetching and lower-word constant arithmetic from the reviewed #310/#313/#315 integration; shader contract v27.
- Reviewed temporal rendering corrections and existing motion-blur/dynamic-shadow switches, shader preparation fitting, Android controls/setup/Flex/save features.
- Fix for native configuration loaded before Android paths: explicitly reload persisted settings after path initialization.
- Basic output scaling by default, with persisted Basic/High and Standard/Expanded RGB choices. RGB range changes colour, not rendering resolution.
- Diagnostic mobile launch baseline: 720p, AA/FXAA/anisotropic filtering/upscaler/sharpening/frame generation/VRR off. Retains saved frame target and motion-blur/shadow choices. Resolution/AA changes made during play are reset at the next launch in this test candidate.
- Separate Vulkan pipeline-cache files per selected driver package, still checked for GPU/driver compatibility. Existing caches are not deleted or combined. New namespace may require first-use preparation.
- Host presentation timing includes driver-cache snapshot work; logs snapshots taking at least 2ms.
- Diagnostic export available from the launcher after failed startup. Reports include recent Android process-exit metadata when supported, memory and driver details. User-initiated export only; no INTERNET permission.

## Test it
1. Keep v34 selected and use the same save/route. Allow shader preparation and the requested restart; changing drivers can trigger additional preparation.
2. Do three consecutive runs at 720p / Dynamic 60 / AA Off / Basic scaling, with identical motion/shadow settings. Dynamic 60 targets up to 60 FPS.
3. Export the diagnostic report after all runs. Note visual errors, repeat hitches, approximate FPS and thermal conditions. Verify fold/Flex switching and Options pause.
4. Users who crash after preparation can export from the app main menu: Options → Setup, features & reports → Save diagnostic report. Request device/chipset/RAM, driver, settings, reproduction and whether first encounter only.

## Verification and limits
Native Android build and Java/D8 compilation passed. Production settings reload and mobile configuration transformation tests passed; register snapshot, bounded vertex cache, occlusion and temporal-scene host tests passed. Original signing certificate, Android 9+ ARM64, APK integrity, 16KiB ELF/ZIP alignment and absence of INTERNET permission verified. See Test6-verification.json for checksum.

Device rendering/performance and reported post-preparation crashes are not verified or declared fixed. Compiled shaders do not guarantee all pipelines/resources have been encountered, or zero stutter. More eager preparation can also increase memory pressure; this candidate does not blindly preload every asset.

This is a selective reviewed backport, not every latest upstream change. SDL3 migration, newer AO/HDR/upscaling contracts and other follow-ups remain separate work. Existing Test2/Test3 integration notes describe historical validation and should not be read as Test6 completion status.

Based on freefrank’s LostOdysseyRecomp and contributors. Development/integration is AI-assisted. Public identity: Saltlord. Public beta unchanged.

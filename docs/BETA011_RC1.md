# LO: Saltlord Edition v0.1.1-beta-rc1

Private release candidate; public v0.1.0 beta remains unchanged. Based on freefrank's LostOdysseyRecomp and its contributors. Development and integration are AI-assisted. Public identity: Saltlord.

## Candidate changes
- Retain the tested Test7/Test8 native runtime and its upstream CPU/rendering integrations, custom Vulkan driver dispatch preservation and active-driver diagnostics. All nine native libraries are byte-for-byte unchanged from Test8.
- Remove Output scaling from Android Graphics; Basic bilinear output filtering is fixed. This does not remove render-resolution choices or change Dynamic 60 into dynamic resolution.
- Expanded RGB is the initial default for new installs and upgrades from diagnostic builds, and is selected by all four performance profiles. An explicit later Standard choice remains available and is retained until another profile is selected. No universal RGB performance claim is made.
- Restore saved resolution and AA on every launch. Preserve all four Low-spec/Balanced/Performance/Quality tuples, individual FXAA/SMAA choices, frame-rate target, motion blur and dynamic shadows. Fill missing/invalid resolution and AA choices with conservative 720p/AA Off defaults. Existing mobile defaults keep AF/upscaling/frame generation/VRR off.
- The unchanged native mobile baseline still momentarily resets resolution/AA before GPU preparation. The Android startup path snapshots the current choices after the launcher/wizard, applies that baseline and immediately restores the snapshot before rendering starts. This avoids replacing the native runtime solely for launch/UI policy.
- Remove the cold-pipeline test checkbox from normal Graphics. Disarm pending diagnostic reset requests before merging warmup recipes so this candidate does not unexpectedly erase a player's pipeline coverage.
- Retain the 202 bundled portable pipeline recipes, automatic local recipe learning, compatible driver binary caches and identical-driver import reuse. The phone's additional learned recipes are preserved; the latest report contained 313. Bundled coverage does not represent the entire game.
- Keep diagnostic report buttons available from setup and shader preparation, before gameplay. Reports are saved explicitly through Android's file picker; no reports are sent automatically and the APK requests no INTERNET permission.

## Verification
Production Java compilation against Android 35 and D8 release/min API 28 passed. Host tests verify all four profile tuples, FXAA, saved 1080p/60, conservative missing settings, fixed Basic filtering, idempotence, colour migration and subsequent Standard override. JNI getter/setter sequencing was inspected against the unchanged native implementations; resolution/AA setters do not alter the frame-rate choice. Signed APK integrity, original signer, all nine unchanged ARM64 libraries, 16KiB ELF/ZIP alignment, unchanged existing assets/resources and unchanged bundled recipe bytes checked. These checks do not replace device testing.

## Final phone check
1. Install over Test8 or the current beta. Back up saves as usual; do not uninstall or clear app data/caches. Keep V34 selected.
2. Check Graphics: Output scaling and Cold pipeline test rows are absent; RGB initially shows Expanded. Leave detailed timing collection off for normal performance comparisons.
3. Play the same route at 720p/Dynamic 60/AA Off; restart and repeat. Compare after allowing the phone to cool.
4. Select Performance (720p/Dynamic 60/SMAA), restart and confirm its choices remain. Select Quality (1080p/Steady 30/SMAA), restart and confirm. These are settings-persistence checks, not requests for equal performance at higher workload. Return to the preferred profile afterwards.
5. Check save loading/Save Anywhere and report export. A fresh first-run wizard/shader completion/restart test remains separate; the current phone's cache has not been wiped to claim one.
6. Export the diagnostic report if anything regresses. Do not publish the candidate until this check is complete.

## Beta limits
Primarily developed/playtested on Saltlord's Samsung Galaxy Z Fold 8 Ultra. Android 9+/ARM64; other hardware and drivers vary. Users supply supported game files; no original game assets are bundled. First shader preparation requires waiting and then a restart; new pipeline combinations, texture uploads and other workloads can still cause first-encounter hitches. Dynamic 60 targets up to 60 FPS. No full-game/device compatibility or fully reproducible end-to-end build claim. Save Anywhere is not a save state; Xbox 360 save compatibility remains unverified. Existing Flex auto-hide limitation remains; manual Hide works.

## Recovery correction
The prior Test8 source checkpoint omitted its bundled `.bin` recipe asset because of the ignore rule. This checkpoint explicitly tracks and includes the exact verified Test8 APK recipe bytes. No signing secrets, original game assets or private compiled recovery are included in this source checkpoint.

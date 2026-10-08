# Lost Odyssey Android Phase 10 — full Disc 1 import and first boot attempt

## Status

A signed ARM64 first-boot test APK was produced. Phase 10 has not run on the phone here. No game frame, game audio or gameplay success is claimed.

The supplied android-phase1.log (7).txt confirms Phase 9 exact Disc 1 executable loading, all touch input categories, production Vulkan initialization and completion without the previous shader-identity crash. Game shader prebuilding was deliberately skipped in that test. This is the verified phone baseline.

## What this build does

1. Offers Share log before proceeding, including recovery of an earlier run's appended crash log.
2. Reuses a validated app-private files/game/disc1 installation, or asks for the user's supported Disc 1 ISO and copies its complete 15-file content through the normal staged installer. About 5.7 GB additional storage is required; the source ISO remains untouched. The existing free-space check includes 64 MiB overhead. Copy progress is visible, cancellation is checked during copying, and the screen stays awake.
3. Verifies the installed executable fingerprint, disc identity, FPI structure and presence of the thirteen nonempty archives. This is structural validation, not reference-hash authentication of every archive. Loads the production executable and its variable imports into guest memory with the normal allocators, filesystem and Xam setup.
4. After Continue, calls the existing RunGuest path: actual GPU command processor, timestamp thread, audio/XMA initialization and GuestThread::Start at 0x827ca440. Shader/pipeline prebuilding remains deferred for this bounded test; real game shaders can compile on demand. No fake shader or guest implementation is introduced.
5. Observes for two minutes from boot initialization. Heartbeats report the guest-entry launch marker, successful guest frontbuffer presentations, DXC call outcomes and last opened file. The launch marker precedes GuestThread::Start; by itself it is not proof of guest instruction execution. A presentation is counted only when a request from PresentFrontbuffer advances the successful swapchain-present counter. Image content, useful graphics and playability still require visual confirmation.
6. At the observation limit, requests the existing cooperative game pause and offers Share log. Continue terminates the isolated boot process without destructing shared native state while guest threads may remain. Earlier game return, normal Back exit or crash can close the test before this dialog; reopen to share the appended log.

The Activity runs in :bootcheck, a separate Android process from the log provider. This is a bounded bring-up policy, not a complete mobile game-session lifecycle implementation. Activity destruction during boot writes an emergency reason and terminates this isolated process, avoiding SDL unloading underneath guest threads. Import destruction requests cancellation instead. Backgrounding does not promise to suspend every game subsystem; keep the first test in the foreground.

Touch sticks, D-pad arrows and physical controller merging are retained from Phase 9. No controller is required. No other discs, DLC, control editor or release UI is added.

## Implementation

- Android descriptor overload uses the existing PrepareDisc and ImportContentImpl paths, restricted to the supplied Disc 1 execution identity and exact SHA-256. Borrowed descriptors are duplicated by the ISO reader; no /proc reopening or whole-ISO copy occurs. Normal staged publication, safe entry names, required files, existing-slot preservation and cancellation/write-error rollback remain in use.
- Android importer uses a persistent lock file with an exclusive kernel flock. Process termination releases the kernel lock; keeping the inode avoids close/unlink/reopen races. A crash can still leave an abandoned staging directory; this build does not delete such recovery data automatically.
- RunAndroidGuestBoot wraps the existing RunGuest. Added Android-only atomic guest-presentation observation runs on the GPU owner and does not read its non-atomic present counter from another thread. Existing desktop launch behavior is retained.
- The readiness shell still uses the same Runtime Check package and signing key. versionCode 5, versionName phase10-disc1-first-boot; ARM64, min API 28, target API 35.

## Checks

- Four changed native units compiled using NDK r29/API 28, and the complete existing runtime relinked successfully. Debug information is retained in the changed native objects.
- Android-branch host descriptor-import test passed using a private synthetic ISO containing the user's verified XEX and synthetic asset payloads: complete publication, exact executable size, descriptor ownership/seek preservation, existing installation preservation, cancellation cleanup, injected write-failure rollback, concurrent lock exclusion, lock reuse after release and wrong-fingerprint rejection. This does not verify the phone provider, copying 5.7 GB, real asset integrity or game execution.
- Existing desktop importer tests passed, including cancellation rollback and mixed-edition rejection.
- Java compilation, DEX packaging, APK signature and 16 KiB ZIP alignment checks passed. The signing certificate matches Phase 9. All four packaged native libraries match checked stripped inputs and their ELF LOAD alignment is at least 16 KiB. The compiled manifest includes :bootcheck.
- All 62,857 generated PPC definitions are present. The 226 strong import-function exports exactly match the Phase 6 base, retaining the previously verified 222 required hooks. Symbol coverage is not execution correctness.
- APK bytes: 114,459,708. SHA-256: 598ecc3612472945a6ed4bb9931eb64831825b05fbb726c19a662c4bc1d3cc69.

## Phone run

Install over Lost Odyssey Runtime Check. Continue, select Disc 1 ISO, keep the app open during copying, then Continue on the installation/executable results dialog. Observe up to two minutes, share the final log and describe any visible graphics/audio. If it closes first, reopen and Share log at the initial dialog. Future runs should reuse the completed installation. An invalid existing installation is preserved and reported rather than silently replaced.

The next decision comes from this phone log: verify actual guest/file/shader/presentation milestones and diagnose the first concrete failure. Full gameplay, multi-disc switching, valid game audio/video, saves and mobile lifecycle behavior remain unverified.

## Recovery

Apply the combined source patch to upstream 66f033b76a7ad44ca8d3491adb3a83a933dd6c1e. Initialize pinned submodules, apply plume-lostodyssey.patch before plume-android.patch and apply XenonRecomp-lostodyssey.patch. Restore private generated sources from the Phase 2 checkpoint and pinned FFmpeg/DXC sources from Phase 6 Source when needed. Use NDK r29, SDK 35 and Java 17.

For fast relinking extract COMPLETE Phase 6 Private Compiled Checkpoint, then overlay Phase 10 Private Checkpoint. It includes all eight replaced objects relative to Phase 6 (the previous six plus main.cpp and gpu/video.cpp), the updated unstripped runtime and this report. No Phase 7/8/9 overlay is needed. Preserve base archives, SDL/DXC, link-argv.json/relink.py and signing key. Evidence compile helpers retain this run's absolute workspace paths; adjust after recovery. Keep game bytes, generated guest code, private checkpoints and keys private. The public source archive excludes all game bytes, generated code, keys and test fixture payloads.

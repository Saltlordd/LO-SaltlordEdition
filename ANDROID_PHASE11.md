# Lost Odyssey Android — Phase 11 storage-path fix

The Phase 10 phone screenshot reported "Destination ancestors must not contain links or junctions" before the Disc 1 import. The exact offending ancestor was not supplied in a log. Inspection found that Android app storage roots were passed unchanged to a strict all-ancestor link check. A framework alias such as /data/user/0 can therefore reject legitimate app-private storage.

## Changes

InitializeAndroid now resolves only the trusted, existing files/cache directories supplied by Android using filesystem::canonical, validates both before replacing the stored roots, and retains all importer checks on game descendants. It does not canonicalize arbitrary import destinations or disable symlink checks. Logs now identify Phase 11 and record resolved files/cache roots.

After a preparation exception, Share log and Continue remain available. Once the dialog closes, the isolated :bootcheck process flushes logs and exits rather than returning through partially initialized SDL/runtime teardown. This avoids reusing that process's native state. The screenshot does not establish the cause of the separate refusal-to-reopen symptom; the revised exit behavior requires phone verification.

The Disc 1 descriptor importer, staged copy, exact executable fingerprint, cancellation/rollback, persistent kernel lock, touch controls and bounded two-minute first-boot attempt remain from Phase 10. Source ISO is never written. Import requires about 5.7 GB additional space plus overhead.

## Validation

- Host Android-path test passed: framework symlink resolved, config/data/log/cache/settings/profile paths use resolved roots, missing-root initialization fails without replacing existing roots. Desktop path fixture passed.
- Production Android descriptor importer host integration passed with a private synthetic ISO containing the supplied verified XEX and synthetic assets: publication through resolved app files/game; a symlink beneath the app root is rejected without writing outside it; borrowed descriptor seek/ownership retained; existing install preserved; cancellation and write failure roll back; lock exclusion/reuse and wrong executable rejection pass. These do not emulate the phone's provider or copy real 5.7 GB assets.
- Changed main and readiness units compiled with NDK r29/API 28/ARM64; full runtime linked against the verified Phase 10 object overlay and unchanged dependencies. Debug information retained.
- Java/DEX packaging and APK signature verification passed. Signing certificate matches the original restored Runtime Check key. 16 KiB ZIP alignment passed; all four native ELF LOAD alignments are at least 16 KiB and packaged library bytes match checked stripped inputs.
- All 62,857 generated PPC definitions present. All 226 strong import-function exports match Phase 6, retaining previously checked required hooks. Symbol presence does not establish execution correctness.
- Cumulative source patch applies to upstream 66f033b76a7ad44ca8d3491adb3a83a933dd6c1e.
- Package io.github.freefrank.lostodyssey.runtimecheck; versionCode 6; versionName phase11-storage-path-fix; Activity process :bootcheck; ARM64/min API 28/target API 35.
- APK bytes 114459708; SHA-256 365a2bfd58c49951df49358cf874e769c0943c980db97b3eb84f5f32fce22459.

## Phone test

Force stop the existing Runtime Check app, then install LostOdyssey-Android-Import-Fix-Test.apk over it. No uninstall is required. Open, Continue and select Disc 1 ISO. Allow the full import to finish, then Continue at the executable/setup dialog to start the bounded guest boot attempt. Keep the app in the foreground and share the final log. If it stops with a preparation error, share that log before Continue; reopen afterward to check a fresh launch. Gamepad is optional; touch controls remain.

No successful Phase 11 phone import, game boot, useful frame, game audio or gameplay is claimed.

## Recovery

Apply the combined patch to the pinned upstream baseline and initialize submodules. Apply plume-lostodyssey.patch before plume-android.patch, and the XenonRecomp patch. Restore private generated headers/sources from Phase 2 and pinned DXC/FFmpeg sources from Phase 6 Source. Use NDK r29, SDK 35 and Java 17.

For fast relink, restore COMPLETE Phase 6 Private Compiled Checkpoint then overlay Phase 11 Private Checkpoint. It contains the runtime and all eight replacement objects relative to Phase 6; earlier overlays are unnecessary. Preserve base dependency archives, SDL/DXC, relink helper/link argv and original signing key. The recorded compile helper uses this task's absolute paths and must be adjusted on recovery. Keep private checkpoints/generated game code/keys private. Public source contains no game bytes, generated PPC sources or signing keys.

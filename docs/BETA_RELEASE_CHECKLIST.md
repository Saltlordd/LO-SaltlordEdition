# First beta release gates

- Build and test restart-only initial shader completion; cache-only launches must not loop.
- [x] Compile/link the corrected public Android runtime recipe using private generated game inputs.
- [ ] Independently validate game-input regeneration, DXC rebuilding and the freshly linked runtime on a phone.
- Preserve the maintainer signing key privately; confirm update compatibility and APK checksum.
- Recheck fresh wizard, driver retention, cold-launch graphics, save/layout backup and restore, main-menu Back, controller profiles, fold/unfold/Flex, and cancelled/repeated diagnostic exports.
- Publish source and full notices with the APK; attach release notes and checksum.
- Enable Issues in repository settings if fork Issues are currently disabled.

The APK has been extensively tested on the maintainer's Z Fold 8 Ultra. Other devices and later game progression remain beta testing areas.

# Lost Odyssey Android XEX Test — Phase 3

Install LostOdyssey-Android-XEX-Test-debug.apk over the Audio Test (same package and signing certificate). Open **Lost Odyssey XEX Test**, tap **Choose Disc 1 default.xex**, and select the extracted executable previously supplied. Do not select the ISO. After tests finish, tap **Share log** and send the log back.

This is a loader diagnostic, not a playable build. It does not execute guest instructions or render game frames. No game files are bundled. The selected file is copied privately so the app can read it without broad storage permissions.

## Verified

- Host integration test passed the production XEX decrypt/decompress and guest-memory copy; wrong-size, missing, and same-size modified files were rejected.
- Input: 6,623,232 bytes; SHA-256 175ae53d109d480a83bebbd186e7b6871f7b03ce80af69ab388db2f747640de3.
- Decoded image: base 0x82000000, size 0x13C0000, entry 0x827CA440; 17 sections, 222 symbols. SHA-256 cb756b46092e448923517bf660b44ad1a0651c2860882b43ae021f4ad4290f71.
- All 62,857 generated PPC functions are present in the compiled ARM64 static library (247 archive members). This library is not bundled into the diagnostic APK and compilation does not establish correct execution.
- Full Android runtime CMake configuration succeeds after excluding desktop updater implementations. Common version parsing remains. Desktop branches retain their previous behavior.
- Android runtime precompiled header, main.cpp, and DXC compiler translation unit compile successfully in direct audits. The entire game runtime has not been linked or launched; Android lifecycle integration and renderer/runtime behavior remain future work.
- APK signature verified, same certificate as Audio Test, version code 4, ARM64 shared libraries with at least 16 KiB ELF load alignment.
- Previous phone audio test passed, and the user confirmed hearing its tone. The new XEX loader still needs phone verification.

## Rebuild and source

Source bundle contains the patch against upstream 66f033b76a7ad44ca8d3491adb3a83a933dd6c1e, incremental patch from 6109f84, changed files, pinned decoder source and notices, and build evidence. Initialize upstream submodules and apply the existing XenonRecomp and plume patches described in the repository. Use NDK r27d, SDK 35, Java 17, CMake and Ninja; run tools/build_android_probe.py --sdk YOUR_SDK. The private compiled checkpoint is separate and contains game-derived generated object code; it is not a public source distribution.

APK SHA-256: bf0daf2fd3a9a9548293a8cb1534f3799d987dd3c5f3f6ca2196f3924385f678.

# macOS releases

macOS support is an experimental Apple Silicon integration. v0.7.35 is the first
release with a macOS package: `LostOdysseyRecomp-macos-arm64-v0.7.35.dmg`, a
disk image that is ad-hoc signed and not notarized. GitHub CI builds the source
and tests that do not require private game data; it does not link a complete
runtime with game data, so the disk image is built on a Mac and uploaded to the
release by hand.

## Disk image (v0.7.35)

Build the runtime first using [the macOS build instructions](BUILDING.md#building-on-macos).
Then run this on the Mac that built the executable:

```sh
python3 tools/package_macos.py --build <build dir> --output <dir> --version v0.7.35 --dmg
```

- `--dmg` writes a compressed HFS+ disk image, with the volume name "Lost
  Odyssey Recomp", that holds `LostOdysseyRecomp.app` and a link to
  `/Applications`. Without it the script writes a ZIP.
- `--version v0.7.35` sets the asset tag, so the file is named
  `LostOdysseyRecomp-macos-arm64-v0.7.35.dmg`. Keep the `v`: the script uses
  the text as given, and the in-game updater looks for exactly this name. Do not
  use `--release` here; it needs a Developer ID identity, and the script exits
  without one.
- Without `--identity` the app is signed ad hoc and the image is not signed.
  Neither is notarized.
- The app declares macOS 14.0 as its minimum (`MINIMUM_MACOS` in the script),
  but the bundled `libdxcompiler.dylib` (DXC v1.8.2407 from renderbag/dxc-bin)
  is built for macOS 15.0. The runtime loads it to compile shaders that the
  downloaded pack does not cover. No run below macOS 26.6.2 is recorded.
- Upload the image to the release by hand. The release workflow's publish step
  accepts the three packages built by CI and, at most, this one disk image
  beside them.

### Published image, 2026-10-02

The v0.7.35 release was published at 2026-10-02T09:13:35Z ([release
record](STATUS.md#v0735-published--2026-10-02)). The image was built on the
maintainer's M1 Max (macOS 26.6.2) from tag `v0.7.35` with
`python3 tools/package_macos.py --version v0.7.35 --dmg` and uploaded to the
draft release by hand before publication. GitHub lists
`LostOdysseyRecomp-macos-arm64-v0.7.35.dmg` at 60,429,795 bytes with SHA-256
`4c8998fe42105c7db5492b5fc666a1185692682a54287866a61bd27076d76d94`, which equals
the SHA-256 computed on the Mac. Checks on the built file:

- The binary inside embeds revision `95f2c8968d0c`, and the bundle version
  (`CFBundleShortVersionString`) is `0.7.35`.
- `hdiutil verify` reports the image valid, and the mounted image holds
  `LostOdysseyRecomp.app` and an Applications link.
- `codesign --verify --strict --deep` passes on the app. The signature is ad hoc
  and carries no team ID.

Before the tag, `LoUpdaterTest` passed on Windows, including the new assertions
that macOS ignores a ZIP and selects its disk image. A test image built from the
release branch on the Mac passed the same image checks, and
`publish_shader_packs.py --check` passed on the Mac on the release branch before
its final rebase, whose code equals the tag apart from one test-fixture generator
and documents.

Limits: the image was not installed from a download and the Gatekeeper approval
flow was not exercised. No game run was made with the published image or any
other v0.7.35 package.

### First launch and updates

Gatekeeper blocks the first launch of an app that is not notarized. The player
tries to open the app, opens System Settings → Privacy & Security, chooses
**Open Anyway** and confirms. Running
`xattr -dr com.apple.quarantine /Applications/LostOdysseyRecomp.app` in Terminal
does the same. The [installation guide](INSTALLING.md#macos), the README and the
release notes carry these steps.

The macOS updater never replaces the app. At startup it reads the latest
release; when a newer version has a `.dmg` asset, it offers to open the release
page. A release without the disk image gets no update prompt on macOS.

## Local package

For a development package, run this on the Mac that built the executable:

```sh
python3 tools/package_macos.py
```

This creates an ad-hoc signed development ZIP under `out/releases/`, named with
the source version, the commit and `-dev`. Add `--dmg` for a disk image instead.
It is for local testing and is not a published release. The app stores game
data, settings, saves and shader cache under the normal macOS
application-support paths; check the runtime logs on the test machine before
sharing them.

## Signed release workflow

This is the Developer ID workflow. v0.7.35 does not use it, because no Developer
ID certificate is available. It remains for a later release that is signed and
notarized.

The release helper performs a clean-tree check, configures and builds the
runtime, signs the app, and notarizes it using the default `lo-notary` profile
or the profile supplied on the command line. An unavailable identity or notary
profile fails the helper. It writes a ZIP to `out/releases/` and does not
publish a GitHub release.

```sh
tools/release_macos.sh "Developer ID Application: Your Name (TEAMID1234)"
```

Optional arguments are `[notary-profile] [version-suffix]`. The default suffix
is empty; pass a suffix when a separate macOS version is intentionally needed:

```sh
tools/release_macos.sh \
  "Developer ID Application: Your Name (TEAMID1234)" \
  lo-notary macos.1
```

The resulting tag and ZIP name use the source version, with the optional
suffix. Confirm the identity and notary credentials locally before running the
helper. The repository bundle identifier is
`io.github.freefrank.LostOdysseyRecomp`.

The helper calls `tools/package_macos.py` without `--dmg`, so it still writes a
ZIP and prints a `.zip` path, while the updater only selects a `.dmg`. Before the
next Developer ID release, add `--dmg` to that call and update the file name the
helper prints. With `--identity` and `--notarize`, the script notarizes and
staples the app first and then signs the image; that combination has not been
run.

## Validation boundary

A successful package, signature or notarization check does not establish
gameplay, image quality, performance or complete-playthrough acceptance. Record
the Mac model, macOS version, source commit, game edition and tested scenes
alongside any local result. Keep the existing Windows/Linux release records
separate from this experimental path.

For v0.7.35 the recorded runs are on one Mac, the maintainer's M1 Max (macOS
26.6.2). The opening new-game battle scenario ran with Metal. The Metal shader
pack was downloaded from the `shader-packs` release and used
([details](PORTABLE_SHADER_PACK.md#validation-2026-10-02)). HDR requested on an
external display without EDR headroom stayed in SDR, so Metal HDR output itself
has not been seen ([details](notes/hdr-output.md)). These runs were made earlier
on 2026-10-02, on `main` at `2cd3fe6` and on the HDR review branch, whose runtime
code matches the tag apart from the updater's macOS asset type; none used the
published image. Long play, broader scenes, image quality, performance and other
Macs are untested. No recorded run installed the disk image from a download and
followed the first-launch approval;
the Gatekeeper steps above follow Apple's documented behavior for apps that are
not notarized. This is not acceptance; [STATUS.md](STATUS.md) records acceptance
and publication.

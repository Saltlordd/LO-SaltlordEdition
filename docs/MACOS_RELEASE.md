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
- The app declares macOS 14.0 as its minimum (`MINIMUM_MACOS` in the script).
  The bundled `libdxcompiler.dylib` (DXC v1.8.2407 from renderbag/dxc-bin) is
  built for macOS 15.0, but it was checked on macOS 14; see
  [macOS 14 and the bundled DXC](#macos-14-and-the-bundled-dxc).
- Upload the image to the release by hand. The release workflow's publish step
  accepts the three packages built by CI and, at most, this one disk image
  beside them.

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

## macOS 14 and the bundled DXC

Checked on 2026-10-02 for v0.7.35, which keeps macOS 14.0 as its minimum. The
runtime loads `libdxcompiler.dylib` with `dlopen` to compile shaders that the
downloaded pack does not cover. Both slices of the library are marked
`minos 15.0` (SDK 15.2), yet dyld on macOS 14.8.9 loaded it. What the library
needs from the system is its imports: the arm64 slice links only `libSystem`
and `libc++` and uses chained fixups, so dyld binds all 330 of its imports (167
from libSystem, 163 from libc++, none weak) when it loads the library.

- On GitHub's macOS 14 runner, every import links against the macOS 14.0 SDK
  (Xcode 15.0.1). A control symbol from libc++ 18,
  `__cxa_init_primary_exception` (macOS 15.0), does not. The libc++ imports
  also all appear in LLVM 16's `arm64-apple-darwin` ABI list, and the SDK's
  libc++ availability header maps LLVM 16 to macOS 14.0.
- On the same runner (macOS 14.8.9), `LoRenderResolutionShaderTest` loaded the
  library and passed its 22 DXIL and SPIR-V compilations, with the dxc-bin copy
  and again with the copy from the v0.7.35 disk image, which has the same code
  and a new ad-hoc signature. macOS CI built the test with the current SDK and
  deployment target 14.0, as the runtime is built.
- The v0.7.35 app started there headless without game data: it logged its
  version and the host and exited at the missing game data, so dyld loaded the
  executable on macOS 14.8.9. It did not open a window, create a Metal device
  or run the game, which has only run on macOS 26.6.2.

CI runs: [DXC check](https://github.com/freefrank/LostOdysseyRecomp/actions/runs/36989074352),
[disk-image check](https://github.com/freefrank/LostOdysseyRecomp/actions/runs/36989983256).
The macOS 14 job ran only for this check, because GitHub stops supporting its
macos-14 image on 2026-11-02. macOS CI still builds and runs
`LoRenderResolutionShaderTest` on every run, so the bundled DXC compiles shaders
on the CI image (macOS 26). Before a different DXC ships, link its imports
against the macOS 14.0 SDK and run the test on macOS 14 again, or raise the
minimum.

## Validation boundary

A successful package, signature or notarization check does not establish
gameplay, image quality, performance or complete-playthrough acceptance. Record
the Mac model, macOS version, source commit, game edition and tested scenes
alongside any local result. Keep the existing Windows/Linux release records
separate from this experimental path.

For v0.7.35 the recorded game runs are on one Mac, the maintainer's M1 Max
(macOS 26.6.2). The opening new-game battle scenario ran with Metal. The Metal
shader pack was downloaded from the `shader-packs` release and used
([details](PORTABLE_SHADER_PACK.md#validation-2026-10-02)). HDR requested on an
external display without EDR headroom stayed in SDR, so Metal HDR output itself
has not been seen ([details](notes/hdr-output.md)). Long play, broader scenes,
image quality, performance and other Macs are untested. No recorded run
installed the disk image from a download and followed the first-launch approval;
the Gatekeeper steps above follow Apple's documented behavior for apps that are
not notarized. This is not acceptance; [STATUS.md](STATUS.md) records acceptance
and publication.

# macOS releases

macOS support is an experimental Apple Silicon integration. This branch has no
published macOS package yet. GitHub CI builds the source and tests that do not
require private game data; it does not link a complete runtime with game data.

## Local package

Build the runtime first using [the macOS build instructions](BUILDING.md#building-on-macos).
For a local development package, run this on the Mac that built the executable:

```sh
python3 tools/package_macos.py
```

This creates an ad-hoc signed development ZIP under `out/releases/`. It is for
local testing and is not a published release. The app stores game data,
settings, saves and shader cache under the normal macOS application-support
paths; check the runtime logs on the test machine before sharing them.

## Signed release workflow

The release helper performs a clean-tree check, configures and builds the
runtime, signs the app, and notarizes it using the default `lo-notary` profile
or the profile supplied on the command line. An unavailable identity or notary
profile fails the helper. It writes the ZIP to `out/releases/` and does not
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

## Validation boundary

A successful package, signature or notarization check does not establish
gameplay, image quality, performance or complete-playthrough acceptance. Record
the Mac model, macOS version, source commit, game edition and tested scenes
alongside any local result. Keep the existing Windows/Linux release records
separate from this experimental path.

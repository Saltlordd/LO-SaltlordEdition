#!/bin/zsh
# Build, sign, notarize and package a macOS release.
# See docs/MACOS_RELEASE.md for the one-time certificate and notary setup.
#
#   tools/release_macos.sh "<Developer ID Application: Name (TEAMID)>" [notary-profile] [version-suffix]
#   tools/release_macos.sh "Developer ID Application: Jane Doe (ABCDE12345)"
#
# The version is the source version, optionally followed by -<version-suffix>; the
# ZIP lands in out/releases as LostOdysseyRecomp-macos-arm64-<tag>.zip.
set -euo pipefail
cd "$(dirname "$0")/.."

if [[ $# -lt 1 || $# -gt 3 ]]; then
  sed -n 2,9p "$0"
  exit 2
fi
identity=$1
profile=${2:-lo-notary}
suffix=${3:-}
build=out/build/macos-gpu

if [[ -n "$(git status --porcelain --untracked-files=no)" ]]; then
  echo "Commit or stash tracked changes first: a release is built from a clean tree." >&2
  exit 1
fi
if ! security find-identity -v -p codesigning | grep -qF "$identity"; then
  echo "Signing identity not found in the keychain: $identity" >&2
  security find-identity -v -p codesigning >&2
  exit 1
fi

cmake -S . -B "$build" -DLO_VERSION_SUFFIX="$suffix" >/dev/null
cmake --build "$build" --target LostOdysseyRecomp
version=$(<"$build/LostOdysseyRecomp/source-version.txt")
echo "Version $version (tag v$version)"

python3 tools/package_macos.py --build "$build" --release --identity "$identity" --notarize "$profile"

# Later local builds keep reporting this release's version until the next one;
# reset with: cmake -S . -B out/build/macos-gpu -DLO_VERSION_SUFFIX=
echo
echo "Next: publish out/releases/LostOdysseyRecomp-macos-arm64-v$version.zip as release v$version"
echo "on github.com/freefrank/LostOdysseyRecomp (see docs/MACOS_RELEASE.md)."

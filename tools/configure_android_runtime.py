#!/usr/bin/env python3
"""Configure or build the full Android ARM64 SDL shared library (not an installable APK)."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk', type=Path, default=os.environ.get('ANDROID_HOME') or os.environ.get('ANDROID_SDK_ROOT'))
    parser.add_argument('--ndk', type=Path, default=os.environ.get('ANDROID_NDK_HOME'))
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'out/android-runtime-r29')
    parser.add_argument('--cmake', default='cmake')
    parser.add_argument('--ninja', default='ninja')
    parser.add_argument('--build', action='store_true')
    parser.add_argument('--jobs', type=int, default=4)
    args = parser.parse_args()
    if args.ndk:
        ndk = args.ndk.resolve()
    elif args.sdk:
        ndk = args.sdk.resolve() / 'ndk/29.0.14206865'
    else:
        parser.error('Pass --ndk, or --sdk containing NDK 29.0.14206865')
    if not (ndk / 'build/cmake/android.toolchain.cmake').is_file():
        parser.error(f'Android toolchain not found: {ndk}')
    if args.jobs < 1:
        parser.error('--jobs must be positive')
    build = args.build_dir.resolve()
    subprocess.run([args.cmake, '-S', str(ROOT), '-B', str(build), '-G', 'Ninja',
        '-DCMAKE_MAKE_PROGRAM=' + str(Path(shutil.which(args.ninja) or args.ninja).resolve()),
        '-DCMAKE_TOOLCHAIN_FILE=' + str(ndk / 'build/cmake/android.toolchain.cmake'),
        '-DANDROID_ABI=arm64-v8a', '-DANDROID_PLATFORM=android-28', '-DANDROID_STL=c++_shared',
        '-DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON', '-DCMAKE_BUILD_TYPE=Release',
        '-DLO_ANDROID_DIAGNOSTICS=OFF', '-DLO_BUILD_RUNTIME=ON', '-DLO_BUILD_GPU=ON'], check=True)
    if args.build:
        subprocess.run([args.cmake, '--build', str(build), '--target', 'LostOdysseyRecomp', '-j', str(args.jobs)], check=True)
    print('Configured runtime; this target is not an APK or a verified Android game launch.')

if __name__ == '__main__':
    main()

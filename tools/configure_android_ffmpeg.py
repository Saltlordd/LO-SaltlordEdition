#!/usr/bin/env python3
"""Generate the pinned Xenia FFmpeg fork's Android ARM64 configuration."""
import argparse
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
PIN = '15ece0882e8d5875051ff5b73c5a8326f7cee9f5'

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source', type=Path, required=True)
    p.add_argument('--ndk', type=Path, required=True)
    a = p.parse_args()
    source = a.source.resolve()
    actual = subprocess.check_output(['git', '-C', str(source), 'rev-parse', 'HEAD'], text=True).strip()
    if actual != PIN:
        p.error(f'Expected pinned FFmpeg {PIN}; got {actual}')
    if sys.platform not in ('linux', 'darwin'):
        p.error('FFmpeg configure requires a POSIX host (Linux or macOS)')
    host = 'darwin-x86_64' if sys.platform == 'darwin' else 'linux-x86_64'
    llvm = a.ndk.resolve() / 'toolchains/llvm/prebuilt' / host / 'bin'
    # This fork checks in config.h, making normal out-of-tree configure fail.
    # Generate in an isolated source copy; leave FetchContent's source untouched.
    work = ROOT / 'out/ffmpeg-android-config-src'
    if work.exists():
        shutil.rmtree(work)
    shutil.copytree(source, work, ignore=shutil.ignore_patterns('.git'))
    (work / 'config.h').unlink(missing_ok=True)
    args = [str(work / 'configure'),
            '--cc=' + str(llvm / 'aarch64-linux-android28-clang'),
            '--cxx=' + str(llvm / 'aarch64-linux-android28-clang++'),
            '--ar=' + str(llvm / 'llvm-ar'), '--ranlib=' + str(llvm / 'llvm-ranlib'),
            '--strip=' + str(llvm / 'llvm-strip'),
            '--enable-cross-compile', '--arch=aarch64', '--target-os=android',
            '--enable-pic', '--disable-everything', '--disable-programs',
            '--disable-all', '--disable-autodetect', '--enable-avcodec',
            '--enable-avutil', '--enable-decoder=xmaframes']
    subprocess.run(args, cwd=work, check=True)
    destination = ROOT / 'thirdparty/ffmpeg-config/android-aarch64'
    destination.mkdir(parents=True, exist_ok=True)
    header = (work / 'config.h').read_text()
    (destination / 'config.h').write_text('\n'.join(line.rstrip() for line in header.splitlines()) + '\n')
    print('Generated', destination / 'config.h')

if __name__ == '__main__':
    main()

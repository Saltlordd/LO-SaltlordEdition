#!/usr/bin/env python3
"""Build the real Android platform probe using NDK + SDK tools, without Gradle.
This packages diagnostics, SDL, plume, the XMA decoder, XEX loader and libc++; it cannot launch the game.
"""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]

def run(args):
    print('+', ' '.join(map(str, args)), flush=True)
    subprocess.run(list(map(str, args)), cwd=ROOT, check=True)

def tool_at(directory, name):
    suffixes = ['.exe', '.bat', ''] if os.name == 'nt' else ['']
    for base in [directory, *sorted(directory.glob('*'))]:
        for suffix in suffixes:
            candidate = base / (name + suffix)
            if candidate.is_file():
                return candidate
    raise FileNotFoundError(f'{name} not found below {directory}')

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--sdk', default=os.environ.get('ANDROID_HOME') or os.environ.get('ANDROID_SDK_ROOT'))
    p.add_argument('--cmake', default='cmake')
    p.add_argument('--ninja', default='ninja')
    p.add_argument('--jobs', default='4')
    p.add_argument('--skip-native-build', action='store_true', help='Package an already built out/android-probe')
    a = p.parse_args()
    if not a.sdk: p.error('Set ANDROID_HOME or pass --sdk')
    sdk = Path(a.sdk).resolve()
    ndk = sdk / 'ndk/27.3.13750724'
    build = ROOT / 'out/android-probe'
    package = build / 'package'
    # Prevent accidentally packaging stale Java classes/dex from an older build.
    if package.exists(): shutil.rmtree(package)
    classes = package / 'classes'
    dex = package / 'dex'
    classes.mkdir(parents=True)
    dex.mkdir()
    if not a.skip_native_build:
        run([a.cmake, '-S', ROOT, '-B', build, '-G', 'Ninja',
             '-DCMAKE_MAKE_PROGRAM=' + str(Path(shutil.which(a.ninja) or a.ninja).resolve()),
             '-DCMAKE_TOOLCHAIN_FILE=' + str(ndk / 'build/cmake/android.toolchain.cmake'),
             '-DANDROID_ABI=arm64-v8a', '-DANDROID_PLATFORM=android-28',
             '-DANDROID_STL=c++_shared', '-DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON',
             '-DLO_ANDROID_DIAGNOSTICS=ON', '-DCMAKE_BUILD_TYPE=Debug'])
        run([a.cmake, '--build', build, '--target', 'lo_android_probe', '-j', a.jobs])
    android_jar = sdk / 'platforms/android-35/android.jar'
    sources = sorted((ROOT / 'thirdparty/SDL/android-project/app/src/main/java').rglob('*.java'))
    sources += sorted((ROOT / 'android/app/src/main/java').rglob('*.java'))
    javac = [shutil.which('javac')] if shutil.which('javac') else ['java', '-m', 'jdk.compiler/com.sun.tools.javac.Main']
    run([*javac, '-encoding', 'UTF-8', '--release', '8', '-classpath', android_jar, '-d', classes, *sources])
    class_jar = package / 'classes.jar'
    with zipfile.ZipFile(class_jar, 'w') as z:
        for f in sorted(classes.rglob('*.class')): z.write(f, f.relative_to(classes).as_posix())
    tools = sdk / 'build-tools/35.0.0'
    run([tool_at(tools, 'd8'), '--min-api', '28', '--lib', android_jar, '--output', dex, class_jar])
    unsigned = package / 'unsigned.apk'
    # aapt2 requires package on source manifest when not running via AGP.
    manifest = (ROOT / 'android/app/src/main/AndroidManifest.xml').read_text()
    manifest = manifest.replace('<manifest xmlns:', '<manifest package="io.github.freefrank.lostodyssey.probe" xmlns:')
    generated_manifest = package / 'AndroidManifest.xml'
    generated_manifest.write_text(manifest)
    run([tool_at(tools, 'aapt2'), 'link', '-o', unsigned, '--manifest', generated_manifest,
         '-I', android_jar, '--min-sdk-version', '28', '--target-sdk-version', '35',
         '--version-code', '4', '--version-name', 'phase3-xex-probe'])
    llvm_host = 'windows-x86_64' if os.name == 'nt' else 'darwin-x86_64' if sys.platform == 'darwin' else 'linux-x86_64'
    libraries = [build / 'android/native/liblo_android_probe.so', build / 'android/native/SDL/libSDL2.so',
                 ndk / f'toolchains/llvm/prebuilt/{llvm_host}/sysroot/usr/lib/aarch64-linux-android/libc++_shared.so']
    with zipfile.ZipFile(unsigned, 'a') as z:
        for f in sorted(dex.glob('*.dex')): z.write(f, f.name)
        for f in libraries: z.write(f, 'lib/arm64-v8a/' + f.name, compress_type=zipfile.ZIP_STORED)
    aligned = package / 'aligned.apk'
    run([tool_at(tools, 'zipalign'), '-P', '16', '-f', '4', unsigned, aligned])
    keystore = build / 'probe-debug.keystore'
    if not keystore.exists():
        run(['keytool', '-genkeypair', '-keystore', keystore, '-storepass', 'android', '-keypass', 'android',
             '-alias', 'androiddebugkey', '-dname', 'CN=Android Debug,O=Android,C=US',
             '-keyalg', 'RSA', '-keysize', '2048', '-validity', '10000'])
    apk = build / 'LostOdyssey-Android-XEX-Test-debug.apk'
    run([tool_at(tools, 'apksigner'), 'sign', '--ks', keystore, '--ks-key-alias', 'androiddebugkey',
         '--ks-pass', 'pass:android', '--key-pass', 'pass:android', '--out', apk, aligned])
    run([tool_at(tools, 'apksigner'), 'verify', '--verbose', apk])
    run([tool_at(tools, 'zipalign'), '-c', '-P', '16', '4', apk])
    print(f'Diagnostic APK: {apk}')

if __name__ == '__main__': main()

#!/usr/bin/env python3
"""Package the full SDL runtime and ARM64 DXC into a bounded diagnostic APK."""
import argparse
from pathlib import Path
import shutil
import sys
import zipfile
from build_android_probe import ROOT, run, tool_at

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--sdk', type=Path, required=True)
    p.add_argument('--ndk', type=Path, required=True)
    p.add_argument('--runtime-build', type=Path, default=ROOT / 'out/android-runtime-r29')
    p.add_argument('--dxc-build', type=Path, default=ROOT / 'out/android-dxc')
    p.add_argument('--adrenotools-build',type=Path,default=ROOT/'out/android-adrenotools')
    p.add_argument('--dxc-source', type=Path, default=ROOT / 'out/dxc-source')
    a = p.parse_args()
    sdk, ndk, runtime, dxc = [x.resolve() for x in (a.sdk, a.ndk, a.runtime_build, a.dxc_build)]
    build = ROOT / 'out/android-runtime-check'
    package = build / 'package'
    if package.exists(): shutil.rmtree(package)
    classes, dex, native = package / 'classes', package / 'dex', package / 'native'
    for directory in (classes, dex, native): directory.mkdir(parents=True)
    host = 'windows-x86_64' if sys.platform == 'win32' else 'darwin-x86_64' if sys.platform == 'darwin' else 'linux-x86_64'
    llvm = ndk / 'toolchains/llvm/prebuilt' / host
    libraries = [runtime / 'LostOdysseyRecomp/libLostOdysseyRecomp.so',
                 runtime / 'thirdparty/SDL/libSDL2.so', dxc / 'lib/libdxcompiler.so',
                 llvm / 'sysroot/usr/lib/aarch64-linux-android/libc++_shared.so']
    libraries += [a.adrenotools_build.resolve()/('lib'+name+'.so') for name in ['adrenotools','hook_impl','main_hook','file_redirect_hook','gsl_alloc_hook']]
    strip = llvm / 'bin' / ('llvm-strip.exe' if sys.platform == 'win32' else 'llvm-strip')
    for source in libraries:
        if not source.is_file(): p.error(f'Missing native input: {source}')
        destination = native / source.name
        shutil.copy2(source, destination)
        run([strip, '--strip-unneeded', destination])
    android_jar = sdk / 'platforms/android-35/android.jar'
    sources = sorted((ROOT / 'thirdparty/SDL/android-project/app/src/main/java').rglob('*.java'))
    java_root = ROOT / 'android/app/src/main/java'
    sources += sorted(f for f in java_root.rglob('*.java') if f.name not in ('ProbeActivity.java', 'TestLauncherActivity.java'))
    javac = [shutil.which('javac')] if shutil.which('javac') else ['java', '-m', 'jdk.compiler/com.sun.tools.javac.Main']
    run([*javac, '-encoding', 'UTF-8', '--release', '8', '-classpath', android_jar, '-d', classes, *sources])
    jar = package / 'classes.jar'
    with zipfile.ZipFile(jar, 'w') as z:
        for f in sorted(classes.rglob('*.class')): z.write(f, f.relative_to(classes).as_posix())
    tools = sdk / 'build-tools/35.0.0'
    run([tool_at(tools, 'd8'), '--min-api', '28', '--lib', android_jar, '--output', dex, jar])
    resources = package / 'resources.zip'
    run([tool_at(tools, 'aapt2'), 'compile', '--dir', ROOT / 'android/app/src/main/res', '-o', resources])
    unsigned, aligned = package / 'unsigned.apk', package / 'aligned.apk'
    run([tool_at(tools, 'aapt2'), 'link', '-o', unsigned, '--manifest', ROOT / 'android/runtime-check-manifest.xml',
         '-I', android_jar, '--min-sdk-version', '28', '--target-sdk-version', '35',
         '--version-code', '51', '--version-name', '0.1.0-beta', resources])
    with zipfile.ZipFile(unsigned, 'a') as z:
        asset_root=ROOT/'android/app/src/main/assets'
        for f in sorted(asset_root.rglob('*')):
            if f.is_file():z.write(f,'assets/'+f.relative_to(asset_root).as_posix(), compress_type=zipfile.ZIP_STORED if f.suffix == '.wav' else zipfile.ZIP_DEFLATED)
        for f in sorted(dex.glob('*.dex')): z.write(f, f.name)
        for f in sorted(native.glob('*.so')): z.write(f, 'lib/arm64-v8a/' + f.name, compress_type=zipfile.ZIP_STORED)
        notices = [ROOT/'thirdparty/libadrenotools/LICENSE', ROOT/'thirdparty/libadrenotools/lib/linkernsbypass/LICENSE',ROOT / 'LICENSE', ROOT / 'thirdparty/SDL/LICENSE.txt',
                   a.dxc_source.resolve() / 'LICENSE.TXT', a.dxc_source.resolve() / 'ThirdPartyNotices.txt',
                   llvm / 'NOTICE']
        for index, f in enumerate(notices):
            if f.is_file(): z.write(f, f'assets/licenses/{index}-{f.name}')
    run([tool_at(tools, 'zipalign'), '-P', '16', '-f', '4', unsigned, aligned])
    key = build / 'runtime-check-debug.keystore'
    if not key.exists():
        run(['keytool', '-genkeypair', '-keystore', key, '-storepass', 'android', '-keypass', 'android',
             '-alias', 'androiddebugkey', '-dname', 'CN=Android Debug,O=Android,C=US',
             '-keyalg', 'RSA', '-keysize', '2048', '-validity', '10000'])
    apk = build / 'LostOdyssey-Android-Runtime-Check-debug.apk'
    run([tool_at(tools, 'apksigner'), 'sign', '--ks', key, '--ks-key-alias', 'androiddebugkey',
         '--ks-pass', 'pass:android', '--key-pass', 'pass:android', '--out', apk, aligned])
    run([tool_at(tools, 'apksigner'), 'verify', '--verbose', apk])
    run([tool_at(tools, 'zipalign'), '-c', '-P', '16', '4', apk])
    print('Diagnostic APK:', apk)

if __name__ == '__main__': main()

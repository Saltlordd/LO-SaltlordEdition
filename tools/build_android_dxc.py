#!/usr/bin/env python3
"""Build pinned Android ARM64 DXC with SPIR-V and native host table generators."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
REVISION = 'b106a961d09221b3c5bdb37be45b679257da08b8'

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--ndk', type=Path, required=True)
    p.add_argument('--source', type=Path, default=ROOT / 'out/dxc-source')
    p.add_argument('--cmake', default='cmake')
    p.add_argument('--ninja', default='ninja')
    p.add_argument('--jobs', type=int, default=4)
    a = p.parse_args()
    if a.jobs < 1: p.error('--jobs must be positive')
    ndk, source = a.ndk.resolve(), a.source.resolve()
    if not (ndk / 'build/cmake/android.toolchain.cmake').is_file(): p.error('Invalid NDK directory')
    cmake = str(Path(shutil.which(a.cmake) or a.cmake).resolve())
    ninja = str(Path(shutil.which(a.ninja) or a.ninja).resolve())
    env = dict(os.environ)
    env['PATH'] = str(Path(ninja).parent) + os.pathsep + env.get('PATH', '')
    def run(command, cwd=ROOT):
        subprocess.run(list(map(str, command)), cwd=cwd, env=env, check=True)
    if not source.exists():
        source.parent.mkdir(parents=True, exist_ok=True)
        run(['git', 'clone', '--depth', '1', '--branch', 'v1.8.2505.1',
             'https://github.com/microsoft/DirectXShaderCompiler.git', source])
    actual = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=source, text=True).strip()
    if actual != REVISION: p.error(f'DXC must be pinned to {REVISION}; found {actual}')
    run(['git', 'submodule', 'update', '--init', '--depth', '1', '--jobs', '3',
         'external/SPIRV-Headers', 'external/SPIRV-Tools', 'external/DirectX-Headers'], source)
    patch = ROOT / 'tools/patches/dxc-android.patch'
    applied = subprocess.run(['git', 'apply', '--reverse', '--check', str(patch)], cwd=source,
                             stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0
    if not applied: run(['git', 'apply', str(patch)], source)
    host, target = ROOT / 'out/host-dxc', ROOT / 'out/android-dxc'
    common = [cmake, '-S', source, '-G', 'Ninja', '-DCMAKE_POLICY_VERSION_MINIMUM=3.5',
        '-C', source / 'cmake/caches/PredefinedParams.cmake',
        '-DCMAKE_MAKE_PROGRAM=' + ninja, '-DCMAKE_BUILD_TYPE=Release',
        '-DHLSL_INCLUDE_TESTS=OFF', '-DSPIRV_BUILD_TESTS=OFF', '-DLLVM_INCLUDE_TESTS=OFF',
        '-DLLVM_ENABLE_ZLIB=OFF', '-DLLVM_ENABLE_TERMINFO=OFF']
    run([*common, '-B', host, '-DENABLE_SPIRV_CODEGEN=OFF'])
    run([cmake, '--build', host, '--target', 'llvm-tblgen', 'clang-tblgen', '-j', a.jobs])
    run([*common, '-B', target,
        '-DCMAKE_TOOLCHAIN_FILE=' + str(ndk / 'build/cmake/android.toolchain.cmake'),
        '-DANDROID_ABI=arm64-v8a', '-DANDROID_PLATFORM=android-28', '-DANDROID_STL=c++_shared',
        '-DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON', '-DENABLE_SPIRV_CODEGEN=ON',
        '-DLLVM_TABLEGEN=' + str(host / 'bin/llvm-tblgen'),
        '-DCLANG_TABLEGEN=' + str(host / 'bin/clang-tblgen')])
    run([cmake, '--build', target, '--target', 'dxcompiler', '-j', a.jobs])
    print('Android compiler:', target / 'lib/libdxcompiler.so')
    print('Target phone compilation still requires the readiness check.')

if __name__ == '__main__': main()

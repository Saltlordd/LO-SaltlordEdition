#!/usr/bin/env python3
from pathlib import Path
import subprocess
import argparse
p=argparse.ArgumentParser(description='Build upstream libadrenotools and hook libraries for ARM64 Android API28 with 16KiB alignment.')
p.add_argument('--ndk',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
root=Path(__file__).resolve().parents[1];src=root/'thirdparty/libadrenotools';out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
llvm=a.ndk.resolve()/'toolchains/llvm/prebuilt/linux-x86_64'
inc=['-I'+str(src/p) for p in ['include','.','lib/linkernsbypass','src/hook']]
base=['--target=aarch64-none-linux-android28','--sysroot='+str(llvm/'sysroot'),'-O3','-fPIC']+inc
objects={}
for name in ['lib/linkernsbypass/android_linker_ns.cpp','lib/linkernsbypass/elf_soname_patcher.cpp','src/driver.cpp','src/bcenabler.cpp','src/hook/hook_impl.cpp','src/hook/main_hook.c','src/hook/file_redirect_hook.c','src/hook/gsl_alloc_hook.c']:
 cpp=name.endswith('.cpp');o=out/(Path(name).stem+'.o');flags=base+(['-std=c++17'] if cpp else [])+(['-fvisibility=hidden'] if name=='src/hook/hook_impl.cpp' else [])
 subprocess.run([str(llvm/'bin'/('clang++' if cpp else 'clang'))]+flags+['-c',str(src/name),'-o',str(o)],check=True);objects[name]=o
bypass=[objects[x] for x in objects if x.startswith('lib/')]
libs={'adrenotools':[objects['src/driver.cpp'],objects['src/bcenabler.cpp']]+bypass,'hook_impl':[objects['src/hook/hook_impl.cpp']]+bypass}
for name,obs in libs.items():subprocess.run([str(llvm/'bin/clang++')]+base+['-shared','-Wl,-z,max-page-size=16384','-Wl,-soname,lib'+name+'.so','-Wl,--no-undefined']+list(map(str,obs))+['-landroid','-ldl','-llog','-o',str(out/('lib'+name+'.so'))],check=True)
for name in ['main_hook','file_redirect_hook','gsl_alloc_hook']:subprocess.run([str(llvm/'bin/clang++')]+base+['-shared','-Wl,-z,max-page-size=16384','-Wl,-z,global','-Wl,-soname,lib'+name+'.so','-Wl,--no-undefined',str(objects['src/hook/'+name+'.c']),'-L'+str(out),'-lhook_impl','-o',str(out/('lib'+name+'.so'))],check=True)
print('Built adrenotools and four hook libraries')

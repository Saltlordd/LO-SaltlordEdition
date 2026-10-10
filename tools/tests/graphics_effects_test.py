#!/usr/bin/env python3
"""Execute production effect hooks with mocked guest imports and settings.

This checks dispatch and guest-register contracts, not visual output or FPS.
"""
from pathlib import Path
import subprocess
import tempfile
import argparse

ROOT = Path(__file__).resolve().parents[2]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cxx', default='g++')
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='lo-effects-') as directory:
        work = Path(directory)
        (work / 'settings').mkdir()
        (work / 'os').mkdir()
        (work / 'stdafx.h').write_text('''#pragma once
#include <cstdint>
struct Register { uint64_t u64=0; };
struct PPCContext { Register r3; };
#define PPC_FUNC(name) void name(PPCContext& ctx, uint8_t* base)
''')
        (work / 'settings/config.h').write_text('''#pragma once
namespace settings {
struct Config { bool motionBlur=true, dynamicShadows=true; };
inline Config config;
inline Config GetConfig() { return config; }
}
''')
        (work / 'os/logger.h').write_text('#pragma once\n#define LOG_INFO(...) ((void)0)\n')
        source = ROOT / 'LostOdysseyRecomp/patches/graphics_effects.cpp'
        harness = work / 'test.cpp'
        harness.write_text('#include "' + source.as_posix() + '"\n' + '''
#include <cassert>
#include <cstdio>
unsigned blurCalls=0, shadowCalls=0;
extern "C" PPC_FUNC(__imp__sub_826E1860) { ++blurCalls;ctx.r3.u64=123; }
extern "C" PPC_FUNC(__imp__sub_823D5E50) { ++shadowCalls;ctx.r3.u64=456; }
int main() {
    PPCContext ctx;
    sub_826E1860(ctx,nullptr);assert(blurCalls==1 && ctx.r3.u64==123);
    settings::config.motionBlur=false;ctx.r3.u64=999;
    sub_826E1860(ctx,nullptr);assert(blurCalls==1 && ctx.r3.u64==0);
    settings::config.motionBlur=true;
    sub_826E1860(ctx,nullptr);assert(blurCalls==2 && ctx.r3.u64==123);
    sub_823D5E50(ctx,nullptr);assert(shadowCalls==1 && ctx.r3.u64==456);
    settings::config.dynamicShadows=false;ctx.r3.u64=999;
    sub_823D5E50(ctx,nullptr);assert(shadowCalls==1 && ctx.r3.u64==999);
    settings::config.dynamicShadows=true;
    sub_823D5E50(ctx,nullptr);assert(shadowCalls==2 && ctx.r3.u64==456);
    std::puts("PASS: production effects enabled, disabled and live re-enabled dispatch/register contracts");
}
''')
        executable = work / 'test'
        subprocess.run([args.cxx, '-std=c++20', '-O2', '-I', str(work), str(harness), '-o', str(executable)], check=True)
        subprocess.run([str(executable)], check=True)

if __name__ == '__main__':
    main()

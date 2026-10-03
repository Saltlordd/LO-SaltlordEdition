#!/usr/bin/env python3
"""Build the standalone metadata decoder using the native host compiler."""
import argparse
from pathlib import Path
import shutil
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "tests"))
from run import compiler_environment, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True, help="build directory outside imported game data")
    args = parser.parse_args()
    build = args.output.resolve()
    build.mkdir(parents=True, exist_ok=True)
    binary = build / ("asset_decoder.exe" if sys.platform == "win32" else "asset_decoder")
    source = ROOT / "tools" / "asset_inventory" / "decode.cpp"
    if sys.platform == "win32":
        env = compiler_environment()
        compiler = shutil.which("clang-cl", path=next(v for k, v in env.items() if k.upper() == "PATH"))
        if not compiler:
            parser.error("native clang-cl not found")
        run([compiler, "/nologo", "/std:c++20", "/EHsc", "/O2", "/MT", "/utf-8",
             f"/I{ROOT / 'LostOdysseyRecomp'}", source, f"/Fo{build / 'decode.obj'}", f"/Fe{binary}"], env=env)
    else:
        run(["c++", "-std=c++20", "-O2", f"-I{ROOT / 'LostOdysseyRecomp'}", source, "-o", binary])
    print(binary)


if __name__ == "__main__":
    main()

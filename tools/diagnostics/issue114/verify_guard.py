"""Compile a small C1 fixture against the actual generated retail bodies.

No game launch, repository scan, guest-image allocation, or runtime build.
Only the declaration header and three explicitly selected PPC chunks are read.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess


ROOT = Path(__file__).resolve().parents[3]
PPC = ROOT / "LostOdysseyRecompLib" / "ppc"
FUNCTIONS = ("sub_8229E0C8", "sub_829E54F0", "sub_82A4FA38")


def retail_bodies() -> str:
    names = re.findall(r"PPC_EXTERN_FUNC\(([^)]+)\)",
                       (PPC / "ppc_recomp_shared.h").read_text(encoding="utf-8"))
    # Chunks contain 256 declarations, including non-sub_ helpers. Do not
    # filter the header names before calculating the chunk number.
    chunks: dict[int, str] = {}
    bodies = []
    for name in FUNCTIONS:
        number = names.index(name) // 256
        if number not in chunks:
            chunks[number] = (PPC / f"ppc_recomp.{number}.cpp").read_text(encoding="utf-8")
        source = chunks[number]
        start = source.index(f"PPC_FUNC_IMPL(__imp__{name})")
        end = source.index("\nPPC_WEAK_FUNC(", start)
        bodies.append(source[start:end])
    if "ctx.lr = 0x82A4FBA4;" not in bodies[-1]:
        raise ValueError("C1 call-site contract changed; review the guard")
    return "\n".join(bodies)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", default=shutil.which("clang++"))
    parser.add_argument("--output", type=Path, default=ROOT / "out/issue114/guard-test")
    args = parser.parse_args()
    if not args.compiler:
        parser.error("supply --compiler with clang++ (the generated bodies use Clang builtins)")
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    (output / "native_bodies.inc").write_text(retail_bodies(), encoding="utf-8")
    fixture = ROOT / "tools/tests/field_interaction"
    executable = output / ("check.exe" if os.name == "nt" else "check")
    command = [args.compiler, "-std=c++20", "-O0", "-I", str(fixture), "-I", str(output),
               str(fixture / "check.cpp"),
               str(ROOT / "LostOdysseyRecomp/patches/field_interaction.cpp"),
               "-o", str(executable)]
    if os.name == "nt" and "clang" in Path(args.compiler).name:
        command.append("-fuse-ld=lld")
    subprocess.run(command, cwd=ROOT, check=True, timeout=60)
    subprocess.run([str(executable)], cwd=ROOT, check=True, timeout=10)


if __name__ == "__main__":
    main()

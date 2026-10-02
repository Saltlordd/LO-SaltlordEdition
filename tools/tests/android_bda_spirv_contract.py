#!/usr/bin/env python3
"""Compile Android's production vertex-fetch prelude and validate its SPIR-V.

Requires a host C++ compiler, DXC and SPIRV-Tools. This is an opt-in compiler
check; it does not need Android assets or a full runtime build.
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


REPO = Path(__file__).resolve().parents[2]
EMITTER = REPO / "tools/tests/android_bda_shader_fixture.cpp"


def resolve_tool(value: str, launch_dir: Path) -> str:
    """Keep PATH commands intact; anchor explicit relative paths before changing cwd."""
    path = Path(value)
    if not path.is_absolute() and ("/" in value or "\\" in value):
        return str((launch_dir / path).resolve())
    return value


def run(command: list[str], *, cwd: Path, stdout=None) -> None:
    completed = subprocess.run(command, cwd=cwd, stdout=stdout, stderr=subprocess.PIPE,
                               text=stdout is None, check=False)
    if completed.returncode:
        if completed.stderr:
            sys.stderr.write(completed.stderr.decode(errors="replace")
                             if isinstance(completed.stderr, bytes) else completed.stderr)
        raise RuntimeError(f"command failed ({completed.returncode}): {' '.join(command)}")


def main() -> int:
    launch_dir = Path.cwd()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cxx", default=os.environ.get("CXX", "c++"))
    parser.add_argument("--dxc", default=os.environ.get("LO_DXC_PATH", "dxc"))
    parser.add_argument("--spirv-val", default="spirv-val")
    parser.add_argument("--spirv-dis", default="spirv-dis")
    parser.add_argument("--header", type=Path,
                        help="Use a header snapshot to check an earlier implementation")
    parser.add_argument("--output-dir", type=Path,
                        help="Retain generated HLSL and SPIR-V for inspection")
    args = parser.parse_args()
    args.cxx = resolve_tool(args.cxx, launch_dir)
    args.dxc = resolve_tool(args.dxc, launch_dir)
    args.spirv_val = resolve_tool(args.spirv_val, launch_dir)
    args.spirv_dis = resolve_tool(args.spirv_dis, launch_dir)

    output_parent = REPO / "out"
    output_parent.mkdir(exist_ok=True)
    temporary = None
    if args.output_dir:
        working = args.output_dir.resolve()
        working.mkdir(parents=True, exist_ok=True)
    else:
        temporary = tempfile.TemporaryDirectory(prefix="android-bda-contract-", dir=output_parent)
        working = Path(temporary.name)

    try:
        emitter = working / ("emit.exe" if os.name == "nt" else "emit")
        hlsl = working / "vertex.hlsl"
        spirv = working / "vertex.spv"
        assembly = working / "vertex.spvasm"
        includes = []
        if args.header:
            override = working / "override/LostOdysseyRecomp/gpu/shader"
            override.mkdir(parents=True, exist_ok=True)
            shutil.copy2(args.header.resolve(), override / "common_hlsl.h")
            shutil.copy2(REPO / "LostOdysseyRecomp/gpu/shader/vertex_fetch_contract.h",
                         override / "vertex_fetch_contract.h")
            includes = ["-I", str(working / "override")]
        run([args.cxx, "-std=c++20", "-DLO_SHADER_VERTEX_BDA=1", *includes, "-I", str(REPO),
             str(EMITTER), "-o", str(emitter)], cwd=working)
        with hlsl.open("wb") as output:
            run([str(emitter)], cwd=working, stdout=output)
        # Relative paths also work when Windows DXC is invoked from WSL.
        run([args.dxc, "-T", "vs_6_0", "-E", "main", "-HV", "2021",
             "-all-resources-bound", "-spirv", "-fspv-target-env=vulkan1.2",
             "-fvk-use-dx-layout", "-fvk-invert-y", "-O3", "-Qstrip_debug",
             "-Fo", spirv.name, hlsl.name], cwd=working)
        run([args.spirv_val, "--scalar-block-layout", "--relax-block-layout",
             "--target-env", "vulkan1.2", str(spirv)], cwd=working)
        with assembly.open("wb") as output:
            run([args.spirv_dis, str(spirv)], cwd=working, stdout=output)
        disassembly = assembly.read_text(encoding="utf-8")
        if "OpCapability PhysicalStorageBufferAddresses" not in disassembly:
            raise RuntimeError("fixture did not compile a physical device-address read")
        int64_types = set(re.findall(r"(?m)^[ \t]*(%\w+) = OpTypeInt 64 0$", disassembly))
        aligned_loads = [int(alignment) for t in int64_types
                         for alignment in re.findall(rf"OpLoad {re.escape(t)} %\w+ Aligned (\d+)\b",
                                                     disassembly)]
        if not any(alignment >= 8 for alignment in aligned_loads):
            raise RuntimeError("fixture did not retain an aligned 64-bit device-address load")
        print(f"Android vertex BDA SPIR-V contract PASS ({spirv.stat().st_size} bytes)")
        if args.output_dir:
            print(f"Artifacts: {working}")
        return 0
    finally:
        if temporary:
            temporary.cleanup()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError) as error:
        print(f"Android vertex BDA SPIR-V contract FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)

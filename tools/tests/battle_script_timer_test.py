"""Exercise the production battle script timer hook with extracted generated PPC.

Generated game code is copied only to ignored out/. Requires the local PPC
sources, SIMDe headers, and a C++20 compiler. No game assets are needed.
"""

import argparse
import os
from pathlib import Path
import re
import subprocess


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_GENERATED = ROOT / "LostOdysseyRecompLib/ppc"
DEFAULT_SIMDE = ROOT / "tools/XenonRecomp/thirdparty/simde"


def extract(generated_dir: Path, shard: int, address: str) -> str:
    path = generated_dir / f"ppc_recomp.{shard}.cpp"
    source = path.read_text(encoding="utf-8")
    signature = f"PPC_FUNC_IMPL(__imp__sub_{address})"
    match = re.search(r"^" + re.escape(signature) + r" \{.*?^\}", source, re.M | re.S)
    if not match:
        raise RuntimeError(f"Missing generated function {address} in {path}")
    # The instruction body stays verbatim; only its exported fixture name changes.
    return match.group().replace(signature, f"PPC_FUNC(sub_{address})", 1) + "\n"


def msvc_environment(vcvars: Path | None, output: Path) -> dict[str, str]:
    if os.name != "nt" or vcvars is None:
        return os.environ.copy()
    if not vcvars.is_file():
        raise FileNotFoundError(vcvars)
    launcher = output / "vcvars-env.cmd"
    launcher.write_text(
        f'@echo off\r\ncall "{vcvars}" >nul\r\nif errorlevel 1 exit /b 1\r\n'
        'set INCLUDE\r\nset LIB\r\nset LIBPATH\r\nset PATH\r\n',
        encoding="utf-8",
    )
    result = subprocess.run(
        ["cmd.exe", "/d", "/c", str(launcher)],
        check=True,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    env = os.environ.copy()
    for line in result.stdout.splitlines():
        if "=" in line and not line.startswith("="):
            key, value = line.split("=", 1)
            env[key] = value
    return env


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--generated-dir", type=Path, default=DEFAULT_GENERATED)
    parser.add_argument("--simde-dir", type=Path, default=DEFAULT_SIMDE)
    parser.add_argument("--compiler", default="clang++")
    parser.add_argument("--vcvars", type=Path, help="optional Visual Studio vcvars64.bat on Windows")
    parser.add_argument("--output", type=Path, default=ROOT / "out/battle-script-timer-test")
    args = parser.parse_args()

    needed = [(10, "8238AC40"), (210, "82E74250"), (10, "8238ACC8"), (156, "82A9BF40")]
    declarations = [
        '#include "ppc_context.h"\n',
        *(f"PPC_EXTERN_FUNC(sub_{address});\n" for _, address in needed),
        *(f"PPC_EXTERN_FUNC({name});\n" for name in (
            "__savegprlr_29", "__restgprlr_29", "sub_823A5058", "sub_82B07848",
            "sub_8238AE08", "sub_8238B4A8", "sub_8238B5A0", "sub_8238B850",
            "sub_8238B900", "sub_8238C708", "sub_8238BE38",
        )),
    ]
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / "stdafx.h").write_text('#include "ppc_context.h"\n', encoding="utf-8")
    fixture = args.output / "generated_fixture.cpp"
    fixture.write_text(
        "".join(declarations + [extract(args.generated_dir, shard, address) for shard, address in needed]),
        encoding="utf-8",
    )

    binary = args.output / ("battle_script_timer_test.exe" if os.name == "nt" else "battle_script_timer_test")
    compile_command = [
        args.compiler, "-std=c++20", "-O0", "-Wno-ignored-attributes", "-msse4.1",
        "-I" + str(args.output),
        "-I" + str(args.generated_dir),
        "-I" + str(args.simde_dir),
        str(fixture),
        str(ROOT / "tools/tests/battle_script_timer_test.cpp"),
        str(ROOT / "LostOdysseyRecomp/patches/battle_script_timer.cpp"),
        "-o", str(binary),
    ]
    environment = msvc_environment(args.vcvars, args.output)
    subprocess.run(compile_command, env=environment, check=True)
    enabled_env = environment.copy()
    enabled_env.pop("LO_BATTLE_SCRIPT_TIMER", None)
    subprocess.run([str(binary)], env=enabled_env, check=True)
    baseline_env = environment.copy()
    baseline_env["LO_BATTLE_SCRIPT_TIMER"] = "0"
    subprocess.run([str(binary), "--baseline"], env=baseline_env, check=True)


if __name__ == "__main__":
    main()

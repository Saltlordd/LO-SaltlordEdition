"""Compile and run production guest-allocation regressions without game assets.

The allocator declarations, method bodies, rounding helper and allocation imports
are extracted unchanged. Only guest storage, logging and the unordered-map
dependency are supplied by the fixture; guest endian wrappers use XenonUtils.
"""
import argparse
from pathlib import Path
import shutil
import subprocess


ROOT = Path(__file__).resolve().parents[2]
RUNTIME = ROOT / "LostOdysseyRecomp"


def definition(text, marker):
    start = text.index(marker)
    end = text.index("{", start) + 1
    depth = 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[start:end]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--compiler", default="g++")
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    compiler = shutil.which(args.compiler)
    if not compiler:
        raise RuntimeError(f"Compiler not found: {args.compiler}")

    memory = (RUNTIME / "kernel/memory.cpp").read_text(encoding="utf-8")
    header = (RUNTIME / "kernel/memory.h").read_text(encoding="utf-8")
    imports = (RUNTIME / "kernel/imports.cpp").read_text(encoding="utf-8")
    framework = (RUNTIME / "framework.h").read_text(encoding="utf-8")
    xbox = (ROOT / "tools/XenonRecomp/XenonUtils/xbox.h").read_text(encoding="utf-8")
    source = (Path(__file__).with_name("page_allocator_test.cpp")).read_text(encoding="utf-8")
    definitions = [
        definition(xbox, "template<typename T>\nstruct be") + ";",
        definition(framework, "template<typename T>\ninline T RoundUp("),
        definition(header, "struct PageAllocator") + ";",
        definition(memory, "uint32_t PageAllocator::Alloc("),
        definition(memory, "uint32_t PageAllocator::AllocationSize("),
        definition(memory, "bool PageAllocator::FindAllocation("),
        "PageAllocator g_pageAllocator;",
        "constexpr uint32_t X_MEM_LARGE_PAGES = 0x20000000;",
        definition(imports, "static uint32_t NtAllocateVirtualMemory("),
        definition(imports, "static uint32_t MmAllocatePhysicalMemoryEx("),
    ]
    cpp = output / "page_allocator_test.cpp"
    cpp.write_text(source.replace("// PRODUCTION_DEFINITIONS", "\n\n".join(definitions)), encoding="utf-8")
    executable = output / "page_allocator_test"
    command = [compiler, "-std=c++20", "-O1", "-g", "-fsanitize=address,undefined",
               "-fno-omit-frame-pointer", "-pthread", "-I", str(RUNTIME), "-I",
               str(ROOT / "tools/XenonRecomp/XenonUtils"), str(cpp), "-o", str(executable)]
    subprocess.run(command, check=True, timeout=120)
    result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=20)
    (output / "run.log").write_text(result.stdout + result.stderr, encoding="utf-8")
    print(result.stdout + result.stderr, end="")
    result.check_returncode()


if __name__ == "__main__":
    main()

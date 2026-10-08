#!/usr/bin/env python3
"""Compile the configured Android runtime without generating success-returning import stubs.
This audit does not link or execute the runtime. Configure the full ARM64 GPU
runtime first, then pass --build-dir out/android-full-runtime.
"""
import argparse
import concurrent.futures
import json
from pathlib import Path
import re
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--output', type=Path, default=ROOT / 'out/android-runtime-audit')
    parser.add_argument('--jobs', type=int, default=4)
    options = parser.parse_args()
    if options.jobs < 1:
        parser.error('--jobs must be positive')
    build = options.build_dir.resolve()
    output = options.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    # SDL's public headers and the build-revision header are generated even
    # for isolated audits. Build only these prerequisites, never import stubs.
    cache = (build / 'CMakeCache.txt').read_text()
    cmake = next(line.split('=', 1)[1] for line in cache.splitlines()
                 if line.startswith('CMAKE_COMMAND:INTERNAL='))
    subprocess.run([cmake, '--build', str(build), '--target', 'sdl_headers_copy',
                    'LoBuildRevision', '-j', str(options.jobs)], check=True)
    entries = []
    for entry in json.loads((build / 'compile_commands.json').read_text()):
        args = shlex.split(entry['command'])
        target_output = args[args.index('-o') + 1]
        if '/LostOdysseyRecomp.dir/' in target_output.replace('\\', '/'):
            if 'aarch64' not in entry['command'] or 'android' not in entry['command']:
                raise RuntimeError('Audit requires an Android ARM64 configuration')
            entries.append(entry)
    if not entries:
        raise RuntimeError('No configured runtime compile commands')
    pch_entry = next(e for e in entries if e['file'].endswith('cmake_pch.hxx.cxx'))
    pch = output / 'runtime.pch'

    def compile_source(entry):
        source = Path(entry['file'])
        name = str(source.relative_to(ROOT)).replace('/', '_')
        destination = pch if entry is pch_entry else output / (name + '.o')
        args = shlex.split(entry['command'])
        args[args.index('-o') + 1] = str(destination)
        # These sources contain no C++ modules; Ninja's generated response map
        # is unnecessary for independent translation-unit compilation.
        args = [str(pch) if '/LostOdysseyRecomp.dir/cmake_pch.hxx.pch' in arg else arg
                for arg in args if not (arg.startswith('@') and arg.endswith('.modmap'))]
        log = output / (name + '.log')
        with log.open('w') as stream:
            result = subprocess.run(args, cwd=entry['directory'], stdout=stream, stderr=subprocess.STDOUT)
        record = {'source': str(source.relative_to(ROOT)), 'returncode': result.returncode,
                  'object': str(destination), 'log': str(log)}
        print(('PASS ' if result.returncode == 0 else 'FAIL ') + record['source'], flush=True)
        return record

    pch_result = compile_source(pch_entry)
    results = [pch_result]
    if pch_result['returncode'] == 0:
        work = [e for e in entries if e is not pch_entry and not e['file'].endswith('imports_stubs.cpp')]
        with concurrent.futures.ThreadPoolExecutor(max_workers=options.jobs) as pool:
            results.extend(pool.map(compile_source, work))
    sym = ROOT / 'LostOdysseyRecompLib/private/image_disc1.bin.sym'
    imports = {parts[3] for line in sym.read_text().splitlines()
               if len(parts := line.split()) >= 4 and parts[3].startswith('__imp__')}
    pattern = re.compile(r'(?:GUEST_FUNCTION_HOOK|GUEST_FUNCTION_STUB|PPC_FUNC)\s*\(\s*(__imp__\w+)')
    definitions = set()
    for source in (ROOT / 'LostOdysseyRecomp').rglob('*'):
        if source.suffix not in ('.cpp', '.inc'):
            continue
        if source.name != 'imports_stubs.cpp':
            definitions.update(pattern.findall(source.read_text(errors='replace')))
    report = {'compilation': results, 'imports': len(imports),
              'existing_import_definitions': len(imports & definitions),
              'missing_imports': sorted(imports - definitions),
              'linked': False, 'guest_executed': False}
    (output / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
    failures = sum(item['returncode'] != 0 for item in results)
    print(f'COMPLETE: {len(results)} units, {failures} compile failures, '
          f'{len(report["missing_imports"])} imports without definitions', flush=True)
    return 1 if failures or report['missing_imports'] else 0

if __name__ == '__main__':
    raise SystemExit(main())

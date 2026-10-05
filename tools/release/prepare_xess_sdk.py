#!/usr/bin/env python3
"""Verify and extract the pinned official Intel XeSS SDK release archive.

Only the headers, the three runtime DLLs and the license are extracted; the
release package ships the DLLs and the license.
"""
import argparse
import hashlib
from pathlib import Path
from zipfile import ZipFile


RELEASE = 'v3.0.2'
ASSET = 'XeSS_SDK_3.0.2.zip'
ASSET_SIZE = 76839749
ASSET_SHA256 = '88b8a373f30e33f3558a77a93e634f11b8132fc3047ea1a8edeead32b8471990'
RUNTIME_FILES = ('libxess.dll', 'libxess_fg.dll', 'libxell.dll')
HEADER_DIRS = ('inc/xess/', 'inc/xess_fg/', 'inc/xell/')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', type=Path, required=True)
    parser.add_argument('--dest', type=Path, required=True)
    args = parser.parse_args()

    archive = args.archive.resolve()
    if archive.name != ASSET or archive.stat().st_size != ASSET_SIZE:
        raise SystemExit('Unexpected XeSS SDK archive name or size.')
    with archive.open('rb') as source:
        digest = hashlib.file_digest(source, 'sha256').hexdigest()
    if digest != ASSET_SHA256:
        raise SystemExit('XeSS SDK archive digest differs from pinned release.')

    with ZipFile(archive) as source:
        names = set(source.namelist())
        selected = [name for name in names if name.startswith(HEADER_DIRS) and not name.endswith('/')]
        selected += [f'bin/{name}' for name in RUNTIME_FILES]
        selected.append('LICENSE.txt')
        missing = [name for name in selected if name not in names]
        if not selected or missing:
            raise SystemExit(f'XeSS SDK archive is incomplete: {missing}')
        dest = args.dest.resolve()
        for name in sorted(selected):
            target = dest.joinpath(*Path(name).parts)
            target.parent.mkdir(parents=True, exist_ok=True)
            with source.open(name) as content, target.open('wb') as output:
                while block := content.read(1024 * 1024):
                    output.write(block)
    print(f'Prepared verified Intel XeSS {RELEASE} SDK at {dest}')


if __name__ == '__main__':
    main()

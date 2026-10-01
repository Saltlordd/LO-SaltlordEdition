#!/usr/bin/env python3
"""Convert Xenia save folders into the runtime's save layout.

Xenia keeps each save as <...>/00000001/<name>/ (save.bin, __thumbnail.png).
The runtime expects save/<name>/ with a .lo-content descriptor (the guest
XCONTENT_DATA, see kernel/xam.cpp) and an optional .lo-thumbnail.png.

Usage:
  python3 tools/import_xenia_saves.py <source> <save-root> [--replace] [--dry-run]

<source> is a folder containing save folders, or one level above them (for
example the 00000001 folder or its parent). Existing slots are kept unless
--replace is given. Timestamps are preserved, so "last saved game" ordering
stays the same.
"""

from __future__ import annotations

import argparse
import shutil
import struct
import sys
from pathlib import Path

XCONTENTTYPE_SAVEDATA = 1
DEVICE_ID = 1               # XamMakeContent's device
MAX_DISPLAY_NAME = 128      # UTF-16 code units
MAX_FILE_NAME = 42
CONTENT_DATA_SIZE = 308     # sizeof(XCONTENT_DATA), including tail padding
XENIA_THUMBNAIL = "__thumbnail.png"
RUNTIME_THUMBNAIL = ".lo-thumbnail.png"


def content_data(name: str) -> bytes:
    """Big-endian XCONTENT_DATA; the display name repeats the folder name."""
    encoded = name.encode("ascii")
    if not encoded or len(encoded) >= MAX_FILE_NAME or any(c in name for c in "/\\:"):
        raise ValueError(f"unsupported save name {name!r}")
    display = name.encode("utf-16-be")[:(MAX_DISPLAY_NAME - 1) * 2]
    data = struct.pack(">II", DEVICE_ID, XCONTENTTYPE_SAVEDATA)
    data += display.ljust(MAX_DISPLAY_NAME * 2, b"\0")
    data += encoded.ljust(MAX_FILE_NAME, b"\0")
    return data.ljust(CONTENT_DATA_SIZE, b"\0")


def find_saves(source: Path) -> list[Path]:
    def is_save(p: Path) -> bool:
        return p.is_dir() and (p / "save.bin").is_file()
    for root in (source, *(p for p in sorted(source.iterdir()) if p.is_dir())):
        saves = [p for p in sorted(root.iterdir()) if is_save(p)]
        if saves:
            return saves
    return []


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("source", type=Path)
    ap.add_argument("save_root", type=Path)
    ap.add_argument("--replace", action="store_true", help="replace slots that already exist")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    saves = find_saves(args.source)
    if not saves:
        print(f"error: no save folders (with save.bin) under {args.source}", file=sys.stderr)
        return 2
    args.save_root.mkdir(parents=True, exist_ok=True)
    for save in saves:
        target = args.save_root / save.name
        if target.exists() and not args.replace:
            print(f"skip {save.name}: {target} exists (use --replace)")
            continue
        print(f"{save.name} -> {target}")
        if args.dry_run:
            continue
        staging = args.save_root / f".import-{save.name}"
        if staging.exists():
            shutil.rmtree(staging)
        shutil.copytree(save, staging, copy_function=shutil.copy2,
                        ignore=shutil.ignore_patterns(".DS_Store"))
        thumbnail = staging / XENIA_THUMBNAIL
        if thumbnail.exists():
            thumbnail.rename(staging / RUNTIME_THUMBNAIL)
        (staging / ".lo-content").write_bytes(content_data(save.name))
        if target.exists():
            shutil.rmtree(target)
        staging.rename(target)
        shutil.copystat(save, target)
    return 0


if __name__ == "__main__":
    sys.exit(main())

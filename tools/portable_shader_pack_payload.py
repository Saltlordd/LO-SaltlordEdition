"""Packages carry no shader pack: the game downloads the one for its renderer
at startup (docs/PORTABLE_SHADER_PACK.md#startup-download). They carry the
license of the zstd reader that opens it."""
from __future__ import annotations
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[1]


def stage_shader_pack_license(licenses: Path) -> None:
    licenses.mkdir(parents=True, exist_ok=True)
    shutil.copy2(ROOT / "thirdparty/zstd-LICENSE.txt", licenses / "zstd-LICENSE.txt")

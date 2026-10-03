"""Publish distribution shader packs for the runtime's startup download.

Packs are assets of the `shader-packs` prerelease, named by renderer and
contract. index.json on the same release lists them; a runtime computes its
contract, finds its entry and downloads the file beside the index. Entries for
other contracts stay, so older runtimes keep finding their packs.

  python tools/release/publish_shader_packs.py --tool <LoShaderPackTool> \
      --image LostOdysseyRecompLib/private/image_disc1.bin PACK... [--publish]

Each pack must carry this runtime's contract for one renderer and pass
`LoShaderPackTool verify-runtime`. Without --publish the script only stages the
renamed packs and the merged index in out/shader-packs. With --publish it
uploads the packs first and the index last.

  python tools/release/publish_shader_packs.py --tool <LoShaderPackTool> \
      --image <image> --check

--check only reports whether the published index lists both contracts of this
runtime; the release workflow runs it before publishing a version. The vulkan
pack also serves Android and Metal on macOS.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import urllib.error
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
REPOSITORY = "freefrank/LostOdysseyRecomp"
RELEASE_TAG = "shader-packs"
RENDERERS = {
    # renderer: (asset stem, extension, pack format)
    "vulkan": ("portable_vk", ".lospv", "spirv"),
    "d3d12": ("portable_dx12", ".lospd", "dxil"),
}
MAGIC = {b"LOSPVPK1": "spirv", b"LOSPDPK1": "dxil"}
NOTES = (
    "Precompiled shaders that the game downloads at startup, one file per renderer "
    "and shader contract. index.json lists them. Version releases no longer include "
    "these files; this prerelease is not a game version."
)


def read_header(path: Path) -> tuple[str, str]:
    """(format, contract hex) from the pack header: magic, schema, size, contract."""
    with open(path, "rb") as stream:
        head = stream.read(48)
    if len(head) < 48 or head[:8] not in MAGIC:
        raise SystemExit(f"{path}: not a portable shader pack")
    return MAGIC[head[:8]], head[16:48].hex()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with open(path, "rb") as stream:
        while chunk := stream.read(1 << 20):
            digest.update(chunk)
    return digest.hexdigest()


def asset_name(renderer: str, contract: str) -> str:
    stem, extension, _ = RENDERERS[renderer]
    return f"{stem}-{contract[:16]}{extension}"


def merge_index(existing: dict | None, entries: list[dict]) -> dict:
    """Replace entries for the same renderer and contract; keep all others."""
    packs = list(existing.get("packs", [])) if existing else []
    replaced = {(entry["renderer"], entry["contract"]) for entry in entries}
    packs = [pack for pack in packs if (pack.get("renderer"), pack.get("contract")) not in replaced]
    packs.extend(entries)
    packs.sort(key=lambda pack: (pack["renderer"], pack["contract"]))
    return {"schema": 1, "packs": packs}


def missing_contracts(index: dict | None, contracts: dict[str, str]) -> list[str]:
    listed = {(pack.get("renderer"), pack.get("contract")) for pack in (index or {}).get("packs", [])}
    return [renderer for renderer, contract in sorted(contracts.items()) if (renderer, contract) not in listed]


def runtime_contracts(tool: Path, image: Path) -> dict[str, str]:
    result = subprocess.run([str(tool), "contract", str(image)], check=True, capture_output=True, text=True)
    contracts = json.loads(result.stdout)
    if set(contracts) != set(RENDERERS) or len(set(contracts.values())) != len(RENDERERS):
        raise SystemExit(f"unexpected contract output: {result.stdout}")
    return contracts


def fetch_index(repository: str) -> dict | None:
    url = f"https://github.com/{repository}/releases/download/{RELEASE_TAG}/index.json"
    try:
        with urllib.request.urlopen(url, timeout=30) as response:
            return json.loads(response.read().decode("utf-8"))
    except urllib.error.HTTPError as error:
        if error.code == 404:
            return None
        raise


def gh(*arguments: str, capture: bool = False) -> str:
    result = subprocess.run(["gh", *arguments], check=True, text=True,
                            capture_output=capture, env=dict(os.environ, GH_PROMPT_DISABLED="1"))
    return result.stdout if capture else ""


def stage(pack: Path, destination: Path) -> None:
    destination.unlink(missing_ok=True)
    try:
        os.link(pack, destination)
    except OSError:
        shutil.copy2(pack, destination)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--tool", type=Path, required=True, help="LoShaderPackTool built from this tree")
    parser.add_argument("--image", type=Path, required=True, help="xexdump image of disc 1 (image_disc1.bin)")
    parser.add_argument("--repo", default=REPOSITORY)
    parser.add_argument("--output", type=Path, default=ROOT / "out/shader-packs")
    parser.add_argument("--publish", action="store_true", help="upload the packs and the index")
    parser.add_argument("--check", action="store_true", help="only check that the published index covers this runtime")
    parser.add_argument("packs", nargs="*", type=Path)
    args = parser.parse_args()

    contracts = runtime_contracts(args.tool, args.image)
    for renderer, contract in sorted(contracts.items()):
        print(f"{renderer}: contract {contract}")
    if args.check:
        missing = missing_contracts(fetch_index(args.repo), contracts)
        if missing:
            print(f"error: the {RELEASE_TAG} index has no pack for: {', '.join(missing)}", file=sys.stderr)
            return 1
        print(f"The {RELEASE_TAG} index lists packs for all renderers of this runtime.")
        return 0
    if not args.packs:
        parser.error("no packs given")

    renderer_of = {contract: renderer for renderer, contract in contracts.items()}
    args.output.mkdir(parents=True, exist_ok=True)
    entries, staged = [], []
    for pack in args.packs:
        pack_format, contract = read_header(pack)
        renderer = renderer_of.get(contract)
        if renderer is None:
            raise SystemExit(f"{pack}: contract {contract[:16]} belongs to no renderer of this runtime")
        if pack_format != RENDERERS[renderer][2]:
            raise SystemExit(f"{pack}: {pack_format} pack carries the {renderer} contract")
        if any(entry["renderer"] == renderer for entry in entries):
            raise SystemExit(f"{pack}: a second {renderer} pack")
        subprocess.run([str(args.tool), "verify-runtime", str(pack), str(args.image)], check=True, capture_output=True)
        name = asset_name(renderer, contract)
        destination = args.output / name
        stage(pack, destination)
        entry = {"renderer": renderer, "contract": contract, "file": name,
                 "size": destination.stat().st_size, "sha256": sha256_file(destination)}
        print(f"{renderer}: {name} {entry['size']} bytes sha256 {entry['sha256']}")
        entries.append(entry)
        staged.append(destination)

    index = merge_index(fetch_index(args.repo), entries)
    index_path = args.output / "index.json"
    index_path.write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    print(f"index: {len(index['packs'])} packs -> {index_path}")
    if not args.publish:
        print("Staged only; rerun with --publish to upload.")
        return 0

    try:
        gh("release", "view", RELEASE_TAG, "--repo", args.repo, capture=True)
    except subprocess.CalledProcessError:
        # A prerelease is never the repository's latest release, which the updater reads.
        gh("release", "create", RELEASE_TAG, "--repo", args.repo, "--prerelease", "--latest=false",
           "--title", "Shader packs", "--notes", NOTES)
    gh("release", "upload", RELEASE_TAG, "--repo", args.repo, "--clobber", *map(str, staged))
    gh("release", "upload", RELEASE_TAG, "--repo", args.repo, "--clobber", str(index_path))
    latest = gh("api", f"repos/{args.repo}/releases/latest", "--jq", ".tag_name", capture=True).strip()
    if latest == RELEASE_TAG:
        raise SystemExit(f"{RELEASE_TAG} became the latest release; the updater would reject it")
    missing = missing_contracts(fetch_index(args.repo), contracts)
    print(f"published; latest release is still {latest}; "
          + (f"index lacks {', '.join(missing)}" if missing else "index covers all renderers"))
    return 0


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
"""Export original FMV streams from an inventory, without transcoding or modifying game data."""
from __future__ import annotations

import argparse
from contextlib import closing
import csv
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import shutil
import sqlite3
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from tools.asset_inventory.inventory import open_readonly, require

ASF_HEADER = bytes.fromhex("3026b2758e66cf11a6d900aa0062ce6c")
MAX_CPX = 128 * 1024 * 1024


def copy_extent(source: Path, offset: int, length: int, destination: Path | None) -> str:
    require(type(offset) is int and type(length) is int and offset >= 0 and length > 0,
            "invalid archive extent")
    require(offset + length <= source.stat().st_size, f"extent outside archive: {source}")
    digest = hashlib.sha256()
    output = destination.open("xb") if destination else None
    try:
        with source.open("rb") as stream:
            stream.seek(offset)
            remaining = length
            while remaining:
                block = stream.read(min(4 * 1024 * 1024, remaining))
                require(bool(block), f"archive short read: {source}")
                remaining -= len(block)
                digest.update(block)
                if output:
                    output.write(block)
    finally:
        if output:
            output.close()
    return digest.hexdigest()


def run_tool(command, **kwargs):
    try:
        return subprocess.run(command, capture_output=True, timeout=60, check=True, **kwargs)
    except subprocess.CalledProcessError as exc:
        detail = (exc.stderr or b"").decode("utf-8", errors="replace")[:4096].strip()
        raise ValueError(f"{Path(command[0]).name} failed ({exc.returncode}): {detail}") from exc


def probe_movie(path: Path, executable: str) -> dict:
    result = run_tool([executable, "-v", "error", "-show_format", "-show_streams", "-of", "json", str(path)])
    info = json.loads(result.stdout)
    require(info.get("format", {}).get("format_name") == "asf", f"not an ASF movie: {path.name}")
    require(any(s.get("codec_type") == "video" for s in info.get("streams", [])),
            f"movie has no video stream: {path.name}")
    info["format"].pop("filename", None)
    return info


def export_fmv(args) -> dict:
    output = args.output.resolve()
    probe = shutil.which(str(args.ffprobe))
    require(probe is not None, "ffprobe is required; install FFmpeg or supply --ffprobe")
    with closing(open_readonly(args.db)) as db:
        db.execute("BEGIN")
        meta = {row[0]: json.loads(row[1]) for row in db.execute("SELECT key,value FROM metadata")}
        require(type(meta.get("schema_version")) is int and meta["schema_version"] == 1, "unsupported catalog schema")
        require(meta.get("complete") is True, "catalog is incomplete")
        recorded_root = Path(meta["game_root"]).resolve()
        game = (args.game or recorded_root).resolve()
        require(game.is_dir(), "game root does not exist; use --game if it moved")
        require(not output.is_relative_to(game) and not output.is_relative_to(recorded_root),
                "output must be outside game data")
        require(not output.exists(), "output already exists; choose a new directory")
        # Include ASF signatures outside the usual movie directory as well as .wmv names.
        entries = [dict(row) for row in db.execute("""SELECT f.*,p.encoding,p.decoded_size,p.status
            FROM files f JOIN payloads p USING(sha256)
            WHERE lower(f.extension)='.wmv' OR p.magic='3026b275'
            ORDER BY f.path,f.sha256,f.disc,f.archive,f.offset""")]
        require(bool(entries), "catalog contains no WMV/ASF movies")
        require(all(r["status"] != "error" and r["encoding"] in ("raw", "cpx") for r in entries),
                "movie inventory contains failed or unsupported payloads")
        decoder = args.decoder.resolve() if args.decoder else None
        if any(r["encoding"] == "cpx" for r in entries):
            require(decoder is not None and decoder.is_file(), "CPX movies require --decoder built with --decode-cpx support")
        recorded_sources = {r["path"]: dict(r) for r in db.execute("SELECT * FROM sources")}

    source_stats = {}
    direct_disc = (game / "LO.fpi").is_file()
    for entry in entries:
        source = (game / entry["archive"] if direct_disc else game / entry["disc"] / entry["archive"]).resolve()
        require(source.is_relative_to(game), "archive path escapes game root")
        relative = source.relative_to(game).as_posix()
        require(relative in recorded_sources, f"archive absent from catalog sources: {relative}")
        stat = source.stat()
        recorded = recorded_sources[relative]
        require(stat.st_size == recorded["size"], f"source size changed: {relative}")
        source_stats[source] = (stat.st_size, stat.st_mtime_ns)
        require(re.fullmatch(r"[0-9a-f]{64}", entry["sha256"]) is not None, "invalid stored SHA-256")
        require(type(entry["offset"]) is int and type(entry["length"]) is int
                and entry["offset"] >= 0 and entry["length"] > 0
                and entry["offset"] + entry["length"] <= stat.st_size, "invalid movie extent")
        entry["source_file"] = source

    output.mkdir(parents=True)
    movies = output / "movies"
    movies.mkdir()
    manifest = {"schema_version": 1, "complete": False,
                "started_utc": datetime.now(timezone.utc).isoformat(),
                "catalog": str(args.db.resolve()), "game_root": str(game),
                "scope": "all .wmv entries and raw ASF signatures in the supplied catalog",
                "transcoded": False, "files": [], "occurrences": []}
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    by_stored, by_decoded = {}, {}
    try:
        for number, entry in enumerate(entries, 1):
            stored = entry["sha256"]
            partial = output / f"{stored}.partial"
            first = stored not in by_stored
            digest = copy_extent(entry["source_file"], entry["offset"], entry["length"], partial if first else None)
            require(digest == stored, f"source content changed: {entry['disc']}:{entry['path']}")
            if first:
                if entry["encoding"] == "cpx":
                    require(entry["length"] <= MAX_CPX, "compressed movie exceeds CPX input limit")
                    result = run_tool([str(decoder), "--decode-cpx"], input=partial.read_bytes())
                    require(len(result.stdout) == entry["decoded_size"], "decoded movie size differs from inventory")
                    require(result.stdout.startswith(ASF_HEADER), "decoded CPX movie is not ASF")
                    partial.write_bytes(result.stdout)
                with partial.open("rb") as file:
                    require(file.read(16) == ASF_HEADER, f"invalid ASF header: {entry['path']}")
                    file.seek(0)
                    decoded_hash = hashlib.sha256()
                    while block := file.read(4 * 1024 * 1024):
                        decoded_hash.update(block)
                decoded = decoded_hash.hexdigest()
                if decoded in by_decoded:
                    partial.unlink()  # Only the temporary file created by this export.
                    item = by_decoded[decoded]
                else:
                    info = probe_movie(partial, probe)
                    stem = re.sub(r"[^A-Za-z0-9._-]", "_", PurePosixPath(entry["path"]).stem)[:80] or "movie"
                    destination = movies / f"{stem}--{decoded[:12]}.wmv"
                    require(not destination.exists(), "output filename hash collision")
                    size = partial.stat().st_size
                    partial.rename(destination)
                    item = {"output": destination.relative_to(output).as_posix(), "sha256": decoded,
                            "size": size, "probe": info}
                    by_decoded[decoded] = item
                    manifest["files"].append(item)
                by_stored[stored] = item
            item = by_stored[stored]
            manifest["occurrences"].append({"disc": entry["disc"], "archive": entry["archive"],
                "path": entry["path"], "offset": entry["offset"], "stored_size": entry["length"],
                "stored_sha256": stored, "encoding": entry["encoding"],
                "output": item["output"], "decoded_sha256": item["sha256"], "decoded_size": item["size"]})
            print(f"[{number}/{len(entries)}] {entry['disc']}:{entry['path']}", flush=True)
        for source, original in source_stats.items():
            stat = source.stat()
            require((stat.st_size, stat.st_mtime_ns) == original, f"source changed during export: {source}")
        manifest["summary"] = {"occurrences": len(entries), "unique_stored": len(by_stored),
            "unique_movies": len(by_decoded), "output_bytes": sum(item["size"] for item in by_decoded.values()),
            "source_hashes_verified": len(entries), "ffprobe_passed": len(by_decoded),
            "duration_seconds": sum(float(item["probe"]["format"].get("duration", 0)) for item in by_decoded.values())}
        manifest["complete"] = True
        manifest["finished_utc"] = datetime.now(timezone.utc).isoformat()
    except BaseException as exc:
        manifest["error"] = str(exc)
        raise
    finally:
        (output / "manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        with (output / "manifest.csv").open("w", encoding="utf-8-sig", newline="") as file:
            fields = ["disc", "archive", "path", "offset", "stored_size", "stored_sha256", "encoding",
                      "output", "decoded_sha256", "decoded_size"]
            writer = csv.DictWriter(file, fieldnames=fields)
            writer.writeheader()
            writer.writerows(manifest["occurrences"])
    return manifest


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--db", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True, help="new directory outside game data")
    parser.add_argument("--game", type=Path, help="override the catalog's game root if it moved")
    parser.add_argument("--decoder", type=Path, help="native asset_decoder with --decode-cpx support")
    parser.add_argument("--ffprobe", default="ffprobe", help="FFmpeg ffprobe executable")
    args = parser.parse_args(argv)
    try:
        result = export_fmv(args)
        print(json.dumps(result["summary"], indent=2))
        return 0
    except (ValueError, OSError, sqlite3.Error, subprocess.SubprocessError) as exc:
        print(f"export_fmv: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())

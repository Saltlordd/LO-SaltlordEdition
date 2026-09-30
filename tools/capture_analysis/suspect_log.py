#!/usr/bin/env python3
"""Summarize runtime "temporal suspect" lines without an F1 capture.

The renderer logs each unmapped scene-camera VS/PS pair once when it draws into
the main scene depth under active jitter and either writes depth or repeats an
earlier jittered depth writer's geometry (the #67/#102 class). This reads those
lines from ordinary runtime logs, attaches the most recent "current map" line,
and can emit the same compact C++ fixture as export_jitter_fixture.py for pairs
whose depth companion uses slot 4. It never edits the production map.
"""

import argparse
import json
import sys
from pathlib import Path
import re

try:
    from .export_jitter_fixture import IDENTIFIER, render_header
except ImportError:  # Direct script invocation.
    from export_jitter_fixture import IDENTIFIER, render_header


MAP = re.compile(r"current map available=(\w+) id=(\d+) name=(.*) package=(\S*)\s*$")
SUSPECT = re.compile(r"temporal suspect: (.*)$")
BANKS = re.compile(r"temporal suspect banks: (.*)$")
TIME = re.compile(r"^\[\s*([\d.]+)")
BANK_LENGTHS = {"material": 64, "late": 8, "camera": 16, "pixel": 64, "companion_world": 16, "companion_vp": 16}
INTEGERS = ("frame", "draw", "camera_slot", "position_kind", "evidence_slot", "zfunc", "index_count",
            "base_vertex", "companion_slot", "companion_draw")


def fields(text):
    return dict(item.split("=", 1) for item in text.split() if "=" in item)


def words(text, count):
    if text == "-":
        return None
    values = [int(value, 16) for value in text.split(",")]
    if len(values) != count:
        raise ValueError(f"bank has {len(values)} words, expected {count}")
    return values


def parse_log(path, strict=False):
    """Crash logs are often truncated: malformed lines are skipped with a warning."""
    rows, current_map, pending = [], None, {}
    for number, line in enumerate(path.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
        try:
            if match := MAP.search(line):
                current_map = {"available": match[1] == "true", "id": int(match[2]),
                               "name": match[3], "package": match[4]}
            elif match := BANKS.search(line):
                data = fields(match[1])
                row = pending.pop((data.get("vs"), data.get("ps")), None)
                if row is None:
                    raise ValueError("banks line without a preceding suspect line")
                row["banks"] = {name: words(data.get(name, "-"), count) for name, count in BANK_LENGTHS.items()}
            elif match := SUSPECT.search(line):
                data = fields(match[1])
                time = TIME.search(line)
                row = {"log": str(path), "line": number, "time": float(time[1]) if time else None,
                       "map": current_map, "banks": None, **data}
                for key in INTEGERS:
                    row[key] = int(row[key])
                for key in ("zwrite", "same_world", "same_camera"):
                    row[key] = row[key] == "true"
                pending[(row["vs"], row["ps"])] = row
                rows.append(row)
        except (KeyError, ValueError) as exc:
            if strict:
                raise ValueError(f"{path}:{number}: {exc}") from exc
            print(f"warning: {path}:{number}: skipped ({exc})", file=sys.stderr)
    return rows


def fixture_row(row):
    """Return a render_header row when the log carries a slot-4 depth companion."""
    banks = row["banks"]
    if not banks or row["companion_slot"] != 4 or not (row["same_world"] and row["same_camera"]):
        return None
    if not 0 <= row["camera_slot"] <= 12 or banks["companion_world"] is None:
        return None
    return {"vs": row["vs"], "ps": row["ps"], "depth_vs": row["companion_vs"], "slot": row["camera_slot"],
            "draw": row["draw"], "depth_draw": row["companion_draw"],
            "vertex": banks["material"], "vertex_late": banks["late"],
            "depth": banks["companion_world"] + banks["companion_vp"], "pixel": banks["pixel"]}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--log", required=True, action="append", type=Path, help="runtime log; repeat for several")
    parser.add_argument("--output", required=True, type=Path, help="new JSON summary")
    parser.add_argument("--fixture", type=Path, help="optional new C++ fixture for slot-4 companion pairs")
    parser.add_argument("--namespace", help="fixture namespace (required with --fixture)")
    parser.add_argument("--strict", action="store_true", help="fail on malformed suspect lines instead of skipping")
    args = parser.parse_args(argv)
    for output in filter(None, (args.output, args.fixture)):
        if output.exists():
            parser.error(f"{output} already exists")
    if args.fixture and not (args.namespace and IDENTIFIER.fullmatch(args.namespace)):
        parser.error("--fixture needs a C++ identifier --namespace")
    try:
        rows = [row for path in args.log for row in parse_log(path, args.strict)]
        fixtures = [row for row in map(fixture_row, rows) if row]
        if args.fixture:
            if not fixtures:
                raise ValueError("no suspect carries a slot-4 depth companion with matching world and camera")
            sources = ", ".join(sorted({Path(row["log"]).name for row in rows}))
            header = render_header(fixtures, args.namespace, f"runtime suspect lines in {sources}").replace(
                " before the first resolve.", " at the first reported draw.")
    except (OSError, ValueError, KeyError) as exc:
        parser.error(str(exc))
    summary = {"schema": "lostodyssey.temporal-suspects.v1", "logs": [str(path) for path in args.log],
               "suspects": len(rows), "fixture_rows": len(fixtures),
               "rows": [{k: v for k, v in row.items() if k != "banks"} | {"has_banks": bool(row["banks"])} for row in rows]}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(summary, indent=1), encoding="utf-8")
    if args.fixture:
        args.fixture.parent.mkdir(parents=True, exist_ok=True)
        args.fixture.write_text(header, encoding="utf-8")
    for row in rows:
        place = row["map"]["package"] if row["map"] else "unknown map"
        print(f"{row['kind']} vs={row['vs']} ps={row['ps']} slot={row['camera_slot']} "
              f"companion={row['companion_vs']}/{row['companion_slot']} map={place}")


if __name__ == "__main__":
    main()

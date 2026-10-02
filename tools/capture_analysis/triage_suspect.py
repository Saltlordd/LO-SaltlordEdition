#!/usr/bin/env python3
"""Review packet for runtime "temporal suspect" pairs in one pass.

Joins the renderer's suspect lines (suspect_log.py), optional F1 captures
(jitter_candidates.py: exact scene VP, depth companions, recorded jitter
state), the static shader audits (audit_vs.py: oPos slot and other outputs
that carry the position matrix; audit_ps.py: which components of those outputs
the PS reads, texture coordinate sources) and the current production map. For
each pair it writes the evidence, a suggested action with its reasons, and the
fixture command and code snippets that action would need.

It never edits the production map. A "map" suggestion is a review packet: the
reviewer still checks the cited HLSL lines and the scene before mapping.
"""

import argparse
import json
import math
import re
import shutil
import struct
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parent / "shader_analysis"))

import audit_ps  # noqa: E402
import audit_vs  # noqa: E402
import jitter_candidates  # noqa: E402
import suspect_log  # noqa: E402
import trace  # noqa: E402

SKY_PAIR = re.compile(r"\{\s*0x([0-9a-fA-F]{16})ull\s*,\s*0x([0-9a-fA-F]{16})ull\s*,\s*(true|false)\s*\}")
DECLARATION = re.compile(r"^(?:float[234]?|int|uint|bool|CubeMapData)\s+\w+\s*=\s*[^;]*;$")
DEBUG_COPY = re.compile(r"^xeDbgTex\s*=\s*r\d+\s*;$")
GUEST_REGISTER = re.compile(r"\b(?:r\d+|xePV|ps)\b")
SCREEN_POSITION = re.compile(r"\biPos\b")


def mapping_state(header: Path) -> tuple[dict[str, int], dict[tuple[str, str], bool]]:
    slots = jitter_candidates.position_slots(header)
    text = header.read_text(encoding="utf-8")
    pairs = {(vs.lower(), ps.lower()): fallback == "true" for vs, ps, fallback in SKY_PAIR.findall(text)}
    return slots, pairs


def review_decisions(directory: Path) -> dict[str, dict]:
    """Latest reviewed decision per VS from the review manifests; a later
    manifest (by name) overrides an earlier one."""
    decisions = {}
    if not directory.is_dir():
        return decisions
    for path in sorted(directory.glob("*.json")):
        data = json.loads(path.read_text(encoding="utf-8"))
        for item in data.get("candidates", []):
            if isinstance(item, dict) and "vs" in item and "decision" in item:
                decisions[item["vs"].lower()] = {"decision": item["decision"], "reason": item.get("reason", ""),
                                                 "manifest": path.name}
    return decisions


def finite_words(values) -> bool:
    return values is not None and all(math.isfinite(struct.unpack(">f", struct.pack(">I", v))[0]) for v in values)


def capture_evidence(paths: list[Path], mapping: dict[str, int], wanted: set[tuple[str, str]]):
    """Draws of the wanted pairs in each capture frame, plus their HLSL text."""
    draws, hlsl = {}, {}
    for path in paths:
        with trace.load_capture(path) as capture:
            for directory in capture.frame_names():
                frame = jitter_candidates.analyze_frame(capture, directory, mapping)
                for candidate in frame["candidates"]:
                    for draw in candidate["draws"]:
                        key = (candidate["vs"], (draw.get("ps") or "").lower())
                        if key not in wanted:
                            continue
                        draws.setdefault(key, []).append({
                            "capture": str(path), "frame": frame["frame"], "directory": directory,
                            "draw": draw["draw"], "vp_slot": candidate["candidate_slot"],
                            "depth_draws": draw["matching_depth_draws"],
                            "geometry_depth_draws": [{"draw": d["draw"], "vs": d["vs"]}
                                                     for d in draw["matching_geometry_depth_draws"]],
                            "runtime_jitter": draw.get("runtime_jitter"),
                            "texture_bindings": draw.get("texture_bindings", [])})
            for vs, ps in wanted:
                for shader in (vs, ps):
                    name = f"shaders/{shader}.hlsl"
                    if shader not in hlsl and name in capture.names:
                        hlsl[shader] = capture.read_text(name)
    return draws, hlsl


def review_shaders(vs_text: str | None, ps_text: str | None, scratch: Path) -> dict:
    result = {"vs": None, "ps": None}
    if vs_text:
        vs = audit_vs.analyze(vs_text)
        result["vs"] = {key: vs[key] for key in ("classification", "slots", "vp_outputs", "position_write_lines")}
        result["vs"]["matrix_lines"] = [block["lines"] for block in vs["matrix_blocks"] if block["slot"] in vs["slots"]]
    if ps_text:
        path = scratch / "ps_review.hlsl"
        path.write_text(ps_text, encoding="utf-8")
        copies = [int(item["output"][1:]) for item in (result["vs"] or {}).get("vp_outputs", [])]
        ps = audit_ps.analyze(path, [n for n in copies if 0 <= n <= 15] or None)
        if ps is None:
            result["ps"] = {"error": "not a translated PS"}
            return result
        body = ps_text[ps_text.index("void main"):]
        signature_end = body.index("{")
        review = ps.get("clip_input_review") or {}
        # Translator scaffolding the clip review cannot parse: declarations,
        # debug texture copies, the host alpha-test epilogue and the host debug
        # view (xeFlags, i15), none of which use a guest register or the clip
        # copy. Guest control flow keeps its registers and stays.
        clip_inputs = {f"i{n}" for n in copies}
        unsupported = [entry for entry in review.get("unsupported", [])
                       if not DECLARATION.match(entry["text"]) and not DEBUG_COPY.match(entry["text"])
                       and (GUEST_REGISTER.search(entry["text"]) or
                            any(re.search(rf"\b{name}\b", entry["text"]) for name in clip_inputs))]
        screen_fetches = [f for f in ps["fetches"] if any(
            dep.split(".")[0] in clip_inputs and dep.endswith((".x", ".y")) for dep in f["coordinate_dependencies"])]
        result["ps"] = {
            "clip_inputs": sorted(clip_inputs),
            "clip_xy_reads": [entry["line"] for entry in review.get("clip_xy_reads", [])],
            "clip_w_reads": [entry["line"] for entry in review.get("clip_w_reads", [])],
            "unsupported": unsupported,
            "screen_position_reads": bool(SCREEN_POSITION.search(body[signature_end:])),
            "screen_space_fetches": [f["line"] for f in screen_fetches],
            "fetch_lines": [f["line"] for f in ps["fetches"]],
            "dependent_reads": ps["dependent_read_count"],
            "branches_or_loops": ps["branches_or_loops"]}
    return result


def suggest(pair: dict, slots: dict[str, int], sky_pairs: dict, decisions: dict | None = None) -> tuple[str, list[str]]:
    vs, ps = pair["vs"], pair["ps"]
    if vs in slots:
        return "already_mapped_vs", [f"PositionVPSlot maps {vs} to slot {slots[vs]}"]
    if (vs, ps) in sky_pairs:
        return "already_mapped_pair", ["exact pair is in SkyMaterialPairs"]
    reasons, blockers = [], []
    rows = pair["log_rows"]
    if rows and not any(row["vp_finite"] for row in rows):
        note = "logged camera bank is not finite (map transition)"
        if pair["capture_draws"]:
            reasons.append(note + "; the capture supplies the constants")
        else:
            blockers.append(note + "; take an F1 capture there")
    shaders = pair["shaders"]
    vs_review, ps_review = shaders.get("vs"), shaders.get("ps")
    camera_slots = {row["camera_slot"] for row in rows} | {d["vp_slot"] for d in pair["capture_draws"]}
    if not vs_review:
        blockers.append("no VS HLSL (pass a capture or --hlsl-dir)")
    elif vs_review["classification"] != "matrix_position_candidate" or len(vs_review["slots"]) != 1:
        blockers.append(f"VS position is {vs_review['classification']} slots={vs_review['slots']}")
    elif camera_slots and vs_review["slots"][0] not in camera_slots:
        blockers.append(f"VS oPos slot {vs_review['slots'][0]} differs from the observed camera slot(s) {sorted(camera_slots)}")
    else:
        reasons.append(f"oPos uses only the slot-{vs_review['slots'][0]} matrix (lines {vs_review['matrix_lines']})")
    if not ps_review or "error" in (ps_review or {}):
        blockers.append("no PS HLSL")
    else:
        if ps_review["screen_position_reads"]:
            blockers.append("PS reads SV_Position")
        if ps_review["clip_xy_reads"]:
            blockers.append(f"PS reads clip X/Y of {ps_review['clip_inputs']} at lines {ps_review['clip_xy_reads']}")
        if ps_review["screen_space_fetches"]:
            blockers.append(f"PS samples at clip-derived coordinates (lines {ps_review['screen_space_fetches']})")
        if ps_review["unsupported"]:
            lines_ = sorted({entry["line"] for entry in ps_review["unsupported"]})
            blockers.append(f"PS has guest control flow or statements the clip review cannot follow (lines {lines_[:8]})")
        if not blockers:
            reasons.append(f"PS reads only W of {ps_review['clip_inputs'] or 'no clip copy'}; "
                           f"fetches at lines {ps_review['fetch_lines']} use mesh inputs")
    companions = [row for row in rows if row["companion_slot"] == 4 and row["same_world"] and row["same_camera"]]
    depth = [d for d in pair["capture_draws"] if d["depth_draws"] or d["geometry_depth_draws"]]
    if companions or depth:
        reasons.append("drawn over the same geometry as a jittered depth draw (log companion or capture match)")
    elif any(row["zwrite"] for row in rows):
        reasons.append("writes main scene depth under jitter (no companion seen)")
    if blockers:
        return "hold", blockers + reasons
    review = (decisions or {}).get(vs)
    if review and review["decision"] == "hold":
        return "hold", [f"{review['manifest']} holds this VS entirely ({review['reason'][:160]})"] + reasons
    if any(vs == other_vs for other_vs, _ in sky_pairs):
        return "map_exact_pair", reasons + ["this VS is already mapped per PS; keep exact pairs"]
    if review and review["decision"] == "held":
        return "map_exact_pair", reasons + [f"{review['manifest']} holds this VS ({review['reason'][:160]}); "
                                            "map only this reviewed pair"]
    return "map_vs_wide", reasons + ["no other reviewed PS policy for this VS; check its other PS partners first"]


def snippets(pair: dict) -> dict:
    vs, ps = pair["vs"], pair["ps"]
    out = {}
    if pair["action"] == "map_vs_wide":
        out["temporal_scene.h"] = f"    case 0x{vs}ull:  // add to the slot-{pair['slot']} group in PositionVPSlot"
    elif pair["action"] == "map_exact_pair":
        out["temporal_scene.h"] = f"    {{0x{vs}ull, 0x{ps}ull, true}},  // SkyMaterialPairs; copy the VS's existing fallback"
    draw = next((d for d in pair["capture_draws"] if d["depth_draws"]), None)
    if draw:
        depth = draw["depth_draws"][0]
        out["fixture"] = (f"python -B tools/capture_analysis/export_jitter_fixture.py --input {draw['capture']} "
                          f"--frame {draw['frame']} --draw {draw['draw']}:{vs}:{draw['vp_slot']} "
                          f"--depth-vs {depth['vs']} --depth-slot 4 --namespace <name> --output tools/tests/<name>.h")
        test = "CapturedMaterialOverDepth" if pair["action"] == "map_vs_wide" else "CapturedSky"
        out["test"] = (f"{test}({{\"<label>\",0x{vs}ull,0x{ps}ull,0x{depth['vs']}ull,{draw['draw']},{depth['draw']}...}}, "
                       f"<namespace>::draws[0]); and one namedCases row")
    elif any(row["companion_slot"] == 4 for row in pair["log_rows"]):
        out["fixture"] = "python -B tools/capture_analysis/suspect_log.py --log <log> --output <json> --fixture tools/tests/<name>.h --namespace <name>"
    return out


def markdown(report: dict) -> str:
    lines = ["| Action | VS / PS | Maps | Kind | Key reasons |", "|---|---|---|---|---|"]
    for pair in report["pairs"]:
        maps = sorted({row["map"] for row in pair["log_rows"] if row["map"]})
        kinds = sorted({row["kind"] for row in pair["log_rows"]})
        lines.append(f"| {pair['action']} | `{pair['vs'][:8]}/{pair['ps'][:8]}` | {', '.join(maps)[:60]} | "
                     f"{', '.join(kinds)} | {'; '.join(pair['reasons'])[:300]} |")
    return "\n".join(lines) + "\n"


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--log", action="append", default=[], type=Path, help="runtime log with suspect lines")
    parser.add_argument("--capture", action="append", default=[], type=Path, help="F1 capture ZIP or directory")
    parser.add_argument("--hlsl-dir", action="append", default=[], type=Path,
                        help="directory with <hash>.hlsl (e.g. LoShaderTool output) when no capture has the shader")
    parser.add_argument("--mapping", type=Path, default=Path("LostOdysseyRecomp/gpu/temporal_scene.h"))
    parser.add_argument("--reviews", type=Path, default=Path("tools/shader_analysis/reviews"),
                        help="review manifests; a VS held there is only ever suggested as exact pairs")
    parser.add_argument("--include-mapped", action="store_true", help="also list pairs the map already covers")
    parser.add_argument("--output", required=True, type=Path, help="new JSON report")
    parser.add_argument("--markdown", type=Path, help="optional new Markdown table")
    args = parser.parse_args(argv)
    for path in (args.output, args.markdown):
        if path and path.exists():
            parser.error(f"{path} exists; choose a new path")
    if not args.log and not args.capture:
        parser.error("pass at least one --log or --capture")

    slots, sky_pairs = mapping_state(args.mapping)
    decisions = review_decisions(args.reviews)
    rows = [row for log in args.log for row in suspect_log.parse_log(log)]
    pairs = {}
    for row in rows:
        key = (row["vs"].lower(), row["ps"].lower())
        camera = (row["banks"] or {}).get("camera")
        pairs.setdefault(key, []).append({
            "log": row["log"], "line": row["line"], "kind": row["kind"], "frame": row["frame"],
            "draw": row["draw"], "camera_slot": row["camera_slot"], "zwrite": row["zwrite"],
            "zfunc": row["zfunc"], "companion_vs": row["companion_vs"], "companion_slot": row["companion_slot"],
            "same_world": row["same_world"], "same_camera": row["same_camera"],
            "map": (row["map"] or {}).get("package") if (row["map"] or {}).get("available") else None,
            "vp_finite": finite_words(camera)})
    draws, hlsl = capture_evidence(args.capture, slots, set(pairs)) if args.capture else ({}, {})
    for directory in args.hlsl_dir:
        for path in directory.glob("*.hlsl"):
            name = path.stem.removeprefix("vs_").removeprefix("ps_").lower()
            hlsl.setdefault(name, path.read_text(encoding="utf-8-sig"))

    report = {"schema": "lostodyssey.suspect-triage.v1", "mapping": str(args.mapping),
              "logs": [str(p) for p in args.log], "captures": [str(p) for p in args.capture], "pairs": []}
    scratch = Path(tempfile.mkdtemp(prefix="lo-triage-"))
    try:
        for (vs, ps), log_rows in sorted(pairs.items()):
            pair = {"vs": vs, "ps": ps, "log_rows": log_rows, "capture_draws": draws.get((vs, ps), [])}
            pair["shaders"] = review_shaders(hlsl.get(vs), hlsl.get(ps), scratch)
            pair["action"], pair["reasons"] = suggest(pair, slots, sky_pairs, decisions)
            if pair["action"].startswith("already") and not args.include_mapped:
                continue
            vs_slots = (pair["shaders"].get("vs") or {}).get("slots") or []
            pair["slot"] = vs_slots[0] if vs_slots else log_rows[0]["camera_slot"]
            pair["snippets"] = snippets(pair)
            report["pairs"].append(pair)
    finally:
        shutil.rmtree(scratch, ignore_errors=True)
    order = {"map_vs_wide": 0, "map_exact_pair": 1, "hold": 2}
    report["pairs"].sort(key=lambda p: (order.get(p["action"], 3), p["vs"], p["ps"]))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=1), encoding="utf-8")
    if args.markdown:
        args.markdown.write_text(markdown(report), encoding="utf-8")
    for pair in report["pairs"]:
        print(f"{pair['action']:15s} {pair['vs']}/{pair['ps']}  {'; '.join(pair['reasons'])[:200]}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

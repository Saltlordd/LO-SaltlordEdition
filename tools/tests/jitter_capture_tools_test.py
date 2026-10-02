#!/usr/bin/env python3
"""Focused offline candidate and reviewed fixture export checks."""

import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.capture_analysis import export_jitter_fixture, jitter_candidates, suspect_log, trace, triage_suspect


DEPTH = "1111111111111111"
MATERIAL = "2222222222222222"
PIXEL = "3333333333333333"
VP = list(range(101, 117))


def reg(address, value):
    return f"{address:04x} {value:08x}\n"


class JitterCaptureToolsTest(unittest.TestCase):
    def make_capture(self, root):
        capture = root / "capture"
        frame = capture / "frame-01-f42"
        shaders = capture / "shaders"
        frame.mkdir(parents=True)
        shaders.mkdir()
        (capture / "capture-info.txt").write_text("status=complete\n")
        (frame / "temporal-scene.json").write_text(json.dumps({"vp_u32": VP}))
        (shaders / f"{MATERIAL}.hlsl").write_text(
            "xePV = XeConst(10);\nxePV = XeConst(9);\n"
            "xePV = XeConst(8);\nxePV = XeConst(7);\noPos.xyzw = xePV.xyzw;\n")
        mapping = root / "temporal_scene.h"
        mapping.write_text("inline int PositionVPSlot(uint64_t shader) { switch(shader) {\n"
                           f"case 0x{DEPTH}ull:return 4;\n"
                           "default:return -1; }}\n")
        state = {address: 0 for address in range(0x4000, 0x4400)}
        state.update({address: 0 for address in range(0x4400, 0x4440)})
        state.update({0x2000: 0x14000500, 0x2002: 0x10000, 0x2080: 0,
                      0x2081: 0, 0x2082: 0x02d00500, 0x2200: 0x700766,
                      0x2206: 0x43f, 0x2208: 5,
                      0x48be: 0x100, 0x48bf: 0x200})
        state.update({0x210f + i: i + 1 for i in range(6)})
        state.update({0x4000 + i: i + 10 for i in range(16)})
        state.update({0x4010 + i: value for i, value in enumerate(VP)})
        state.update({0x4000 + 230*4 + i: value for i, value in enumerate(VP)})
        lines = ["Frame 42\n", "draw 0 prim=4 indices=6 indexed=true base=0x80\n"]
        lines.extend(reg(address, value) for address, value in sorted(state.items()))
        lines.append(f"shaders vs={DEPTH} ps={'0'*16} ps_status=not_bound\n")
        lines.append("draw 1 prim=4 indices=6 indexed=true base=0x80\n")
        lines.extend(reg(0x401c + i, value) for i, value in enumerate(VP))
        lines.append(reg(0x2208, 4))
        lines.append(f"shaders vs={MATERIAL} ps={PIXEL} ps_status=bound\n")
        lines.append("draw 2 prim=8 indices=3 indexed=false base=0x0\n")
        lines.append("resolve f42_seq00.bin draw=2 address=0x100 width=2 height=2 plume_format=20 bpp=4 raw_ok=true\n")
        lines.append("draw 3 prim=4 indices=6 indexed=true base=0x80\n")
        lines.append(f"shaders vs={MATERIAL} ps={PIXEL} ps_status=bound\n")
        lines.append("end frame=42 submitted_draws=3\n")
        (frame / "render-state.txt").write_text("".join(lines))
        return capture, mapping

    def test_candidates_keep_stale_matches_unproven(self):
        with tempfile.TemporaryDirectory() as tmp:
            capture, mapping = self.make_capture(Path(tmp))
            result = jitter_candidates.analyze(capture, mapping, 42)
            frame = result["frames"][0]
            self.assertEqual((frame["before_draw"], frame["cutoff_source"]), (4, "full_frame"))
            hinted = [candidate for candidate in frame["candidates"] if candidate["position_chain_hint"]]
            self.assertEqual([(item["vs"], item["candidate_slot"]) for item in hinted], [(MATERIAL, 7)])
            self.assertEqual(hinted[0]["draws"][0]["matching_depth_draws"], [{"draw": 0, "vs": DEPTH}])
            self.assertTrue(any(candidate["candidate_slot"] == 230 and not candidate["position_chain_hint"]
                                for candidate in frame["candidates"]))
            later = jitter_candidates.analyze(capture, mapping, 42, before_draw=4)["frames"][0]
            self.assertEqual((later["before_draw"], later["cutoff_source"]), (4, "explicit"))
            self.assertEqual(len(next(item for item in later["candidates"] if item["candidate_slot"] == 7)["draws"]), 2)

    def test_late_light_geometry_survives_pass_changes_without_strict_pair(self):
        with tempfile.TemporaryDirectory() as tmp:
            capture, mapping = self.make_capture(Path(tmp))
            path = capture / "frame-01-f42" / "render-state.txt"
            text = path.read_text().replace("draw 3 prim=4 indices=6 indexed=true base=0x80\n",
                "draw 3 prim=4 indices=6 indexed=true base=0x80\n" + reg(0x2200, 0x00700263) +
                reg(0x2201, 0x01000101) + reg(0x2081, 0x00100010))
            path.write_text(text)
            frame = jitter_candidates.analyze(capture, mapping, 42)["frames"][0]
            late = next(c for c in frame["candidates"] if c["candidate_slot"] == 7)["draws"][1]
            self.assertEqual(late["matching_depth_draws"], [])
            self.assertEqual(late["matching_geometry_depth_draws"][0]["draw"], 0)
            self.assertIn("0x2200", late["matching_geometry_depth_draws"][0]["pass_differences"])
            with trace.load_capture(capture) as opened:
                with self.assertRaises(ValueError):
                    export_jitter_fixture.collect(opened, "frame-01-f42", [(3, MATERIAL, 7)], DEPTH, 4, before_draw=4)
            # Same registers with different geometry must not receive this association.
            path.write_text(text.replace("draw 3 prim=4 indices=6 indexed=true base=0x80",
                                         "draw 3 prim=4 indices=6 indexed=true base=0x90"))
            frame = jitter_candidates.analyze(capture, mapping, 42)["frames"][0]
            late = next(c for c in frame["candidates"] if c["candidate_slot"] == 7)["draws"][1]
            self.assertEqual(late["matching_geometry_depth_draws"], [])

    def test_default_scan_does_not_require_a_resolve(self):
        with tempfile.TemporaryDirectory() as tmp:
            capture, mapping = self.make_capture(Path(tmp))
            path = capture / "frame-01-f42" / "render-state.txt"
            path.write_text("\n".join(line for line in path.read_text().splitlines() if not line.startswith("resolve ")))
            frame = jitter_candidates.analyze(capture, mapping, 42)["frames"][0]
            self.assertEqual(frame["cutoff_source"], "full_frame")
            self.assertEqual(len(next(c for c in frame["candidates"] if c["candidate_slot"] == 7)["draws"]), 2)

    def test_fixture_needs_explicit_identity_and_exact_pair(self):
        with tempfile.TemporaryDirectory() as tmp:
            capture, _ = self.make_capture(Path(tmp))
            with trace.load_capture(capture) as opened:
                rows = export_jitter_fixture.collect(opened, "frame-01-f42", [(1, MATERIAL, 7)], DEPTH, 4)
                self.assertEqual((rows[0]["draw"], rows[0]["depth_draw"]), (1, 0))
                self.assertEqual(rows[0]["vertex"][28:44], tuple(VP))
                self.assertIn("0x2222222222222222ull", export_jitter_fixture.render_header(rows, "fixture_42", "f42"))
                with self.assertRaisesRegex(ValueError, "identity differs"):
                    export_jitter_fixture.collect(opened, "frame-01-f42", [(1, DEPTH, 7)], DEPTH, 4)
                with self.assertRaisesRegex(ValueError, "at/after draw boundary"):
                    export_jitter_fixture.collect(opened, "frame-01-f42", [(2, MATERIAL, 7)], DEPTH, 4)
                self.assertEqual(export_jitter_fixture.collect(opened, "frame-01-f42",
                    [(3, MATERIAL, 7)], DEPTH, 4, before_draw=4)[0]["draw"], 3)

    def test_missing_fetch_is_recorded_and_cannot_pair(self):
        with tempfile.TemporaryDirectory() as tmp:
            capture, mapping = self.make_capture(Path(tmp))
            path = capture / "frame-01-f42" / "render-state.txt"
            path.write_text(path.read_text().replace("48be 00000100\n", ""))
            frame = jitter_candidates.analyze(capture, mapping, 42)["frames"][0]
            hinted = next(item for item in frame["candidates"] if item["position_chain_hint"])
            self.assertEqual(hinted["draws"][0]["matching_depth_draws"], [])
            self.assertIn("0x48be", frame["incomplete_depth_draws"][0]["missing_evidence"])
            self.assertIn("0x48be", hinted["draws"][0]["missing_evidence"])
            with trace.load_capture(capture) as opened:
                with self.assertRaisesRegex(ValueError, "missing exact pair evidence"):
                    export_jitter_fixture.collect(opened, "frame-01-f42", [(1, MATERIAL, 7)], DEPTH, 4)

    def test_export_rejects_missing_header_invalid_ps_and_unsupported_slots(self):
        with tempfile.TemporaryDirectory() as tmp:
            capture, mapping = self.make_capture(Path(tmp))
            with trace.load_capture(capture) as opened:
                with self.assertRaisesRegex(ValueError, "material slots 0..12"):
                    export_jitter_fixture.collect(opened, "frame-01-f42", [(1, MATERIAL, 13)], DEPTH, 4)
                with self.assertRaisesRegex(ValueError, "depth slots 0..4"):
                    export_jitter_fixture.collect(opened, "frame-01-f42", [(1, MATERIAL, 7)], DEPTH, 5)
            path = capture / "frame-01-f42" / "render-state.txt"
            text = path.read_text()
            path.write_text(text.replace("draw 1 prim=4 indices=6 indexed=true base=0x80",
                                         "draw 1 prim=4 indices=6 indexed=true"))
            frame = jitter_candidates.analyze(capture, mapping, 42)["frames"][0]
            hinted = next(item for item in frame["candidates"] if item["position_chain_hint"])
            self.assertEqual(hinted["draws"][0]["matching_depth_draws"], [])
            self.assertIn("base", hinted["draws"][0]["missing_evidence"])
            with trace.load_capture(capture) as opened:
                with self.assertRaisesRegex(ValueError, "missing exact pair evidence"):
                    export_jitter_fixture.collect(opened, "frame-01-f42", [(1, MATERIAL, 7)], DEPTH, 4)
            path.write_text(text.replace(f"ps={PIXEL} ps_status=bound", "ps=0000000000000000 ps_status=bound"))
            with trace.load_capture(capture) as opened:
                with self.assertRaisesRegex(ValueError, "no valid bound material PS"):
                    export_jitter_fixture.collect(opened, "frame-01-f42", [(1, MATERIAL, 7)], DEPTH, 4)


def hex_words(values):
    return ",".join(f"{value:08x}" for value in values)


def suspect_lines(companion_slot=4, same=True, banks=True):
    material, late, pixel = list(range(64)), list(range(200, 208)), list(range(300, 364))
    world, vp = list(range(400, 416)), list(range(500, 516))
    companion = f"companion_vs={DEPTH} companion_slot={companion_slot} companion_draw=351"
    lines = [
        "[   24.602 tca42] [info]  current map available=true id=243 name=Legacy of the Eastern Tribe package=ev4_0_scrw",
        f"[   26.477 t613b] [info]  temporal suspect: kind=depth_writer_after_jittered_geometry vs={MATERIAL} "
        f"ps={PIXEL} frame=1138 draw=1231 camera_slot=7 position_kind=1 evidence_slot=7 depth_control=0x00700766 "
        f"zfunc=6 zwrite=true index_base=0x0b8f3c00 index_count=2484 base_vertex=0 fetch95=0x0c258003 {companion} "
        f"same_world={str(same).lower()} same_camera={str(same).lower()}"]
    if banks:
        lines.append(f"[   26.477 t613b] [info]  temporal suspect banks: vs={MATERIAL} ps={PIXEL} "
                     f"material={hex_words(material)} late={hex_words(late)} camera={hex_words(material[28:44])} "
                     f"pixel={hex_words(pixel)} "
                     f"companion_world={hex_words(world)} companion_vp={hex_words(vp)}")
    return "\n".join(lines) + "\n", material, late, pixel, world + vp


class SuspectLogTest(unittest.TestCase):
    def test_log_lines_become_located_fixture_rows(self):
        with tempfile.TemporaryDirectory() as tmp:
            log = Path(tmp) / "runtime.log"
            text, material, late, pixel, depth = suspect_lines()
            log.write_text(text)
            rows = suspect_log.parse_log(log)
            self.assertEqual(len(rows), 1)
            row = rows[0]
            self.assertEqual((row["vs"], row["ps"], row["camera_slot"], row["companion_slot"], row["position_kind"]),
                             (MATERIAL, PIXEL, 7, 4, 1))
            self.assertEqual(row["banks"]["camera"], material[28:44])
            self.assertEqual(row["map"]["package"], "ev4_0_scrw")
            self.assertEqual(row["map"]["name"], "Legacy of the Eastern Tribe")
            fixture = suspect_log.fixture_row(row)
            self.assertEqual((fixture["vertex"], fixture["vertex_late"], fixture["pixel"], fixture["depth"]),
                             (material, late, pixel, depth))
            output, header = Path(tmp) / "summary.json", Path(tmp) / "fixture.h"
            suspect_log.main(["--log", str(log), "--output", str(output), "--fixture", str(header),
                              "--namespace", "runtime_sky"])
            self.assertEqual(json.loads(output.read_text())["fixture_rows"], 1)
            self.assertIn(f"{{0x{MATERIAL}ull,0x{PIXEL}ull,0x{DEPTH}ull,7,1231,351, {{", header.read_text())
            self.assertIn("at the first reported draw.", header.read_text())

    def test_unpaired_or_unsupported_suspects_do_not_make_fixtures(self):
        with tempfile.TemporaryDirectory() as tmp:
            log = Path(tmp) / "runtime.log"
            for kwargs in ({"companion_slot": 0}, {"same": False}, {"banks": False}):
                log.write_text(suspect_lines(**kwargs)[0])
                self.assertIsNone(suspect_log.fixture_row(suspect_log.parse_log(log)[0]))
            log.write_text(suspect_lines()[0].splitlines()[2] + "\n")
            with self.assertRaisesRegex(ValueError, "without a preceding suspect line"):
                suspect_log.parse_log(log, strict=True)
            self.assertEqual(suspect_log.parse_log(log), [])
            lines = suspect_lines()[0].splitlines()
            log.write_text("\n".join([lines[0], lines[1][:120], *lines[1:]]) + "\n")
            rows = suspect_log.parse_log(log)
            self.assertEqual(len(rows), 1)
            self.assertIsNotNone(suspect_log.fixture_row(rows[0]))


TRIAGE_VS = """void main()
{
r0.x = float(xeVertexId);
r1 = XeVF_0;
xePV = r1.xxxx * c[7].xyzw;
xePV = r1.yyyy * c[8].xyzw + xePV;
xePV = r1.zzzz * c[9].xyzw + xePV;
xePV = r1.wwww * c[10].xyzw + xePV;
oPos = xePV;
o4.xyzw = xePV.xyzw;
o0.xyzw = r1.xyzw;
if ((xeFlags & 8u) != 0u) oPos.xy = oPos.xy;
}
"""
TRIAGE_PS = """void main(
	in float4 iPos : SV_Position,
	in float4 i0 : TEXCOORD0,
	in float4 i4 : TEXCOORD4,
	out float4 oC0 : SV_Target0)
{
	int a0 = 0;
	r0 = i0;
	r4 = i4;
	r1 = XeTextureResult(XeTex2D(tex2D_0, XeSampler(0u), r0.xy, float2(0, 0), 0u, false), 0u);
	xeDbgTex = r1;
	ps = max(r4.w, r4.w);
	oC0.w = ps;
	oC0.xyz = r1.xyz;
	if (func == 0) pass = false;
}
"""


class TriageSuspectTest(unittest.TestCase):
    def run_triage(self, root, ps_text=TRIAGE_PS, mapping_extra="", pairs="", include_mapped=False, held=False):
        log = root / "runtime.log"
        log.write_text(suspect_lines()[0])
        hlsl = root / "hlsl"
        hlsl.mkdir(exist_ok=True)
        (hlsl / f"vs_{MATERIAL}.hlsl").write_text(TRIAGE_VS)
        (hlsl / f"ps_{PIXEL}.hlsl").write_text(ps_text)
        mapping = root / "temporal_scene.h"
        mapping.write_text("inline int PositionVPSlot(uint64_t shader) { switch(shader) {\n"
                           f"case 0x{DEPTH}ull:return 4;\n{mapping_extra}"
                           "default:return -1; }}\n"
                           f"inline constexpr SkyMaterialPair SkyMaterialPairs[]{{\n{pairs}}};\n")
        output = root / f"triage-{len(list(root.glob('triage-*.json')))}.json"
        reviews = root / "reviews"
        reviews.mkdir(exist_ok=True)
        decision = "held" if held else "implemented"
        (reviews / "manifest.json").write_text(json.dumps({"candidates": [
            {"vs": MATERIAL, "decision": decision, "reason": "another PS samples the clip copy"}]}))
        args = ["--log", str(log), "--hlsl-dir", str(hlsl), "--mapping", str(mapping), "--reviews", str(reviews),
                "--output", str(output)]
        triage_suspect.main(args + (["--include-mapped"] if include_mapped else []))
        return json.loads(output.read_text())["pairs"]

    def test_safe_material_over_jittered_depth_suggests_vs_wide_mapping(self):
        with tempfile.TemporaryDirectory() as tmp:
            pair, = self.run_triage(Path(tmp))
            self.assertEqual(pair["action"], "map_vs_wide")
            self.assertEqual(pair["slot"], 7)
            self.assertEqual(pair["shaders"]["vs"]["vp_outputs"], [{"output": "o4", "components": "xyzw"}])
            self.assertEqual(pair["shaders"]["ps"]["clip_inputs"], ["i4"])
            self.assertEqual(pair["shaders"]["ps"]["unsupported"], [])
            self.assertIn(f"case 0x{MATERIAL}ull", pair["snippets"]["temporal_scene.h"])
            self.assertIn("suspect_log.py", pair["snippets"]["fixture"])

    def test_vs_already_paired_with_another_ps_keeps_exact_pairs(self):
        with tempfile.TemporaryDirectory() as tmp:
            pair, = self.run_triage(Path(tmp), pairs=f"    {{0x{MATERIAL}ull, 0x{'4'*16}ull, true}},\n")
            self.assertEqual(pair["action"], "map_exact_pair")
            self.assertIn(f"0x{PIXEL}ull, true", pair["snippets"]["temporal_scene.h"])

    def test_vs_held_by_a_review_manifest_gets_exact_pairs_only(self):
        with tempfile.TemporaryDirectory() as tmp:
            pair, = self.run_triage(Path(tmp), held=True)
            self.assertEqual(pair["action"], "map_exact_pair")
            self.assertTrue(any("manifest.json holds this VS" in reason for reason in pair["reasons"]))

    def test_clip_xy_sampling_or_screen_position_holds(self):
        with tempfile.TemporaryDirectory() as tmp:
            screen = TRIAGE_PS.replace("r0.xy, float2", "r4.xy, float2")
            pair, = self.run_triage(Path(tmp), ps_text=screen)
            self.assertEqual(pair["action"], "hold")
            self.assertTrue(any("clip X/Y" in reason for reason in pair["reasons"]))
            position = TRIAGE_PS.replace("oC0.xyz = r1.xyz;", "oC0.xyz = iPos.xyz;")
            pair, = self.run_triage(Path(tmp), ps_text=position)
            self.assertEqual(pair["action"], "hold")
            self.assertIn("PS reads SV_Position", pair["reasons"])

    def test_mapped_pairs_are_listed_only_on_request(self):
        with tempfile.TemporaryDirectory() as tmp:
            mapped = f"case 0x{MATERIAL}ull:return 7;\n"
            self.assertEqual(self.run_triage(Path(tmp), mapping_extra=mapped), [])
            pair, = self.run_triage(Path(tmp), mapping_extra=mapped, include_mapped=True)
            self.assertEqual(pair["action"], "already_mapped_vs")


if __name__ == "__main__":
    unittest.main()

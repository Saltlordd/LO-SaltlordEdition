"""Source preservation, deduplication and failure boundaries for FMV export."""
from contextlib import closing, redirect_stdout
import hashlib
import io
import json
from pathlib import Path
import sqlite3
import subprocess
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from tools.asset_inventory import inventory, export_fmv as fmv


class FmvExportTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="lo-fmv-test-")
        self.root = Path(self.temp.name)
        self.game = self.root / "game"
        self.game.mkdir()
        self.db = self.root / "catalog.sqlite"
        self.output = self.root / "export"
        self.args = SimpleNamespace(db=self.db, output=self.output, game=None, decoder=None, ffprobe="ffprobe")
        with closing(sqlite3.connect(self.db)) as db:
            db.executescript(inventory.SCHEMA)
            db.executemany("INSERT INTO metadata VALUES (?,?)",
                           [("schema_version", "1"), ("complete", "true"), ("game_root", json.dumps(str(self.game)))])
            db.commit()
        self.probe = patch.object(fmv, "probe_movie", return_value={
            "format": {"format_name": "asf", "duration": "1.5"},
            "streams": [{"codec_type": "video", "codec_name": "wmv3", "width": 1280, "height": 720}]})
        self.which = patch.object(fmv.shutil, "which", return_value="ffprobe")
        self.probe.start()
        self.which.start()
        self.addCleanup(self.probe.stop)
        self.addCleanup(self.which.stop)

    def tearDown(self):
        self.temp.cleanup()

    def add(self, disc, payload, encoding="raw", decoded_size=0):
        folder = self.game / disc
        folder.mkdir()
        file = folder / "xenon_mov.fpd"
        prefix = b"archive prefix"
        file.write_bytes(prefix + payload + b"archive suffix")
        stat = file.stat()
        sha = hashlib.sha256(payload).hexdigest()
        with closing(sqlite3.connect(self.db)) as db:
            db.execute("INSERT INTO sources VALUES (?,?,?,?)", (f"{disc}/xenon_mov.fpd", stat.st_size, stat.st_mtime_ns, ""))
            db.execute("INSERT OR IGNORE INTO payloads(sha256,encoding,decoded_size,status,magic) VALUES (?,?,?,?,?)",
                       (sha, encoding, decoded_size, "not_package", payload[:4].hex()))
            db.execute("INSERT INTO files(disc,archive,path,offset,length,sha256,extension) VALUES (?,?,?,?,?,?,?)",
                       (disc, "xenon_mov.fpd", "bin/xenon/mov/mv_01v.wmv", len(prefix), len(payload), sha, ".wmv"))
            db.commit()
        return file

    def export(self):
        with redirect_stdout(io.StringIO()):
            return fmv.export_fmv(self.args)

    def test_exact_extents_variants_and_disc_duplicates(self):
        a, b = fmv.ASF_HEADER + b"first movie", fmv.ASF_HEADER + b"second movie"
        files = [self.add("disc1", a), self.add("disc2", a), self.add("disc3", b)]
        original = [f.read_bytes() for f in files]
        result = self.export()
        self.assertTrue(result["complete"])
        self.assertEqual(result["summary"]["source_hashes_verified"], 3)
        self.assertEqual(result["summary"]["unique_movies"], 2)
        self.assertEqual(result["summary"]["duration_seconds"], 3)
        self.assertEqual(result["summary"]["output_bytes"], len(a) + len(b))
        self.assertEqual({(self.output / item["output"]).read_bytes() for item in result["files"]}, {a, b})
        self.assertEqual(len(result["occurrences"]), 3)
        self.assertEqual([f.read_bytes() for f in files], original)
        self.assertEqual(len((self.output / "manifest.csv").read_text(encoding="utf-8-sig").splitlines()), 4)

    def test_cpx_is_decoded_and_deduplicated_against_raw_content(self):
        movie = fmv.ASF_HEADER + b"video\0\n\x1a\xff"
        self.add("disc1", b"cpx\xffsynthetic", encoding="cpx", decoded_size=len(movie))
        self.add("disc2", movie)
        self.args.decoder = self.root / "decoder"
        self.args.decoder.write_bytes(b"test tool")
        with patch.object(fmv.subprocess, "run", return_value=SimpleNamespace(stdout=movie)) as run:
            result = self.export()
        self.assertEqual(run.call_args.args[0][-1], "--decode-cpx")
        self.assertEqual(run.call_args.kwargs["input"], b"cpx\xffsynthetic")
        self.assertEqual(result["summary"]["unique_stored"], 2)
        self.assertEqual(result["summary"]["unique_movies"], 1)
        self.assertEqual(len({r["decoded_sha256"] for r in result["occurrences"]}), 1)
        self.assertEqual(len({r["stored_sha256"] for r in result["occurrences"]}), 2)
        self.assertFalse(list(self.output.glob("*.partial")))

    def test_changed_repeated_source_does_not_mark_partial_export_complete(self):
        movie = fmv.ASF_HEADER + b"identical"
        self.add("disc1", movie)
        second = self.add("disc2", movie)
        second.write_bytes(second.read_bytes().replace(b"identical", b"different"))
        with self.assertRaisesRegex(ValueError, "source content changed"):
            self.export()
        result = json.loads((self.output / "manifest.json").read_text())
        self.assertFalse(result["complete"])
        self.assertEqual(len(result["occurrences"]), 1)
        self.assertIn("source content changed", result["error"])

    def test_input_output_boundaries_and_invalid_extent(self):
        self.add("disc1", fmv.ASF_HEADER + b"movie")
        self.args.output = self.game / "forbidden"
        with self.assertRaisesRegex(ValueError, "outside game data"):
            self.export()
        self.assertFalse(self.args.output.exists())
        self.args.output = self.output
        self.output.mkdir()
        with self.assertRaisesRegex(ValueError, "already exists"):
            self.export()
        self.output.rmdir()
        with closing(sqlite3.connect(self.db)) as db:
            db.execute("UPDATE files SET offset=999999")
            db.commit()
        with self.assertRaisesRegex(ValueError, "invalid movie extent"):
            self.export()
        self.assertFalse(self.output.exists())

    def test_invalid_asf_and_missing_cpx_decoder_fail(self):
        self.add("disc1", b"not a real ASF movie")
        with self.assertRaisesRegex(ValueError, "invalid ASF header"):
            self.export()
        self.assertFalse(json.loads((self.output / "manifest.json").read_text())["complete"])
        self.assertEqual(list((self.output / "movies").iterdir()), [])
        self.args.output = self.root / "missing-decoder-export"
        with closing(sqlite3.connect(self.db)) as db:
            db.execute("UPDATE payloads SET encoding='cpx'")
            db.commit()
        with self.assertRaisesRegex(ValueError, "CPX movies require"):
            self.export()
        self.assertFalse(self.args.output.exists())

    def test_failed_decoder_retains_bounded_diagnostic(self):
        self.add("disc1", b"cpx\xffbroken", encoding="cpx", decoded_size=20)
        self.args.decoder = self.root / "decoder"
        self.args.decoder.write_bytes(b"test tool")
        error = subprocess.CalledProcessError(1, ["decoder"], stderr=b"invalid CPX stream\n" + b"x" * 5000)
        with patch.object(fmv.subprocess, "run", side_effect=error), self.assertRaisesRegex(ValueError, "invalid CPX stream"):
            self.export()
        result = json.loads((self.output / "manifest.json").read_text())
        self.assertFalse(result["complete"])
        self.assertIn("invalid CPX stream", result["error"])
        self.assertLess(len(result["error"]), 4200)


if __name__ == "__main__":
    unittest.main()

"""Synthetic resource inventory checks; no installed game files are read."""
from contextlib import closing, redirect_stdout
import io
import json
from pathlib import Path
import sqlite3
import struct
import tempfile
import unittest
from unittest.mock import patch

from tools.asset_inventory import inventory


def packed_name(value):
    chars = [inventory.ALPHABET.index(c) for c in value.upper()]
    chars.extend([0] * ((2 - len(chars)) % 3))
    output = bytearray()
    groups = max(0, (len(chars) - 2) // 3)
    output += struct.pack("<H", chars[0] * 40 + chars[1] * 1600 + groups)
    for pos in range(2, len(chars), 3):
        output += struct.pack("<H", chars[pos] + chars[pos + 1] * 40 + chars[pos + 2] * 1600)
    return output


def make_disc(root, number, good, bad=b"cpx\0bad-package"):
    disc = root / f"disc{number}"
    disc.mkdir()
    entries = 64 + 13 * 48
    dictionary = entries + 3 * 24
    data = bytearray(2048)
    struct.pack_into("<I", data, 8, 0x10000)
    struct.pack_into("<H", data, 12, 1)
    data[20:22] = bytes((number, 4))
    struct.pack_into("<HHIIIII", data, 24, 1, 13, 3, 64, entries, dictionary, dictionary - 4)
    cursor = dictionary + 2

    def name_bits(value, ext=False, directory=False):
        nonlocal cursor
        relative = (cursor - dictionary) // 2
        word = packed_name(value)
        data[cursor:cursor + len(word)] = word
        cursor += len(word)
        return relative | ((1 << 23) if ext else 0) | (0x10000000 if directory else 0)

    ext_relative = (cursor - dictionary) // 2
    ext = packed_name("fpd")
    data[cursor:cursor + len(ext)] = ext
    cursor += len(ext)
    struct.pack_into("<H", data, dictionary - 4, ext_relative)
    archive_names = [name.removesuffix(".fpd") for name in inventory.ARCHIVES]
    for index, stem in enumerate(archive_names):
        ar = 64 + index * 48
        struct.pack_into("<I", data, ar + 24, name_bits(stem, ext=True))
        struct.pack_into("<I", data, ar + 4, entries + (0 if index == 0 else 3) * 24 - ar)
        if index == 0:
            struct.pack_into("<H", data, ar + 2, 1)
    struct.pack_into("<I", data, entries, name_bits("menu", directory=True))
    struct.pack_into("<H", data, entries + 14, 2)
    struct.pack_into("<I", data, entries + 20, 1)
    struct.pack_into("<I", data, entries + 24, name_bits("atlas", ext=True))
    struct.pack_into("<I", data, entries + 24 + 16, len(good))
    struct.pack_into("<I", data, entries + 48, name_bits("broken", ext=True))
    struct.pack_into("<I", data, entries + 48 + 8, 1)
    struct.pack_into("<I", data, entries + 48 + 16, len(bad))
    (disc / "LO.fpi").write_bytes(data)
    (disc / "LO.fpd").write_bytes(good.ljust(2048, b"\0") + bad)
    for archive in inventory.ARCHIVES[1:]:
        (disc / archive).write_bytes(b"")
    return disc


class FakeDecoder:
    def __init__(self, *args, **kwargs):
        self.stdin = self
        self.stdout = self
        self.current = None
        self.calls = []
        self.closed = False

    def write(self, line):
        source, offset, length = line.strip().split("\t")
        with open(source, "rb") as stream:
            stream.seek(int(offset))
            payload = stream.read(int(length))
        self.current = payload
        self.calls.append(payload)

    def flush(self):
        pass

    def readline(self):
        if self.current.startswith(b"cpx\0bad"):
            return json.dumps({"status": "error", "encoding": "raw", "error": "synthetic parse failure"}) + "\n"
        return json.dumps({"status": "ok", "encoding": "cpx", "decoded_size": 128,
                           "exports": [{"index": 0, "name": "Atlas", "class_name": "Texture2D",
                                        "class_ref": -1, "outer_ref": 0, "object_path": "Atlas",
                                        "serial_offset": 8, "serial_size": 16, "width": 4,
                                        "height": 4, "format": 1}], "imports": []}) + "\n"

    def close(self):
        self.closed = True

    def wait(self, timeout=None):
        return 0

    def poll(self):
        return 0 if self.closed else None

    def kill(self):
        self.closed = True


class InventoryTest(unittest.TestCase):
    def test_scan_rejects_incomplete_disc_set(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            game = root / "game"
            game.mkdir()
            for number in (1, 2, 3):
                make_disc(game, number, b"cpx\0same")
            decoder = root / "fake-decoder.exe"
            decoder.write_bytes(b"fixture")
            args = type("Args", (), {"game": game, "output": root / "result", "decoder": decoder,
                                      "allow_partial": False})()
            with self.assertRaisesRegex(ValueError, "requires discs 1..4"):
                inventory.scan(args)

    def test_fpi_tree_and_corruption(self):
        with tempfile.TemporaryDirectory() as tmp:
            disc = make_disc(Path(tmp), 1, b"cpx\0good")
            files, stats = inventory.read_fpi(disc)
            self.assertEqual((stats["records"], stats["directories"], stats["files"]), (3, 1, 2))
            self.assertEqual({item["path"] for item in files}, {"menu/atlas.fpd", "menu/broken.fpd"})
            self.assertEqual([item["offset"] for item in files], [0, 2048])
            original = (disc / "LO.fpi").read_bytes()
            for label, position, value in (("bad offset", 36, 0xFFFFFFFE),
                                           ("cycle", 64 + 13 * 48 + 20, 0)):
                damaged = bytearray(original)
                struct.pack_into("<I", damaged, position, value)
                (disc / "LO.fpi").write_bytes(damaged)
                with self.subTest(label=label), self.assertRaises(ValueError):
                    inventory.read_fpi(disc)

    def test_scan_variants_failures_query_and_ui(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            game = root / "game"
            game.mkdir()
            make_disc(game, 1, b"cpx\x09same")
            make_disc(game, 2, b"cpx\x09same")
            make_disc(game, 3, b"cpx\x09different")
            decoder = root / "fake-decoder.exe"
            decoder.write_bytes(b"fixture")
            output = root / "result"
            decoder_instances = []

            def create_decoder(*args, **kwargs):
                instance = FakeDecoder(*args, **kwargs)
                decoder_instances.append(instance)
                return instance

            args = type("Args", (), {"game": game, "output": output, "decoder": decoder,
                                      "allow_partial": True})()
            with patch.object(inventory.subprocess, "check_output", return_value="fixture-commit\n"), \
                 patch.object(inventory.subprocess, "Popen", side_effect=create_decoder), \
                 redirect_stdout(io.StringIO()):
                code = inventory.scan(args)
            self.assertEqual(code, 2)  # Failed candidate remains visible in the catalog.
            self.assertEqual(len(decoder_instances[0].calls), 3)  # Two good contents and one bad content.
            with closing(inventory.open_readonly(output / "catalog.sqlite")) as db:
                counts = inventory.summarize(db)
                self.assertEqual(counts["file_occurrences"], 6)
                self.assertEqual(counts["unique_payloads"], 3)
                self.assertEqual(counts["package_occurrences"], 3)
                self.assertEqual(counts["asset_variants"], 2)
                self.assertEqual(counts["keys_with_content_variants"], 1)
                self.assertEqual(counts["failed_file_occurrences"], 3)
                self.assertEqual(db.execute("SELECT count(*) FROM catalog WHERE category='textures' AND ui_reason!=''").fetchone()[0], 2)
                with self.assertRaisesRegex(ValueError, "outside game data"):
                    inventory.report(db, game / "should-not-exist")
                self.assertFalse((game / "should-not-exist").exists())
            self.assertEqual(len((output / "failures.csv").read_text(encoding="utf-8-sig").splitlines()), 4)
            with redirect_stdout(io.StringIO()) as stdout:
                self.assertEqual(inventory.main(["query", "--db", str(output / "catalog.sqlite"),
                                                 "--category", "textures", "--class-name", "Texture2D",
                                                 "--search", "MENU", "--ui"]), 0)
            rows = [json.loads(line) for line in stdout.getvalue().splitlines()]
            self.assertEqual(len(rows), 2)
            self.assertEqual(sorted(len(row["sources"]) for row in rows), [1, 2])
            self.assertEqual({row["package"] for row in rows}, {"menu/atlas.fpd"})
            self.assertEqual({row["serial_offset"] for row in rows}, {8})
            with redirect_stdout(io.StringIO()) as stdout:
                self.assertEqual(inventory.main(["query", "--db", str(output / "catalog.sqlite"),
                                                 "--class-name", "StaticMesh", "--ui"]), 0)
            self.assertEqual(stdout.getvalue(), "")
            reparse_args = type("Args", (), {"db": output / "catalog.sqlite", "decoder": decoder, "pending_only": False})()
            with patch.object(inventory.subprocess, "Popen", side_effect=create_decoder), \
                 redirect_stdout(io.StringIO()):
                self.assertEqual(inventory.reparse(reparse_args), 2)
            self.assertEqual(len(decoder_instances[-1].calls), 3)
            with closing(inventory.open_readonly(output / "catalog.sqlite")) as db:
                self.assertEqual(inventory.summarize(db)["asset_variants"], 2)
                self.assertEqual(json.loads(db.execute("SELECT value FROM metadata WHERE key='reparse'").fetchone()[0])
                                 ["changed_metadata"], 0)
                self.assertTrue(json.loads(db.execute("SELECT value FROM metadata WHERE key='complete'").fetchone()[0]))
            reparse_args.pending_only = True
            with patch.object(inventory.subprocess, "Popen", side_effect=create_decoder), redirect_stdout(io.StringIO()):
                self.assertEqual(inventory.reparse(reparse_args), 2)
            self.assertEqual(len(decoder_instances[-1].calls), 1)
            changed = game / "disc3" / "LO.fpd"
            with changed.open("ab") as file:
                file.write(b"x")
            with self.assertRaisesRegex(ValueError, "source changed"):
                inventory.reparse(reparse_args)

    def test_ui_is_overlapping_use_tag_and_chr_name_is_not_ui(self):
        self.assertEqual(inventory.classify("Texture2D"), "textures")
        self.assertEqual(inventory.classify("SkeletalMesh"), "models")
        self.assertEqual(inventory.classify("SkeletalMesh_VC"), "scene_actors")
        self.assertNotEqual(inventory.classify("SkeletalMesh_VC"), "models")
        self.assertEqual(inventory.classify("ParticleSystemComponent"), "scene_components")
        self.assertEqual(inventory.classify("AudioComponent"), "scene_components")
        self.assertEqual(inventory.ui_reason("bin/xenon/menu/atlas.xxx", "Texture2D"), "package_directory_heuristic")
        self.assertEqual(inventory.ui_reason("bin/xenon/chr/hero_ui.xxx", "Texture2D"), "")
        self.assertEqual(inventory.ui_reason("bin/xenon/obj/prop_ui.xxx", "StaticMesh"), "")
        self.assertEqual(inventory.ui_reason("bin/xenon/chr/hero_ui.xxx", "UIObject"), "export_class")


if __name__ == "__main__":
    unittest.main()

"""Exercise the native inventory decoder with small synthetic UE3 and CPX files."""
import argparse
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest


DEFAULT_DECODER = Path(__file__).resolve().parents[2] / "build" / (
    "asset_decoder.exe" if os.name == "nt" else "asset_decoder"
)
DECODER = Path(os.environ.get("LO_ASSET_DECODER", DEFAULT_DECODER))


def be32(value):
    return struct.pack(">i", value)


def fname(index):
    return struct.pack(">II", index, 0)


def ue_string(value, utf16=False):
    if utf16:
        body = value.encode("utf-16le") + b"\0\0"
        return be32(-len(body) // 2) + body
    body = value.encode("ascii") + b"\0"
    return be32(len(body)) + body


def make_package():
    names = ["None", "Core", "Class", "Texture2D", "贴图😀", "SizeX", "SizeY",
             "Format", "IntProperty", "ByteProperty"]
    encoded_names = b"".join(ue_string(name, utf16=not name.isascii()) + b"\0" * 8 for name in names)
    name_offset = 64
    import_offset = name_offset + len(encoded_names)
    imports = fname(1) + fname(2) + be32(0) + fname(3)
    export_offset = import_offset + len(imports)
    depends_offset = export_offset + 68
    properties = bytearray(be32(0))  # Net index.
    for prop, value in ((5, 64), (6, 32)):
        properties += fname(prop) + fname(8) + be32(4) + be32(0) + be32(value)
    properties += fname(7) + fname(9) + be32(1) + be32(0) + b"\x07"
    properties += fname(0)
    serial_offset = depends_offset
    exports = (be32(-1) + be32(0) + be32(0) + fname(4) + be32(0) + b"\0" * 8 +
               be32(len(properties)) + be32(serial_offset) + be32(0) + be32(0) +
               be32(0) + b"\0" * 16)
    assert len(exports) == 68
    header = (struct.pack(">III", 0x9E2A83C1, 0x002A01A3, depends_offset) +
              ue_string("") + be32(0) + be32(len(names)) + be32(name_offset) +
              be32(1) + be32(export_offset) + be32(1) + be32(import_offset) +
              be32(depends_offset))
    assert len(header) <= name_offset
    result = bytearray(header.ljust(name_offset, b"\0") + encoded_names + imports + exports + properties)
    positions = {"name_offset": 25, "class_ref": export_offset,
                 "export_outer": export_offset + 8, "import_outer": import_offset + 16,
                 "serial_offset": export_offset + 36, "serial_size": export_offset + 32}
    return result, positions


def raw_cpx(package):
    """One uncompressed CPX block with the codec's normal extent table."""
    assert len(package) <= 65536
    block = bytes((255, 0)) + struct.pack("<H", len(package) - 1) + package
    header = bytearray(20)
    header[:3] = b"cpx"
    struct.pack_into("<H", header, 6, 1)
    struct.pack_into("<I", header, 8, len(header) + len(block))
    struct.pack_into("<I", header, 12, len(package))
    struct.pack_into("<I", header, 16, len(header))
    return header + block


def decode_payloads(payloads):
    with tempfile.TemporaryDirectory(prefix="lo-decoder-test-") as tmp:
        archive = Path(tmp) / "synthetic archive.bin"
        buffer = bytearray(b"offset-prefix")
        extents = []
        for payload in payloads:
            extents.append((len(buffer), len(payload)))
            buffer += payload
        archive.write_bytes(buffer)
        lines = [f"{archive}\t{offset}\t{length}" for offset, length in extents]
        lines.extend((f"{archive}\t{len(buffer)+1}\t1", "bad request"))
        process = subprocess.run([str(DECODER)], input="\n".join(lines) + "\n", text=True,
                                 encoding="utf-8", capture_output=True, timeout=15, check=True)
        assert not process.stderr, process.stderr
        rows = [json.loads(line) for line in process.stdout.splitlines()]
        assert len(rows) == len(lines), (len(rows), len(lines))
        assert rows[-2]["status"] == "error" and "extent outside archive" in rows[-2]["error"]
        assert rows[-1]["status"] == "error" and "expected archive path" in rows[-1]["error"]
        return rows[:-2]


class AssetDecoderTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not DECODER.is_file():
            raise unittest.SkipTest(f"native decoder unavailable: {DECODER}")

    def test_raw_and_cpx_decode_unicode_imports_textures(self):
        package, _ = make_package()
        raw, cpx = decode_payloads((package, raw_cpx(package)))
        for row, encoding in ((raw, "raw"), (cpx, "cpx")):
            with self.subTest(encoding=encoding):
                self.assertEqual((row["status"], row["encoding"], row["decoded_size"]),
                                 ("ok", encoding, len(package)))
                self.assertEqual(row["version"], "002a01a3")
                obj = row["exports"][0]
                self.assertEqual((obj["name"], obj["object_path"], obj["class_name"]),
                                 ("贴图😀", "贴图😀", "Texture2D"))
                self.assertEqual((obj["width"], obj["height"], obj["format"]), (64, 32, 7))
                self.assertIsNone(obj["property_error"])
                self.assertEqual((row["imports"][0]["object_name"],
                                  row["imports"][0]["object_path"]), ("Texture2D", "Texture2D"))

    def test_bad_tables_refs_bounds_and_outer_cycles_are_errors(self):
        package, pos = make_package()
        cases = []
        def changed(label, at, value, expected):
            payload = bytearray(package)
            struct.pack_into(">I", payload, at, value & 0xffffffff)
            cases.append((label, payload, expected))
        changed("table offset", pos["name_offset"], len(package) + 1,
                "invalid UE package table offsets")
        changed("class ref", pos["class_ref"], -2, "object reference outside package tables")
        changed("serial bounds", pos["serial_offset"], len(package) + 1,
                "export data outside package")
        changed("export outer cycle", pos["export_outer"], 1,
                "cyclic object outer reference")
        changed("import outer cycle", pos["import_outer"], -1,
                "cyclic object outer reference")
        rows = decode_payloads([case[1] for case in cases])
        for (label, _, expected), row in zip(cases, rows):
            with self.subTest(label=label):
                self.assertEqual(row["status"], "error")
                self.assertIn(expected, row["error"])

    def test_corrupt_cpx_block_rejected(self):
        package, _ = make_package()
        broken = raw_cpx(package)
        struct.pack_into("<I", broken, 16, 21)  # First block begins after the declared header.
        row, = decode_payloads((broken,))
        self.assertEqual((row["status"], row["encoding"], row["error"]),
                         ("error", "cpx", "invalid CPX stream"))

    def test_binary_cpx_pipe_preserves_bytes_and_rejects_corruption(self):
        payload = b"\x00\x0a\x1a\xffFMV\x00\xff\x0a"
        encoded = raw_cpx(payload)
        encoded[3] = 0x7f  # Reserve-size byte is not part of the CPX signature.
        process = subprocess.run([str(DECODER), "--decode-cpx"], input=encoded,
                                 capture_output=True, timeout=15)
        self.assertEqual((process.returncode, process.stdout, process.stderr),
                         (0, payload, b""))

        struct.pack_into("<I", encoded, 16, 21)  # Invalid first block offset.
        process = subprocess.run([str(DECODER), "--decode-cpx"], input=encoded,
                                 capture_output=True, timeout=15)
        self.assertNotEqual(process.returncode, 0)
        self.assertEqual(process.stdout, b"")
        self.assertIn(b"invalid CPX stream", process.stderr)

    def test_unsupported_mode_rejected(self):
        process = subprocess.run([str(DECODER), "--unknown"], input=b"",
                                 capture_output=True, timeout=15)
        self.assertNotEqual(process.returncode, 0)
        self.assertEqual(process.stdout, b"")
        self.assertIn(b"expected no arguments or --decode-cpx", process.stderr)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--decoder", type=Path, default=DECODER)
    args, unittest_args = parser.parse_known_args()
    DECODER = args.decoder
    unittest.main(argv=[__file__, *unittest_args])

"""Generation replacement and rollback using temporary files; no game required."""
import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location("ppc_codegen", Path(__file__).parents[1] / "ppc_codegen.py")
codegen = importlib.util.module_from_spec(spec)
spec.loader.exec_module(codegen)


class CodegenTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name).resolve()
        self.output = self.root / "LostOdysseyRecompLib/ppc"
        self.output.mkdir(parents=True)
        config = self.root / "LostOdysseyRecompLib/config/LostOdysseyRecomp.toml"
        config.parent.mkdir()
        config.write_text('[main]\nout_directory_path="../ppc"\n')
        self.previous = {name: b"previous output" for name in (
            "ppc_config.h", "ppc_context.h", "ppc_recomp_shared.h",
            "ppc_recomp.0.cpp", "ppc_recomp.1.cpp", "ppc_func_mapping.cpp")}
        self.previous["notes.txt"] = b"keep unrelated files"
        for name, data in self.previous.items():
            (self.output / name).write_bytes(data)

    def test_replace_generated_set(self):
        def generate(*args, **kwargs):
            for name in self.previous:
                if name.endswith((".cpp", ".h")) and name != "ppc_recomp.1.cpp":
                    (self.output / name).write_bytes(b"new output")
        with patch.object(codegen.subprocess, "run", side_effect=generate):
            codegen.generate(self.root, self.root / "tool")
        self.assertFalse((self.output / "ppc_recomp.1.cpp").exists())
        self.assertEqual((self.output / "ppc_recomp.0.cpp").read_bytes(), b"new output")
        self.assertEqual((self.output / "notes.txt").read_bytes(), self.previous["notes.txt"])

    def test_failed_generation_restores_previous_set(self):
        for failure in (None, subprocess.CalledProcessError(1, "tool")):
            with self.subTest(failure=failure):
                def incomplete(*args, **kwargs):
                    (self.output / "ppc_recomp.99.cpp").write_bytes(b"partial output")
                    if failure:
                        raise failure
                with patch.object(codegen.subprocess, "run", side_effect=incomplete), self.assertRaises(
                    (ValueError, subprocess.CalledProcessError)
                ):
                    codegen.generate(self.root, self.root / "tool")
                self.assertEqual({p.name: p.read_bytes() for p in self.output.iterdir()}, self.previous)

    def test_barriers_keep_order_and_are_idempotent(self):
        source = self.output / "ppc_recomp.0.cpp"
        source.write_text(
            '#include "ppc_recomp_shared.h"\n'
            'void publish() {\n'
            '\t// lwsync \n\tstore_payload();\n'
            '\t// eieio \n\tstore_mmio();\n'
            '\t// sync \n\tload_result();\n'
            '\t// isync \n}\n', encoding="utf-8")
        self.assertEqual(codegen.insert_barriers(self.output), 3)
        updated = source.read_text(encoding="utf-8")
        self.assertEqual(updated.count("#include <atomic>"), 1)
        self.assertIn('std::atomic_thread_fence(std::memory_order_acq_rel);\n\tstore_payload();', updated)
        self.assertIn('std::atomic_thread_fence(std::memory_order_acq_rel);\n\tstore_mmio();', updated)
        self.assertIn('std::atomic_thread_fence(std::memory_order_seq_cst);\n\tload_result();', updated)
        self.assertIn('\t// isync \n}', updated)
        baseline = source.read_bytes()
        self.assertEqual(codegen.insert_barriers(self.output), 0)
        self.assertEqual(source.read_bytes(), baseline)

    def test_barrier_failure_restores_previous_set(self):
        def invalid_layout(*args, **kwargs):
            for name in self.previous:
                if name.endswith((".cpp", ".h")):
                    (self.output / name).write_text("\t// sync \n", encoding="utf-8")
        with patch.object(codegen.subprocess, "run", side_effect=invalid_layout), self.assertRaisesRegex(
            ValueError, "Unexpected generated layout"
        ):
            codegen.generate(self.root, self.root / "tool")
        self.assertEqual({p.name: p.read_bytes() for p in self.output.iterdir()}, self.previous)


if __name__ == "__main__":
    unittest.main()

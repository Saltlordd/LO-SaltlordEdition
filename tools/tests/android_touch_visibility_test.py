#!/usr/bin/env python3
"""Run physical-controller auto-hide state checks without Android dependencies."""
import argparse
from pathlib import Path
import subprocess
import tempfile


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--javac", default="javac")
    parser.add_argument("--java", default="java")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    model = root / "packaging/android/runtime/src/main/java/io/github/freefrank/lostodyssey/TouchControllerVisibility.java"
    with tempfile.TemporaryDirectory(prefix="lo-touch-visibility-") as directory:
        subprocess.run([args.javac, "-d", directory, str(model),
                        str(Path(__file__).with_name("TouchControllerVisibilityTest.java"))], check=True)
        subprocess.run([args.java, "-cp", directory,
                        "io.github.freefrank.lostodyssey.TouchControllerVisibilityTest"], check=True)


if __name__ == "__main__":
    main()

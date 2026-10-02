#!/usr/bin/env python3
"""Run the Android layout's dependency-free behavioral checks with JDK 17+."""
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
    model = root / "packaging/android/runtime/src/main/java/io/github/freefrank/lostodyssey/TouchControlLayout.java"
    with tempfile.TemporaryDirectory(prefix="lo-touch-layout-") as directory:
        subprocess.run([args.javac, "-d", directory, str(model),
                        str(Path(__file__).with_name("TouchControlLayoutTest.java"))], check=True)
        subprocess.run([args.java, "-cp", directory,
                        "io.github.freefrank.lostodyssey.TouchControlLayoutTest"], check=True)


if __name__ == "__main__":
    main()

"""Give a compiling fixture a fresh output directory on every CTest run."""
import argparse
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--work-dir", required=True, type=Path)
    parser.add_argument("script", type=Path)
    parser.add_argument("arguments", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    args.work_dir.mkdir(parents=True, exist_ok=True)
    root = Path(tempfile.mkdtemp(prefix=args.script.stem + "-", dir=args.work_dir))
    output = root / "output"  # Some fixtures require that this does not exist.
    command = [sys.executable, "-B", str(args.script),
               *[str(output) if value == "{out}" else value for value in args.arguments]]
    print(f"Fixture artifacts: {root}", flush=True)
    return subprocess.run(command, timeout=140).returncode


if __name__ == "__main__":
    raise SystemExit(main())

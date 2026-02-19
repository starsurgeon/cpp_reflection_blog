#!/usr/bin/env python3
import argparse
import json
import subprocess
import sys
from pathlib import Path


def run(cmd: list[str]) -> None:
    subprocess.run(cmd, check=True)


def main() -> int:
    parser = argparse.ArgumentParser(description="Run gcovr and enforce line coverage threshold")
    parser.add_argument("--root", default=".")
    parser.add_argument("--filter", default="jsonserializer")
    parser.add_argument("--build-dir", required=True)
    parser.add_argument("--threshold", type=float, default=70.0)
    parser.add_argument("--json-out", default="coverage-summary.json")
    args = parser.parse_args()

    json_out = Path(args.json_out)
    cmd = [
        "gcovr",
        "--root",
        args.root,
        "--filter",
        args.filter,
        "--object-directory",
        args.build_dir,
        "--json-summary-pretty",
        "--json-summary",
        str(json_out),
    ]
    run(cmd)

    data = json.loads(json_out.read_text(encoding="utf-8"))
    line_percent = float(data["line_percent"])
    print(f"Line coverage: {line_percent:.2f}% (threshold: {args.threshold:.2f}%)")
    if line_percent < args.threshold:
        print("Coverage gate failed", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

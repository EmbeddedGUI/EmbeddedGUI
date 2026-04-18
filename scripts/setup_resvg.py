#!/usr/bin/env python3
"""Install the official resvg CLI into the repo-local tools directory."""

from __future__ import annotations

import argparse
from pathlib import Path

import resvg_tool


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Install repo-local resvg binary")
    parser.add_argument("--install", action="store_true", help="Download/install resvg if missing.")
    parser.add_argument("--force", action="store_true", help="Redownload even if the local binary already exists.")
    parser.add_argument("--print-path", action="store_true", help="Print the resolved resvg path.")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    root_dir = Path(__file__).resolve().parents[1]

    try:
        if args.install:
            resvg_path = resvg_tool.install_resvg(root_dir, force=args.force)
        else:
            resvg_path = resvg_tool.find_resvg_binary(root_dir)
            if resvg_path is None:
                raise RuntimeError(resvg_tool.describe_resvg_missing(root_dir))
    except Exception as exc:
        print("[!!] %s" % exc)
        return 1

    if args.print_path:
        print(resvg_path)
    else:
        print("[OK] resvg: %s" % resvg_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

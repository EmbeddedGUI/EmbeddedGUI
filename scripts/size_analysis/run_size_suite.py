#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
import subprocess
import sys
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent.parent

STEP_TABLE = {
    "hq": {
        "label": "HQ Path Report",
        "script": SCRIPT_DIR / "hq_size_to_doc.py",
        "quick_variants": "line_hq",
    },
    "canvas_path": {
        "label": "Canvas Path Report",
        "script": SCRIPT_DIR / "canvas_path_size_to_doc.py",
        "quick_variants": "rect_fill",
    },
    "canvas_feature": {
        "label": "Canvas Feature Report",
        "script": SCRIPT_DIR / "canvas_feature_size_to_doc.py",
        "quick_variants": "qoi_codec_feature",
    },
    "widget": {
        "label": "Widget Feature Report",
        "script": SCRIPT_DIR / "widget_feature_size_to_doc.py",
        "quick_variants": "slider_widget",
    },
    "preset": {
        "label": "Preset Validation",
        "script": SCRIPT_DIR / "size_preset_validation_to_doc.py",
        "quick_variants": "tiny_rom",
    },
}

FULL_ORDER = ["hq", "canvas_path", "canvas_feature", "widget", "preset"]
QUICK_ORDER = ["hq", "canvas_path", "canvas_feature", "widget", "preset"]


def run_cmd(cmd):
    return subprocess.run(cmd, cwd=PROJECT_ROOT)


def parse_args():
    parser = argparse.ArgumentParser(description="Run size analysis report suite")
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--quick", action="store_true", help="Run a reduced representative subset for each report")
    group.add_argument("--full", action="store_true", help="Run the full default variants for each report")
    parser.add_argument("--only", help="Comma-separated step list: hq,canvas_path,canvas_feature,widget,preset")
    parser.add_argument("--list-steps", action="store_true", help="List available suite steps and exit")
    parser.add_argument("--no-doc", action="store_true", help="Skip markdown report generation in child scripts")
    parser.add_argument("--report-dir", help="Override output directory for generated JSON/Markdown reports")
    parser.add_argument("--jobs", type=int, default=0, help="Parallel variant builds forwarded to child report scripts. 0=auto.")
    return parser.parse_args()


def select_steps(args):
    if args.only:
        selected = []
        for raw_name in args.only.split(","):
            name = raw_name.strip()
            if not name:
                continue
            if name not in STEP_TABLE:
                raise ValueError("Unknown step: %s" % raw_name)
            if name not in selected:
                selected.append(name)
        return selected

    if args.quick:
        return list(QUICK_ORDER)

    return list(FULL_ORDER)


def main():
    args = parse_args()

    if args.list_steps:
        for name in FULL_ORDER:
            print("%s: %s" % (name, STEP_TABLE[name]["label"]))
        return 0

    steps = select_steps(args)
    quick_mode = args.quick and not args.full

    for name in steps:
        step = STEP_TABLE[name]
        cmd = [sys.executable, str(step["script"])]
        if quick_mode:
            cmd.extend(["--variants", step["quick_variants"]])
        if args.no_doc:
            cmd.append("--no-doc")
        if args.report_dir:
            cmd.extend(["--report-dir", args.report_dir])
        if args.jobs is not None:
            cmd.extend(["--jobs", str(args.jobs)])

        print("=== %s ===" % step["label"], flush=True)
        result = run_cmd(cmd)
        if result.returncode != 0:
            return result.returncode

    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as exc:
        print(str(exc), file=sys.stderr)
        sys.exit(1)

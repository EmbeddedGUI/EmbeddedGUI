#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Release readiness check for EmbeddedGUI.

Runs all release-quality checks in sequence: code formatting, unit tests,
full compile check, runtime verification, binary size analysis,
QEMU performance regression, and performance documentation generation.

Usage:
    python scripts/release_check.py
    python scripts/release_check.py --skip perf,perf_doc
    python scripts/release_check.py --keep-going
    python scripts/release_check.py --cmake
    python scripts/release_check.py --skip runtime --keep-going
"""

import os
import sys
import time
import argparse
import subprocess
from pathlib import Path

SCRIPT_DIR = Path(__file__).parent
PROJECT_ROOT = SCRIPT_DIR.parent

# ── Step definitions ──────────────────────────────────────────────────────────
# Each step: (name, description, command_list)
# command_list is built lazily in build_steps() so CLI args can influence it.

ALL_STEP_NAMES = [
    "format",
    "pytest",
    "compile",
    "runtime",
    "size",
    "size_doc",
    "perf",
    "perf_doc",
]

STEP_DESCRIPTIONS = {
    "format":   "Code formatting (clang-format)",
    "pytest":   "UI Designer unit tests (pytest)",
    "compile":  "Full compile check (all examples)",
    "runtime":  "Runtime verification (screenshots)",
    "size":     "Binary size analysis (ELF)",
    "size_doc": "Size report generation",
    "perf":     "QEMU performance regression test",
    "perf_doc": "Performance report generation",
}


def build_steps(args):
    """Build the list of (name, description, command) tuples based on CLI args."""
    py = sys.executable  # use the same Python interpreter

    compile_cmd = [py, str(SCRIPT_DIR / "code_compile_check.py"), "--full-check"]
    if args.bits64:
        compile_cmd.append("--bits64")
    if args.cmake:
        compile_cmd.append("--cmake")

    perf_cmd = [py, str(SCRIPT_DIR / "code_perf_check.py"), "--full-check"]

    steps = [
        ("format",   STEP_DESCRIPTIONS["format"],
         [py, str(SCRIPT_DIR / "code_format.py")]),

        ("pytest",   STEP_DESCRIPTIONS["pytest"],
         [py, "-m", "pytest", "-c", str(SCRIPT_DIR / "ui_designer" / "pyproject.toml"), str(SCRIPT_DIR / "ui_designer" / "tests"), "-v", "--tb=short"]),

        ("compile",  STEP_DESCRIPTIONS["compile"],
         compile_cmd),

        ("runtime",  STEP_DESCRIPTIONS["runtime"],
         [py, str(SCRIPT_DIR / "code_runtime_check.py"), "--full-check"]),

        ("size",     STEP_DESCRIPTIONS["size"],
         [py, str(SCRIPT_DIR / "utils_analysis_elf_size.py")]),

        ("size_doc", STEP_DESCRIPTIONS["size_doc"],
         [py, str(SCRIPT_DIR / "size_to_doc.py")]),

        ("perf",     STEP_DESCRIPTIONS["perf"],
         perf_cmd),

        ("perf_doc", STEP_DESCRIPTIONS["perf_doc"],
         [py, str(SCRIPT_DIR / "perf_to_doc.py")]),
    ]
    return steps


# ── Pretty output helpers ─────────────────────────────────────────────────────

STATUS_PASS = "PASS"
STATUS_FAIL = "FAIL"
STATUS_SKIP = "SKIP"

BANNER_WIDTH = 72


def banner(text):
    print("\n" + "=" * BANNER_WIDTH)
    print(f"  {text}")
    print("=" * BANNER_WIDTH)


def format_duration(seconds):
    """Format seconds into human-readable string."""
    if seconds < 60:
        return f"{seconds:.1f}s"
    minutes = int(seconds) // 60
    secs = seconds - minutes * 60
    if minutes < 60:
        return f"{minutes}m {secs:.0f}s"
    hours = minutes // 60
    minutes = minutes % 60
    return f"{hours}h {minutes}m {secs:.0f}s"


def print_summary(results, total_elapsed):
    """Print a summary table of all step results."""
    print("\n" + "=" * BANNER_WIDTH)
    print("  RELEASE CHECK SUMMARY")
    print("=" * BANNER_WIDTH)

    name_width = max(len(desc) for _, desc, _, _ in results) + 2

    for name, desc, status, elapsed in results:
        if status == STATUS_PASS:
            mark = "[PASS]"
        elif status == STATUS_FAIL:
            mark = "[FAIL]"
        else:
            mark = "[SKIP]"

        time_str = format_duration(elapsed) if elapsed > 0 else "-"
        print(f"  {mark}  {desc:<{name_width}}  {time_str:>10}")

    print("-" * BANNER_WIDTH)
    print(f"  Total time: {format_duration(total_elapsed)}")

    passed = sum(1 for _, _, s, _ in results if s == STATUS_PASS)
    failed = sum(1 for _, _, s, _ in results if s == STATUS_FAIL)
    skipped = sum(1 for _, _, s, _ in results if s == STATUS_SKIP)
    print(f"  {passed} passed, {failed} failed, {skipped} skipped")

    if failed > 0:
        print("\n  ** RELEASE CHECK FAILED **")
    else:
        print("\n  ** ALL CHECKS PASSED **")
    print("=" * BANNER_WIDTH)

    return failed == 0


# ── Main ──────────────────────────────────────────────────────────────────────

def parse_args():
    parser = argparse.ArgumentParser(
        description="EmbeddedGUI release readiness check.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=f"Available steps: {', '.join(ALL_STEP_NAMES)}\n"
               f"\nExamples:\n"
               f"  python scripts/release_check.py\n"
               f"  python scripts/release_check.py --skip perf,perf_doc\n"
               f"  python scripts/release_check.py --keep-going\n"
               f"  python scripts/release_check.py --cmake --skip runtime\n",
    )
    parser.add_argument(
        "--skip",
        type=str,
        default="",
        help="Comma-separated list of steps to skip. "
             f"Available: {', '.join(ALL_STEP_NAMES)}",
    )
    parser.add_argument(
        "--keep-going",
        action="store_true",
        default=False,
        help="Continue executing subsequent steps after a failure "
             "(default: stop on first failure).",
    )
    parser.add_argument(
        "--cmake",
        action="store_true",
        default=False,
        help="Use CMake build system for compile check.",
    )
    parser.add_argument(
        "--bits64",
        action="store_true",
        default=True,
        help="Use 64-bit build (default: enabled).",
    )
    parser.add_argument(
        "--no-bits64",
        action="store_true",
        default=False,
        help="Disable 64-bit build flag.",
    )
    return parser.parse_args()


def main():
    args = parse_args()

    if args.no_bits64:
        args.bits64 = False

    # Parse skip list
    skip_set = set()
    if args.skip:
        for name in args.skip.split(","):
            name = name.strip()
            if name not in ALL_STEP_NAMES:
                print(f"Error: unknown step '{name}'. "
                      f"Available: {', '.join(ALL_STEP_NAMES)}")
                sys.exit(1)
            skip_set.add(name)

    steps = build_steps(args)
    results = []
    had_failure = False
    total_start = time.time()

    for name, desc, cmd in steps:
        if name in skip_set:
            results.append((name, desc, STATUS_SKIP, 0))
            continue

        if had_failure and not args.keep_going:
            results.append((name, desc, STATUS_SKIP, 0))
            continue

        banner(f"[{name}] {desc}")
        print(f"  Command: {' '.join(cmd)}")
        print()

        step_start = time.time()
        try:
            ret = subprocess.run(cmd, cwd=PROJECT_ROOT)
            elapsed = time.time() - step_start
            if ret.returncode == 0:
                results.append((name, desc, STATUS_PASS, elapsed))
            else:
                results.append((name, desc, STATUS_FAIL, elapsed))
                had_failure = True
        except FileNotFoundError as e:
            elapsed = time.time() - step_start
            print(f"\n  Error: {e}")
            results.append((name, desc, STATUS_FAIL, elapsed))
            had_failure = True
        except KeyboardInterrupt:
            elapsed = time.time() - step_start
            results.append((name, desc, STATUS_FAIL, elapsed))
            # Still print summary on Ctrl+C
            total_elapsed = time.time() - total_start
            print_summary(results, total_elapsed)
            sys.exit(130)

    total_elapsed = time.time() - total_start
    success = print_summary(results, total_elapsed)
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()

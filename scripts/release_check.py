#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Release readiness check for EmbeddedGUI.

Runs all release-quality checks in sequence: code formatting, example icon font
explicitness, Keil project sync, full compile check, WASM build, runtime
verification, binary size analysis, size report generation, QEMU performance
regression, performance report generation, and Sphinx documentation build.

Usage:
    python scripts/release_check.py
    python scripts/release_check.py --skip perf,wasm,doc
    python scripts/release_check.py --keep-going
    python scripts/release_check.py --cmake
    python scripts/release_check.py --skip runtime --keep-going
"""

import os
import sys
import time
import argparse
import subprocess
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

SCRIPT_DIR = Path(__file__).parent
PROJECT_ROOT = SCRIPT_DIR.parent

ALL_STEP_NAMES = [
    "format",
    "icon_font",
    "keil_sync",
    "compile",
    "wasm",
    "runtime",
    "dirty_anim",
    "stage_parity",
    "virtual_render",
    "size",
    "perf",
    "doc",
]

STEP_DESCRIPTIONS = {
    "format": "Code formatting (clang-format)",
    "icon_font": "Example icon font explicitness check",
    "keil_sync": "Keil project file sync (src/ vs .uvprojx)",
    "compile": "Full compile check (all examples)",
    "wasm": "WASM demos build",
    "runtime": "Runtime verification (screenshots)",
    "dirty_anim": "Dirty-region animation verification",
    "stage_parity": "Virtual stage showcase parity verification",
    "virtual_render": "HelloVirtual render and interaction workflow",
    "size": "Binary size analysis and documentation",
    "perf": "QEMU performance test and documentation",
    "doc": "Sphinx documentation build",
}

LOCAL_DIRTY_SCENARIO_GROUPS = (
    ("core", ["activity_ring", "analog_clock", "spinner"]),
    ("showcase", ["animated_image", "showcase", "virtual_stage_showcase"]),
)

LOCAL_VIRTUAL_RENDER_SHARDS = 3
LOCAL_VIRTUAL_RENDER_MAKE_JOBS = 4


def build_steps(args):
    """Build the list of (name, description, command) tuples based on CLI args."""
    py = sys.executable
    local_emsdk = PROJECT_ROOT / "tools" / "emsdk"
    if local_emsdk.exists():
        emsdk_path = str(local_emsdk)
    else:
        emsdk_path = os.environ.get("EMSDK_PATH") or os.environ.get("EMSDK")

    compile_cmd = [py, str(SCRIPT_DIR / "code_compile_check.py"), "--full-check"]
    if args.bits64:
        compile_cmd.append("--bits64")
    if args.cmake:
        compile_cmd.append("--cmake")
    compile_cmd.append("--skip-icon-font-check")

    runtime_cmd = [py, str(SCRIPT_DIR / "code_runtime_check.py"), "--full-check"]
    if not args.runtime_include_custom_widgets:
        runtime_cmd.append("--skip-custom-widgets")
    dirty_anim_cmd = [py, str(SCRIPT_DIR / "checks" / "code_dirty_animation_check.py")]
    stage_parity_cmd = [py, str(SCRIPT_DIR / "checks" / "showcase_stage_parity_check.py"), "--timeout", "35", "--jobs", "2"]
    virtual_render_cmd = [
        py,
        str(SCRIPT_DIR / "checks" / "hello_basic_render_workflow.py"),
        "--app",
        "HelloVirtual",
        "--suite",
        "basic",
        "--skip-unit-tests",
    ]
    if args.bits64:
        runtime_cmd.append("--bits64")
        dirty_anim_cmd.append("--bits64")
        stage_parity_cmd.append("--bits64")
        virtual_render_cmd.append("--bits64")

    perf_cmd = [py, str(SCRIPT_DIR / "perf_analysis" / "code_perf_check.py"), "--full-check", "--doc"]

    wasm_cmd = [py, str(SCRIPT_DIR / "web" / "wasm_build_demos.py")]
    if emsdk_path:
        wasm_cmd += ["--emsdk-path", emsdk_path]

    return [
        ("format", STEP_DESCRIPTIONS["format"], [py, str(SCRIPT_DIR / "code_format.py")]),
        ("icon_font", STEP_DESCRIPTIONS["icon_font"], [py, str(SCRIPT_DIR / "checks" / "check_example_icon_font.py")]),
        ("keil_sync", STEP_DESCRIPTIONS["keil_sync"], [py, str(SCRIPT_DIR / "platform" / "keil_project_sync.py")]),
        ("compile", STEP_DESCRIPTIONS["compile"], compile_cmd),
        ("wasm", STEP_DESCRIPTIONS["wasm"], wasm_cmd),
        ("runtime", STEP_DESCRIPTIONS["runtime"], runtime_cmd),
        ("dirty_anim", STEP_DESCRIPTIONS["dirty_anim"], dirty_anim_cmd),
        ("stage_parity", STEP_DESCRIPTIONS["stage_parity"], stage_parity_cmd),
        ("virtual_render", STEP_DESCRIPTIONS["virtual_render"], virtual_render_cmd),
        ("size", STEP_DESCRIPTIONS["size"], [py, str(SCRIPT_DIR / "size_analysis" / "utils_analysis_elf_size.py"), "--case-set", "typical", "--doc"]),
        ("perf", STEP_DESCRIPTIONS["perf"], perf_cmd),
        ("doc", STEP_DESCRIPTIONS["doc"], [py, "-m", "sphinx", "-M", "html", str(PROJECT_ROOT / "doc" / "source"), str(PROJECT_ROOT / "doc" / "build")]),
    ]


def format_command(cmd):
    return " ".join(cmd)


def build_parallel_step_commands(step_name, args):
    py = sys.executable

    if step_name == "dirty_anim":
        commands = []
        for label, scenarios in LOCAL_DIRTY_SCENARIO_GROUPS:
            cmd = [py, str(SCRIPT_DIR / "checks" / "code_dirty_animation_check.py")]
            if args.bits64:
                cmd.append("--bits64")
            for scenario in scenarios:
                cmd.extend(["--scenario", scenario])
            commands.append((label, cmd))
        return commands

    if step_name == "virtual_render":
        commands = []
        for shard_index in range(1, LOCAL_VIRTUAL_RENDER_SHARDS + 1):
            cmd = [
                py,
                str(SCRIPT_DIR / "checks" / "hello_basic_render_workflow.py"),
                "--app",
                "HelloVirtual",
                "--suite",
                "basic",
                "--skip-unit-tests",
                "--shard-count",
                str(LOCAL_VIRTUAL_RENDER_SHARDS),
                "--shard-index",
                str(shard_index),
                "--make-jobs",
                str(LOCAL_VIRTUAL_RENDER_MAKE_JOBS),
            ]
            if args.bits64:
                cmd.append("--bits64")
            commands.append((f"shard-{shard_index}", cmd))
        return commands

    return None


def run_parallel_commands(step_name, commands, env):
    print("  Parallel subcommands:", flush=True)
    for label, cmd in commands:
        print(f"    - [{label}] {format_command(cmd)}", flush=True)

    failures = []
    with ThreadPoolExecutor(max_workers=len(commands)) as executor:
        future_map = {
            executor.submit(
                subprocess.run,
                cmd,
                cwd=PROJECT_ROOT,
                env=env,
                capture_output=True,
                text=True,
            ): (label, cmd)
            for label, cmd in commands
        }

        completed = 0
        total = len(commands)
        for future in as_completed(future_map):
            completed += 1
            label, cmd = future_map[future]
            result = future.result()
            status = "PASS" if result.returncode == 0 else "FAIL"
            print(f"  [{completed}/{total}] {label} {status}", flush=True)
            if result.returncode != 0:
                failures.append((label, cmd, result))

    if not failures:
        return 0

    print(f"\n  {step_name} failed in {len(failures)} subcommand(s):", flush=True)
    for label, cmd, result in failures:
        print(f"  --- {label} ---", flush=True)
        print(f"  Command: {format_command(cmd)}", flush=True)
        stdout_tail = (result.stdout or "").splitlines()[-80:]
        stderr_tail = (result.stderr or "").splitlines()[-80:]
        if stdout_tail:
            print("  stdout:", flush=True)
            for line in stdout_tail:
                print("    " + line, flush=True)
        if stderr_tail:
            print("  stderr:", flush=True)
            for line in stderr_tail:
                print("    " + line, flush=True)

    return 1


STATUS_PASS = "PASS"
STATUS_FAIL = "FAIL"
STATUS_SKIP = "SKIP"
BANNER_WIDTH = 72


def banner(text):
    print("\n" + "=" * BANNER_WIDTH, flush=True)
    print(f"  {text}", flush=True)
    print("=" * BANNER_WIDTH, flush=True)


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
    print("\n" + "=" * BANNER_WIDTH, flush=True)
    print("  RELEASE CHECK SUMMARY", flush=True)
    print("=" * BANNER_WIDTH, flush=True)

    name_width = max(len(desc) for _, desc, _, _ in results) + 2

    for name, desc, status, elapsed in results:
        if status == STATUS_PASS:
            mark = "[PASS]"
        elif status == STATUS_FAIL:
            mark = "[FAIL]"
        else:
            mark = "[SKIP]"

        time_str = format_duration(elapsed) if elapsed > 0 else "-"
        print(f"  {mark}  {desc:<{name_width}}  {time_str:>10}", flush=True)

    print("-" * BANNER_WIDTH, flush=True)
    print(f"  Total time: {format_duration(total_elapsed)}", flush=True)

    passed = sum(1 for _, _, s, _ in results if s == STATUS_PASS)
    failed = sum(1 for _, _, s, _ in results if s == STATUS_FAIL)
    skipped = sum(1 for _, _, s, _ in results if s == STATUS_SKIP)
    print(f"  {passed} passed, {failed} failed, {skipped} skipped", flush=True)

    if failed > 0:
        print("\n  ** RELEASE CHECK FAILED **", flush=True)
    else:
        print("\n  ** ALL CHECKS PASSED **", flush=True)
    print("=" * BANNER_WIDTH, flush=True)

    return failed == 0


def parse_args():
    parser = argparse.ArgumentParser(
        description="EmbeddedGUI release readiness check.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=f"Available steps: {', '.join(ALL_STEP_NAMES)}\n"
        f"\nExamples:\n"
        f"  python scripts/release_check.py\n"
        f"  python scripts/release_check.py --skip perf,wasm,doc\n"
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
        help="Continue executing subsequent steps after a failure (default: stop on first failure).",
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
    parser.add_argument(
        "--runtime-include-custom-widgets",
        action="store_true",
        default=False,
        help="Include HelloCustomWidgets in the runtime step (default: skipped to keep release_check faster).",
    )
    return parser.parse_args()


def main():
    args = parse_args()

    if args.no_bits64:
        args.bits64 = False

    skip_set = set()
    if args.skip:
        for name in args.skip.split(","):
            name = name.strip()
            if name not in ALL_STEP_NAMES:
                print(f"Error: unknown step '{name}'. Available: {', '.join(ALL_STEP_NAMES)}")
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
        print(f"  Command: {' '.join(cmd)}", flush=True)
        print(flush=True)

        step_start = time.time()
        try:
            env = os.environ.copy()
            env.setdefault("PYTHONUNBUFFERED", "1")
            parallel_cmds = build_parallel_step_commands(name, args)
            if parallel_cmds:
                retcode = run_parallel_commands(name, parallel_cmds, env)
                ret = None
            else:
                ret = subprocess.run(cmd, cwd=PROJECT_ROOT, env=env)
                retcode = ret.returncode
            elapsed = time.time() - step_start
            if retcode == 0:
                results.append((name, desc, STATUS_PASS, elapsed))
            else:
                results.append((name, desc, STATUS_FAIL, elapsed))
                had_failure = True
        except FileNotFoundError as exc:
            elapsed = time.time() - step_start
            print(f"\n  Error: {exc}", flush=True)
            results.append((name, desc, STATUS_FAIL, elapsed))
            had_failure = True
        except KeyboardInterrupt:
            elapsed = time.time() - step_start
            results.append((name, desc, STATUS_FAIL, elapsed))
            total_elapsed = time.time() - total_start
            print_summary(results, total_elapsed)
            sys.exit(130)

    total_elapsed = time.time() - total_start
    success = print_summary(results, total_elapsed)
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()


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
    python scripts/release_check.py --scope multi-display
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
    "basic_render",
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
    "basic_render": "HelloBasic render and interaction workflow",
    "virtual_render": "HelloVirtual render and interaction workflow",
    "size": "Binary size analysis and documentation",
    "perf": "QEMU performance test and documentation",
    "doc": "Sphinx documentation build",
}

LOCAL_DIRTY_SCENARIO_GROUPS = (
    ("core", ["activity_ring", "analog_clock", "spinner"]),
    ("showcase", ["animated_image", "showcase", "virtual_stage_showcase"]),
)

LOCAL_BASIC_RENDER_SHARDS = 4
LOCAL_BASIC_RENDER_MAKE_JOBS = 4
LOCAL_VIRTUAL_RENDER_SHARDS = 3
LOCAL_VIRTUAL_RENDER_MAKE_JOBS = 4
LOCAL_RELEASE_RUNTIME_JOBS = 1
LOCAL_RELEASE_SIZE_JOBS = 2
SCOPED_RELEASE_COMPILE_CASE_JOBS = 2
SCOPED_RELEASE_RUNTIME_JOBS = 2
SCOPED_RELEASE_RUNTIME_TIMEOUT = 10
RELEASE_SCOPE_NAMES = ("multi-display",)
RELEASE_SCOPE_OVERVIEWS = {
    "multi-display": {
        "title": "Scoped Profile: multi-display",
        "lines": (
            "Apps: HelloMultiDisplay, HelloMultiDisplayHetero",
            "Runtime checks: frame suite, stage labels, primary click isolation, concurrent activity animations, sub-display tick continuity/reset, shutdown order",
            "Steps: format + scoped compile + scoped runtime + Sphinx dummy doc build",
            "Optional probe: --queue-capacity-probe N forwards to scoped compile/runtime builds",
            "Optional stress: --queue-stress-probe N enables runtime-only burst core-task pressure",
            "Optional stress pacing: --queue-stress-post-gap-probe N inserts ms gap between synthetic burst posts",
            "Optional backpressure probe: --queue-post-retry-probe N / --queue-post-retry-delay-probe N",
            "Optional runtime guardrail: --max-core-task-retries N / --max-single-post-core-task-retries N / --fail-on-full-core-task-queue",
            "Optional peak-time guardrail: --max-shutdown-queue-wait-ms N / --max-shutdown-exec-time-ms N",
        ),
        "step_names": ("format", "compile", "runtime", "doc"),
        "drilldown": (
            "python scripts/code_compile_check.py --scope multi-display --case-jobs 2",
            "python scripts/code_runtime_check.py --scope multi-display --jobs 2 --timeout 10 --keep-screenshots",
            "python -m sphinx -b dummy doc/source doc/build/dummy",
        ),
        "artifacts": (
            "runtime_check_output/HelloMultiDisplay/default/",
            "runtime_check_output/HelloMultiDisplayHetero/default/",
            "doc/build/dummy/",
        ),
    },
}


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

    runtime_cmd = [py, str(SCRIPT_DIR / "code_runtime_check.py"), "--full-check", "--jobs", str(LOCAL_RELEASE_RUNTIME_JOBS)]
    dirty_anim_cmd = [py, str(SCRIPT_DIR / "checks" / "code_dirty_animation_check.py")]
    stage_parity_cmd = [py, str(SCRIPT_DIR / "checks" / "showcase_stage_parity_check.py"), "--timeout", "35", "--jobs", "2"]
    basic_render_cmd = [
        py,
        str(SCRIPT_DIR / "checks" / "hello_basic_render_workflow.py"),
        "--app",
        "HelloBasic",
        "--suite",
        "basic",
        "--skip-unit-tests",
    ]
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
        basic_render_cmd.append("--bits64")
        virtual_render_cmd.append("--bits64")

    perf_cmd = [py, str(SCRIPT_DIR / "perf_analysis" / "code_perf_check.py"), "--full-check", "--doc"]

    wasm_cmd = [py, str(SCRIPT_DIR / "web" / "wasm_build_demos.py")]
    if emsdk_path:
        wasm_cmd += ["--emsdk-path", emsdk_path]

    if args.scope == "multi-display":
        compile_cmd = [
            py,
            str(SCRIPT_DIR / "code_compile_check.py"),
            "--scope",
            "multi-display",
            "--case-jobs",
            str(SCOPED_RELEASE_COMPILE_CASE_JOBS),
        ]
        if args.bits64:
            compile_cmd.append("--bits64")
        if args.cmake:
            compile_cmd.append("--cmake")
        if args.queue_capacity_probe > 0:
            compile_cmd.extend(["--queue-capacity-probe", str(args.queue_capacity_probe)])
        if args.queue_stress_probe > 0:
            compile_cmd.extend(["--queue-stress-probe", str(args.queue_stress_probe)])
        if args.queue_post_retry_probe >= 0:
            compile_cmd.extend(["--queue-post-retry-probe", str(args.queue_post_retry_probe)])
        if args.queue_post_retry_delay_probe >= 0:
            compile_cmd.extend(["--queue-post-retry-delay-probe", str(args.queue_post_retry_delay_probe)])
        if args.queue_stress_post_gap_probe >= 0:
            compile_cmd.extend(["--queue-stress-post-gap-probe", str(args.queue_stress_post_gap_probe)])

        runtime_cmd = [
            py,
            str(SCRIPT_DIR / "code_runtime_check.py"),
            "--scope",
            "multi-display",
            "--jobs",
            str(SCOPED_RELEASE_RUNTIME_JOBS),
            "--timeout",
            str(SCOPED_RELEASE_RUNTIME_TIMEOUT),
        ]
        if args.bits64:
            runtime_cmd.append("--bits64")
        if args.queue_capacity_probe > 0:
            runtime_cmd.extend(["--queue-capacity-probe", str(args.queue_capacity_probe)])
        if args.queue_stress_probe > 0:
            runtime_cmd.extend(["--queue-stress-probe", str(args.queue_stress_probe)])
        if args.queue_post_retry_probe >= 0:
            runtime_cmd.extend(["--queue-post-retry-probe", str(args.queue_post_retry_probe)])
        if args.queue_post_retry_delay_probe >= 0:
            runtime_cmd.extend(["--queue-post-retry-delay-probe", str(args.queue_post_retry_delay_probe)])
        if args.queue_stress_post_gap_probe >= 0:
            runtime_cmd.extend(["--queue-stress-post-gap-probe", str(args.queue_stress_post_gap_probe)])
        if args.max_core_task_retries >= 0:
            runtime_cmd.extend(["--max-core-task-retries", str(args.max_core_task_retries)])
        if args.max_single_post_core_task_retries >= 0:
            runtime_cmd.extend(["--max-single-post-core-task-retries", str(args.max_single_post_core_task_retries)])
        if args.fail_on_full_core_task_queue:
            runtime_cmd.append("--fail-on-full-core-task-queue")
        if args.max_shutdown_queue_wait_ms >= 0:
            runtime_cmd.extend(["--max-shutdown-queue-wait-ms", str(args.max_shutdown_queue_wait_ms)])
        if args.max_shutdown_exec_time_ms >= 0:
            runtime_cmd.extend(["--max-shutdown-exec-time-ms", str(args.max_shutdown_exec_time_ms)])

        doc_cmd = [
            py,
            "-m",
            "sphinx",
            "-b",
            "dummy",
            str(PROJECT_ROOT / "doc" / "source"),
            str(PROJECT_ROOT / "doc" / "build" / "dummy"),
        ]

        return [
            ("format", STEP_DESCRIPTIONS["format"], [py, str(SCRIPT_DIR / "code_format.py")]),
            ("compile", "Compile check (multi-display apps)", compile_cmd),
            ("runtime", "Runtime verification (frames/stages/self-checks/shutdown)", runtime_cmd),
            ("doc", "Sphinx documentation build (dummy, multi-display docs)", doc_cmd),
        ]

    return [
        ("format", STEP_DESCRIPTIONS["format"], [py, str(SCRIPT_DIR / "code_format.py")]),
        ("icon_font", STEP_DESCRIPTIONS["icon_font"], [py, str(SCRIPT_DIR / "checks" / "check_example_icon_font.py")]),
        ("keil_sync", STEP_DESCRIPTIONS["keil_sync"], [py, str(SCRIPT_DIR / "platform" / "keil_project_sync.py")]),
        ("compile", STEP_DESCRIPTIONS["compile"], compile_cmd),
        ("wasm", STEP_DESCRIPTIONS["wasm"], wasm_cmd),
        ("runtime", STEP_DESCRIPTIONS["runtime"], runtime_cmd),
        ("dirty_anim", STEP_DESCRIPTIONS["dirty_anim"], dirty_anim_cmd),
        ("stage_parity", STEP_DESCRIPTIONS["stage_parity"], stage_parity_cmd),
        ("basic_render", STEP_DESCRIPTIONS["basic_render"], basic_render_cmd),
        ("virtual_render", STEP_DESCRIPTIONS["virtual_render"], virtual_render_cmd),
        ("size", STEP_DESCRIPTIONS["size"], [py, str(SCRIPT_DIR / "size_analysis" / "utils_analysis_elf_size.py"), "--case-set", "full", "--jobs", str(LOCAL_RELEASE_SIZE_JOBS), "--doc"]),
        ("perf", STEP_DESCRIPTIONS["perf"], perf_cmd),
        ("doc", STEP_DESCRIPTIONS["doc"], [py, "-m", "sphinx", "-M", "html", str(PROJECT_ROOT / "doc" / "source"), str(PROJECT_ROOT / "doc" / "build")]),
    ]


def format_command(cmd):
    return " ".join(cmd)


def dedupe_commands(commands):
    seen = set()
    deduped = []
    for command in commands:
        if not command or command in seen:
            continue
        seen.add(command)
        deduped.append(command)
    return tuple(deduped)


def get_scope_drilldown_commands(scope):
    if scope is None:
        return ()
    overview = RELEASE_SCOPE_OVERVIEWS.get(scope)
    if not overview:
        return ()
    return tuple(overview.get("drilldown", ()))


def get_scope_artifact_paths(scope):
    if scope is None:
        return ()
    overview = RELEASE_SCOPE_OVERVIEWS.get(scope)
    if not overview:
        return ()
    return tuple(overview.get("artifacts", ()))


def get_scope_step_names(scope):
    if scope is None:
        return ()
    overview = RELEASE_SCOPE_OVERVIEWS.get(scope)
    if not overview:
        return ()
    return tuple(overview.get("step_names", ()))


def build_failure_hint_commands(step_name, cmd, args):
    commands = [format_command(cmd)]
    if args.scope is not None:
        scope_drilldown = get_scope_drilldown_commands(args.scope)
        if step_name in {"compile", "runtime", "doc"}:
            commands.extend(scope_drilldown)
    return dedupe_commands(commands)


def build_rerun_current_step_command(step_name, args):
    py = Path(sys.executable).name or "python"
    command = [py, "scripts/release_check.py"]
    if args.scope is not None:
        command.extend(["--scope", args.scope])
    command.extend(["--only", step_name])
    if args.cmake:
        command.append("--cmake")
    if not args.bits64:
        command.append("--no-bits64")
    if args.queue_capacity_probe > 0:
        command.extend(["--queue-capacity-probe", str(args.queue_capacity_probe)])
    if args.queue_stress_probe > 0:
        command.extend(["--queue-stress-probe", str(args.queue_stress_probe)])
    if args.queue_post_retry_probe >= 0:
        command.extend(["--queue-post-retry-probe", str(args.queue_post_retry_probe)])
    if args.queue_post_retry_delay_probe >= 0:
        command.extend(["--queue-post-retry-delay-probe", str(args.queue_post_retry_delay_probe)])
    if args.queue_stress_post_gap_probe >= 0:
        command.extend(["--queue-stress-post-gap-probe", str(args.queue_stress_post_gap_probe)])
    if args.max_core_task_retries >= 0:
        command.extend(["--max-core-task-retries", str(args.max_core_task_retries)])
    if args.max_single_post_core_task_retries >= 0:
        command.extend(["--max-single-post-core-task-retries", str(args.max_single_post_core_task_retries)])
    if args.fail_on_full_core_task_queue:
        command.append("--fail-on-full-core-task-queue")
    if args.max_shutdown_queue_wait_ms >= 0:
        command.extend(["--max-shutdown-queue-wait-ms", str(args.max_shutdown_queue_wait_ms)])
    if args.max_shutdown_exec_time_ms >= 0:
        command.extend(["--max-shutdown-exec-time-ms", str(args.max_shutdown_exec_time_ms)])
    return format_command(command)


def build_failure_hint_artifacts(step_name, args):
    if args.scope is None:
        return ()
    scope_artifacts = get_scope_artifact_paths(args.scope)
    if step_name == "runtime":
        return dedupe_commands(path for path in scope_artifacts if path.startswith("runtime_check_output/"))
    if step_name == "doc":
        return dedupe_commands(path for path in scope_artifacts if path.startswith("doc/build/"))
    if step_name == "compile":
        return ()
    return ()


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

    if step_name == "basic_render":
        commands = []
        for shard_index in range(1, LOCAL_BASIC_RENDER_SHARDS + 1):
            cmd = [
                py,
                str(SCRIPT_DIR / "checks" / "hello_basic_render_workflow.py"),
                "--app",
                "HelloBasic",
                "--suite",
                "basic",
                "--skip-unit-tests",
                "--shard-count",
                str(LOCAL_BASIC_RENDER_SHARDS),
                "--shard-index",
                str(shard_index),
                "--make-jobs",
                str(LOCAL_BASIC_RENDER_MAKE_JOBS),
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

    retried_failures = []
    print(f"\n  Retrying failed {step_name} subcommand(s) serially once...", flush=True)
    for label, cmd, _ in failures:
        print(f"  [retry] {label}", flush=True)
        retry_result = subprocess.run(
            cmd,
            cwd=PROJECT_ROOT,
            env=env,
            capture_output=True,
            text=True,
        )
        if retry_result.returncode == 0:
            print(f"  [retry] {label} PASS", flush=True)
            continue
        print(f"  [retry] {label} FAIL", flush=True)
        retried_failures.append((label, cmd, retry_result))

    failures = retried_failures
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


def print_scope_overview(scope):
    overview = RELEASE_SCOPE_OVERVIEWS.get(scope)
    if not overview:
        return

    banner(overview["title"])
    for line in overview["lines"]:
        print(f"  {line}", flush=True)

    drilldown = overview.get("drilldown", ())
    if drilldown:
        print("  Drill-down commands:", flush=True)
        for command in drilldown:
            print(f"    {command}", flush=True)
    step_names = overview.get("step_names", ())
    if step_names:
        print(f"  Step filters (--only): {', '.join(step_names)}", flush=True)
    artifacts = overview.get("artifacts", ())
    if artifacts:
        print("  Key artifacts:", flush=True)
        for path in artifacts:
            print(f"    {path}", flush=True)


def print_active_filters(args, steps):
    active_step_names = [name for name, _, _ in steps]
    if args.only:
        print(f"  Active step filter (--only): {', '.join(active_step_names)}", flush=True)
    elif args.skip:
        print(f"  Active skip filter (--skip): {args.skip}", flush=True)


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

    name_width = max(len(item["desc"]) for item in results) + 2

    for item in results:
        name = item["name"]
        desc = item["desc"]
        status = item["status"]
        elapsed = item["elapsed"]
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

    passed = sum(1 for item in results if item["status"] == STATUS_PASS)
    failed = sum(1 for item in results if item["status"] == STATUS_FAIL)
    skipped = sum(1 for item in results if item["status"] == STATUS_SKIP)
    print(f"  {passed} passed, {failed} failed, {skipped} skipped", flush=True)

    if failed > 0:
        print("\n  Failed step rerun commands:", flush=True)
        for item in results:
            if item["status"] != STATUS_FAIL:
                continue
            print(f"  [{item['name']}] {item['desc']}", flush=True)
            rerun_current = item.get("rerun_current_step")
            if rerun_current:
                print(f"    {rerun_current}", flush=True)
            for command in item.get("hint_commands", ()):
                print(f"    {command}", flush=True)
            artifact_paths = item.get("artifact_paths", ())
            if artifact_paths:
                print("    Artifacts:", flush=True)
                for path in artifact_paths:
                    print(f"      {path}", flush=True)
        print("\n  ** RELEASE CHECK FAILED **", flush=True)
    else:
        passed_artifacts = []
        for item in results:
            if item["status"] != STATUS_PASS:
                continue
            for path in item.get("artifact_paths", ()):
                passed_artifacts.append((item["name"], path))
        if passed_artifacts:
            print("\n  Key artifacts from passed steps:", flush=True)
            for step_name, path in passed_artifacts:
                print(f"  [{step_name}] {path}", flush=True)
        print("\n  ** ALL CHECKS PASSED **", flush=True)
    print("=" * BANNER_WIDTH, flush=True)

    return failed == 0


def parse_args():
    scope_epilog_lines = []
    for scope_name in RELEASE_SCOPE_NAMES:
        overview = RELEASE_SCOPE_OVERVIEWS.get(scope_name)
        if not overview:
            continue
        scope_epilog_lines.append(f"  {scope_name}:")
        for line in overview["lines"]:
            scope_epilog_lines.append(f"    {line}")
        step_names = overview.get("step_names", ())
        if step_names:
            scope_epilog_lines.append(f"    Step filters (--only): {', '.join(step_names)}")
        for command in overview.get("drilldown", ()):
            scope_epilog_lines.append(f"    Drill-down: {command}")
        for path in overview.get("artifacts", ()):
            scope_epilog_lines.append(f"    Artifact: {path}")

    scope_epilog = ""
    if scope_epilog_lines:
        scope_epilog = "\nScoped profiles:\n" + "\n".join(scope_epilog_lines)

    parser = argparse.ArgumentParser(
        description="EmbeddedGUI release readiness check.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=f"Available steps: {', '.join(ALL_STEP_NAMES)}\n"
        f"\nExamples:\n"
        f"  python scripts/release_check.py\n"
        f"  python scripts/release_check.py --skip perf,wasm,doc\n"
        f"  python scripts/release_check.py --scope multi-display\n"
        f"  python scripts/release_check.py --scope multi-display --only runtime\n"
        f"  python scripts/release_check.py --scope multi-display --only compile,runtime --queue-capacity-probe 1\n"
        f"  python scripts/release_check.py --scope multi-display --only compile,runtime --queue-stress-probe 8\n"
        f"  python scripts/release_check.py --scope multi-display --only compile,runtime --queue-stress-probe 8 --queue-stress-post-gap-probe 2\n"
        f"  python scripts/release_check.py --scope multi-display --only compile,runtime --queue-post-retry-probe 0\n"
        f"  python scripts/release_check.py --scope multi-display --only runtime --max-core-task-retries 0\n"
        f"  python scripts/release_check.py --scope multi-display --only runtime --max-single-post-core-task-retries 0\n"
        f"  python scripts/release_check.py --scope multi-display --only runtime --max-shutdown-queue-wait-ms 2\n"
        f"  python scripts/release_check.py --scope multi-display --only runtime --max-shutdown-exec-time-ms 2\n"
        f"  python scripts/release_check.py --keep-going\n"
        f"  python scripts/release_check.py --cmake --skip runtime\n"
        f"{scope_epilog}\n",
    )
    parser.add_argument(
        "--scope",
        choices=RELEASE_SCOPE_NAMES,
        default=None,
        help="Run one targeted release profile instead of the full release pipeline.",
    )
    parser.add_argument(
        "--only",
        type=str,
        default="",
        help="Comma-separated list of steps to run exclusively. "
        f"Available: {', '.join(ALL_STEP_NAMES)}",
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
        "--queue-capacity-probe",
        type=int,
        default=0,
        help="Only for scoped multi-display checks: forward -DEGUI_PORT_PC_CORE_TASK_QUEUE_CAPACITY=N into compile/runtime commands (0=disabled).",
    )
    parser.add_argument(
        "--queue-stress-probe",
        type=int,
        default=0,
        help="Only for scoped multi-display checks: forward -DEGUI_MULTI_DISPLAY_CORE_TASK_STRESS_BURST_COUNT=N into compile/runtime commands (0=disabled).",
    )
    parser.add_argument(
        "--queue-post-retry-probe",
        type=int,
        default=-1,
        help="Only for scoped multi-display checks: forward -DEGUI_PORT_PC_CORE_TASK_POST_RETRY_COUNT=N into compile/runtime commands (-1=disabled, 0=disable retries).",
    )
    parser.add_argument(
        "--queue-post-retry-delay-probe",
        type=int,
        default=-1,
        help="Only for scoped multi-display checks: forward -DEGUI_PORT_PC_CORE_TASK_POST_RETRY_DELAY_MS=N into compile/runtime commands (-1=disabled, 0=no delay).",
    )
    parser.add_argument(
        "--queue-stress-post-gap-probe",
        type=int,
        default=-1,
        help="Only for scoped multi-display checks: forward -DEGUI_MULTI_DISPLAY_CORE_TASK_STRESS_POST_GAP_MS=N into compile/runtime commands (-1=disabled, 0=no gap).",
    )
    parser.add_argument(
        "--max-core-task-retries",
        type=int,
        default=-1,
        help="Only for scoped multi-display runtime checks: fail if any display exceeds this retry count (-1=disabled).",
    )
    parser.add_argument(
        "--max-single-post-core-task-retries",
        type=int,
        default=-1,
        help="Only for scoped multi-display runtime checks: fail if any display exceeds this single-post retry burst (-1=disabled).",
    )
    parser.add_argument(
        "--fail-on-full-core-task-queue",
        action="store_true",
        default=False,
        help="Only for scoped multi-display runtime checks: fail if any display peak_queue reaches queue_capacity.",
    )
    parser.add_argument(
        "--max-shutdown-queue-wait-ms",
        type=int,
        default=-1,
        help="Only for scoped multi-display runtime checks: fail if any display shutdown max_queue_wait_ms exceeds this threshold (-1=disabled).",
    )
    parser.add_argument(
        "--max-shutdown-exec-time-ms",
        type=int,
        default=-1,
        help="Only for scoped multi-display runtime checks: fail if any display shutdown max_exec_time_ms exceeds this threshold (-1=disabled).",
    )
    return parser.parse_args()


def main():
    args = parse_args()

    if args.no_bits64:
        args.bits64 = False

    if args.queue_capacity_probe < 0:
        print("Error: --queue-capacity-probe must be >= 0.")
        sys.exit(1)
    if args.queue_stress_probe < 0:
        print("Error: --queue-stress-probe must be >= 0.")
        sys.exit(1)
    if args.queue_post_retry_probe < -1:
        print("Error: --queue-post-retry-probe must be >= -1.")
        sys.exit(1)
    if args.queue_post_retry_delay_probe < -1:
        print("Error: --queue-post-retry-delay-probe must be >= -1.")
        sys.exit(1)
    if args.queue_stress_post_gap_probe < -1:
        print("Error: --queue-stress-post-gap-probe must be >= -1.")
        sys.exit(1)
    if args.max_core_task_retries < -1:
        print("Error: --max-core-task-retries must be >= -1.")
        sys.exit(1)
    if args.max_single_post_core_task_retries < -1:
        print("Error: --max-single-post-core-task-retries must be >= -1.")
        sys.exit(1)
    if args.max_shutdown_queue_wait_ms < -1:
        print("Error: --max-shutdown-queue-wait-ms must be >= -1.")
        sys.exit(1)
    if args.max_shutdown_exec_time_ms < -1:
        print("Error: --max-shutdown-exec-time-ms must be >= -1.")
        sys.exit(1)
    if args.queue_capacity_probe > 0 and args.scope != "multi-display":
        print("Error: --queue-capacity-probe currently requires --scope multi-display.")
        sys.exit(1)
    if args.queue_stress_probe > 0 and args.scope != "multi-display":
        print("Error: --queue-stress-probe currently requires --scope multi-display.")
        sys.exit(1)
    if args.queue_post_retry_probe >= 0 and args.scope != "multi-display":
        print("Error: --queue-post-retry-probe currently requires --scope multi-display.")
        sys.exit(1)
    if args.queue_post_retry_delay_probe >= 0 and args.scope != "multi-display":
        print("Error: --queue-post-retry-delay-probe currently requires --scope multi-display.")
        sys.exit(1)
    if args.queue_stress_post_gap_probe >= 0 and args.scope != "multi-display":
        print("Error: --queue-stress-post-gap-probe currently requires --scope multi-display.")
        sys.exit(1)
    if args.max_core_task_retries >= 0 and args.scope != "multi-display":
        print("Error: --max-core-task-retries currently requires --scope multi-display.")
        sys.exit(1)
    if args.max_single_post_core_task_retries >= 0 and args.scope != "multi-display":
        print("Error: --max-single-post-core-task-retries currently requires --scope multi-display.")
        sys.exit(1)
    if args.fail_on_full_core_task_queue and args.scope != "multi-display":
        print("Error: --fail-on-full-core-task-queue currently requires --scope multi-display.")
        sys.exit(1)
    if args.max_shutdown_queue_wait_ms >= 0 and args.scope != "multi-display":
        print("Error: --max-shutdown-queue-wait-ms currently requires --scope multi-display.")
        sys.exit(1)
    if args.max_shutdown_exec_time_ms >= 0 and args.scope != "multi-display":
        print("Error: --max-shutdown-exec-time-ms currently requires --scope multi-display.")
        sys.exit(1)

    if args.only and args.skip:
        print("Error: --only and --skip cannot be used together.")
        sys.exit(1)

    skip_set = set()
    if args.skip:
        for name in args.skip.split(","):
            name = name.strip()
            if name not in ALL_STEP_NAMES:
                print(f"Error: unknown step '{name}'. Available: {', '.join(ALL_STEP_NAMES)}")
                sys.exit(1)
            skip_set.add(name)

    steps = build_steps(args)
    available_step_names = [name for name, _, _ in steps]

    if args.only:
        only_set = set()
        for name in args.only.split(","):
            name = name.strip()
            if name not in ALL_STEP_NAMES:
                print(f"Error: unknown step '{name}'. Available: {', '.join(ALL_STEP_NAMES)}")
                sys.exit(1)
            if name not in available_step_names:
                print(f"Error: step '{name}' is not available for the current profile. Available here: {', '.join(available_step_names)}")
                sys.exit(1)
            only_set.add(name)
        steps = [step for step in steps if step[0] in only_set]

    results = []
    had_failure = False
    total_start = time.time()

    if args.scope is not None:
        print_scope_overview(args.scope)
    if args.only or args.skip:
        print_active_filters(args, steps)

    for name, desc, cmd in steps:
        hint_commands = build_failure_hint_commands(name, cmd, args)
        artifact_paths = build_failure_hint_artifacts(name, args)
        rerun_current_step = build_rerun_current_step_command(name, args)
        if name in skip_set:
            results.append({"name": name, "desc": desc, "status": STATUS_SKIP, "elapsed": 0, "hint_commands": hint_commands, "artifact_paths": artifact_paths, "rerun_current_step": rerun_current_step})
            continue

        if had_failure and not args.keep_going:
            results.append({"name": name, "desc": desc, "status": STATUS_SKIP, "elapsed": 0, "hint_commands": hint_commands, "artifact_paths": artifact_paths, "rerun_current_step": rerun_current_step})
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
                results.append({"name": name, "desc": desc, "status": STATUS_PASS, "elapsed": elapsed, "hint_commands": hint_commands, "artifact_paths": artifact_paths, "rerun_current_step": rerun_current_step})
            else:
                results.append({"name": name, "desc": desc, "status": STATUS_FAIL, "elapsed": elapsed, "hint_commands": hint_commands, "artifact_paths": artifact_paths, "rerun_current_step": rerun_current_step})
                had_failure = True
        except FileNotFoundError as exc:
            elapsed = time.time() - step_start
            print(f"\n  Error: {exc}", flush=True)
            results.append({"name": name, "desc": desc, "status": STATUS_FAIL, "elapsed": elapsed, "hint_commands": hint_commands, "artifact_paths": artifact_paths, "rerun_current_step": rerun_current_step})
            had_failure = True
        except KeyboardInterrupt:
            elapsed = time.time() - step_start
            results.append({"name": name, "desc": desc, "status": STATUS_FAIL, "elapsed": elapsed, "hint_commands": hint_commands, "artifact_paths": artifact_paths, "rerun_current_step": rerun_current_step})
            total_elapsed = time.time() - total_start
            print_summary(results, total_elapsed)
            sys.exit(130)

    total_elapsed = time.time() - total_start
    success = print_summary(results, total_elapsed)
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()


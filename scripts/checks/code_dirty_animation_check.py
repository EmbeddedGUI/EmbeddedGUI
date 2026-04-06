#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Dirty-region animation regression checks for representative examples.
"""

import argparse
import hashlib
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Optional

SCRIPT_DIR = Path(__file__).resolve().parent
SCRIPTS_ROOT = SCRIPT_DIR.parent
ROOT_DIR = SCRIPTS_ROOT.parent
PERF_ANALYSIS_DIR = SCRIPTS_ROOT / "perf_analysis"

if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))
if str(SCRIPTS_ROOT) not in sys.path:
    sys.path.insert(0, str(SCRIPTS_ROOT))
if str(PERF_ANALYSIS_DIR) not in sys.path:
    sys.path.insert(0, str(PERF_ANALYSIS_DIR))

import code_runtime_check as runtime_check
import dirty_region_stats_report as dirty_stats_report

DEFAULT_OUTPUT_DIR = ROOT_DIR / runtime_check.SCREENSHOT_DIR / "dirty_animation_check"


@dataclass(frozen=True)
class Scenario:
    name: str
    app: str
    app_sub: Optional[str]
    duration: int
    fps: int
    min_unique_frames: int
    min_partial_frames: int
    max_avg_partial_ratio: float
    extra_cflags: str = ""
    clock_scale: int = 1


SCENARIOS = (
    Scenario(
        name="activity_ring",
        app="HelloBasic",
        app_sub="activity_ring",
        duration=4,
        fps=8,
        min_unique_frames=8,
        min_partial_frames=8,
        max_avg_partial_ratio=20.0,
        extra_cflags="-DEGUI_EXAMPLE_DIRTY_ANIMATION_CHECK=1",
    ),
    Scenario(
        name="analog_clock",
        app="HelloBasic",
        app_sub="analog_clock",
        duration=4,
        fps=8,
        min_unique_frames=8,
        min_partial_frames=8,
        max_avg_partial_ratio=12.0,
        extra_cflags="-DEGUI_EXAMPLE_DIRTY_ANIMATION_CHECK=1",
    ),
    Scenario(
        name="spinner",
        app="HelloBasic",
        app_sub="spinner",
        duration=4,
        fps=8,
        min_unique_frames=8,
        min_partial_frames=20,
        max_avg_partial_ratio=8.0,
    ),
    Scenario(
        name="animated_image",
        app="HelloBasic",
        app_sub="animated_image",
        duration=4,
        fps=8,
        min_unique_frames=2,
        min_partial_frames=20,
        max_avg_partial_ratio=15.0,
    ),
    Scenario(
        name="showcase",
        app="HelloShowcase",
        app_sub=None,
        duration=4,
        fps=6,
        min_unique_frames=8,
        min_partial_frames=20,
        max_avg_partial_ratio=8.0,
    ),
    Scenario(
        name="virtual_stage_showcase",
        app="HelloVirtual",
        app_sub="virtual_stage_showcase",
        duration=4,
        fps=6,
        min_unique_frames=8,
        min_partial_frames=12,
        max_avg_partial_ratio=26.0,
        extra_cflags="-DEGUI_EXAMPLE_DIRTY_ANIMATION_CHECK=1",
    ),
)


def parse_args():
    parser = argparse.ArgumentParser(description="Check representative animations for partial dirty-region refresh")
    parser.add_argument(
        "--scenario",
        action="append",
        help="Run only the named scenario. Available: %s" % ", ".join(s.name for s in SCENARIOS),
    )
    parser.add_argument("--bits64", action="store_true", help="Build for 64-bit")
    parser.add_argument("--output-dir", default=str(DEFAULT_OUTPUT_DIR), help="Directory used for logs and captured frames")
    return parser.parse_args()


def get_scenarios(selected_names):
    if not selected_names:
        return list(SCENARIOS)

    scenario_map = {scenario.name: scenario for scenario in SCENARIOS}
    missing = [name for name in selected_names if name not in scenario_map]
    if missing:
        raise ValueError("unknown scenario(s): %s" % ", ".join(missing))

    return [scenario_map[name] for name in selected_names]


def build_user_cflags(extra_cflags):
    flags = ["-DEGUI_CONFIG_DEBUG_DIRTY_REGION_STATS=1"]
    if extra_cflags:
        flags.append(extra_cflags)
    return " ".join(flags)


def get_recording_paths(output_root, scenario):
    scenario_dir = output_root / scenario.name
    frames_dir = scenario_dir / "frames"
    log_path = scenario_dir / "dirty_stats.log"
    return scenario_dir, frames_dir, log_path


def clean_frames(frames_dir):
    frames_dir.mkdir(parents=True, exist_ok=True)
    for frame_file in frames_dir.glob("frame_*.png"):
        frame_file.unlink()


def hash_frame(frame_path):
    return hashlib.sha256(frame_path.read_bytes()).hexdigest()


def collect_frame_hashes(frames_dir):
    frame_files = sorted(frames_dir.glob("frame_*.png"))
    unique_hashes = {hash_frame(frame_path) for frame_path in frame_files}
    return frame_files, unique_hashes


def get_build_output_dir(scenario, bits64):
    return runtime_check.get_runtime_build_output_dir(
        scenario.app,
        app_sub=scenario.app_sub,
        bits64=bits64,
        user_cflags=build_user_cflags(scenario.extra_cflags),
        recording_test=False,
    )


def run_recording(scenario, bits64, frames_dir, log_path):
    build_output_dir = get_build_output_dir(scenario, bits64)
    exe_path = runtime_check.get_runtime_executable_path(build_output_dir)
    resource_path = runtime_check.get_runtime_resource_path(build_output_dir)

    command = [
        str(exe_path),
        str(resource_path),
        "--record",
        str(frames_dir),
        str(scenario.fps),
        str(scenario.duration),
        "--speed",
        "1",
        "--clock-scale",
        str(scenario.clock_scale),
        "--headless",
    ]

    result = subprocess.run(
        command,
        cwd=ROOT_DIR,
        timeout=scenario.duration + 12,
        capture_output=True,
        text=True,
        **runtime_check.get_windows_hidden_run_kwargs()
    )

    combined_output = "%s\n%s" % ((result.stdout or ""), (result.stderr or ""))
    log_path.write_text(combined_output, encoding="utf-8")

    if result.returncode != 0:
        stderr_msg = result.stderr.strip() if result.stderr else ""
        message = "exit code %d%s" % (result.returncode, (": " + stderr_msg) if stderr_msg else "")
        return False, message

    return True, combined_output


def summarize_scenario(output_root, scenario):
    _, frames_dir, log_path = get_recording_paths(output_root, scenario)

    entries = dirty_stats_report.parse_log_entries(log_path)
    if not entries:
        return False, "no DIRTY_REGION_STATS lines found", None

    summary = dirty_stats_report.summarize_entries(entries)
    frame_files, unique_hashes = collect_frame_hashes(frames_dir)

    if not frame_files:
        return False, "no frames generated", None

    if len(unique_hashes) < scenario.min_unique_frames:
        return False, "unique frames too few: %d < %d" % (len(unique_hashes), scenario.min_unique_frames), None

    if summary["partial_frames"] < scenario.min_partial_frames:
        return False, "partial dirty frames too few: %d < %d" % (summary["partial_frames"], scenario.min_partial_frames), None

    if summary["avg_partial_dirty_ratio_percent"] > scenario.max_avg_partial_ratio:
        return False, "avg partial dirty ratio too high: %.2f%% > %.2f%%" % (
            summary["avg_partial_dirty_ratio_percent"],
            scenario.max_avg_partial_ratio,
        ), None

    return True, "ok", {
        "entries": entries,
        "summary": summary,
        "frame_files": frame_files,
        "unique_frame_count": len(unique_hashes),
        "log_path": log_path,
    }


def write_report(output_root, report_sources):
    if not report_sources:
        return

    report_path = output_root / "dirty_animation_report.md"
    report_text = dirty_stats_report.generate_markdown_report("EmbeddedGUI Dirty Animation Check", report_sources)
    report_path.write_text(report_text, encoding="utf-8")


def main():
    args = parse_args()

    try:
        scenarios = get_scenarios(args.scenario)
    except ValueError as exc:
        print("ERROR: %s" % exc)
        return 1

    output_root = Path(args.output_dir)
    if not output_root.is_absolute():
        output_root = ROOT_DIR / output_root
    output_root.mkdir(parents=True, exist_ok=True)

    report_sources = []
    all_passed = True

    print("== Dirty Animation Check ==")
    for scenario in scenarios:
        print("\n" + "=" * 60)
        print("Scenario: %s (%s%s)" % (
            scenario.name,
            scenario.app,
            ("/" + scenario.app_sub) if scenario.app_sub else "",
        ))
        print("=" * 60)

        scenario_dir, frames_dir, log_path = get_recording_paths(output_root, scenario)
        scenario_dir.mkdir(parents=True, exist_ok=True)
        clean_frames(frames_dir)

        compiled = runtime_check.compile_app(
            scenario.app,
            scenario.app_sub,
            args.bits64,
            user_cflags=build_user_cflags(scenario.extra_cflags),
            recording_test=False,
            build_output_dir=get_build_output_dir(scenario, args.bits64),
        )
        if not compiled:
            print("[FAIL] compile failed")
            all_passed = False
            continue

        try:
            success, run_message = run_recording(scenario, args.bits64, frames_dir, log_path)
        except subprocess.TimeoutExpired:
            print("[FAIL] timeout after %ds" % (scenario.duration + 12))
            all_passed = False
            continue

        if not success:
            print("[FAIL] %s" % run_message)
            all_passed = False
            continue

        success, verify_message, stats = summarize_scenario(output_root, scenario)
        if not success:
            print("[FAIL] %s (log=%s)" % (verify_message, log_path.relative_to(ROOT_DIR)))
            all_passed = False
            continue

        summary = stats["summary"]
        print(
            "[PASS] frames=%d unique=%d partial=%d avg_partial=%.2f%% log=%s" % (
                len(stats["frame_files"]),
                stats["unique_frame_count"],
                summary["partial_frames"],
                summary["avg_partial_dirty_ratio_percent"],
                stats["log_path"].relative_to(ROOT_DIR),
            )
        )

        report_sources.append(
            {
                "label": scenario.name,
                "path": dirty_stats_report.to_display_path(stats["log_path"]),
                "entries": stats["entries"],
                "summary": summary,
            }
        )

    write_report(output_root, report_sources)

    print("\nResult: %s" % ("ALL PASSED" if all_passed else "FAILED"))
    return 0 if all_passed else 1


if __name__ == "__main__":
    sys.exit(main())

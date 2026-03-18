#!/usr/bin/env python3
"""Batch render and interaction workflow for HelloBasic widgets."""

from __future__ import annotations

import argparse
import json
import os
import platform
import re
import subprocess
import sys
import time
from pathlib import Path

from PIL import Image, ImageChops, ImageStat

SCRIPT_DIR = Path(__file__).resolve().parent
ROOT_DIR = SCRIPT_DIR.parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

import code_runtime_check as runtime_check


CONFIG_PATH = SCRIPT_DIR / "hello_basic_render_workflow.json"
OUTPUT_ROOT = ROOT_DIR / runtime_check.SCREENSHOT_DIR
HELLO_BASIC_DIR = ROOT_DIR / "example" / "HelloBasic"


def load_config() -> dict:
    return json.loads(CONFIG_PATH.read_text(encoding="utf-8"))


def get_all_widgets() -> list[str]:
    return sorted(path.name for path in HELLO_BASIC_DIR.iterdir() if path.is_dir() and (path / "test.c").exists())


def clean_runtime_build() -> tuple[bool, str]:
    result = subprocess.run(["make", "clean"], cwd=ROOT_DIR, capture_output=True, text=True)
    if result.returncode != 0:
        return False, "make clean failed before HelloBasic workflow"
    return True, "make clean passed"


def compile_widget_runtime(widget: str, bits64: bool = False) -> tuple[bool, str]:
    recording_flag = "-DEGUI_CONFIG_RECORDING_TEST=1"
    target_name = f"HelloBasic_{widget}"
    cmd = ["make", "-j", "APP=HelloBasic", "PORT=pc"] + runtime_check.COMPILE_FAST_FLAGS
    cmd.append(f"APP_SUB={widget}")
    cmd.append(f"TARGET={target_name}")
    if bits64:
        cmd.append("BITS=64")
    cmd.append(f"USER_CFLAGS={recording_flag}")

    last_message = "compile failed"
    for attempt in range(3):
        result = subprocess.run(cmd, cwd=ROOT_DIR, capture_output=True, text=True)
        if result.returncode == 0:
            return True, target_name
        combined_output = ((result.stdout or "") + "\n" + (result.stderr or "")).strip()
        last_message = combined_output or "compile failed"
        time.sleep(1)

    return False, last_message


def resolve_suite(config: dict, suite: str, explicit_widgets: str) -> list[str]:
    if explicit_widgets.strip():
        widgets = [item.strip() for item in explicit_widgets.split(",") if item.strip()]
        return sorted(dict.fromkeys(widgets))

    suites = config.get("suites", {})
    if suite == "full":
        return get_all_widgets()
    if suite == "basic":
        excluded = set(suites.get("basic_exclude", []))
        return [widget for widget in get_all_widgets() if widget not in excluded]
    if suite in suites:
        return list(suites[suite])
    raise ValueError(f"unknown suite: {suite}")


def detect_action_types(test_text: str) -> list[str]:
    action_tokens = {
        "click": ("EGUI_SIM_ACTION_CLICK", "EGUI_SIM_SET_CLICK_VIEW"),
        "drag": ("EGUI_SIM_ACTION_DRAG", "EGUI_SIM_SET_DRAG_VIEW"),
        "swipe": ("EGUI_SIM_ACTION_SWIPE", "EGUI_SIM_SET_SWIPE_VIEW"),
        "wait": ("EGUI_SIM_ACTION_WAIT", "EGUI_SIM_SET_WAIT"),
    }
    action_types = []
    for name, tokens in action_tokens.items():
        if any(token in test_text for token in tokens):
            action_types.append(name)
    return action_types


def count_recording_cases(test_text: str) -> int:
    return len(re.findall(r"\bcase\s+\d+\s*:", test_text))


def is_interactive(action_types: list[str]) -> bool:
    return any(action in action_types for action in ("click", "drag", "swipe"))


def get_widget_profile(config: dict, widget: str, action_types: list[str]) -> dict:
    defaults = config.get("defaults", {})
    profile = dict(config.get("profiles", {}).get(widget, {}))
    if "min_interaction_transitions" not in profile:
        profile["min_interaction_transitions"] = defaults.get("interactive_min_transitions", 1) if is_interactive(action_types) else 0
    profile.setdefault("interaction_diff_threshold", defaults.get("interaction_diff_threshold", 0.1))
    profile.setdefault("render_min_stddev", defaults.get("render_min_stddev", 2.0))
    profile.setdefault("expected_actions", [])
    profile.setdefault("min_case_count", 0)
    profile.setdefault("require_runtime_marker", False)
    return profile


def get_test_file(widget: str) -> Path:
    return HELLO_BASIC_DIR / widget / "test.c"


def validate_static_expectations(widget: str, config: dict) -> tuple[bool, dict]:
    test_file = get_test_file(widget)
    if not test_file.exists():
        return False, {"reason": f"missing test file: {test_file}"}

    text = test_file.read_text(encoding="utf-8", errors="ignore")
    has_recording_action = "egui_port_get_recording_action" in text
    action_types = detect_action_types(text)
    case_count = count_recording_cases(text)
    profile = get_widget_profile(config, widget, action_types)
    expected_actions = set(profile["expected_actions"])
    actual_actions = set(action_types)
    missing_actions = sorted(expected_actions - actual_actions)
    has_runtime_marker = "[RUNTIME_CHECK_FAIL]" in text

    ok = (
        has_recording_action
        and not missing_actions
        and case_count >= profile["min_case_count"]
        and (has_runtime_marker or not profile["require_runtime_marker"])
    )
    if not has_recording_action:
        reason = "missing egui_port_get_recording_action"
    elif missing_actions:
        reason = "missing expected actions: " + ", ".join(missing_actions)
    elif case_count < profile["min_case_count"]:
        reason = f"recording case count too low: {case_count}/{profile['min_case_count']}"
    elif profile["require_runtime_marker"] and not has_runtime_marker:
        reason = "missing [RUNTIME_CHECK_FAIL] runtime self-check"
    else:
        reason = ""

    return ok, {
        "test_file": str(test_file.relative_to(ROOT_DIR)).replace("\\", "/"),
        "action_types": action_types,
        "case_count": case_count,
        "has_recording_action": has_recording_action,
        "has_runtime_marker": has_runtime_marker,
        "profile": profile,
        "reason": reason,
    }


def get_frame_paths(widget: str) -> list[Path]:
    frame_dir = OUTPUT_ROOT / f"HelloBasic_{widget}" / "default"
    return sorted(frame_dir.glob("frame_*.png"))


def ensure_not_blank(frame_path: Path, min_stddev: float) -> tuple[bool, float]:
    image = Image.open(frame_path).convert("L")
    stddev = ImageStat.Stat(image).stddev[0]
    return stddev >= min_stddev, float(stddev)


def evaluate_render(widget: str, min_stddev: float) -> tuple[bool, dict]:
    frames = get_frame_paths(widget)
    if not frames:
        return False, {"reason": "no frames generated", "frames": []}

    frame_results = []
    all_ok = True
    for frame in frames:
        ok, stddev = ensure_not_blank(frame, min_stddev)
        frame_results.append(
            {
                "frame": frame.name,
                "blank_stddev": round(stddev, 4),
                "passed": bool(ok),
            }
        )
        if not ok:
            all_ok = False

    reason = "" if all_ok else "blank or near-blank frame detected"
    return all_ok, {"frames": frame_results, "reason": reason}


def calculate_frame_diff(prev_frame: Path, curr_frame: Path) -> float:
    prev = Image.open(prev_frame).convert("L")
    curr = Image.open(curr_frame).convert("L")
    if prev.size != curr.size:
        curr = curr.resize(prev.size)
    diff = ImageChops.difference(prev, curr)
    return float(ImageStat.Stat(diff).mean[0])


def evaluate_interaction(widget: str, min_transitions: int, diff_threshold: float) -> tuple[bool, dict]:
    frames = get_frame_paths(widget)
    if min_transitions <= 0:
        return True, {
            "frame_count": len(frames),
            "changed_transitions": 0,
            "required_transitions": min_transitions,
            "diff_threshold": diff_threshold,
            "transitions": [],
            "reason": "",
        }

    if len(frames) < 2:
        return False, {
            "frame_count": len(frames),
            "changed_transitions": 0,
            "required_transitions": min_transitions,
            "diff_threshold": diff_threshold,
            "transitions": [],
            "reason": "insufficient_frames",
        }

    transitions = []
    changed = 0
    for idx in range(1, len(frames)):
        mad = calculate_frame_diff(frames[idx - 1], frames[idx])
        changed_flag = mad >= diff_threshold
        if changed_flag:
            changed += 1
        transitions.append(
            {
                "from": frames[idx - 1].name,
                "to": frames[idx].name,
                "mad": round(mad, 4),
                "changed": bool(changed_flag),
            }
        )

    ok = changed >= min_transitions
    reason = "" if ok else f"insufficient_changed_transitions:{changed}/{min_transitions}"
    return ok, {
        "frame_count": len(frames),
        "changed_transitions": changed,
        "required_transitions": min_transitions,
        "diff_threshold": diff_threshold,
        "transitions": transitions,
        "reason": reason,
    }


def get_windows_hidden_run_kwargs() -> dict:
    if platform.system() != "Windows":
        return {}

    kwargs = {}
    if hasattr(subprocess, "CREATE_NO_WINDOW"):
        kwargs["creationflags"] = subprocess.CREATE_NO_WINDOW
    if hasattr(subprocess, "STARTUPINFO"):
        startupinfo = subprocess.STARTUPINFO()
        if hasattr(subprocess, "STARTF_USESHOWWINDOW"):
            startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
        if hasattr(subprocess, "SW_HIDE"):
            startupinfo.wShowWindow = subprocess.SW_HIDE
        kwargs["startupinfo"] = startupinfo
    return kwargs


def run_target_app(target_name: str, widget: str, args: argparse.Namespace) -> tuple[bool, str]:
    frames_dir = OUTPUT_ROOT / f"HelloBasic_{widget}" / "default"
    frames_dir.mkdir(parents=True, exist_ok=True)

    exe_candidates = []
    if platform.system() == "Windows":
        exe_candidates.append(ROOT_DIR / "output" / f"{target_name}.exe")
        exe_candidates.append(ROOT_DIR / "output" / "main.exe")
    else:
        exe_candidates.append(ROOT_DIR / "output" / target_name)
        exe_candidates.append(ROOT_DIR / "output" / "main")

    exe_path = next((candidate for candidate in exe_candidates if candidate.exists()), None)
    resource_path = ROOT_DIR / "output" / "app_egui_resource_merge.bin"

    if exe_path is None:
        return False, "executable not found: " + ", ".join(str(path) for path in exe_candidates)

    cmd = [
        str(exe_path),
        str(resource_path),
        "--record",
        str(frames_dir),
        str(runtime_check.RECORDING_FPS),
        str(runtime_check.RECORDING_DURATION),
        "--speed",
        str(args.speed),
        "--clock-scale",
        str(args.clock_scale),
        "--snapshot-settle-ms",
        str(args.snapshot_settle_ms),
        "--snapshot-stable-cycles",
        str(args.snapshot_stable_cycles),
        "--snapshot-max-wait-ms",
        str(args.snapshot_max_wait_ms),
        "--headless",
    ]

    last_error = ""
    timeout = max(runtime_check.RECORDING_DURATION + 5, args.timeout)
    for attempt in range(runtime_check.RUN_RETRY_COUNT):
        for old_frame in frames_dir.glob("frame_*.png"):
            old_frame.unlink()

        try:
            result = subprocess.run(
                cmd,
                timeout=timeout,
                capture_output=True,
                text=True,
                **get_windows_hidden_run_kwargs(),
            )
            if result.returncode != 0:
                stderr_msg = result.stderr.strip() if result.stderr else ""
                last_error = f"exit code {result.returncode}" + (f": {stderr_msg}" if stderr_msg else "")
                if attempt < runtime_check.RUN_RETRY_COUNT - 1:
                    time.sleep(1)
                    continue
                return False, last_error
        except subprocess.TimeoutExpired:
            return False, f"timeout after {timeout}s"
        except Exception as exc:
            return False, f"run error: {exc}"

        combined_output = f"{result.stdout or ''}\n{result.stderr or ''}"
        for marker in runtime_check.RUNTIME_FAIL_MARKERS:
            if marker in combined_output:
                marker_line = marker
                for line in combined_output.splitlines():
                    if marker in line:
                        marker_line = line.strip()
                        break
                return False, f"runtime self-check failed: {marker_line}"
        break

    frame_files = sorted(frames_dir.glob("frame_*.png"))
    if not frame_files:
        return False, "no frames generated"

    for frame in frame_files:
        if frame.stat().st_size < 100:
            return False, f"frame {frame.name} too small ({frame.stat().st_size} bytes)"

    return True, f"{len(frame_files)} frames captured -> {frames_dir}"


def run_hello_unit_test() -> tuple[bool, str]:
    clean_result = subprocess.run(["make", "clean"], cwd=ROOT_DIR, capture_output=True)
    if clean_result.returncode != 0:
        return False, "make clean failed for HelloUnitTest"

    build_result = subprocess.run(
        ["make", "-j", "APP=HelloUnitTest", "PORT=pc_test"],
        cwd=ROOT_DIR,
        capture_output=True,
        text=True,
    )
    if build_result.returncode != 0:
        return False, "make all APP=HelloUnitTest PORT=pc_test failed"

    exe_path = ROOT_DIR / "output" / "main.exe"
    if not exe_path.exists():
        exe_path = ROOT_DIR / "output" / "main"
    if not exe_path.exists():
        return False, "HelloUnitTest executable missing"

    run_result = subprocess.run(
        [str(exe_path)],
        cwd=ROOT_DIR,
        capture_output=True,
        text=True,
        **get_windows_hidden_run_kwargs(),
    )
    if run_result.returncode != 0:
        return False, "HelloUnitTest failed"

    return True, "HelloUnitTest passed"


def run_widget(widget: str, args: argparse.Namespace, config: dict) -> dict:
    static_ok, static_info = validate_static_expectations(widget, config)
    result = {
        "widget": widget,
        "static_check": static_info,
        "compile_check": {"passed": False, "message": ""},
        "runtime_check": {"passed": False, "message": ""},
        "render_check": {"passed": False, "reason": "", "frames": []},
        "interaction_check": {"passed": False, "reason": "", "transitions": []},
        "passed": False,
    }
    if not static_ok:
        result["runtime_check"]["message"] = static_info["reason"]
        return result

    compile_ok, compile_message = compile_widget_runtime(widget)
    result["compile_check"] = {"passed": bool(compile_ok), "message": compile_message}
    if not compile_ok:
        return result

    success, message = run_target_app(compile_message, widget, args)
    result["runtime_check"] = {"passed": bool(success), "message": message}
    if not success:
        return result

    render_ok, render_info = evaluate_render(widget, static_info["profile"]["render_min_stddev"])
    result["render_check"] = {"passed": bool(render_ok), **render_info}
    interaction_ok, interaction_info = evaluate_interaction(
        widget,
        static_info["profile"]["min_interaction_transitions"],
        static_info["profile"]["interaction_diff_threshold"],
    )
    result["interaction_check"] = {"passed": bool(interaction_ok), **interaction_info}
    result["passed"] = bool(render_ok and interaction_ok)
    return result


def write_report(report: dict, report_path: Path) -> None:
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")


def print_result_line(result: dict) -> None:
    widget = result["widget"]
    if result["passed"]:
        changed = result["interaction_check"].get("changed_transitions", 0)
        required = result["interaction_check"].get("required_transitions", 0)
        print(f"[PASS] {widget} - render ok, interaction {changed}/{required}")
        return

    reasons = []
    static_reason = result["static_check"].get("reason", "")
    if static_reason:
        reasons.append(static_reason)
    compile_message = result["compile_check"].get("message", "")
    if compile_message and not result["compile_check"]["passed"]:
        reasons.append(compile_message)
    runtime_message = result["runtime_check"].get("message", "")
    if runtime_message and not result["runtime_check"]["passed"]:
        reasons.append(runtime_message)
    render_reason = result["render_check"].get("reason", "")
    if render_reason and not result["render_check"]["passed"]:
        reasons.append(render_reason)
    interaction_reason = result["interaction_check"].get("reason", "")
    if interaction_reason and not result["interaction_check"]["passed"]:
        reasons.append(interaction_reason)
    print(f"[FAIL] {widget} - {'; '.join(reasons) if reasons else 'unknown failure'}")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Run HelloBasic render and interaction workflow")
    parser.add_argument("--suite", default="basic", choices=["smoke", "interactive", "basic", "full"], help="Widget suite to run")
    parser.add_argument("--widgets", default="", help="Comma-separated widget names, overrides --suite")
    parser.add_argument("--timeout", type=int, default=20, help="Per-widget runtime timeout in seconds")
    parser.add_argument("--speed", type=int, default=runtime_check.RECORDING_SPEED, help="Recording speed multiplier")
    parser.add_argument("--clock-scale", type=int, default=runtime_check.RECORDING_CLOCK_SCALE, help="Recording clock scale")
    parser.add_argument("--snapshot-settle-ms", type=int, default=runtime_check.RECORDING_SNAPSHOT_SETTLE_MS, help="Snapshot settle delay")
    parser.add_argument("--snapshot-stable-cycles", type=int, default=runtime_check.RECORDING_SNAPSHOT_STABLE_CYCLES, help="Stable cycles before snapshot")
    parser.add_argument("--snapshot-max-wait-ms", type=int, default=runtime_check.RECORDING_SNAPSHOT_MAX_WAIT_MS, help="Snapshot max wait")
    parser.add_argument("--skip-unit-tests", action="store_true", help="Skip HelloUnitTest pre-check")
    parser.add_argument("--report", default="", help="Report output path")
    return parser


def main() -> int:
    os.chdir(ROOT_DIR)
    parser = build_parser()
    args = parser.parse_args()

    config = load_config()
    widgets = resolve_suite(config, args.suite, args.widgets)
    widget_set = set(get_all_widgets())
    missing = [widget for widget in widgets if widget not in widget_set]
    if missing:
        print("[FAIL] Unknown widgets: " + ", ".join(missing))
        return 1

    report = {
        "suite": args.suite if not args.widgets else "custom",
        "widgets": widgets,
        "unit_test": {"skipped": bool(args.skip_unit_tests), "passed": False, "message": ""},
        "results": [],
    }

    if not args.skip_unit_tests:
        unit_ok, unit_message = run_hello_unit_test()
        report["unit_test"] = {"skipped": False, "passed": bool(unit_ok), "message": unit_message}
        print(("[PASS]" if unit_ok else "[FAIL]") + " " + unit_message)
        if not unit_ok:
            report_path = Path(args.report) if args.report else OUTPUT_ROOT / f"hello_basic_render_workflow_{report['suite']}.json"
            write_report(report, report_path)
            print(f"[INFO] Report: {report_path}")
            return 1

    clean_ok, clean_message = clean_runtime_build()
    report["runtime_clean"] = {"passed": bool(clean_ok), "message": clean_message}
    print(("[PASS]" if clean_ok else "[FAIL]") + " " + clean_message)
    if not clean_ok:
        report_path = Path(args.report) if args.report else OUTPUT_ROOT / f"hello_basic_render_workflow_{report['suite']}.json"
        write_report(report, report_path)
        print(f"[INFO] Report: {report_path}")
        return 1

    all_passed = True
    total = len(widgets)
    for index, widget in enumerate(widgets, start=1):
        print(f"[{index}/{total}] HelloBasic/{widget}")
        result = run_widget(widget, args, config)
        report["results"].append(result)
        print_result_line(result)
        if not result["passed"]:
            all_passed = False

    report["summary"] = {
        "passed": sum(1 for item in report["results"] if item["passed"]),
        "failed": sum(1 for item in report["results"] if not item["passed"]),
        "all_passed": bool(all_passed),
    }
    report_path = Path(args.report) if args.report else OUTPUT_ROOT / f"hello_basic_render_workflow_{report['suite']}.json"
    write_report(report, report_path)
    print(f"[INFO] Report: {report_path}")
    print(f"[INFO] Summary: {report['summary']['passed']} passed, {report['summary']['failed']} failed")
    return 0 if all_passed else 1


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
"""Manual widget acceptance workflow for HelloBasic controls."""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageChops, ImageStat

RUNTIME_ROOT = Path("runtime_check_output")
BASELINE_ROOT = Path(os.environ.get("EGUI_BASELINE_DIR", str(RUNTIME_ROOT / "baseline_local")))


def default_iteration_dir(widget: str) -> Path:
    return RUNTIME_ROOT / f"HelloBasic_{widget}" / "iteration_records"


def default_iteration_report_dir(widget: str) -> Path:
    return RUNTIME_ROOT / f"HelloBasic_{widget}" / "iteration_reports"


def run_runtime_check(widget: str, timeout: int, speed: int,
                      clock_scale: int, snapshot_settle_ms: int, snapshot_stable_cycles: int,
                      snapshot_max_wait_ms: int) -> None:
    cmd = [
        sys.executable,
        "scripts/code_runtime_check.py",
        "--app",
        "HelloBasic",
        "--app-sub",
        widget,
        "--keep-screenshots",
        "--timeout",
        str(timeout),
        "--speed",
        str(speed),
        "--clock-scale",
        str(clock_scale),
        "--snapshot-settle-ms",
        str(snapshot_settle_ms),
        "--snapshot-stable-cycles",
        str(snapshot_stable_cycles),
        "--snapshot-max-wait-ms",
        str(snapshot_max_wait_ms),
    ]
    result = subprocess.run(cmd)
    if result.returncode != 0:
        raise RuntimeError("runtime check failed")


def find_widget_registry(widget: str) -> Path | None:
    direct = Path("scripts/ui_designer/custom_widgets") / f"{widget}.py"
    if direct.exists():
        return direct

    pattern = re.compile(rf"type_name\s*=\s*['\"]{re.escape(widget)}['\"]")
    for py_file in Path("scripts/ui_designer/custom_widgets").glob("*.py"):
        content = py_file.read_text(encoding="utf-8")
        if pattern.search(content):
            return py_file
    return None


def ensure_not_blank(image_path: Path, min_stddev: float) -> tuple[bool, float]:
    image = Image.open(image_path).convert("L")
    stddev = ImageStat.Stat(image).stddev[0]
    return stddev >= min_stddev, stddev


def get_rendered_frames(widget: str) -> list[Path]:
    app_name = f"HelloBasic_{widget}"
    rendered_dir = RUNTIME_ROOT / app_name / "default"
    frames = sorted(rendered_dir.glob("frame_*.png"))
    if not frames:
        raise RuntimeError(f"no rendered frames found in {rendered_dir}")
    return frames


def evaluate_interaction(widget: str, min_transitions: int, diff_threshold: float) -> dict:
    frames = get_rendered_frames(widget)
    if len(frames) < 2:
        return {
            "app": f"HelloBasic_{widget}",
            "frame_count": len(frames),
            "min_transitions": min_transitions,
            "diff_threshold": diff_threshold,
            "transitions": [],
            "changed_transitions": 0,
            "passed": False,
            "reason": "insufficient_frames",
        }

    transitions = []
    changed = 0
    for i in range(1, len(frames)):
        prev = Image.open(frames[i - 1]).convert("L")
        curr = Image.open(frames[i]).convert("L")
        if prev.size != curr.size:
            curr = curr.resize(prev.size, Image.LANCZOS)
        prev_np = np.array(prev, dtype=np.int16)
        curr_np = np.array(curr, dtype=np.int16)
        mad = float(np.mean(np.abs(prev_np - curr_np)))
        changed_flag = mad >= diff_threshold
        if changed_flag:
            changed += 1
        transitions.append(
            {
                "from": frames[i - 1].name,
                "to": frames[i].name,
                "mad": round(mad, 4),
                "changed": bool(changed_flag),
            }
        )

    passed = changed >= min_transitions
    reason = "" if passed else f"insufficient_changed_transitions:{changed}/{min_transitions}"
    return {
        "app": f"HelloBasic_{widget}",
        "frame_count": len(frames),
        "min_transitions": min_transitions,
        "diff_threshold": diff_threshold,
        "transitions": transitions,
        "changed_transitions": changed,
        "passed": bool(passed),
        "reason": reason,
    }


def write_interaction_report(widget: str, result: dict) -> Path:
    report_path = RUNTIME_ROOT / f"HelloBasic_{widget}" / "interaction_report.json"
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    return report_path


def promote_baseline(widget: str) -> Path:
    app_name = f"HelloBasic_{widget}"
    rendered_dir = RUNTIME_ROOT / app_name / "default"
    baseline_dir = BASELINE_ROOT / app_name / "default"
    frames = sorted(rendered_dir.glob("frame_*.png"))
    if not frames:
        raise RuntimeError(f"no rendered frames found in {rendered_dir}")

    if baseline_dir.exists():
        shutil.rmtree(baseline_dir)
    baseline_dir.mkdir(parents=True, exist_ok=True)
    for frame in frames:
        shutil.copy2(frame, baseline_dir / frame.name)
    return baseline_dir


def has_baseline(widget: str) -> bool:
    app_name = f"HelloBasic_{widget}"
    baseline_dir = BASELINE_ROOT / app_name / "default"
    return baseline_dir.exists() and any(baseline_dir.glob("frame_*.png"))


def first_acceptance_check(widget: str, min_stddev: float) -> dict:
    app_name = f"HelloBasic_{widget}"
    rendered_dir = RUNTIME_ROOT / app_name / "default"
    frames = get_rendered_frames(widget)
    frame_results = []
    for frame in frames:
        visual_ok, stddev = ensure_not_blank(frame, min_stddev)
        frame_results.append(
            {
                "frame": frame.name,
                "blank_stddev": round(stddev, 4),
                "visual_ok": bool(visual_ok),
            }
        )

    visual_ok_all = all(item["visual_ok"] for item in frame_results)
    return {
        "app": app_name,
        "mode": "first_acceptance",
        "baseline_state": "missing",
        "regression_status": "NOT_EVALUATED",
        "min_stddev": min_stddev,
        "rendered_dir": str(rendered_dir),
        "frames": frame_results,
        "rendered_validation_passed": bool(visual_ok_all),
        "passed": False,
    }


def compute_ssim(reference_path: Path, rendered_path: Path) -> float:
    ref = Image.open(reference_path).convert("RGB")
    cur = Image.open(rendered_path).convert("RGB")

    if ref.size != cur.size:
        cur = cur.resize(ref.size, Image.LANCZOS)

    ref_np = np.array(ref, dtype=np.float64)
    cur_np = np.array(cur, dtype=np.float64)

    try:
        from skimage.metrics import structural_similarity

        score = structural_similarity(ref_np, cur_np, channel_axis=2)
        return float(score)
    except Exception:
        mse = np.mean((ref_np - cur_np) ** 2)
        return max(0.0, 1.0 - (mse / (255.0 ** 2)))


def save_diff(reference_path: Path, rendered_path: Path, out_path: Path) -> None:
    ref = Image.open(reference_path).convert("RGB")
    cur = Image.open(rendered_path).convert("RGB")
    if ref.size != cur.size:
        cur = cur.resize(ref.size, Image.LANCZOS)
    diff = ImageChops.difference(ref, cur)
    diff = diff.point(lambda x: min(255, x * 4))

    panel_w, panel_h = ref.size
    panel = Image.new("RGB", (panel_w * 3, panel_h + 20), (28, 28, 28))
    panel.paste(ref, (0, 20))
    panel.paste(cur, (panel_w, 20))
    panel.paste(diff, (panel_w * 2, 20))
    out_path.parent.mkdir(parents=True, exist_ok=True)
    panel.save(out_path)


def compare_frames(widget: str, threshold: float, min_stddev: float, strict_frame_count: bool = True) -> dict:
    app_name = f"HelloBasic_{widget}"
    rendered_dir = RUNTIME_ROOT / app_name / "default"
    baseline_dir = BASELINE_ROOT / app_name / "default"
    diff_dir = RUNTIME_ROOT / app_name / "visual_diff"

    rendered_frames = sorted(rendered_dir.glob("frame_*.png"))
    baseline_frames = sorted(baseline_dir.glob("frame_*.png"))

    if not rendered_frames:
        raise RuntimeError(f"no rendered frames found in {rendered_dir}")
    if not baseline_frames:
        raise RuntimeError(f"no baseline frames found in {baseline_dir}")

    baseline_map = {p.name: p for p in baseline_frames}
    rendered_map = {p.name: p for p in rendered_frames}
    common_names = sorted(set(baseline_map) & set(rendered_map))
    missing_in_rendered = sorted(set(baseline_map) - set(rendered_map))
    extra_in_rendered = sorted(set(rendered_map) - set(baseline_map))

    if not common_names:
        raise RuntimeError("no common frame names between baseline and rendered output")

    frame_results = []
    for name in common_names:
        baseline_frame = baseline_map[name]
        rendered_frame = rendered_map[name]
        visual_ok, stddev = ensure_not_blank(rendered_frame, min_stddev)
        score = float(compute_ssim(baseline_frame, rendered_frame))
        passed = bool(visual_ok and score >= threshold)
        if not passed:
            save_diff(baseline_frame, rendered_frame, diff_dir / name)
        frame_results.append(
            {
                "frame": name,
                "ssim": round(score, 4),
                "blank_stddev": round(stddev, 4),
                "visual_ok": bool(visual_ok),
                "passed": passed,
            }
        )

    frames_passed = all(item["passed"] for item in frame_results)
    frame_shape_ok = True
    if strict_frame_count:
        frame_shape_ok = (not missing_in_rendered and not extra_in_rendered)

    all_passed = frame_shape_ok and frames_passed

    return {
        "app": app_name,
        "mode": "regression",
        "baseline_state": "ready",
        "regression_status": "PASS" if bool(all_passed) else "FAIL",
        "threshold": threshold,
        "min_stddev": min_stddev,
        "baseline_dir": str(baseline_dir),
        "rendered_dir": str(rendered_dir),
        "diff_dir": str(diff_dir),
        "missing_in_rendered": missing_in_rendered,
        "extra_in_rendered": extra_in_rendered,
        "strict_frame_count": strict_frame_count,
        "frames": frame_results,
        "passed": bool(all_passed),
    }


def write_report(widget: str, report: dict) -> Path:
    report_path = RUNTIME_ROOT / f"HelloBasic_{widget}" / "visual_report.json"
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(
        json.dumps(report, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )
    return report_path


def run_iteration_gate(widget: str, min_iterations: int, iteration_dir: Path, report_dir: Path) -> int:
    cmd = [
        sys.executable,
        "scripts/widget/widget_iteration_gate.py",
        "--widget",
        widget,
        "--iteration-dir",
        str(iteration_dir),
        "--report-dir",
        str(report_dir),
        "--min-cycles",
        str(min_iterations),
    ]
    return subprocess.run(cmd).returncode


def run_api_review_gate(widget: str, report_path: Path | None = None) -> int:
    cmd = [
        sys.executable,
        "scripts/widget/widget_api_review_gate.py",
        "--widget",
        widget,
    ]
    if report_path is not None:
        cmd.extend(["--report", str(report_path)])
    return subprocess.run(cmd).returncode


def run_readme_review_gate(widget: str, report_path: Path | None = None) -> int:
    cmd = [
        sys.executable,
        "scripts/widget/widget_readme_review_gate.py",
        "--widget",
        widget,
    ]
    if report_path is not None:
        cmd.extend(["--report", str(report_path)])
    return subprocess.run(cmd).returncode


def verify_runtime_self_check_rule(widget: str) -> tuple[bool, str]:
    test_c_path = Path("example/HelloBasic") / widget / "test.c"
    if not test_c_path.exists():
        return False, f"test source missing: {test_c_path}"

    text = test_c_path.read_text(encoding="utf-8")
    has_runtime_marker = "[RUNTIME_CHECK_FAIL]" in text
    has_clip_logic = (
        "region_screen.size.height <" in text
        or "region_screen.size.width <" in text
        or "region_screen.size.height >" in text
        or "region_screen.size.width >" in text
    )
    if not has_runtime_marker:
        return False, "missing [RUNTIME_CHECK_FAIL] marker in test.c runtime self-check"
    if not has_clip_logic:
        return False, "missing clipping boundary check logic in test.c (region_screen vs region)"
    return True, str(test_c_path)


def main() -> int:
    global BASELINE_ROOT
    parser = argparse.ArgumentParser(description="Run HelloBasic widget acceptance workflow")
    parser.add_argument("--widget", required=True, help="HelloBasic sub-app/widget name")
    parser.add_argument("--threshold", type=float, default=0.92, help="SSIM threshold")
    parser.add_argument("--min-stddev", type=float, default=2.0, help="Min grayscale stddev for non-blank frame")
    parser.add_argument("--timeout", type=int, default=20, help="Runtime check timeout (seconds)")
    parser.add_argument("--speed", type=int, default=1, help="Recording speed multiplier")
    parser.add_argument("--clock-scale", type=int, default=6, help="Recording clock acceleration factor")
    parser.add_argument("--snapshot-settle-ms", type=int, default=0, help="Snapshot settle time in ms")
    parser.add_argument("--snapshot-stable-cycles", type=int, default=1, help="Unchanged frame cycles required before snapshot")
    parser.add_argument("--snapshot-max-wait-ms", type=int, default=1500, help="Snapshot force-capture timeout in ms")
    parser.add_argument("--skip-interaction-check", action="store_true", help="Skip interaction transition validation")
    parser.add_argument("--min-interaction-transitions", type=int, default=2, help="Minimum changed frame transitions for interaction validation")
    parser.add_argument("--interaction-diff-threshold", type=float, default=0.1, help="Mean absolute grayscale diff threshold per frame transition")
    parser.add_argument("--skip-api-review-gate", action="store_true", help="Skip API usability review gate")
    parser.add_argument("--skip-readme-gate", action="store_true", help="Skip readme rationale gate")
    parser.add_argument(
        "--api-review-report",
        default="",
        help="API review report json path (default: runtime_check_output/HelloBasic_<widget>/api_review_report.json)",
    )
    parser.add_argument(
        "--readme-review-report",
        default="",
        help="Readme review report json path (default: runtime_check_output/HelloBasic_<widget>/readme_review_report.json)",
    )
    parser.add_argument("--skip-runtime-check", action="store_true", help="Skip runtime check and reuse existing frames")
    parser.add_argument(
        "--skip-runtime-self-check-rule",
        action="store_true",
        help="Skip test.c runtime clipping self-check rule (for emergency baseline maintenance only)",
    )
    parser.add_argument("--allow-frame-mismatch", action="store_true", help="Ignore frame count mismatch and compare common frames only")
    parser.add_argument(
        "--promote-baseline",
        "--update-baseline",
        dest="promote_baseline",
        action="store_true",
        help="Explicitly promote current frames to baseline (required when baseline does not exist)",
    )
    parser.add_argument(
        "--baseline-dir",
        default=str(BASELINE_ROOT),
        help="Local baseline root directory (default: runtime_check_output/baseline_local or EGUI_BASELINE_DIR)",
    )
    parser.add_argument("--min-iterations", type=int, default=10, help="Required minimum recursive quality iteration cycles")
    parser.add_argument("--iteration-dir", default="", help="Iteration data directory (default: runtime_check_output/HelloBasic_<widget>/iteration_records)")
    parser.add_argument("--iteration-report-dir", default="", help="Iteration gate report directory (default: runtime_check_output/HelloBasic_<widget>/iteration_reports)")
    parser.add_argument("--skip-iteration-gate", action="store_true", help="Skip recursive iteration gate (for baseline maintenance only)")
    args = parser.parse_args()
    BASELINE_ROOT = Path(args.baseline_dir)
    widget = args.widget.strip()
    iteration_dir = Path(args.iteration_dir) if args.iteration_dir.strip() else default_iteration_dir(widget)
    iteration_report_dir = (
        Path(args.iteration_report_dir) if args.iteration_report_dir.strip() else default_iteration_report_dir(widget)
    )
    api_review_report = (
        Path(args.api_review_report)
        if args.api_review_report.strip()
        else RUNTIME_ROOT / f"HelloBasic_{widget}" / "api_review_report.json"
    )
    readme_review_report = (
        Path(args.readme_review_report)
        if args.readme_review_report.strip()
        else RUNTIME_ROOT / f"HelloBasic_{widget}" / "readme_review_report.json"
    )
    example_dir = Path("example/HelloBasic") / widget
    if not example_dir.exists():
        print(f"[FAIL] HelloBasic example missing: {example_dir}")
        return 1

    registry_file = find_widget_registry(widget)
    if registry_file is None:
        print(f"[FAIL] UI Designer registry missing for widget: {widget}")
        return 1

    if not args.skip_runtime_self_check_rule:
        check_ok, check_msg = verify_runtime_self_check_rule(widget)
        if not check_ok:
            print(f"[FAIL] Runtime self-check rule failed: {check_msg}")
            return 1
        print(f"[OK] Runtime self-check rule: {check_msg}")

    print(f"[OK] Widget example: {example_dir}")
    print(f"[OK] Widget registry: {registry_file}")

    def finalize_with_quality_gates() -> bool:
        if not args.skip_readme_gate:
            print("[STEP] Readme rationale gate")
            readme_gate_code = run_readme_review_gate(widget, readme_review_report)
            if readme_gate_code != 0:
                print("[FAIL] Readme rationale gate failed.")
                print(f"[INFO] Readme report: {readme_review_report}")
                return False
        if args.skip_api_review_gate:
            return True
        print("[STEP] API usability review gate")
        gate_code = run_api_review_gate(widget, api_review_report)
        if gate_code == 0:
            return True
        print("[FAIL] API usability review gate failed.")
        print(f"[INFO] API report: {api_review_report}")
        return False

    if not args.skip_runtime_check:
        print("[STEP] Runtime check")
        try:
            run_runtime_check(
                widget,
                timeout=args.timeout,
                speed=args.speed,
                clock_scale=args.clock_scale,
                snapshot_settle_ms=args.snapshot_settle_ms,
                snapshot_stable_cycles=args.snapshot_stable_cycles,
                snapshot_max_wait_ms=args.snapshot_max_wait_ms,
            )
        except RuntimeError as exc:
            print(f"[FAIL] {exc}")
            return 1

    if not args.skip_interaction_check:
        print("[STEP] Interaction validation")
        try:
            interaction = evaluate_interaction(
                widget,
                min_transitions=args.min_interaction_transitions,
                diff_threshold=args.interaction_diff_threshold,
            )
        except RuntimeError as exc:
            print(f"[FAIL] {exc}")
            return 1
        interaction_report_path = write_interaction_report(widget, interaction)
        print(
            "[INFO] Changed transitions: "
            f"{interaction['changed_transitions']}/{max(0, interaction['frame_count'] - 1)} "
            f"(required>={interaction['min_transitions']}, threshold={interaction['diff_threshold']})"
        )
        print(f"[INFO] Interaction report: {interaction_report_path}")
        if not interaction["passed"]:
            print(f"[FAIL] Interaction validation failed: {interaction['reason']}")
            return 1

    baseline_exists = has_baseline(widget)
    if not baseline_exists:
        print("[STEP] First acceptance check (baseline missing)")
        try:
            report = first_acceptance_check(widget, min_stddev=args.min_stddev)
        except RuntimeError as exc:
            print(f"[FAIL] {exc}")
            return 1

        report_path = write_report(widget, report)
        passed_count = sum(1 for item in report["frames"] if item["visual_ok"])
        total_count = len(report["frames"])
        print(f"[INFO] Non-blank frames: {passed_count}/{total_count}")
        print(f"[INFO] Report: {report_path}")

        if not report["rendered_validation_passed"]:
            print("[FAIL] First acceptance check failed (blank/abnormal frame detected).")
            return 1

        if args.promote_baseline:
            try:
                baseline_dir = promote_baseline(widget)
            except RuntimeError as exc:
                print(f"[FAIL] {exc}")
                return 1
            print(f"[OK] Baseline promoted: {baseline_dir}")
            if not args.skip_iteration_gate:
                print("[STEP] Recursive iteration gate")
                gate_code = run_iteration_gate(widget, args.min_iterations, iteration_dir, iteration_report_dir)
                if gate_code == 0:
                    if not finalize_with_quality_gates():
                        return 1
                    print("[PASS] Widget acceptance passed.")
                    print("[INFO] Regression mode is now enabled for next run.")
                    return 0
                if gate_code == 2:
                    print("[HOLD] Baseline promoted, but recursive iteration gate not satisfied yet.")
                    return 2
                print("[FAIL] Recursive iteration gate failed.")
                return 1
            if not finalize_with_quality_gates():
                return 1
            print("[INFO] Regression mode is now enabled for next run.")
            return 0

        print("[HOLD] First acceptance passed, but regression is not evaluated without baseline.")
        print("[HOLD] Run with --promote-baseline to explicitly enter regression mode.")
        return 2

    if args.promote_baseline:
        try:
            baseline_dir = promote_baseline(widget)
        except RuntimeError as exc:
            print(f"[FAIL] {exc}")
            return 1
        print(f"[OK] Baseline promoted: {baseline_dir}")
        if not args.skip_iteration_gate:
            print("[STEP] Recursive iteration gate")
            gate_code = run_iteration_gate(widget, args.min_iterations, iteration_dir, iteration_report_dir)
            if gate_code == 0:
                if not finalize_with_quality_gates():
                    return 1
                print("[INFO] Baseline has been refreshed explicitly.")
                print("[PASS] Widget acceptance passed.")
                return 0
            if gate_code == 2:
                print("[HOLD] Baseline refreshed, but recursive iteration gate not satisfied yet.")
                return 2
            print("[FAIL] Recursive iteration gate failed.")
            return 1
        if not finalize_with_quality_gates():
            return 1
        print("[INFO] Baseline has been refreshed explicitly.")
        return 0

    print("[STEP] Visual compare")
    try:
        report = compare_frames(
            widget,
            threshold=args.threshold,
            min_stddev=args.min_stddev,
            strict_frame_count=not args.allow_frame_mismatch,
        )
    except RuntimeError as exc:
        print(f"[FAIL] {exc}")
        return 1

    report_path = write_report(widget, report)
    passed_count = sum(1 for item in report["frames"] if item["passed"])
    total_count = len(report["frames"])
    print(f"[INFO] Frames passed: {passed_count}/{total_count}")
    print(f"[INFO] Report: {report_path}")

    if report["passed"]:
        if not args.skip_iteration_gate:
            print("[STEP] Recursive iteration gate")
            gate_code = run_iteration_gate(widget, args.min_iterations, iteration_dir, iteration_report_dir)
            if gate_code == 0:
                if not finalize_with_quality_gates():
                    return 1
                print("[PASS] Widget acceptance passed.")
                return 0
            if gate_code == 2:
                print("[HOLD] Visual regression passed, but recursive iteration gate not satisfied yet.")
                return 2
            print("[FAIL] Recursive iteration gate failed.")
            return 1
        if not finalize_with_quality_gates():
            return 1
        print("[PASS] Widget acceptance passed.")
        return 0

    print("[FAIL] Widget acceptance failed.")
    if report["missing_in_rendered"]:
        print(f"Missing frames: {', '.join(report['missing_in_rendered'])}")
    if report["extra_in_rendered"]:
        print(f"Extra frames: {', '.join(report['extra_in_rendered'])}")
    print(f"Diff images: {report['diff_dir']}")
    return 1


if __name__ == "__main__":
    sys.exit(main())

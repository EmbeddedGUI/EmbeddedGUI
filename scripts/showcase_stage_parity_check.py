#!/usr/bin/env python3
"""Compare HelloShowcase and HelloVirtual/virtual_stage_showcase key render states."""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

import numpy as np
from PIL import Image, ImageChops

SCRIPT_DIR = Path(__file__).resolve().parent
ROOT_DIR = SCRIPT_DIR.parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

import code_runtime_check as runtime_check


SHOWCASE_APP = "HelloShowcase"
STAGE_APP = "HelloVirtual"
STAGE_SUB_APP = "virtual_stage_showcase"
SHOWCASE_APP_NAME = SHOWCASE_APP
STAGE_APP_NAME = f"{STAGE_APP}_{STAGE_SUB_APP}"
SHOWCASE_OUTPUT_SUBDIR = "parity_showcase"
STAGE_OUTPUT_SUBDIR = "parity_stage"
SHOWCASE_SNAPSHOT_MAX_WAIT_MS = 200

STATE_LABELS = (
    "dark_initial",
    "light_en",
    "light_cn",
)
DEFAULT_MAX_MEAN_ABS = 0.25
DEFAULT_MAX_RMS = 5.5
DEFAULT_MAX_HOT_PIXEL_RATIO = 0.003
DEFAULT_HOT_PIXEL_DELTA = 10


@dataclass(frozen=True)
class FrameMetrics:
    mean_abs: float
    rms: float
    hot_pixel_ratio: float


def output_dir_for(app_name: str, output_subdir: str) -> Path:
    return ROOT_DIR / runtime_check.SCREENSHOT_DIR / app_name / output_subdir


def capture_app(
    app: str,
    app_sub: str | None,
    app_name: str,
    output_subdir: str,
    snapshot_max_wait_ms: int,
    timeout: int,
    bits64: bool,
) -> tuple[bool, str]:
    if not compile_app_for_parity(app, app_sub, bits64):
        return False, f"compile failed: {app_name}"

    return runtime_check.run_app(
        app_name,
        output_subdir,
        timeout=timeout,
        duration=runtime_check.RECORDING_DURATION,
        speed=runtime_check.RECORDING_SPEED,
        clock_scale=runtime_check.RECORDING_CLOCK_SCALE,
        snapshot_settle_ms=runtime_check.RECORDING_SNAPSHOT_SETTLE_MS,
        snapshot_stable_cycles=runtime_check.RECORDING_SNAPSHOT_STABLE_CYCLES,
        snapshot_max_wait_ms=snapshot_max_wait_ms,
    )


def compile_app_for_parity(app: str, app_sub: str | None, bits64: bool) -> bool:
    """Build with normal optimization so recording actions keep intermediate frames.

    The generic runtime-check path uses a fast `-O0` build. That is good enough for
    smoke checks, but `HelloShowcase` collapses its recording snapshots into a single
    final frame under that build mode, which makes parity comparison meaningless.
    """
    subprocess.run(["make", "clean"], cwd=ROOT_DIR, capture_output=True)

    command = [
        "make",
        "-j1",
        "all",
        f"APP={app}",
        "PORT=pc",
        "USER_CFLAGS=-DEGUI_CONFIG_RECORDING_TEST=1 -DEGUI_SHOWCASE_PARITY_RECORDING=1",
    ]
    if bits64:
        command.append("BITS=64")
    if app_sub:
        command.append(f"APP_SUB={app_sub}")

    result = subprocess.run(command, cwd=ROOT_DIR, capture_output=True, text=True)
    return result.returncode == 0


def save_side_by_side(label: str, left_path: Path, right_path: Path, output_dir: Path) -> Path:
    left = Image.open(left_path).convert("RGB")
    right = Image.open(right_path).convert("RGB")
    canvas = Image.new("RGB", (left.width + right.width, max(left.height, right.height)), "white")
    canvas.paste(left, (0, 0))
    canvas.paste(right, (left.width, 0))

    output_path = output_dir / f"compare_{label}.png"
    canvas.save(output_path)
    return output_path


def save_diff(label: str, left_path: Path, right_path: Path, output_dir: Path) -> Path:
    left = Image.open(left_path).convert("RGB")
    right = Image.open(right_path).convert("RGB")
    diff = ImageChops.difference(left, right)
    amplified = diff.point(lambda value: min(255, value * 4))
    output_path = output_dir / f"diff_{label}.png"
    amplified.save(output_path)
    return output_path


def compute_metrics(showcase_frame: Path, stage_frame: Path, hot_pixel_delta: int) -> FrameMetrics:
    showcase_image = np.array(Image.open(showcase_frame).convert("RGB"), dtype=np.float32)
    stage_image = np.array(Image.open(stage_frame).convert("RGB"), dtype=np.float32)
    if showcase_image.shape != stage_image.shape:
        raise ValueError(f"image size mismatch: {showcase_frame.name} vs {stage_frame.name}")

    diff = np.abs(showcase_image - stage_image)
    hot_pixel_ratio = float(np.mean(np.max(diff, axis=2) > float(hot_pixel_delta)))
    return FrameMetrics(
        mean_abs=float(np.mean(diff)),
        rms=float(np.sqrt(np.mean(np.square(diff)))),
        hot_pixel_ratio=hot_pixel_ratio,
    )


def load_explicit_state_frames(app_name: str, output_subdir: str) -> dict[str, Path]:
    output_dir = output_dir_for(app_name, output_subdir)
    manifest_path = output_dir / runtime_check.FRAME_LABEL_MANIFEST
    if not manifest_path.exists():
        raise RuntimeError(f"{app_name} did not produce {runtime_check.FRAME_LABEL_MANIFEST}")

    payload = json.loads(manifest_path.read_text(encoding="utf-8"))
    entries = payload.get("entries", [])
    if not isinstance(entries, list) or not entries:
        raise RuntimeError(f"{app_name} produced an empty frame-label manifest")

    state_frames: dict[str, Path] = {}
    state_order: list[str] = []
    duplicates: list[str] = []

    for entry in entries:
        frame_name = str(entry.get("frame", "")).strip()
        label = str(entry.get("label", "")).strip()
        if label not in STATE_LABELS:
            continue
        frame_path = output_dir / frame_name
        if not frame_path.exists():
            raise RuntimeError(f"{app_name} labeled a missing frame: {frame_name}")
        if label in state_frames:
            duplicates.append(f"{label}={frame_name}")
            continue
        state_frames[label] = frame_path
        state_order.append(label)

    missing_labels = [label for label in STATE_LABELS if label not in state_frames]
    if missing_labels:
        raise RuntimeError(f"{app_name} missing labeled parity states: {', '.join(missing_labels)}")
    if state_order != list(STATE_LABELS):
        raise RuntimeError(
            f"{app_name} parity state order mismatch: expected {', '.join(STATE_LABELS)} got {', '.join(state_order)}"
        )
    if duplicates:
        print(f"[WARN] {app_name} produced duplicate labeled snapshots: {', '.join(duplicates)}")

    return state_frames


def compare_state(label: str, showcase_frame: Path, stage_frame: Path, output_dir: Path, max_mean_abs: float, max_rms: float, max_hot_pixel_ratio: float,
                  hot_pixel_delta: int) -> tuple[bool, str]:
    showcase_image = Image.open(showcase_frame).convert("RGB")
    stage_image = Image.open(stage_frame).convert("RGB")
    diff_bbox = ImageChops.difference(showcase_image, stage_image).getbbox()
    metrics = compute_metrics(showcase_frame, stage_frame, hot_pixel_delta)
    side_by_side = save_side_by_side(label, showcase_frame, stage_frame, output_dir)

    passed = metrics.mean_abs <= max_mean_abs and metrics.rms <= max_rms and metrics.hot_pixel_ratio <= max_hot_pixel_ratio
    metric_text = (
        f"showcase={showcase_frame.name} stage={stage_frame.name} "
        f"mean_abs={metrics.mean_abs:.3f} rms={metrics.rms:.3f} hot_ratio={metrics.hot_pixel_ratio:.5f}"
    )

    if passed:
        return True, f"{label}: PASS {metric_text} compare={side_by_side.relative_to(ROOT_DIR)}"

    diff_path = save_diff(label, showcase_frame, stage_frame, output_dir)
    return False, f"{label}: FAIL {metric_text} diff_bbox={diff_bbox} compare={side_by_side.relative_to(ROOT_DIR)} diff={diff_path.relative_to(ROOT_DIR)}"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Check key render parity between HelloShowcase and virtual_stage_showcase",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=(
            "Examples:\n"
            "  python scripts/showcase_stage_parity_check.py --timeout 35 --bits64\n"
            "  python scripts/showcase_stage_parity_check.py --max-mean-abs 0.2 --max-rms 5.0 --bits64\n"
        ),
    )
    parser.add_argument("--timeout", type=int, default=35, help="Per-app runtime timeout in seconds")
    parser.add_argument(
        "--showcase-snapshot-max-wait-ms",
        type=int,
        default=SHOWCASE_SNAPSHOT_MAX_WAIT_MS,
        help="Snapshot force-capture timeout for HelloShowcase",
    )
    parser.add_argument(
        "--stage-snapshot-max-wait-ms",
        type=int,
        default=runtime_check.RECORDING_SNAPSHOT_MAX_WAIT_MS,
        help="Snapshot force-capture timeout for virtual_stage_showcase",
    )
    parser.add_argument("--max-mean-abs", type=float, default=DEFAULT_MAX_MEAN_ABS, help="Maximum allowed mean absolute RGB difference")
    parser.add_argument("--max-rms", type=float, default=DEFAULT_MAX_RMS, help="Maximum allowed RGB RMS difference")
    parser.add_argument("--max-hot-pixel-ratio", type=float, default=DEFAULT_MAX_HOT_PIXEL_RATIO, help="Maximum allowed changed-pixel ratio")
    parser.add_argument("--hot-pixel-delta", type=int, default=DEFAULT_HOT_PIXEL_DELTA, help="Per-pixel RGB delta used for changed-pixel ratio")
    parser.add_argument("--bits64", action="store_true", help="Build and run 64-bit binaries")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    print("== Showcase Stage Parity Check ==")

    success, message = capture_app(
        SHOWCASE_APP,
        None,
        SHOWCASE_APP_NAME,
        SHOWCASE_OUTPUT_SUBDIR,
        args.showcase_snapshot_max_wait_ms,
        args.timeout,
        args.bits64,
    )
    print(f"[{'PASS' if success else 'FAIL'}] {SHOWCASE_APP_NAME}: {message}")
    if not success:
        return 1

    success, message = capture_app(
        STAGE_APP,
        STAGE_SUB_APP,
        STAGE_APP_NAME,
        STAGE_OUTPUT_SUBDIR,
        args.stage_snapshot_max_wait_ms,
        args.timeout,
        args.bits64,
    )
    print(f"[{'PASS' if success else 'FAIL'}] {STAGE_APP_NAME}: {message}")
    if not success:
        return 1

    compare_output_dir = output_dir_for(STAGE_APP_NAME, STAGE_OUTPUT_SUBDIR)
    compare_output_dir.mkdir(parents=True, exist_ok=True)

    try:
        showcase_states = load_explicit_state_frames(SHOWCASE_APP_NAME, SHOWCASE_OUTPUT_SUBDIR)
        stage_states = load_explicit_state_frames(STAGE_APP_NAME, STAGE_OUTPUT_SUBDIR)
    except RuntimeError as exc:
        print(f"[FAIL] {exc}")
        return 1

    print("Showcase states:", ", ".join(f"{label}={showcase_states[label].name}" for label in STATE_LABELS))
    print("Stage states:", ", ".join(f"{label}={stage_states[label].name}" for label in STATE_LABELS))

    all_passed = True
    for label in STATE_LABELS:
        matched, message = compare_state(
            label,
            showcase_states[label],
            stage_states[label],
            compare_output_dir,
            args.max_mean_abs,
            args.max_rms,
            args.max_hot_pixel_ratio,
            args.hot_pixel_delta,
        )
        print(f"[{'PASS' if matched else 'FAIL'}] {message}")
        all_passed = all_passed and matched

    print("Result: %s" % ("ALL PASSED" if all_passed else "FAILED"))
    return 0 if all_passed else 1


if __name__ == "__main__":
    sys.exit(main())

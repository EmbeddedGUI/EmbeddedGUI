#!/usr/bin/env python3
"""Bootstrap baseline frames for HelloBasic widgets."""

import argparse
import json
import os
import subprocess
import sys
from pathlib import Path

BASELINE_ROOT = Path(os.environ.get("EGUI_BASELINE_DIR", "runtime_check_output/baseline_local"))

WIDGET_TO_DEMO_ALIAS = {
    "image_button": "button_img",
}


def list_hellobasic_subapps() -> set[str]:
    root = Path("example/HelloBasic")
    if not root.exists():
        return set()
    return {p.name for p in root.iterdir() if p.is_dir()}


def list_registered_widgets() -> set[str]:
    root = Path("scripts/ui_designer/custom_widgets")
    if not root.exists():
        return set()
    names = set()
    for py in root.glob("*.py"):
        if py.stem.startswith("__"):
            continue
        names.add(py.stem)
    return names


def resolve_registered_targets() -> list[str]:
    demos = list_hellobasic_subapps()
    targets = set()
    for widget in list_registered_widgets():
        if widget in demos:
            targets.add(widget)
            continue
        alias = WIDGET_TO_DEMO_ALIAS.get(widget)
        if alias and alias in demos:
            targets.add(alias)
    return sorted(targets)


def baseline_exists(widget: str) -> bool:
    base = BASELINE_ROOT / f"HelloBasic_{widget}" / "default"
    return base.exists() and any(base.glob("frame_*.png"))


def run_promote(widget: str, baseline_dir: Path, skip_runtime_check: bool = False,
                clock_scale: int = 6, snapshot_settle_ms: int = 0, snapshot_stable_cycles: int = 1,
                snapshot_max_wait_ms: int = 1500) -> int:
    cmd = [
        sys.executable,
        "scripts/widget/widget_acceptance_flow.py",
        "--widget",
        widget,
        "--promote-baseline",
        "--skip-iteration-gate",
        "--skip-api-review-gate",
        "--baseline-dir",
        str(baseline_dir),
        "--clock-scale",
        str(clock_scale),
        "--snapshot-settle-ms",
        str(snapshot_settle_ms),
        "--snapshot-stable-cycles",
        str(snapshot_stable_cycles),
        "--snapshot-max-wait-ms",
        str(snapshot_max_wait_ms),
    ]
    if skip_runtime_check:
        cmd.append("--skip-runtime-check")
    return subprocess.run(cmd).returncode


def main() -> int:
    global BASELINE_ROOT
    parser = argparse.ArgumentParser(description="Bootstrap or refresh widget baseline frames")
    parser.add_argument(
        "--scope",
        choices=["registered", "hellobasic"],
        default="registered",
        help="Target scope: registered widgets only (default) or all HelloBasic sub-apps",
    )
    parser.add_argument(
        "--widgets",
        default="",
        help="Comma-separated widget/sub-app list. If set, overrides --scope",
    )
    parser.add_argument(
        "--refresh",
        action="store_true",
        help="Refresh baseline even if it already exists",
    )
    parser.add_argument(
        "--baseline-dir",
        default=str(BASELINE_ROOT),
        help="Local baseline root directory (default: runtime_check_output/baseline_local or EGUI_BASELINE_DIR)",
    )
    parser.add_argument(
        "--skip-runtime-check",
        action="store_true",
        help="Skip runtime check and use existing rendered frames",
    )
    parser.add_argument("--clock-scale", type=int, default=6, help="Recording clock acceleration factor")
    parser.add_argument("--snapshot-settle-ms", type=int, default=0, help="Snapshot settle time in ms")
    parser.add_argument("--snapshot-stable-cycles", type=int, default=1, help="Unchanged frame cycles required before snapshot")
    parser.add_argument("--snapshot-max-wait-ms", type=int, default=1500, help="Snapshot force-capture timeout in ms")
    args = parser.parse_args()
    BASELINE_ROOT = Path(args.baseline_dir)

    if args.widgets.strip():
        targets = sorted({x.strip() for x in args.widgets.split(",") if x.strip()})
    elif args.scope == "hellobasic":
        targets = sorted(list_hellobasic_subapps())
    else:
        targets = resolve_registered_targets()

    if not targets:
        print("[FAIL] No targets found.")
        return 1

    summary: list[dict] = []
    for idx, widget in enumerate(targets, 1):
        exists = baseline_exists(widget)
        if exists and not args.refresh:
            print(f"[{idx}/{len(targets)}] {widget}: SKIP (baseline exists)")
            summary.append({"widget": widget, "status": "SKIP"})
            continue

        print(f"[{idx}/{len(targets)}] {widget}: RUN")
        code = run_promote(
            widget,
            BASELINE_ROOT,
            args.skip_runtime_check,
            args.clock_scale,
            args.snapshot_settle_ms,
            args.snapshot_stable_cycles,
            args.snapshot_max_wait_ms,
        )
        if code == 0:
            status = "PASS"
        else:
            status = "FAIL"
        summary.append({"widget": widget, "status": status, "code": code})
        if code != 0:
            print(f"  -> FAIL (exit={code})")
        else:
            print("  -> PASS")

    report = {
        "scope": args.scope,
        "refresh": args.refresh,
        "targets": targets,
        "results": summary,
    }
    report_path = BASELINE_ROOT / "baseline_bootstrap_report.json"
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")

    passed = sum(1 for x in summary if x["status"] == "PASS")
    failed = sum(1 for x in summary if x["status"] == "FAIL")
    skipped = sum(1 for x in summary if x["status"] == "SKIP")

    print("=" * 60)
    print(f"PASS={passed} FAIL={failed} SKIP={skipped} TOTAL={len(summary)}")
    print(f"Report: {report_path}")
    print("=" * 60)
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())

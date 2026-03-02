#!/usr/bin/env python3
"""Figma Make → EGUI: Unified pipeline entry point.

Orchestrates the full 4-stage conversion pipeline:
  Stage 1 (CAPTURE):  Playwright captures reference frames from Figma Make
  Stage 2 (CONVERT):  TSX parse → animation extract → XML + C codegen
  Stage 3 (BUILD):    Compile & run EGUI app, capture rendered frames
  Stage 4 (VERIFY):   SSIM regression comparison + HTML report

Usage:
    python scripts/figmamake/figmamake2egui.py \\
        --figma-url "https://www.figma.com/make/Fw9h77LjlAxt4l9Hlt5XP9/..." \\
        --app HelloBattery --width 320 --height 240

    # Skip capture (use existing reference frames):
    python scripts/figmamake/figmamake2egui.py \\
        --project-dir /path/to/figmamake/project \\
        --app HelloBattery --width 320 --height 240 --skip-capture

    # Convert only (no build/verify):
    python scripts/figmamake/figmamake2egui.py \\
        --project-dir /path/to/figmamake/project \\
        --app HelloBattery --convert-only
"""

import argparse
import json
import os
import subprocess
import sys
import time
from pathlib import Path

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPTS_DIR = os.path.dirname(SCRIPT_DIR)
sys.path.insert(0, SCRIPT_DIR)
sys.path.insert(0, SCRIPTS_DIR)


def _find_egui_root():
    """Find EmbeddedGUI root directory."""
    d = os.path.dirname(os.path.abspath(__file__))
    while d != os.path.dirname(d):
        if os.path.isfile(os.path.join(d, "egui.h")):
            return d
        d = os.path.dirname(d)
    # Fallback: assume scripts/figmamake/ is two levels below root
    return os.path.dirname(SCRIPTS_DIR)


def stage_capture(figma_url, app_name, width, height, egui_root):
    """Stage 1: Capture reference frames from Figma Make."""
    print("\n" + "=" * 60)
    print("STAGE 1: CAPTURE — Reference frames from Figma Make")
    print("=" * 60)

    app_dir = os.path.join(egui_root, "example", app_name)
    ref_dir = os.path.join(app_dir, ".eguiproject", "reference_frames")
    os.makedirs(ref_dir, exist_ok=True)

    capture_script = os.path.join(SCRIPT_DIR, "figmamake_capture.py")
    cmd = [
        sys.executable, capture_script,
        "--url", figma_url,
        "--output-dir", ref_dir,
        "--width", str(width),
        "--height", str(height),
    ]

    print(f"  Running: {' '.join(cmd)}")
    result = subprocess.run(cmd, capture_output=True, text=True)

    if result.returncode != 0:
        print(f"  FAILED: {result.stderr}", file=sys.stderr)
        return False, ref_dir

    print(f"  Reference frames saved to: {ref_dir}")
    return True, ref_dir


def stage_convert(project_dir, app_name, width, height, egui_root, skip_c_gen=False):
    """Stage 2: Parse TSX → Extract animations → Generate XML + C code."""
    print("\n" + "=" * 60)
    print("STAGE 2: CONVERT — TSX → XML → C code")
    print("=" * 60)

    from figmamake_codegen import FigmaMakeCodegen

    codegen = FigmaMakeCodegen(app_name, width, height)
    result = codegen.run(project_dir, skip_c_gen=skip_c_gen)

    if result.get("extensions_needed"):
        print(f"\n  WARNING: {len(result['extensions_needed'])} animations "
              "need framework extensions.")
        for ext in result["extensions_needed"]:
            print(f"    - {ext.get('property', '?')}: {ext.get('reason', '')}")

    return result


def stage_build_and_run(app_name, egui_root, width, height):
    """Stage 3: Compile EGUI app and capture rendered frames."""
    print("\n" + "=" * 60)
    print("STAGE 3: BUILD & RUN — Compile and capture rendered frames")
    print("=" * 60)

    app_dir = os.path.join(egui_root, "example", app_name)

    # Generate resources first
    print("  [3a] Generating resources...")
    gen_res_cmd = [
        sys.executable,
        os.path.join(SCRIPTS_DIR, "html2egui_helper.py"),
        "gen-resource", "--app", app_name,
    ]
    result = subprocess.run(gen_res_cmd, capture_output=True, text=True,
                            cwd=egui_root)
    if result.returncode != 0:
        print(f"  Resource generation failed: {result.stderr}", file=sys.stderr)
        return False, ""

    # Compile
    print("  [3b] Compiling...")
    from code_runtime_check import compile_app
    if not compile_app(app_name):
        print("  Compilation FAILED", file=sys.stderr)
        return False, ""

    # Run and capture frames
    print("  [3c] Running and capturing frames...")
    rendered_dir = os.path.join(
        "runtime_check_output", app_name, "regression"
    )
    from code_runtime_check import capture_animation_frames
    success, frames, msg = capture_animation_frames(
        app_name, rendered_dir, fps=10, duration=5, speed=1
    )

    if not success:
        print(f"  Runtime capture FAILED: {msg}", file=sys.stderr)
        return False, rendered_dir

    print(f"  Captured: {msg}")
    return True, rendered_dir


def stage_verify(ref_dir, rendered_dir, app_name, egui_root):
    """Stage 4: SSIM regression verification."""
    print("\n" + "=" * 60)
    print("STAGE 4: VERIFY — SSIM regression comparison")
    print("=" * 60)

    from figmamake_regression import RegressionVerifier, generate_html_report

    verifier = RegressionVerifier(ref_dir, rendered_dir)
    summary = verifier.verify_all()

    # Print results
    for r in summary["results"]:
        status = r["status"]
        print(f"  [{status:4s}] {r['name']:30s}  SSIM={r['ssim']:.4f}")

    # Generate HTML report
    app_dir = os.path.join(egui_root, "example", app_name)
    report_path = os.path.join(app_dir, ".eguiproject", "regression_report.html")
    generate_html_report(summary, report_path)

    # Generate JSON results
    json_path = os.path.join(app_dir, ".eguiproject", "regression_results.json")
    os.makedirs(os.path.dirname(json_path), exist_ok=True)
    with open(json_path, "w", encoding="utf-8", newline="\n") as f:
        json.dump(summary, f, indent=2)

    return summary


def main():
    parser = argparse.ArgumentParser(
        description="Figma Make → EGUI full pipeline",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--figma-url", default=None,
                        help="Figma Make URL (triggers Stage 1 capture)")
    parser.add_argument("--project-dir", default=None,
                        help="Local Figma Make project directory (skip capture)")
    parser.add_argument("--app", required=True, help="EGUI app name")
    parser.add_argument("--width", type=int, default=320)
    parser.add_argument("--height", type=int, default=240)
    parser.add_argument("--skip-capture", action="store_true",
                        help="Skip Stage 1 (use existing reference frames)")
    parser.add_argument("--convert-only", action="store_true",
                        help="Only run Stage 2 (convert), skip build/verify")
    parser.add_argument("--skip-verify", action="store_true",
                        help="Skip Stage 4 (verification)")
    args = parser.parse_args()

    if not args.figma_url and not args.project_dir:
        print("ERROR: Provide either --figma-url or --project-dir",
              file=sys.stderr)
        sys.exit(1)

    egui_root = _find_egui_root()
    os.chdir(egui_root)

    start_time = time.time()
    print(f"Figma Make → EGUI Pipeline")
    print(f"  App: {args.app}")
    print(f"  Screen: {args.width}x{args.height}")
    print(f"  EGUI root: {egui_root}")

    # Stage 1: Capture
    ref_dir = os.path.join(egui_root, "example", args.app,
                           ".eguiproject", "reference_frames")
    if args.figma_url and not args.skip_capture:
        success, ref_dir = stage_capture(
            args.figma_url, args.app, args.width, args.height, egui_root
        )
        if not success:
            print("\nPipeline FAILED at Stage 1 (CAPTURE)")
            sys.exit(1)
    else:
        print("\n[Stage 1: CAPTURE] Skipped")

    # Stage 2: Convert
    project_dir = args.project_dir
    if not project_dir:
        # If we captured from URL, the project should be in a temp dir
        # For now, require --project-dir when not using URL
        print("WARNING: --project-dir not specified. "
              "Stage 2 requires a local Figma Make project.", file=sys.stderr)
        if args.convert_only:
            sys.exit(1)
    else:
        convert_result = stage_convert(
            project_dir, args.app, args.width, args.height, egui_root
        )

    if args.convert_only:
        elapsed = time.time() - start_time
        print(f"\nConversion complete in {elapsed:.1f}s")
        print(f"  App dir: {os.path.join(egui_root, 'example', args.app)}")
        sys.exit(0)

    # Stage 3: Build & Run
    success, rendered_dir = stage_build_and_run(
        args.app, egui_root, args.width, args.height
    )
    if not success:
        print("\nPipeline FAILED at Stage 3 (BUILD & RUN)")
        sys.exit(1)

    # Stage 4: Verify
    if args.skip_verify:
        print("\n[Stage 4: VERIFY] Skipped")
    else:
        summary = stage_verify(ref_dir, rendered_dir, args.app, egui_root)

        elapsed = time.time() - start_time
        print(f"\n{'=' * 60}")
        print(f"PIPELINE COMPLETE ({elapsed:.1f}s)")
        print(f"{'=' * 60}")
        print(f"  Pass: {summary['passed']}/{summary['total']}")
        if summary["failed"] > 0:
            print(f"  FAILED: {summary['failed']} comparisons below threshold")
            sys.exit(1)
        elif summary["warned"] > 0:
            print(f"  WARNINGS: {summary['warned']} comparisons marginal")
            sys.exit(2)
        else:
            print(f"  ALL PASS")
            sys.exit(0)


if __name__ == "__main__":
    main()

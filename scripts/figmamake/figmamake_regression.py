#!/usr/bin/env python3
"""Stage 4: SSIM regression verification for Figma Make → EGUI pipeline.

Compares reference frames (from Figma Make capture) against rendered frames
(from EGUI runtime) using Structural Similarity Index (SSIM).

Thresholds:
  - Static (final state): SSIM >= 0.85 → PASS
  - Animation keyframes:  SSIM >= 0.70 → PASS
  - Below 0.60 → FAIL (hard failure)

Usage:
    python scripts/figmamake/figmamake_regression.py \
        --reference-dir .eguiproject/reference_frames/ \
        --rendered-dir runtime_check_output/HelloBattery/default/ \
        --output regression_report.html
"""

import argparse
import json
import os
import sys
from pathlib import Path
from datetime import datetime

# SSIM thresholds
SSIM_STATIC_PASS = 0.85
SSIM_ANIM_PASS = 0.70
SSIM_HARD_FAIL = 0.60


def _compute_ssim(img_a_path: str, img_b_path: str) -> float:
    """Compute SSIM between two images.

    Uses PIL for image loading. Falls back to simple pixel comparison
    if scikit-image is not available.
    """
    try:
        from PIL import Image
        import numpy as np
    except ImportError:
        print("WARNING: PIL/numpy not available. Using file-size heuristic.",
              file=sys.stderr)
        return _file_size_similarity(img_a_path, img_b_path)

    img_a = Image.open(img_a_path).convert("RGB")
    img_b = Image.open(img_b_path).convert("RGB")

    # Resize to match if different dimensions
    if img_a.size != img_b.size:
        img_b = img_b.resize(img_a.size, Image.LANCZOS)

    arr_a = np.array(img_a, dtype=np.float64)
    arr_b = np.array(img_b, dtype=np.float64)

    try:
        from skimage.metrics import structural_similarity
        score = structural_similarity(arr_a, arr_b, channel_axis=2)
        return float(score)
    except ImportError:
        # Fallback: normalized mean squared error → pseudo-SSIM
        mse = np.mean((arr_a - arr_b) ** 2)
        max_val = 255.0 ** 2
        pseudo_ssim = 1.0 - (mse / max_val)
        return max(0.0, pseudo_ssim)


def _file_size_similarity(path_a: str, path_b: str) -> float:
    """Crude similarity based on file size ratio (last resort)."""
    size_a = os.path.getsize(path_a)
    size_b = os.path.getsize(path_b)
    if size_a == 0 or size_b == 0:
        return 0.0
    ratio = min(size_a, size_b) / max(size_a, size_b)
    return ratio


class RegressionResult:
    """Result of a single frame comparison."""

    def __init__(self, name, ref_path, rendered_path, ssim, threshold, is_anim=False):
        self.name = name
        self.ref_path = ref_path
        self.rendered_path = rendered_path
        self.ssim = ssim
        self.threshold = threshold
        self.is_anim = is_anim

    @property
    def passed(self):
        return self.ssim >= self.threshold

    @property
    def hard_fail(self):
        return self.ssim < SSIM_HARD_FAIL

    @property
    def status(self):
        if self.ssim >= self.threshold:
            return "PASS"
        elif self.ssim >= SSIM_HARD_FAIL:
            return "WARN"
        else:
            return "FAIL"

    def to_dict(self):
        return {
            "name": self.name,
            "ref": self.ref_path,
            "rendered": self.rendered_path,
            "ssim": round(self.ssim, 4),
            "threshold": self.threshold,
            "status": self.status,
            "is_animation": self.is_anim,
        }


class RegressionVerifier:
    """Compare reference frames against rendered frames."""

    def __init__(self, reference_dir: str, rendered_dir: str):
        self.reference_dir = reference_dir
        self.rendered_dir = rendered_dir
        self.results = []

    def verify_static(self, page_name: str = None) -> list:
        """Verify static (final state) frames.

        Matches reference files named like:
            {page_name}_static.png or static_*.png
        Against rendered files named like:
            frame_NNNN.png (last frame = final state)
        """
        ref_dir = Path(self.reference_dir)
        rendered_dir = Path(self.rendered_dir)

        # Find reference static frames
        ref_patterns = ["*_static.png", "static_*.png", "*_final.png"]
        ref_files = []
        for pattern in ref_patterns:
            ref_files.extend(ref_dir.glob(pattern))

        if not ref_files:
            # Use all reference PNGs as static
            ref_files = sorted(ref_dir.glob("*.png"))

        # Find rendered frames (last frame = final state)
        rendered_frames = sorted(rendered_dir.glob("frame_*.png"))
        if not rendered_frames:
            print(f"WARNING: No rendered frames in {rendered_dir}",
                  file=sys.stderr)
            return []

        last_frame = str(rendered_frames[-1])

        results = []
        for ref_file in ref_files:
            name = ref_file.stem
            if page_name and page_name not in name:
                continue

            ssim = _compute_ssim(str(ref_file), last_frame)
            result = RegressionResult(
                name=f"static/{name}",
                ref_path=str(ref_file),
                rendered_path=last_frame,
                ssim=ssim,
                threshold=SSIM_STATIC_PASS,
                is_anim=False,
            )
            results.append(result)
            self.results.append(result)

        return results

    def verify_animation_keyframes(self, page_name: str = None) -> list:
        """Verify animation keyframes at 0%, 50%, 100%.

        Matches reference files named like:
            {page_name}_anim_0.png, {page_name}_anim_50.png, {page_name}_anim_100.png
        Against rendered frames at corresponding positions.
        """
        ref_dir = Path(self.reference_dir)
        rendered_dir = Path(self.rendered_dir)

        rendered_frames = sorted(rendered_dir.glob("frame_*.png"))
        if not rendered_frames:
            return []

        n_frames = len(rendered_frames)

        # Find animation reference frames
        results = []
        for pct in [0, 50, 100]:
            patterns = [
                f"*_anim_{pct}.png",
                f"*_keyframe_{pct}.png",
                f"*_{pct}pct.png",
            ]
            ref_file = None
            for pattern in patterns:
                matches = list(ref_dir.glob(pattern))
                if matches:
                    ref_file = matches[0]
                    break

            if not ref_file:
                continue

            # Map percentage to frame index
            frame_idx = min(int(pct / 100.0 * (n_frames - 1)), n_frames - 1)
            rendered_frame = str(rendered_frames[frame_idx])

            ssim = _compute_ssim(str(ref_file), rendered_frame)
            result = RegressionResult(
                name=f"anim_{pct}%/{ref_file.stem}",
                ref_path=str(ref_file),
                rendered_path=rendered_frame,
                ssim=ssim,
                threshold=SSIM_ANIM_PASS,
                is_anim=True,
            )
            results.append(result)
            self.results.append(result)

        return results

    def verify_all(self, page_name: str = None) -> dict:
        """Run all verifications and return summary."""
        static_results = self.verify_static(page_name)
        anim_results = self.verify_animation_keyframes(page_name)

        total = len(self.results)
        passed = sum(1 for r in self.results if r.passed)
        failed = sum(1 for r in self.results if r.hard_fail)
        warned = total - passed - failed

        return {
            "total": total,
            "passed": passed,
            "warned": warned,
            "failed": failed,
            "all_passed": failed == 0 and warned == 0,
            "results": [r.to_dict() for r in self.results],
        }


def generate_html_report(summary: dict, output_path: str):
    """Generate an HTML regression report with side-by-side comparisons."""
    html_parts = [
        "<!DOCTYPE html>",
        "<html><head><meta charset='utf-8'>",
        "<title>EGUI Regression Report</title>",
        "<style>",
        "body { font-family: monospace; background: #1a1a2e; color: #e0e0e0; padding: 20px; }",
        "h1 { color: #00f3ff; }",
        ".pass { color: #22c55e; } .warn { color: #f59e0b; } .fail { color: #ef4444; }",
        ".comparison { display: flex; gap: 10px; margin: 10px 0; padding: 10px;",
        "  background: #16213e; border-radius: 8px; }",
        ".comparison img { max-width: 320px; border: 1px solid #333; }",
        ".score { font-size: 24px; font-weight: bold; padding: 10px; }",
        "table { border-collapse: collapse; width: 100%; }",
        "th, td { padding: 8px; text-align: left; border-bottom: 1px solid #333; }",
        "th { color: #00f3ff; }",
        "</style></head><body>",
    ]

    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    html_parts.append(f"<h1>EGUI Regression Report</h1>")
    html_parts.append(f"<p>Generated: {timestamp}</p>")

    # Summary
    s = summary
    status_class = "pass" if s["all_passed"] else ("warn" if s["failed"] == 0 else "fail")
    status_text = "ALL PASS" if s["all_passed"] else f"{s['failed']} FAIL, {s['warned']} WARN"
    html_parts.append(f"<h2 class='{status_class}'>{status_text}</h2>")
    html_parts.append(f"<p>Total: {s['total']} | Pass: {s['passed']} | "
                       f"Warn: {s['warned']} | Fail: {s['failed']}</p>")

    # Results table
    html_parts.append("<table><tr><th>Name</th><th>SSIM</th><th>Threshold</th>"
                       "<th>Status</th><th>Type</th></tr>")
    for r in s["results"]:
        cls = r["status"].lower()
        anim_tag = "anim" if r["is_animation"] else "static"
        html_parts.append(
            f"<tr><td>{r['name']}</td>"
            f"<td>{r['ssim']:.4f}</td>"
            f"<td>{r['threshold']:.2f}</td>"
            f"<td class='{cls}'>{r['status']}</td>"
            f"<td>{anim_tag}</td></tr>"
        )
    html_parts.append("</table>")

    # Visual comparisons
    html_parts.append("<h2>Visual Comparisons</h2>")
    for r in s["results"]:
        cls = r["status"].lower()
        ref_rel = os.path.relpath(r["ref"], os.path.dirname(output_path))
        rendered_rel = os.path.relpath(r["rendered"], os.path.dirname(output_path))
        html_parts.append(f"<div class='comparison'>")
        html_parts.append(f"  <div><p>Reference</p><img src='{ref_rel}'></div>")
        html_parts.append(f"  <div><p>Rendered</p><img src='{rendered_rel}'></div>")
        html_parts.append(f"  <div class='score {cls}'>{r['ssim']:.4f}</div>")
        html_parts.append(f"</div>")

    html_parts.append("</body></html>")

    os.makedirs(os.path.dirname(output_path) or ".", exist_ok=True)
    with open(output_path, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(html_parts))

    print(f"Report written to: {output_path}", file=sys.stderr)


def main():
    parser = argparse.ArgumentParser(
        description="SSIM regression verification for Figma Make → EGUI"
    )
    parser.add_argument("--reference-dir", required=True,
                        help="Directory with reference frames from Figma Make capture")
    parser.add_argument("--rendered-dir", required=True,
                        help="Directory with rendered frames from EGUI runtime")
    parser.add_argument("--output", default=None,
                        help="Output HTML report path")
    parser.add_argument("--json", default=None,
                        help="Output JSON results path")
    parser.add_argument("--page", default=None,
                        help="Filter to specific page name")
    args = parser.parse_args()

    verifier = RegressionVerifier(args.reference_dir, args.rendered_dir)
    summary = verifier.verify_all(args.page)

    # Print console summary
    print(f"\n{'=' * 50}")
    print(f"REGRESSION RESULTS")
    print(f"{'=' * 50}")
    for r in summary["results"]:
        status = r["status"]
        print(f"  [{status:4s}] {r['name']:30s}  SSIM={r['ssim']:.4f}  (>={r['threshold']:.2f})")
    print(f"{'=' * 50}")
    print(f"Total: {summary['total']}  Pass: {summary['passed']}  "
          f"Warn: {summary['warned']}  Fail: {summary['failed']}")

    # Generate reports
    if args.output:
        generate_html_report(summary, args.output)

    if args.json:
        os.makedirs(os.path.dirname(args.json) or ".", exist_ok=True)
        with open(args.json, "w", encoding="utf-8", newline="\n") as f:
            json.dump(summary, f, indent=2)
        print(f"JSON results: {args.json}", file=sys.stderr)

    # Exit code
    if summary["failed"] > 0:
        sys.exit(1)
    elif summary["warned"] > 0:
        sys.exit(2)
    else:
        sys.exit(0)


if __name__ == "__main__":
    main()

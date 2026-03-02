#!/usr/bin/env python3
"""Compare Figma design screenshot with EmbeddedGUI rendered output."""
import argparse
import sys

try:
    from PIL import Image, ImageChops, ImageDraw, ImageFont
except ImportError:
    print("ERROR: Pillow required. Install with: pip install Pillow")
    sys.exit(1)


def compare_images(design_path, rendered_path, output_path, target_size=None):
    """Compare two images and generate a side-by-side diff.

    Returns similarity score (0.0 - 1.0).
    """
    design = Image.open(design_path).convert("RGB")
    rendered = Image.open(rendered_path).convert("RGB")

    # Resize to common dimensions
    if target_size:
        w, h = target_size
    else:
        w, h = rendered.size

    design = design.resize((w, h), Image.LANCZOS)
    rendered = rendered.resize((w, h), Image.LANCZOS)

    # Compute pixel diff
    diff = ImageChops.difference(design, rendered)

    # Amplify diff for visibility
    diff_amplified = diff.point(lambda p: min(255, p * 3))

    # Build 3-panel comparison: [Design] [Rendered] [Diff]
    panel_w = w
    total_w = panel_w * 3
    comparison = Image.new("RGB", (total_w, h + 20), (30, 30, 30))

    comparison.paste(design, (0, 20))
    comparison.paste(rendered, (panel_w, 20))
    comparison.paste(diff_amplified, (panel_w * 2, 20))

    # Add labels
    draw = ImageDraw.Draw(comparison)
    labels = ["Design", "Rendered", "Diff (3x)"]
    for i, label in enumerate(labels):
        draw.text((i * panel_w + 4, 2), label, fill=(200, 200, 200))

    comparison.save(output_path)

    # Compute similarity score
    diff_pixels = sum(1 for p in diff.getdata() if sum(p) > 30)
    total = w * h
    similarity = 1.0 - (diff_pixels / total)
    return similarity


def main():
    parser = argparse.ArgumentParser(description="Compare design vs rendered screenshots")
    parser.add_argument("--design", "-d", required=True, help="Design screenshot path")
    parser.add_argument("--rendered", "-r", required=True, help="Rendered screenshot path")
    parser.add_argument("--output", "-o", default="comparison.png", help="Output comparison image")
    parser.add_argument("--size", "-s", default=None, help="Target size WxH (e.g., 320x480)")
    args = parser.parse_args()

    target_size = None
    if args.size:
        target_size = tuple(map(int, args.size.lower().split("x")))

    score = compare_images(args.design, args.rendered, args.output, target_size)
    print(f"Similarity: {score:.1%}")
    print(f"Comparison saved: {args.output}")

    if score >= 0.85:
        print("PASS: High fidelity match")
    elif score >= 0.70:
        print("WARN: Moderate differences detected")
    else:
        print("FAIL: Significant differences - review comparison image")

    return 0 if score >= 0.85 else 1


if __name__ == "__main__":
    sys.exit(main())

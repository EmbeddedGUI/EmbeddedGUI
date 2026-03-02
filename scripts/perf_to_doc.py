#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Performance data to documentation converter for EmbeddedGUI.

Reads JSON performance data from perf_output/ and generates Markdown reports
with matplotlib charts into doc/source/performance/.

Usage:
    python scripts/perf_to_doc.py
    python scripts/perf_to_doc.py --only perf        # only perf_report
    python scripts/perf_to_doc.py --only pfb         # only pfb_matrix_report
    python scripts/perf_to_doc.py --only spi         # only spi_matrix_report
"""

import os
import sys
import json
import argparse
from pathlib import Path
from datetime import datetime

SCRIPT_DIR = Path(__file__).parent
PROJECT_ROOT = SCRIPT_DIR.parent
PERF_OUTPUT = PROJECT_ROOT / "perf_output"
DOC_DIR = PROJECT_ROOT / "doc" / "source" / "performance"
IMG_DIR = DOC_DIR / "images"


def setup_matplotlib():
    """Configure matplotlib for headless rendering with optional CJK font."""
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    import matplotlib.font_manager as fm

    # Try Chinese font, fall back to default
    cjk_font = None
    for name in ["SimHei", "Microsoft YaHei", "WenQuanYi Micro Hei"]:
        matches = [f for f in fm.fontManager.ttflist if name in f.name]
        if matches:
            cjk_font = name
            break

    if cjk_font:
        plt.rcParams["font.sans-serif"] = [cjk_font, "DejaVu Sans"]
        plt.rcParams["axes.unicode_minus"] = False
    else:
        print("  [WARN] CJK font not found, using English labels")

    plt.rcParams["figure.facecolor"] = "white"
    plt.rcParams["axes.facecolor"] = "white"
    plt.rcParams["savefig.facecolor"] = "white"
    return plt


def load_json(path):
    """Load a JSON file, return None if missing."""
    if not path.exists():
        print(f"  [WARN] File not found: {path}")
        return None
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


# ---------------------------------------------------------------------------
# 1. Performance Report (perf_results.json + baseline.json)
# ---------------------------------------------------------------------------

def generate_perf_report():
    """Generate performance comparison report with bar chart."""
    plt = setup_matplotlib()

    results = load_json(PERF_OUTPUT / "perf_results.json")
    baseline = load_json(PERF_OUTPUT / "baseline.json")
    if not results:
        print("  [SKIP] perf_results.json not found")
        return

    timestamp = results.get("timestamp", "N/A")
    commit = results.get("git_commit", "N/A")
    threshold = results.get("threshold_percent", 10)

    # Collect all profile data
    profiles = results.get("profiles", {})
    if not profiles:
        print("  [SKIP] No profile data in perf_results.json")
        return

    # Use first profile for chart
    profile_name = list(profiles.keys())[0]
    data = profiles[profile_name]

    test_names = list(data.keys())
    current_times = [data[t]["time_ms"] for t in test_names]
    baseline_times = [data[t].get("baseline_ms") for t in test_names]
    change_pcts = [data[t].get("change_pct") for t in test_names]

    # --- Bar chart ---
    fig, ax = plt.subplots(figsize=(14, 8), dpi=150)

    colors = []
    for i, t in enumerate(test_names):
        pct = change_pcts[i]
        bl = baseline_times[i]
        if bl is None or pct is None:
            colors.append("#4A90D9")  # blue = new test
        elif pct < -5:
            colors.append("#27AE60")  # green = improved
        elif pct > threshold:
            colors.append("#E74C3C")  # red = regression
        else:
            colors.append("#95A5A6")  # gray = stable

    y_pos = range(len(test_names))
    bars = ax.barh(y_pos, current_times, color=colors, edgecolor="none", height=0.7)

    # Overlay baseline markers
    for i, bl in enumerate(baseline_times):
        if bl is not None:
            ax.plot(bl, i, marker="|", color="black", markersize=10, markeredgewidth=1.5)

    ax.set_yticks(y_pos)
    ax.set_yticklabels(test_names, fontsize=7)
    ax.set_xlabel("Time (ms)")
    ax.set_title(f"EGUI Performance - {profile_name} (commit {commit})")
    ax.invert_yaxis()

    # Legend
    from matplotlib.patches import Patch
    legend_items = [
        Patch(facecolor="#27AE60", label="Improved (>5%)"),
        Patch(facecolor="#E74C3C", label=f"Regression (>{threshold}%)"),
        Patch(facecolor="#95A5A6", label="Stable"),
        Patch(facecolor="#4A90D9", label="New (no baseline)"),
    ]
    ax.legend(handles=legend_items, loc="lower right", fontsize=8)

    plt.tight_layout()
    IMG_DIR.mkdir(parents=True, exist_ok=True)
    chart_path = IMG_DIR / "perf_report.png"
    fig.savefig(chart_path)
    plt.close(fig)
    print(f"  Chart saved: {chart_path}")

    # --- Markdown ---
    lines = [
        "# Performance Report",
        "",
        f"- Commit: `{commit}`",
        f"- Date: {timestamp}",
        f"- Profile: {profile_name}",
        f"- Regression threshold: {threshold}%",
        "",
        "![Performance Chart](images/perf_report.png)",
        "",
        "| Test Case | Current (ms) | Baseline (ms) | Change (%) | Status |",
        "|-----------|-------------|---------------|-----------|--------|",
    ]

    for t in test_names:
        d = data[t]
        cur = f"{d['time_ms']:.3f}"
        bl = f"{d['baseline_ms']:.3f}" if d.get("baseline_ms") is not None else "N/A"
        pct = f"{d['change_pct']:+.1f}" if d.get("change_pct") is not None else "N/A"
        if d.get("regression"):
            status = "REGRESSION"
        elif d.get("baseline_ms") is None:
            status = "NEW"
        elif d.get("change_pct", 0) and d["change_pct"] < -5:
            status = "IMPROVED"
        else:
            status = "OK"
        lines.append(f"| {t} | {cur} | {bl} | {pct} | {status} |")

    md_path = DOC_DIR / "perf_report.md"
    md_path.write_text("\n".join(lines), encoding="utf-8")
    print(f"  Report saved: {md_path}")


# ---------------------------------------------------------------------------
# 2. PFB Matrix Report (pfb_matrix_results.json)
# ---------------------------------------------------------------------------

def generate_pfb_matrix_report():
    """Generate PFB size matrix heatmap report."""
    plt = setup_matplotlib()
    import numpy as np

    data = load_json(PERF_OUTPUT / "pfb_matrix_results.json")
    if not data:
        print("  [SKIP] pfb_matrix_results.json not found")
        return

    timestamp = data.get("timestamp", "N/A")
    commit = data.get("git_commit", "N/A")
    profile = data.get("profile", "N/A")
    matrix = data.get("pfb_matrix", {})

    if not matrix:
        print("  [SKIP] No pfb_matrix data")
        return

    config_names = list(matrix.keys())
    config_labels = []
    for name in config_names:
        cfg = matrix[name]
        w, h = cfg["pfb_width"], cfg["pfb_height"]
        config_labels.append(f"{name}\n({w}x{h})")

    # Collect test names from first config
    first_cfg = matrix[config_names[0]]
    all_tests = list(first_cfg["results"].keys())

    # Filter out tests with near-zero values across all configs (not meaningful)
    test_names = []
    for t in all_tests:
        vals = [matrix[c]["results"].get(t, {}).get("time_ms", 0) for c in config_names]
        if max(vals) > 0.5:
            test_names.append(t)

    # Build matrix
    heat = np.zeros((len(test_names), len(config_names)))
    for j, cfg_name in enumerate(config_names):
        results = matrix[cfg_name]["results"]
        for i, t in enumerate(test_names):
            heat[i, j] = results.get(t, {}).get("time_ms", 0)

    # --- Heatmap ---
    fig, ax = plt.subplots(figsize=(14, 8), dpi=150)
    im = ax.imshow(heat, aspect="auto", cmap="YlOrRd")

    ax.set_xticks(range(len(config_names)))
    ax.set_xticklabels(config_labels, fontsize=8)
    ax.set_yticks(range(len(test_names)))
    ax.set_yticklabels(test_names, fontsize=7)
    ax.set_title(f"PFB Size Matrix - {profile} (commit {commit})")

    # Annotate cells
    for i in range(len(test_names)):
        for j in range(len(config_names)):
            val = heat[i, j]
            color = "white" if val > heat.max() * 0.6 else "black"
            ax.text(j, i, f"{val:.1f}", ha="center", va="center",
                    fontsize=5, color=color)

    fig.colorbar(im, ax=ax, label="Time (ms)", shrink=0.8)
    plt.tight_layout()

    IMG_DIR.mkdir(parents=True, exist_ok=True)
    chart_path = IMG_DIR / "pfb_matrix_report.png"
    fig.savefig(chart_path)
    plt.close(fig)
    print(f"  Chart saved: {chart_path}")

    # --- Markdown ---
    lines = [
        "# PFB Matrix Report",
        "",
        f"- Commit: `{commit}`",
        f"- Date: {timestamp}",
        f"- Profile: {profile}",
        "",
        "![PFB Matrix Heatmap](images/pfb_matrix_report.png)",
        "",
    ]

    # Table header
    header = "| Test Case | " + " | ".join(config_labels).replace("\n", " ") + " |"
    sep = "|-----------|" + "|".join(["----------:" for _ in config_names]) + "|"
    lines.append(header)
    lines.append(sep)

    for t in test_names:
        row = f"| {t} "
        for cfg_name in config_names:
            val = matrix[cfg_name]["results"].get(t, {}).get("time_ms", 0)
            row += f"| {val:.3f} "
        row += "|"
        lines.append(row)

    md_path = DOC_DIR / "pfb_matrix_report.md"
    md_path.write_text("\n".join(lines), encoding="utf-8")
    print(f"  Report saved: {md_path}")


# ---------------------------------------------------------------------------
# 3. SPI Matrix Report (spi_matrix_results.json)
# ---------------------------------------------------------------------------

def generate_spi_matrix_report():
    """Generate SPI buffer configuration grouped bar chart report."""
    plt = setup_matplotlib()
    import numpy as np

    data = load_json(PERF_OUTPUT / "spi_matrix_results.json")
    if not data:
        print("  [SKIP] spi_matrix_results.json not found")
        return

    timestamp = data.get("timestamp", "N/A")
    commit = data.get("git_commit", "N/A")
    profile = data.get("profile", "N/A")
    spi_matrix = data.get("spi_matrix", {})

    if not spi_matrix:
        print("  [SKIP] No spi_matrix data")
        return

    config_names = list(spi_matrix.keys())
    config_labels = []
    for name in config_names:
        cfg = spi_matrix[name]
        spi = cfg["spi_mhz"]
        buf = cfg["buffer_count"]
        if spi == 0:
            config_labels.append("CPU only")
        else:
            config_labels.append(f"SPI {spi}MHz\nbuf={buf}")

    # Collect test names, filter low-value ones
    first_cfg = spi_matrix[config_names[0]]
    all_tests = list(first_cfg["results"].keys())
    test_names = []
    for t in all_tests:
        vals = [spi_matrix[c]["results"].get(t, {}).get("time_ms", 0) for c in config_names]
        if max(vals) > 1.0:
            test_names.append(t)

    # --- Grouped bar chart ---
    n_tests = len(test_names)
    n_configs = len(config_names)
    x = np.arange(n_tests)
    width = 0.8 / n_configs

    palette = ["#2ECC71", "#3498DB", "#E67E22", "#9B59B6", "#E74C3C",
               "#1ABC9C", "#F39C12", "#8E44AD"]

    fig, ax = plt.subplots(figsize=(14, 8), dpi=150)

    for j, cfg_name in enumerate(config_names):
        results = spi_matrix[cfg_name]["results"]
        vals = [results.get(t, {}).get("time_ms", 0) for t in test_names]
        offset = (j - n_configs / 2 + 0.5) * width
        ax.bar(x + offset, vals, width, label=config_labels[j].replace("\n", " "),
               color=palette[j % len(palette)], edgecolor="none")

    ax.set_xticks(x)
    ax.set_xticklabels(test_names, rotation=45, ha="right", fontsize=7)
    ax.set_ylabel("Time (ms)")
    ax.set_title(f"SPI Buffer Configuration Comparison - {profile} (commit {commit})")
    ax.legend(fontsize=8, loc="upper left")

    plt.tight_layout()
    IMG_DIR.mkdir(parents=True, exist_ok=True)
    chart_path = IMG_DIR / "spi_matrix_report.png"
    fig.savefig(chart_path)
    plt.close(fig)
    print(f"  Chart saved: {chart_path}")

    # --- Markdown ---
    lines = [
        "# SPI Matrix Report",
        "",
        f"- Commit: `{commit}`",
        f"- Date: {timestamp}",
        f"- Profile: {profile}",
        "",
        "![SPI Matrix Chart](images/spi_matrix_report.png)",
        "",
    ]

    # Table
    header = "| Test Case | " + " | ".join(
        cl.replace("\n", " ") for cl in config_labels) + " |"
    sep = "|-----------|" + "|".join(["----------:" for _ in config_names]) + "|"
    lines.append(header)
    lines.append(sep)

    for t in test_names:
        row = f"| {t} "
        for cfg_name in config_names:
            val = spi_matrix[cfg_name]["results"].get(t, {}).get("time_ms", 0)
            row += f"| {val:.3f} "
        row += "|"
        lines.append(row)

    md_path = DOC_DIR / "spi_matrix_report.md"
    md_path.write_text("\n".join(lines), encoding="utf-8")
    print(f"  Report saved: {md_path}")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description="Generate performance documentation")
    parser.add_argument("--only", choices=["perf", "pfb", "spi"],
                        help="Generate only one report type")
    args = parser.parse_args()

    DOC_DIR.mkdir(parents=True, exist_ok=True)
    IMG_DIR.mkdir(parents=True, exist_ok=True)

    generators = {
        "perf": ("Performance Report", generate_perf_report),
        "pfb": ("PFB Matrix Report", generate_pfb_matrix_report),
        "spi": ("SPI Matrix Report", generate_spi_matrix_report),
    }

    targets = [args.only] if args.only else list(generators.keys())

    for key in targets:
        label, func = generators[key]
        print(f"\n=== Generating {label} ===")
        try:
            func()
        except Exception as e:
            print(f"  [ERROR] {label}: {e}")
            import traceback
            traceback.print_exc()

    print("\nDone.")


if __name__ == "__main__":
    main()

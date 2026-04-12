#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Performance data to documentation converter for EmbeddedGUI.

Reads JSON performance data from perf_output/ and generates Markdown reports
with matplotlib charts into doc/source/performance/.

Usage:
    python scripts/perf_analysis/main.py perf-to-doc
    python scripts/perf_analysis/main.py perf-to-doc --only perf   # only perf_report
    python scripts/perf_analysis/main.py perf-to-doc --only pfb    # only pfb_matrix_report
    python scripts/perf_analysis/main.py perf-to-doc --only spi    # only spi_matrix_report
"""

import os
import sys
import json
import argparse
import shutil
from pathlib import Path
from datetime import datetime

SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent.parent
PERF_OUTPUT = PROJECT_ROOT / "perf_output"
DOC_DIR = PROJECT_ROOT / "doc" / "source" / "performance"
IMG_DIR = DOC_DIR / "images"
SCENE_SHEET_FILE = PERF_OUTPUT / "perf_scenes.png"
SCENE_INDEX_FILE = PERF_OUTPUT / "perf_scenes_index.json"


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
        print(f"  [INFO] File not found (skipping): {path.name}")
        return None
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def copy_optional_asset(src: Path, dst: Path):
    """Copy an optional asset if it exists."""
    if not src.exists():
        return False
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)
    return True


def has_matching_scene_sheet(commit: str, profile_name: str) -> bool:
    """Return True when perf_scenes.png matches the current perf results."""
    if not SCENE_SHEET_FILE.exists() or not SCENE_INDEX_FILE.exists():
        return False

    scene_index = load_json(SCENE_INDEX_FILE)
    if not scene_index:
        return False

    capture_commit = scene_index.get("git_commit")
    results_commit = scene_index.get("perf_results_commit", capture_commit)
    return capture_commit == commit and results_commit == commit and scene_index.get("profile") == profile_name


# ---------------------------------------------------------------------------
# 1. Performance Report (perf_results.json)
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
# Category definitions for performance report grouping.
# Keys must match the string names emitted by type_string / PERF_RESULT output.
# ---------------------------------------------------------------------------
CATEGORY_GROUPS = [
    ("Basic Shapes", [
        "LINE", "LINE_HQ",
        "RECTANGLE", "RECTANGLE_FILL",
        "CIRCLE", "CIRCLE_FILL", "CIRCLE_HQ", "CIRCLE_FILL_HQ",
        "ARC", "ARC_FILL", "ARC_HQ", "ARC_FILL_HQ",
        "ROUND_RECTANGLE", "ROUND_RECTANGLE_CORNERS",
        "ROUND_RECTANGLE_FILL", "ROUND_RECTANGLE_CORNERS_FILL",
        "TRIANGLE", "TRIANGLE_FILL",
        "ELLIPSE", "ELLIPSE_FILL",
        "POLYGON", "POLYGON_FILL",
        "BEZIER_QUAD", "BEZIER_CUBIC",
        "CIRCLE_FILL_QUARTER", "CIRCLE_FILL_DOUBLE",
        "ROUND_RECTANGLE_FILL_QUARTER", "ROUND_RECTANGLE_FILL_DOUBLE",
        "TRIANGLE_FILL_QUARTER", "TRIANGLE_FILL_DOUBLE",
    ]),
    ("Text", [
        "TEXT", "TEXT_RECT",
        "INTERNAL_TEXT", "INTERNAL_TEXT_RECT",
        "INTERNAL_TEXT_RLE4", "INTERNAL_TEXT_RECT_RLE4",
        "INTERNAL_TEXT_RLE4_XOR", "INTERNAL_TEXT_RECT_RLE4_XOR",
        "EXTERN_TEXT", "EXTERN_TEXT_RECT",
        "EXTERN_TEXT_RLE4", "EXTERN_TEXT_RECT_RLE4",
        "EXTERN_TEXT_RLE4_XOR", "EXTERN_TEXT_RECT_RLE4_XOR",
        "TEXT_ROTATE_NONE", "TEXT_ROTATE", "TEXT_ROTATE_RESIZE",
        "TEXT_ROTATE_QUARTER", "TEXT_ROTATE_DOUBLE",
        "EXTERN_TEXT_ROTATE",
    ]),
    ("Image Direct Draw", [
        "IMAGE_565", "IMAGE_565_1", "IMAGE_565_2", "IMAGE_565_4", "IMAGE_565_8",
        "IMAGE_565_QUARTER", "IMAGE_565_DOUBLE",
        "IMAGE_565_8_QUARTER", "IMAGE_565_8_DOUBLE",
        "EXTERN_IMAGE_565", "EXTERN_IMAGE_565_1", "EXTERN_IMAGE_565_2",
        "EXTERN_IMAGE_565_4", "EXTERN_IMAGE_565_8",
        "IMAGE_TILED_565_0", "IMAGE_TILED_565_1", "IMAGE_TILED_565_2",
        "IMAGE_TILED_565_4", "IMAGE_TILED_565_8",
        "IMAGE_TILED_STAR_565_0", "IMAGE_TILED_STAR_565_1", "IMAGE_TILED_STAR_565_2",
        "IMAGE_TILED_STAR_565_4", "IMAGE_TILED_STAR_565_8",
        "EXTERN_IMAGE_TILED_565_0", "EXTERN_IMAGE_TILED_565_1", "EXTERN_IMAGE_TILED_565_2",
        "EXTERN_IMAGE_TILED_565_4", "EXTERN_IMAGE_TILED_565_8",
    ]),
    ("Image Resize", [
        "IMAGE_RESIZE_565", "IMAGE_RESIZE_565_1", "IMAGE_RESIZE_565_2",
        "IMAGE_RESIZE_565_4", "IMAGE_RESIZE_565_8",
        "EXTERN_IMAGE_RESIZE_565", "EXTERN_IMAGE_RESIZE_565_1",
        "EXTERN_IMAGE_RESIZE_565_2", "EXTERN_IMAGE_RESIZE_565_4",
        "EXTERN_IMAGE_RESIZE_565_8",
        "IMAGE_RESIZE_STAR_565_1", "IMAGE_RESIZE_STAR_565_2",
        "IMAGE_RESIZE_STAR_565_4", "IMAGE_RESIZE_STAR_565_8",
        "IMAGE_RESIZE_TILED_565_0", "IMAGE_RESIZE_TILED_565_1",
        "IMAGE_RESIZE_TILED_565_2", "IMAGE_RESIZE_TILED_565_4",
        "IMAGE_RESIZE_TILED_565_8",
        "IMAGE_RESIZE_TILED_STAR_565_0", "IMAGE_RESIZE_TILED_STAR_565_1",
        "IMAGE_RESIZE_TILED_STAR_565_2", "IMAGE_RESIZE_TILED_STAR_565_4",
        "IMAGE_RESIZE_TILED_STAR_565_8",
        "EXTERN_IMAGE_RESIZE_TILED_565_0", "EXTERN_IMAGE_RESIZE_TILED_565_1",
        "EXTERN_IMAGE_RESIZE_TILED_565_2", "EXTERN_IMAGE_RESIZE_TILED_565_4",
        "EXTERN_IMAGE_RESIZE_TILED_565_8",
    ]),
    ("Image Rotate", [
        "IMAGE_ROTATE_565", "IMAGE_ROTATE_565_1", "IMAGE_ROTATE_565_2",
        "IMAGE_ROTATE_565_4", "IMAGE_ROTATE_565_8",
        "IMAGE_ROTATE_565_RESIZE", "IMAGE_ROTATE_565_QUARTER",
        "IMAGE_ROTATE_565_DOUBLE",
        "EXTERN_IMAGE_ROTATE_565", "EXTERN_IMAGE_ROTATE_565_1",
        "EXTERN_IMAGE_ROTATE_565_2", "EXTERN_IMAGE_ROTATE_565_4",
        "EXTERN_IMAGE_ROTATE_565_8",
        "IMAGE_ROTATE_STAR_565_1", "IMAGE_ROTATE_STAR_565_2",
        "IMAGE_ROTATE_STAR_565_4", "IMAGE_ROTATE_STAR_565_8",
        "IMAGE_ROTATE_TILED_565_0", "IMAGE_ROTATE_TILED_565_1",
        "IMAGE_ROTATE_TILED_565_2", "IMAGE_ROTATE_TILED_565_4",
        "IMAGE_ROTATE_TILED_565_8",
        "IMAGE_ROTATE_TILED_STAR_565_0", "IMAGE_ROTATE_TILED_STAR_565_1",
        "IMAGE_ROTATE_TILED_STAR_565_2", "IMAGE_ROTATE_TILED_STAR_565_4",
        "IMAGE_ROTATE_TILED_STAR_565_8",
        "EXTERN_IMAGE_ROTATE_TILED_565_0", "EXTERN_IMAGE_ROTATE_TILED_565_1",
        "EXTERN_IMAGE_ROTATE_TILED_565_2", "EXTERN_IMAGE_ROTATE_TILED_565_4",
        "EXTERN_IMAGE_ROTATE_TILED_565_8",
    ]),
    ("Image Color Tint", [
        "IMAGE_COLOR", "IMAGE_RESIZE_COLOR",
    ]),
    ("Compress", [
        "EXTERN_IMAGE_QOI_565", "EXTERN_IMAGE_QOI_565_8",
        "EXTERN_MASK_IMAGE_QOI_NO_MASK", "EXTERN_MASK_IMAGE_QOI_ROUND_RECT",
        "EXTERN_MASK_IMAGE_QOI_CIRCLE", "EXTERN_MASK_IMAGE_QOI_IMAGE",
        "EXTERN_MASK_IMAGE_QOI_8_NO_MASK", "EXTERN_MASK_IMAGE_QOI_8_ROUND_RECT",
        "EXTERN_MASK_IMAGE_QOI_8_CIRCLE", "EXTERN_MASK_IMAGE_QOI_8_IMAGE",
        "EXTERN_IMAGE_RLE_565", "EXTERN_IMAGE_RLE_565_8",
        "EXTERN_MASK_IMAGE_RLE_NO_MASK", "EXTERN_MASK_IMAGE_RLE_ROUND_RECT",
        "EXTERN_MASK_IMAGE_RLE_CIRCLE", "EXTERN_MASK_IMAGE_RLE_IMAGE",
        "EXTERN_MASK_IMAGE_RLE_8_NO_MASK", "EXTERN_MASK_IMAGE_RLE_8_ROUND_RECT",
        "EXTERN_MASK_IMAGE_RLE_8_CIRCLE", "EXTERN_MASK_IMAGE_RLE_8_IMAGE",
        "IMAGE_QOI_565", "IMAGE_QOI_565_8",
        "IMAGE_RLE_565", "IMAGE_RLE_565_8",
        "IMAGE_TILED_QOI_565_0", "IMAGE_TILED_QOI_565_8",
        "IMAGE_TILED_RLE_565_0", "IMAGE_TILED_RLE_565_8",
        "MASK_IMAGE_QOI_8_CIRCLE", "MASK_IMAGE_QOI_8_IMAGE",
        "MASK_IMAGE_QOI_8_NO_MASK", "MASK_IMAGE_QOI_8_ROUND_RECT",
        "MASK_IMAGE_QOI_CIRCLE", "MASK_IMAGE_QOI_IMAGE",
        "MASK_IMAGE_QOI_NO_MASK", "MASK_IMAGE_QOI_ROUND_RECT",
        "MASK_IMAGE_RLE_8_CIRCLE", "MASK_IMAGE_RLE_8_IMAGE",
        "MASK_IMAGE_RLE_8_NO_MASK", "MASK_IMAGE_RLE_8_ROUND_RECT",
        "MASK_IMAGE_RLE_CIRCLE", "MASK_IMAGE_RLE_IMAGE",
        "MASK_IMAGE_RLE_NO_MASK", "MASK_IMAGE_RLE_ROUND_RECT",
    ]),
    ("Gradient", [
        "GRADIENT_RECT", "GRADIENT_ROUND_RECT", "GRADIENT_CIRCLE",
        "GRADIENT_TRIANGLE",
        "GRADIENT_ARC_RING", "GRADIENT_ARC_RING_ROUND_CAP",
        "GRADIENT_RADIAL", "GRADIENT_ANGULAR",
        "GRADIENT_ROUND_RECT_RING", "GRADIENT_LINE_CAPSULE",
        "GRADIENT_MULTI_STOP", "GRADIENT_ROUND_RECT_CORNERS",
        "IMAGE_GRADIENT_OVERLAY",
        "MASK_GRADIENT_RECT_FILL", "MASK_GRADIENT_IMAGE", "MASK_GRADIENT_IMAGE_ROTATE",
        "TEXT_GRADIENT", "TEXT_RECT_GRADIENT",
        "TEXT_ROTATE_GRADIENT",
    ]),
    ("Shadow", [
        "SHADOW", "SHADOW_ROUND",
    ]),
    ("Mask", [
        "MASK_RECT_FILL_NO_MASK", "MASK_RECT_FILL_ROUND_RECT",
        "MASK_RECT_FILL_CIRCLE", "MASK_RECT_FILL_IMAGE",
        "MASK_RECT_FILL_NO_MASK_QUARTER", "MASK_RECT_FILL_NO_MASK_DOUBLE",
        "MASK_RECT_FILL_ROUND_RECT_QUARTER", "MASK_RECT_FILL_ROUND_RECT_DOUBLE",
        "MASK_RECT_FILL_CIRCLE_QUARTER", "MASK_RECT_FILL_CIRCLE_DOUBLE",
        "MASK_RECT_FILL_IMAGE_QUARTER", "MASK_RECT_FILL_IMAGE_DOUBLE",
        "MASK_IMAGE_NO_MASK", "MASK_IMAGE_ROUND_RECT",
        "MASK_IMAGE_CIRCLE", "MASK_IMAGE_IMAGE",
        "EXTERN_MASK_IMAGE_NO_MASK", "EXTERN_MASK_IMAGE_ROUND_RECT",
        "EXTERN_MASK_IMAGE_CIRCLE", "EXTERN_MASK_IMAGE_IMAGE",
        "MASK_IMAGE_NO_MASK_QUARTER", "MASK_IMAGE_NO_MASK_DOUBLE",
        "MASK_IMAGE_ROUND_RECT_QUARTER", "MASK_IMAGE_ROUND_RECT_DOUBLE",
        "MASK_IMAGE_CIRCLE_QUARTER", "MASK_IMAGE_CIRCLE_DOUBLE",
        "MASK_IMAGE_IMAGE_QUARTER", "MASK_IMAGE_IMAGE_DOUBLE",
        "MASK_ROUND_RECT_FILL_NO_MASK", "MASK_ROUND_RECT_FILL_WITH_MASK",
        "MASK_IMAGE_TEST_PERF_NO_MASK", "MASK_IMAGE_TEST_PERF_ROUND_RECT",
        "MASK_IMAGE_TEST_PERF_CIRCLE", "MASK_IMAGE_TEST_PERF_IMAGE",
        "EXTERN_MASK_IMAGE_TEST_PERF_NO_MASK", "EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT",
        "EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE", "EXTERN_MASK_IMAGE_TEST_PERF_IMAGE",
    ]),
    ("Widgets", [
        "FILE_IMAGE_BMP_NORMAL",
        "FILE_IMAGE_JPG_RESIZE", "FILE_IMAGE_PNG_RESIZE", "FILE_IMAGE_BMP_RESIZE",
        "FILE_IMAGE_JPG", "FILE_IMAGE_PNG", "FILE_IMAGE_BMP",
        "CHART_LINE_DENSE", "CHART_BAR_DENSE", "CHART_SCATTER_DENSE", "CHART_PIE_DENSE",
    ]),
    ("Animation", [
        "ANIMATION_TRANSLATE", "ANIMATION_ALPHA",
        "ANIMATION_SCALE", "ANIMATION_SET",
    ]),
]


def _get_categorized_tests(available_tests):
    """Return [(category_name, [test_names])] ordered by category.
    Tests not covered by any category are appended as 'Other'."""
    result = []
    seen = set()
    for cat_name, cat_tests in CATEGORY_GROUPS:
        hits = [t for t in cat_tests if t in available_tests]
        if hits:
            result.append((cat_name, hits))
            seen.update(hits)
    remaining = [t for t in sorted(available_tests) if t not in seen]
    if remaining:
        result.append(("Other", remaining))
    return result


def _load_cpu_only_data(expected_commit=None):
    """Try to load CPU-only rendering data from spi_matrix_results.json.

    Returns (data_dict, timestamp, commit, profile_name) or None if unavailable.
    The 'no_spi' (spi_mhz==0) config in the SPI matrix provides pure CPU
    rendering times without SPI transfer overhead.
    """
    spi_data = load_json(PERF_OUTPUT / "spi_matrix_results.json")
    if not spi_data:
        return None
    if expected_commit and spi_data.get("git_commit") != expected_commit:
        return None

    spi_matrix = spi_data.get("spi_matrix", {})
    # Find the config with spi_mhz == 0 (CPU only)
    for cfg_name, cfg_data in spi_matrix.items():
        if cfg_data.get("spi_mhz", -1) == 0:
            return (
                cfg_data["results"],
                spi_data.get("timestamp", "N/A"),
                spi_data.get("git_commit", "N/A"),
                spi_data.get("profile", "N/A"),
            )
    return None


def generate_perf_report():
    """Generate single-profile performance report grouped by category.

    Prefers CPU-only data from spi_matrix_results.json (no SPI overhead).
    Falls back to perf_results.json if SPI matrix data is unavailable.
    """
    plt = setup_matplotlib()

    results = load_json(PERF_OUTPUT / "perf_results.json")
    if not results:
        print("  [SKIP] No performance data found")
        return

    fallback_timestamp = results.get("timestamp", "N/A")
    fallback_commit = results.get("git_commit", "N/A")

    profiles = results.get("profiles", {})
    if not profiles:
        print("  [SKIP] No profile data in perf_results.json")
        return

    fallback_profile_name = list(profiles.keys())[0]
    fallback_data = profiles[fallback_profile_name]

    # Prefer CPU-only data from SPI matrix (no SPI transfer overhead) only when
    # it matches the current perf_results commit. This avoids generating stale docs.
    cpu_only = _load_cpu_only_data(expected_commit=fallback_commit)
    if cpu_only:
        data, timestamp, commit, profile_name = cpu_only
        print("  Using CPU-only data from spi_matrix_results.json (no SPI overhead)")
    else:
        data = fallback_data
        timestamp = fallback_timestamp
        commit = fallback_commit
        profile_name = fallback_profile_name

    # Organize tests by category
    categorized = _get_categorized_tests(set(data.keys()))

    # Build a flat ordered list with per-test category color
    cat_palette = [
        "#4A90D9", "#E74C3C", "#2ECC71", "#F39C12", "#9B59B6",
        "#1ABC9C", "#E67E22", "#2C3E50", "#D35400", "#27AE60",
    ]
    test_order = []
    bar_colors = []
    for idx, (cat_name, tests) in enumerate(categorized):
        color = cat_palette[idx % len(cat_palette)]
        for t in tests:
            test_order.append(t)
            bar_colors.append(color)

    times = [data[t].get("time_ms", 0) for t in test_order]

    # --- Horizontal bar chart with category color coding ---
    n_tests = len(test_order)
    fig, ax = plt.subplots(figsize=(12, max(6, n_tests * 0.25)), dpi=150)

    y_pos = range(n_tests)
    ax.barh(y_pos, times, height=0.6, color=bar_colors, edgecolor="none")

    ax.set_yticks(y_pos)
    ax.set_yticklabels(test_order, fontsize=7)
    ax.set_xlabel("Time (ms)")
    ax.set_title(f"EGUI Performance ({profile_name}) - commit {commit}")
    ax.invert_yaxis()

    # Category legend
    from matplotlib.patches import Patch
    legend_elements = [
        Patch(facecolor=cat_palette[i % len(cat_palette)], label=cat_name)
        for i, (cat_name, _) in enumerate(categorized)
    ]
    ax.legend(handles=legend_elements, loc="lower right", fontsize=7,
              framealpha=0.8)

    plt.tight_layout()
    IMG_DIR.mkdir(parents=True, exist_ok=True)
    chart_path = IMG_DIR / "perf_report.png"
    fig.savefig(chart_path)
    plt.close(fig)
    print(f"  Chart saved: {chart_path}")

    # --- Markdown report: one section per category ---
    lines = [
        "# Performance Report",
        "",
        f"- Commit: `{commit}`",
        f"- Date: {timestamp}",
        f"- Profile: {profile_name}",
        "",
        "![Performance Chart](images/perf_report.png)",
        "",
    ]

    if has_matching_scene_sheet(commit, profile_name):
        copy_optional_asset(SCENE_SHEET_FILE, IMG_DIR / "perf_scenes.png")
        lines.extend([
            "## Scene Contact Sheet",
            "",
            "Timing data comes from QEMU. The contact sheet below is rendered with the PC simulator for scene reference.",
            "",
            "![Scene Contact Sheet](images/perf_scenes.png)",
            "",
        ])

    for cat_name, tests in categorized:
        lines.append(f"## {cat_name}")
        lines.append("")
        lines.append("| Test Case | Time (ms) |")
        lines.append("|-----------|-----------|")
        for t in tests:
            val = data[t].get("time_ms", 0)
            lines.append(f"| {t} | {val:.3f} |")
        lines.append("")

    md_path = DOC_DIR / "perf_report.md"
    md_path.write_text("\n".join(lines), encoding="utf-8")
    print(f"  Report saved: {md_path}")

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

    # --- Grouped horizontal bar chart (one row per test) ---
    n_tests = len(test_names)
    n_configs = len(config_names)
    row_height = 0.35          # inches per test row
    fig_h = max(6, n_tests * row_height + 2)
    fig, ax = plt.subplots(figsize=(12, fig_h), dpi=150)

    palette = ["#3498DB", "#E67E22", "#2ECC71", "#E74C3C", "#9B59B6",
               "#1ABC9C", "#F39C12", "#8E44AD"]

    bar_h = 0.8 / n_configs
    y = np.arange(n_tests)

    for j, cfg_name in enumerate(config_names):
        results = matrix[cfg_name]["results"]
        vals = [results.get(t, {}).get("time_ms", 0) for t in test_names]
        offset = (j - n_configs / 2 + 0.5) * bar_h
        label = config_labels[j].replace("\n", " ")
        ax.barh(y + offset, vals, bar_h, label=label,
                color=palette[j % len(palette)], edgecolor="none")

    ax.set_yticks(y)
    ax.set_yticklabels(test_names, fontsize=7)
    ax.invert_yaxis()
    ax.set_xlabel("Time (ms)")
    ax.set_title(f"PFB Size Matrix - {profile} (commit {commit})")
    ax.legend(fontsize=8, loc="lower right")
    ax.grid(axis="x", linewidth=0.4, alpha=0.5)

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

    # --- Grouped horizontal bar chart (one row per test) ---
    n_tests = len(test_names)
    n_configs = len(config_names)
    row_height = 0.35          # inches per test row
    fig_h = max(6, n_tests * row_height + 2)
    fig, ax = plt.subplots(figsize=(12, fig_h), dpi=150)

    palette = ["#2ECC71", "#3498DB", "#E67E22", "#9B59B6", "#E74C3C",
               "#1ABC9C", "#F39C12", "#8E44AD"]

    for j, cfg_name in enumerate(config_names):
        results = spi_matrix[cfg_name]["results"]
        vals = [results.get(t, {}).get("time_ms", 0) for t in test_names]
        bar_h = 0.8 / n_configs
        y = np.arange(n_tests)
        offset = (j - n_configs / 2 + 0.5) * bar_h
        ax.barh(y + offset, vals, bar_h, label=config_labels[j].replace("\n", " "),
                color=palette[j % len(palette)], edgecolor="none")

    y = np.arange(n_tests)
    ax.set_yticks(y)
    ax.set_yticklabels(test_names, fontsize=7)
    ax.invert_yaxis()
    ax.set_xlabel("Time (ms)")
    ax.set_title(f"SPI Buffer Configuration Comparison - {profile} (commit {commit})")
    ax.legend(fontsize=8, loc="lower right")
    ax.grid(axis="x", linewidth=0.4, alpha=0.5)

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

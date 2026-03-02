#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Binary size data to documentation converter for EmbeddedGUI.

Reads JSON size data from output/size_results.json and generates Markdown reports
with matplotlib charts into doc/source/size/.

Usage:
    python scripts/size_to_doc.py
"""

import os
import sys
import json
import argparse
from pathlib import Path
from datetime import datetime

SCRIPT_DIR = Path(__file__).parent
PROJECT_ROOT = SCRIPT_DIR.parent
SIZE_OUTPUT = PROJECT_ROOT / "output"
DOC_DIR = PROJECT_ROOT / "doc" / "source" / "size"
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


def generate_size_report():
    """Generate binary size report with stacked bar chart."""
    plt = setup_matplotlib()
    import numpy as np

    data = load_json(SIZE_OUTPUT / "size_results.json")
    if not data:
        print("  [SKIP] size_results.json not found")
        return

    timestamp = data.get("timestamp", "N/A")
    commit = data.get("git_commit", "N/A")
    apps = data.get("apps", [])

    if not apps:
        print("  [SKIP] No app data in size_results.json")
        return

    app_names = [a["name"] for a in apps]
    code_sizes = [a["code_bytes"] / 1024 for a in apps]  # KB
    resource_sizes = [a["resource_bytes"] / 1024 for a in apps]
    ram_sizes = [a["ram_bytes"] / 1024 for a in apps]
    pfb_sizes = [a["pfb_bytes"] / 1024 for a in apps]

    # --- Stacked bar chart: Code + Resource (ROM) ---
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(20, max(8, len(apps) * 0.25)), dpi=150)

    y_pos = np.arange(len(app_names))

    # Left chart: ROM (Code + Resource)
    bars_code = ax1.barh(y_pos, code_sizes, color="#3498DB", edgecolor="none",
                         height=0.7, label="Code")
    bars_res = ax1.barh(y_pos, resource_sizes, left=code_sizes, color="#E67E22",
                        edgecolor="none", height=0.7, label="Resource")

    ax1.set_yticks(y_pos)
    ax1.set_yticklabels(app_names, fontsize=6)
    ax1.set_xlabel("Size (KB)")
    ax1.set_title(f"ROM Usage: Code + Resource (commit {commit})")
    ax1.invert_yaxis()
    ax1.legend(fontsize=8, loc="lower right")

    # Right chart: RAM (RAM + PFB)
    bars_ram = ax2.barh(y_pos, ram_sizes, color="#27AE60", edgecolor="none",
                        height=0.7, label="RAM")
    bars_pfb = ax2.barh(y_pos, pfb_sizes, left=ram_sizes, color="#9B59B6",
                        edgecolor="none", height=0.7, label="PFB")

    ax2.set_yticks(y_pos)
    ax2.set_yticklabels(app_names, fontsize=6)
    ax2.set_xlabel("Size (KB)")
    ax2.set_title(f"RAM Usage: Data + PFB (commit {commit})")
    ax2.invert_yaxis()
    ax2.legend(fontsize=8, loc="lower right")

    plt.tight_layout()
    IMG_DIR.mkdir(parents=True, exist_ok=True)
    chart_path = IMG_DIR / "size_report.png"
    fig.savefig(chart_path)
    plt.close(fig)
    print(f"  Chart saved: {chart_path}")

    # --- Summary stats ---
    total_apps = len(apps)
    min_code = min(apps, key=lambda a: a["code_bytes"])
    max_code = max(apps, key=lambda a: a["code_bytes"])
    min_ram = min(apps, key=lambda a: a["ram_bytes"])
    max_ram = max(apps, key=lambda a: a["ram_bytes"])

    # --- Markdown ---
    lines = [
        "# Binary Size Report",
        "",
        f"- Commit: `{commit}`",
        f"- Date: {timestamp}",
        f"- Platform: STM32G0 (Cortex-M0+, stm32g0_empty)",
        f"- Total apps: {total_apps}",
        "",
        "## Overview",
        "",
        f"- Smallest Code: **{min_code['name']}** ({min_code['code_bytes']} bytes)",
        f"- Largest Code: **{max_code['name']}** ({max_code['code_bytes']} bytes)",
        f"- Smallest RAM: **{min_ram['name']}** ({min_ram['ram_bytes']} bytes)",
        f"- Largest RAM: **{max_ram['name']}** ({max_ram['ram_bytes']} bytes)",
        "",
        "## ROM Usage (Code + Resource)",
        "",
        "![Size Report](images/size_report.png)",
        "",
        "## Detailed Size Table",
        "",
        "| App | Code (Bytes) | Resource (Bytes) | RAM (Bytes) | PFB (Bytes) | Total ROM (Bytes) |",
        "|-----|-------------|-----------------|------------|------------|------------------|",
    ]

    for a in apps:
        total_rom = a["code_bytes"] + a["resource_bytes"]
        lines.append(
            f"| {a['name']} | {a['code_bytes']} | {a['resource_bytes']} "
            f"| {a['ram_bytes']} | {a['pfb_bytes']} | {total_rom} |"
        )

    md_path = DOC_DIR / "size_report.md"
    md_path.write_text("\n".join(lines), encoding="utf-8")
    print(f"  Report saved: {md_path}")


def generate_size_overview():
    """Generate size overview documentation page."""
    overview = [
        "# Binary Size Analysis",
        "",
        "EmbeddedGUI is designed for resource-constrained embedded systems. "
        "Binary size analysis helps track ROM/RAM usage across all examples "
        "and detect unexpected size growth.",
        "",
        "## Measurement Method",
        "",
        "- **Platform**: STM32G0 (Cortex-M0+, stm32g0_empty board)",
        "- **Toolchain**: arm-none-eabi-gcc",
        "- **Data Source**: ELF symbols (`__code_size`, `__rodata_size`, "
        "`__data_size`, `__bss_size`, `__bss_pfb_size`)",
        "",
        "## Size Categories",
        "",
        "| Category | Description |",
        "|----------|-------------|",
        "| Code | Text section (.text) - executable code |",
        "| Resource | Read-only data (.rodata) - fonts, images, etc. |",
        "| RAM | Data + BSS sections - global variables |",
        "| PFB | Partial Frame Buffer - BSS reserved for PFB |",
        "",
        "## Generation Command",
        "",
        "```bash",
        "# Generate size analysis",
        "python scripts/utils_analysis_elf_size.py",
        "",
        "# Generate documentation report",
        "python scripts/size_to_doc.py",
        "```",
    ]

    md_path = DOC_DIR / "size_overview.md"
    md_path.write_text("\n".join(overview), encoding="utf-8")
    print(f"  Overview saved: {md_path}")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description="Generate size analysis documentation")
    args = parser.parse_args()

    DOC_DIR.mkdir(parents=True, exist_ok=True)
    IMG_DIR.mkdir(parents=True, exist_ok=True)

    print("\n=== Generating Size Overview ===")
    try:
        generate_size_overview()
    except Exception as e:
        print(f"  [ERROR] Size Overview: {e}")
        import traceback
        traceback.print_exc()

    print("\n=== Generating Size Report ===")
    try:
        generate_size_report()
    except Exception as e:
        print(f"  [ERROR] Size Report: {e}")
        import traceback
        traceback.print_exc()

    print("\nDone.")


if __name__ == "__main__":
    main()

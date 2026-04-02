#!/usr/bin/env python
# -*- coding: utf-8 -*-

import json
import multiprocessing
import argparse
import subprocess
import time
import sys
from datetime import datetime
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent.parent
OUTPUT_DIR = PROJECT_ROOT / "output"
DOC_DIR = PROJECT_ROOT / "doc" / "source" / "size"
PROBE_CONFIG_PATH = PROJECT_ROOT / "example" / "HelloSizeAnalysis" / "widget_feature_probe" / "size_analysis_probe_config.h"

APP = "HelloSizeAnalysis"
APP_SUB = "widget_feature_probe"
PORT = "qemu"
CPU_ARCH = "cortex-m0plus"
MAKE_JOBS = max(1, multiprocessing.cpu_count())
APP_OBJ_SUFFIX = "size_widget_feature"

VARIANTS = [
    {
        "name": "baseline",
        "title": "BASELINE",
        "suffix": "wfp_base",
        "cflags": [],
        "items": [],
        "description": "不强制链接任何 widget probe。",
    },
    {
        "name": "slider_widget",
        "title": "SLIDER",
        "suffix": "wfp_slider",
        "cflags": ["-DEGUI_SIZE_PROBE_WIDGET_SLIDER=1"],
        "items": ["egui_view_slider"],
        "description": "真实 slider 控件成本。",
    },
    {
        "name": "switch_widget",
        "title": "SWITCH",
        "suffix": "wfp_switch",
        "cflags": ["-DEGUI_SIZE_PROBE_WIDGET_SWITCH=1"],
        "items": ["egui_view_switch"],
        "description": "真实 switch 控件成本。",
    },
    {
        "name": "page_indicator_widget",
        "title": "PAGE_INDICATOR",
        "suffix": "wfp_pi",
        "cflags": ["-DEGUI_SIZE_PROBE_WIDGET_PAGE_INDICATOR=1"],
        "items": ["egui_view_page_indicator"],
        "description": "真实 page indicator 控件成本。",
    },
    {
        "name": "stepper_widget",
        "title": "STEPPER",
        "suffix": "wfp_stepper",
        "cflags": ["-DEGUI_SIZE_PROBE_WIDGET_STEPPER=1"],
        "items": ["egui_view_stepper"],
        "description": "真实 stepper 控件成本。",
    },
    {
        "name": "cpb_widget",
        "title": "CIRCULAR_PROGRESS_BAR",
        "suffix": "wfp_cpb",
        "cflags": ["-DEGUI_SIZE_PROBE_WIDGET_CPB=1"],
        "items": ["egui_view_circular_progress_bar"],
        "description": "真实 circular progress bar 控件成本。",
    },
    {
        "name": "gauge_widget",
        "title": "GAUGE",
        "suffix": "wfp_gauge",
        "cflags": ["-DEGUI_SIZE_PROBE_WIDGET_GAUGE=1"],
        "items": ["egui_view_gauge"],
        "description": "真实 gauge 控件成本。",
    },
    {
        "name": "activity_ring_widget",
        "title": "ACTIVITY_RING",
        "suffix": "wfp_ring",
        "cflags": ["-DEGUI_SIZE_PROBE_WIDGET_ACTIVITY_RING=1"],
        "items": ["egui_view_activity_ring"],
        "description": "真实 activity_ring 控件成本。",
    },
    {
        "name": "analog_clock_widget",
        "title": "ANALOG_CLOCK",
        "suffix": "wfp_clock",
        "cflags": ["-DEGUI_SIZE_PROBE_WIDGET_ANALOG_CLOCK=1"],
        "items": ["egui_view_analog_clock"],
        "description": "真实 analog_clock 控件成本。",
    },
    {
        "name": "line_widget",
        "title": "LINE_WIDGET",
        "suffix": "wfp_line",
        "cflags": ["-DEGUI_SIZE_PROBE_WIDGET_LINE=1"],
        "items": ["egui_view_line"],
        "description": "真实 line 控件成本。",
    },
    {
        "name": "chart_line_widget",
        "title": "CHART_LINE",
        "suffix": "wfp_chart",
        "cflags": ["-DEGUI_SIZE_PROBE_WIDGET_CHART_LINE=1"],
        "items": ["egui_view_chart_line"],
        "description": "真实 chart_line 控件成本。",
    },
    {
        "name": "indicator_widget_set",
        "title": "INDICATOR_WIDGET_SET",
        "suffix": "wfp_ind_set",
        "cflags": [
            "-DEGUI_SIZE_PROBE_WIDGET_SLIDER=1",
            "-DEGUI_SIZE_PROBE_WIDGET_SWITCH=1",
            "-DEGUI_SIZE_PROBE_WIDGET_PAGE_INDICATOR=1",
            "-DEGUI_SIZE_PROBE_WIDGET_STEPPER=1",
        ],
        "items": ["slider", "switch", "page_indicator", "stepper"],
        "description": "一组常见 indicator 控件组合。",
    },
    {
        "name": "ring_widget_set",
        "title": "RING_WIDGET_SET",
        "suffix": "wfp_ring_set",
        "cflags": [
            "-DEGUI_SIZE_PROBE_WIDGET_CPB=1",
            "-DEGUI_SIZE_PROBE_WIDGET_GAUGE=1",
            "-DEGUI_SIZE_PROBE_WIDGET_ACTIVITY_RING=1",
        ],
        "items": ["circular_progress_bar", "gauge", "activity_ring"],
        "description": "一组 ring/gauge 类控件组合。",
    },
    {
        "name": "line_visual_widget_set",
        "title": "LINE_VISUAL_WIDGET_SET",
        "suffix": "wfp_line_set",
        "cflags": [
            "-DEGUI_SIZE_PROBE_WIDGET_LINE=1",
            "-DEGUI_SIZE_PROBE_WIDGET_CHART_LINE=1",
            "-DEGUI_SIZE_PROBE_WIDGET_ANALOG_CLOCK=1",
        ],
        "items": ["line", "chart_line", "analog_clock"],
        "description": "一组 line/clock/data-visual 控件组合。",
    },
    {
        "name": "all_widget_probes",
        "title": "ALL_WIDGET_PROBES",
        "suffix": "wfp_all",
        "cflags": [
            "-DEGUI_SIZE_PROBE_WIDGET_SLIDER=1",
            "-DEGUI_SIZE_PROBE_WIDGET_SWITCH=1",
            "-DEGUI_SIZE_PROBE_WIDGET_PAGE_INDICATOR=1",
            "-DEGUI_SIZE_PROBE_WIDGET_STEPPER=1",
            "-DEGUI_SIZE_PROBE_WIDGET_CPB=1",
            "-DEGUI_SIZE_PROBE_WIDGET_GAUGE=1",
            "-DEGUI_SIZE_PROBE_WIDGET_ACTIVITY_RING=1",
            "-DEGUI_SIZE_PROBE_WIDGET_ANALOG_CLOCK=1",
            "-DEGUI_SIZE_PROBE_WIDGET_LINE=1",
            "-DEGUI_SIZE_PROBE_WIDGET_CHART_LINE=1",
        ],
        "items": ["all widget probes"],
        "description": "当前 probe 覆盖的所有真实 widget 一起打开。",
    },
]


WIDGET_FEATURE_DESCRIPTION_OVERRIDES = {
    "baseline": "No widget probe is force-linked.",
    "slider_widget": "Real slider widget cost.",
    "switch_widget": "Real switch widget cost.",
    "page_indicator_widget": "Real page indicator widget cost.",
    "stepper_widget": "Real stepper widget cost.",
    "cpb_widget": "Real circular progress bar widget cost.",
    "gauge_widget": "Real gauge widget cost.",
    "activity_ring_widget": "Real activity ring widget cost.",
    "analog_clock_widget": "Real analog clock widget cost.",
    "line_widget": "Real line widget cost.",
    "chart_line_widget": "Real chart-line widget cost.",
    "indicator_widget_set": "A common indicator widget set.",
    "ring_widget_set": "A ring and gauge widget set.",
    "line_visual_widget_set": "A line, clock and data-visual widget set.",
    "all_widget_probes": "All widget probes enabled together.",
}

WIDGET_FEATURE_DOC_REPLACEMENTS = {
    "## 璇存槑": "## Notes",
    "- 杩欎唤鎶ュ憡瑙ｅ喅鐨勬槸鈥滈€夋煇涓湡瀹炴帶浠惰浠樺嚭澶氬皯 ROM/RAM 鎴愭湰鈥濈殑闂銆?": "- This report answers how much ROM and RAM a real widget or widget set costs to bring in.",
    "- 杩欓噷鐢ㄧ殑涓嶆槸 canvas 鏇胯韩锛岃€屾槸鐪熷疄 widget 鐨?`init_with_params` / setter 璺緞銆?": "- The probe uses real widget `init_with_params` and setter paths rather than canvas-only stand-ins.",
    "- 鐢变簬 widget 涔嬮棿鍏变韩 base/view/theme/canvas 璺緞锛岀粍鍚堥」涓嶈兘绠€鍗曠敱鍗曟帶浠剁洿鎺ョ浉鍔犮€?": "- Widgets share base/view/theme/canvas paths, so widget-set totals must not be computed by summing single-widget deltas.",
}


def normalize_widget_feature_doc_text(text):
    for source, target in WIDGET_FEATURE_DOC_REPLACEMENTS.items():
        text = text.replace(source, target)
    return text


def run_cmd(cmd, timeout=300):
    return subprocess.run(
        cmd,
        cwd=PROJECT_ROOT,
        capture_output=True,
        text=True,
        timeout=timeout,
    )


def run_tool_with_retry(cmd, timeout=30, retries=3, delay_seconds=1.0):
    last_result = None
    for attempt in range(retries):
        result = run_cmd(cmd, timeout=timeout)
        last_result = result
        if result.returncode == 0:
            return result
        if attempt + 1 < retries:
            time.sleep(delay_seconds)
    return last_result


def run_make_with_fallback(cmd, timeout=300):
    result = run_cmd(cmd, timeout=timeout)
    if result.returncode == 0:
        return result

    parallel_marker = next((item for item in cmd if item.startswith("-j")), None)
    if parallel_marker is None:
        return result

    retry_cmd = [item for item in cmd if item != parallel_marker]
    return run_cmd(retry_cmd, timeout=timeout)


def render_probe_config(macros):
    lines = [
        "#ifndef _SIZE_ANALYSIS_PROBE_CONFIG_H_",
        "#define _SIZE_ANALYSIS_PROBE_CONFIG_H_",
        "",
    ]
    for macro in macros:
        token = macro
        if token.startswith("-D"):
            token = token[2:]
        if "=" in token:
            name, value = token.split("=", 1)
            lines.append("#define %s %s" % (name, value))
        else:
            lines.append("#define %s" % token)
    lines.extend(
        [
            "",
            "#endif /* _SIZE_ANALYSIS_PROBE_CONFIG_H_ */",
            "",
        ]
    )
    return "\n".join(lines)


def write_probe_config(macros):
    PROBE_CONFIG_PATH.write_text(render_probe_config(macros), encoding="utf-8")


def select_variants(all_variants, requested):
    if not requested:
        return list(all_variants)

    name_map = {}
    for variant in all_variants:
        name_map[variant["name"]] = variant
        name_map[variant["title"].lower()] = variant

    selected = []
    for raw_name in requested:
        key = raw_name.strip().lower()
        if not key:
            continue
        if key not in name_map:
            raise ValueError("Unknown variant: %s" % raw_name)
        variant = name_map[key]
        if variant not in selected:
            selected.append(variant)
    baseline = next(variant for variant in all_variants if variant["name"] == "baseline")
    if baseline not in selected:
        selected.insert(0, baseline)
    return selected


def get_git_commit():
    try:
        return (
            subprocess.check_output(
                ["git", "rev-parse", "--short", "HEAD"],
                cwd=PROJECT_ROOT,
                stderr=subprocess.DEVNULL,
            )
            .decode("utf-8")
            .strip()
        )
    except Exception:
        return "unknown"


def parse_size_output(text):
    result = {}
    for line in text.splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith("section") or stripped.startswith("Total") or stripped.endswith(":"):
            continue
        parts = stripped.split()
        if len(parts) < 2:
            continue
        try:
            result[parts[0]] = int(parts[1])
        except ValueError:
            continue
    return result


def build_variant(variant):
    write_probe_config(variant["cflags"])

    build_cmd = [
        "make",
        "all",
        "-j%d" % MAKE_JOBS,
        "APP=%s" % APP,
        "APP_SUB=%s" % APP_SUB,
        "PORT=%s" % PORT,
        "CPU_ARCH=%s" % CPU_ARCH,
        "APP_OBJ_SUFFIX=%s" % APP_OBJ_SUFFIX,
    ]

    build_result = run_make_with_fallback(build_cmd, timeout=300)
    if build_result.returncode != 0:
        raise RuntimeError("build failed for %s\n%s" % (variant["name"], build_result.stderr or build_result.stdout))

    elf_path = OUTPUT_DIR / "main.elf"
    size_result = run_tool_with_retry(["llvm-size", "-A", str(elf_path)], timeout=30)
    if size_result.returncode != 0:
        raise RuntimeError("llvm-size failed for %s\n%s" % (variant["name"], size_result.stderr or size_result.stdout))

    sections = parse_size_output(size_result.stdout)
    entry = {
        "name": variant["name"],
        "title": variant["title"],
        "description": WIDGET_FEATURE_DESCRIPTION_OVERRIDES.get(variant["name"], variant["description"]),
        "cflags": variant["cflags"],
        "items": variant["items"],
        "sections": {
            "text": sections.get(".text", 0),
            "rodata": sections.get(".rodata", 0),
            "data": sections.get(".data", 0),
            "bss": sections.get(".bss", 0),
        },
    }
    entry["rom_total"] = entry["sections"]["text"] + entry["sections"]["rodata"]
    return entry


def generate_report(entries, json_path, doc_path=None):
    baseline = next(item for item in entries if item["name"] == "baseline")

    for entry in entries:
        entry["delta"] = {
            "text": entry["sections"]["text"] - baseline["sections"]["text"],
            "rodata": entry["sections"]["rodata"] - baseline["sections"]["rodata"],
            "data": entry["sections"]["data"] - baseline["sections"]["data"],
            "bss": entry["sections"]["bss"] - baseline["sections"]["bss"],
            "rom_total": entry["rom_total"] - baseline["rom_total"],
        }

    payload = {
        "timestamp": datetime.now().isoformat(),
        "git_commit": get_git_commit(),
        "app": APP,
        "port": PORT,
        "cpu_arch": CPU_ARCH,
        "variants": entries,
    }
    json_path.write_text(json.dumps(payload, indent=2, ensure_ascii=False), encoding="utf-8")

    lines = [
        "# Widget Feature QEMU Code Size Report",
        "",
        "- Commit: `%s`" % payload["git_commit"],
        "- Date: %s" % payload["timestamp"],
        "- Build target: `APP=%s APP_SUB=%s PORT=%s CPU_ARCH=%s`" % (APP, APP_SUB, PORT, CPU_ARCH),
        "- Measurement method: compile a dedicated widget probe app, then rewrite the app-local probe config header to force-link a real widget or widget set.",
        "- Scope: widget-level static qemu ELF sections only (`.text/.rodata/.data/.bss`).",
        "",
        "## 说明",
        "",
        "- 这份报告解决的是“选某个真实控件要付出多少 ROM/RAM 成本”的问题。",
        "- 这里用的不是 canvas 替身，而是真实 widget 的 `init_with_params` / setter 路径。",
        "- 由于 widget 之间共享 base/view/theme/canvas 路径，组合项不能简单由单控件直接相加。",
        "",
        "## Baseline",
        "",
        "| Variant | Text | Rodata | Data | Bss | Total ROM |",
        "|---------|-----:|-------:|-----:|----:|----------:|",
        "| %s | %d | %d | %d | %d | %d |"
        % (
            baseline["title"],
            baseline["sections"]["text"],
            baseline["sections"]["rodata"],
            baseline["sections"]["data"],
            baseline["sections"]["bss"],
            baseline["rom_total"],
        ),
        "",
        "## Widget Summary",
        "",
        "| Widget Or Set | Delta Text | Delta Rodata | Delta Data | Delta Bss | Delta ROM |",
        "|---------------|-----------:|-------------:|-----------:|----------:|----------:|",
    ]

    for entry in entries:
        if entry["name"] == "baseline":
            continue
        lines.append(
            "| %s | %+d | %+d | %+d | %+d | %+d |"
            % (
                entry["title"],
                entry["delta"]["text"],
                entry["delta"]["rodata"],
                entry["delta"]["data"],
                entry["delta"]["bss"],
                entry["delta"]["rom_total"],
            )
        )

    lines.extend(
        [
            "",
            "## Definition",
            "",
            "| Widget Or Set | Included Items | Description |",
            "|---------------|----------------|-------------|",
        ]
    )

    for entry in entries:
        if entry["name"] == "baseline":
            continue
        items = "<br>".join("`%s`" % item for item in entry["items"])
        lines.append("| %s | %s | %s |" % (entry["title"], items, entry["description"]))

    lines.extend(
        [
            "",
            "## Detailed Variants",
            "",
            "| Variant | Probe Config | Text | Rodata | Data | Bss | Total ROM |",
            "|---------|-------------------|-----:|-------:|-----:|----:|----------:|",
        ]
    )

    for entry in entries:
        cflags = " ".join(entry["cflags"]) if entry["cflags"] else "(none)"
        lines.append(
            "| %s | `%s` | %d | %d | %d | %d | %d |"
            % (
                entry["title"],
                cflags,
                entry["sections"]["text"],
                entry["sections"]["rodata"],
                entry["sections"]["data"],
                entry["sections"]["bss"],
                entry["rom_total"],
            )
        )

    lines.extend(
        [
            "",
            "## Reproduce",
            "",
            "```bash",
            "python scripts/size_analysis/widget_feature_size_to_doc.py",
            "```",
            "",
            "Raw JSON is written to `output/widget_feature_size_results.json`.",
        ]
    )

    if doc_path is not None:
        doc_path.write_text(normalize_widget_feature_doc_text("\n".join(lines) + "\n"), encoding="utf-8")


def parse_args():
    parser = argparse.ArgumentParser(description="Generate widget feature qemu size report")
    parser.add_argument("--variants", help="Comma-separated variant names/titles to build")
    parser.add_argument("--list-variants", action="store_true", help="List available variants and exit")
    parser.add_argument("--no-doc", action="store_true", help="Skip markdown report generation")
    parser.add_argument("--report-dir", help="Override output directory for generated JSON/Markdown reports")
    return parser.parse_args()


def main():
    args = parse_args()

    if args.list_variants:
        for variant in VARIANTS:
            print("%s (%s)" % (variant["name"], variant["title"]))
        return

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    report_dir = Path(args.report_dir) if args.report_dir else DOC_DIR
    if not report_dir.is_absolute():
        report_dir = (PROJECT_ROOT / report_dir).resolve()
    report_dir.mkdir(parents=True, exist_ok=True)
    json_path = report_dir / "widget_feature_size_results.json"
    doc_path = None if args.no_doc else report_dir / "widget_feature_size_report.md"

    clean_result = run_cmd(["make", "clean"], timeout=180)
    if clean_result.returncode != 0:
        raise RuntimeError("make clean failed\n%s" % (clean_result.stderr or clean_result.stdout))

    requested = args.variants.split(",") if args.variants else []
    variants = select_variants(VARIANTS, requested)

    entries = []
    for variant in variants:
        print("=== Building %s ===" % variant["title"])
        entries.append(build_variant(variant))

    generate_report(entries, json_path, doc_path)
    print("JSON saved: %s" % json_path)
    if doc_path is not None:
        print("Report saved: %s" % doc_path)


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        print(str(exc), file=sys.stderr)
        sys.exit(1)

#!/usr/bin/env python
# -*- coding: utf-8 -*-

import json
import multiprocessing
import argparse
import subprocess
import sys
import time
from datetime import datetime
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent.parent
OUTPUT_DIR = PROJECT_ROOT / "output"
DOC_DIR = PROJECT_ROOT / "doc" / "source" / "size"
PROBE_CONFIG_PATH = PROJECT_ROOT / "example" / "HelloSizeAnalysis" / "hq_path_probe" / "size_analysis_probe_config.h"

APP = "HelloSizeAnalysis"
APP_SUB = "hq_path_probe"
PORT = "qemu"
CPU_ARCH = "cortex-m0plus"
MAKE_JOBS = max(1, multiprocessing.cpu_count())
APP_OBJ_SUFFIX = "size_hq"

VARIANTS = [
    {
        "name": "baseline",
        "title": "Baseline",
        "suffix": "hqb",
        "cflags": [],
        "symbols": [],
        "description": "No forced HQ link probe.",
    },
    {
        "name": "line_hq",
        "title": "LINE_HQ",
        "suffix": "hql",
        "cflags": ["-DEGUI_SIZE_PROBE_LINK_LINE_HQ=1"],
        "symbols": [
            "egui_canvas_draw_line_hq",
            "egui_canvas_draw_line_round_cap_hq",
            "egui_canvas_draw_polyline_hq",
            "egui_canvas_draw_polyline_round_cap_hq",
        ],
        "description": "Force link the line/polyline HQ path only.",
    },
    {
        "name": "circle_hq",
        "title": "CIRCLE_HQ",
        "suffix": "hqc",
        "cflags": ["-DEGUI_SIZE_PROBE_LINK_CIRCLE_HQ=1"],
        "symbols": [
            "egui_canvas_draw_circle_hq",
            "egui_canvas_draw_circle_fill_hq",
        ],
        "description": "Force link the circle HQ path only.",
    },
    {
        "name": "arc_hq",
        "title": "ARC_HQ",
        "suffix": "hqa",
        "cflags": ["-DEGUI_SIZE_PROBE_LINK_ARC_HQ=1"],
        "symbols": [
            "egui_canvas_draw_arc_hq",
            "egui_canvas_draw_arc_fill_hq",
        ],
        "description": "Force link the arc HQ path only. Round-cap arc is excluded to avoid double counting line/circle dependencies.",
    },
    {
        "name": "arc_round_cap_hq",
        "title": "ARC_ROUND_CAP_HQ",
        "suffix": "hqarc",
        "cflags": ["-DEGUI_SIZE_PROBE_LINK_ARC_ROUND_CAP_HQ=1"],
        "symbols": [
            "egui_canvas_draw_arc_round_cap_hq",
            "egui_canvas_draw_arc_hq",
            "egui_canvas_draw_circle_fill_hq",
        ],
        "description": "Force link the round-cap arc helper used by widgets like activity_ring.",
    },
    {
        "name": "all_hq",
        "title": "ALL_HQ",
        "suffix": "hqall",
        "cflags": [
            "-DEGUI_SIZE_PROBE_LINK_LINE_HQ=1",
            "-DEGUI_SIZE_PROBE_LINK_CIRCLE_HQ=1",
            "-DEGUI_SIZE_PROBE_LINK_ARC_HQ=1",
        ],
        "symbols": [
            "egui_canvas_draw_line_hq",
            "egui_canvas_draw_line_round_cap_hq",
            "egui_canvas_draw_polyline_hq",
            "egui_canvas_draw_polyline_round_cap_hq",
            "egui_canvas_draw_circle_hq",
            "egui_canvas_draw_circle_fill_hq",
            "egui_canvas_draw_arc_hq",
            "egui_canvas_draw_arc_fill_hq",
        ],
        "description": "Force link the three core HQ paths together.",
    },
]


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
    content = render_probe_config(macros)
    PROBE_CONFIG_PATH.write_text(content, encoding="utf-8")


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
        name = parts[0]
        try:
            size = int(parts[1])
        except ValueError:
            continue
        result[name] = size
    return result


def parse_symbol_output(text, interested_symbols):
    rows = []
    wanted = set(interested_symbols)
    for line in text.splitlines():
        parts = line.split()
        if len(parts) < 4:
            continue
        symbol_name = parts[3]
        if symbol_name not in wanted:
            continue
        rows.append(
            {
                "address": parts[0],
                "size": int(parts[1], 16),
                "type": parts[2],
                "name": symbol_name,
            }
        )
    rows.sort(key=lambda item: item["size"], reverse=True)
    return rows


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

    nm_result = run_tool_with_retry(["llvm-nm", "-S", "--size-sort", "--print-size", str(elf_path)], timeout=30)
    if nm_result.returncode != 0:
        raise RuntimeError("llvm-nm failed for %s\n%s" % (variant["name"], nm_result.stderr or nm_result.stdout))

    sections = parse_size_output(size_result.stdout)
    symbols = parse_symbol_output(nm_result.stdout, variant["symbols"])

    entry = {
        "name": variant["name"],
        "title": variant["title"],
        "description": variant["description"],
        "cflags": variant["cflags"],
        "sections": {
            "text": sections.get(".text", 0),
            "rodata": sections.get(".rodata", 0),
            "data": sections.get(".data", 0),
            "bss": sections.get(".bss", 0),
            "user_heap_stack": sections.get("._user_heap_stack", 0),
        },
        "symbols": symbols,
        "symbol_text_total": sum(item["size"] for item in symbols),
    }
    entry["rom_total"] = entry["sections"]["text"] + entry["sections"]["rodata"]
    return entry


def generate_report(entries, json_path, doc_path=None):
    baseline = next(item for item in entries if item["name"] == "baseline")
    summary_targets = ["line_hq", "circle_hq", "arc_hq", "arc_round_cap_hq", "all_hq"]
    built_names = {item["name"] for item in entries}

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
        "# HQ Path QEMU Size Report",
        "",
        "- Commit: `%s`" % payload["git_commit"],
        "- Date: %s" % payload["timestamp"],
        "- Build target: `APP=%s APP_SUB=%s PORT=%s CPU_ARCH=%s`" % (APP, APP_SUB, PORT, CPU_ARCH),
        "- Measurement method: build `HelloSizeAnalysis/hq_path_probe` baseline, then rewrite the app-local probe config header to force-link a single HQ path into the final ELF.",
        "- Report scope: isolated static link delta only. This report does not include runtime heap/stack because the HQ geometry paths themselves do not allocate heap.",
        "",
        "## How To Read",
        "",
        "- `Delta Text`: `.text` increment relative to baseline, used as the main code-size number.",
        "- `Delta Rodata`: extra lookup tables / constants brought in by the HQ path.",
        "- `Delta ROM`: `.text + .rodata` total flash increment relative to baseline.",
        "- `ARC_HQ` here measures `egui_canvas_draw_arc_hq()` + `egui_canvas_draw_arc_fill_hq()` only.",
        "- `ARC_ROUND_CAP_HQ` is listed separately because it is a real widget-facing helper and it pulls both arc and circle-fill dependencies.",
        "- `Delta ROM` is the real ELF section delta. It is often larger than the listed public symbol sizes because static helpers and dependent sections are linked too.",
        "- The `+8 BSS` seen in probe variants comes from the probe harness guard in `HelloSizeAnalysis/hq_path_probe`, not from persistent HQ runtime state.",
        "- Turning a path off can only reclaim the same bytes when no other object still references that path.",
        "",
        "## Baseline",
        "",
        "| Variant | Text | Rodata | Data | Bss | Total ROM |",
        "|---------|------|--------|------|-----|-----------|",
        "| %s | %d | %d | %d | %d | %d |" % (
            baseline["title"],
            baseline["sections"]["text"],
            baseline["sections"]["rodata"],
            baseline["sections"]["data"],
            baseline["sections"]["bss"],
            baseline["rom_total"],
        ),
        "",
        "## Increment Summary",
        "",
        "| Path | Delta Text | Delta Rodata | Delta Data | Delta Bss | Delta ROM |",
        "|------|-----------:|-------------:|-----------:|----------:|----------:|",
    ]

    for name in summary_targets:
        if name not in built_names:
            continue
        entry = next(item for item in entries if item["name"] == name)
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
            "## Linked Symbol Breakdown",
            "",
        ]
    )

    for entry in entries:
        if not entry["symbols"]:
            continue
        lines.append("### %s" % entry["title"])
        lines.append("")
        lines.append("- Description: %s" % entry["description"])
        lines.append("- Symbol text total: %d bytes" % entry["symbol_text_total"])
        lines.append("")
        lines.append("| Symbol | Size (bytes) | Type |")
        lines.append("|--------|-------------:|------|")
        for item in entry["symbols"]:
            lines.append("| `%s` | %d | `%s` |" % (item["name"], item["size"], item["type"]))
        lines.append("")

    lines.extend(
        [
            "## Reproduce",
            "",
            "```bash",
            "python scripts/size_analysis/main.py hq-size-to-doc",
            "```",
            "",
            "The raw JSON is written to `output/hq_size_results.json`.",
        ]
    )

    if doc_path is not None:
        doc_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def parse_args():
    parser = argparse.ArgumentParser(description="Generate HQ path qemu size report")
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
    json_path = report_dir / "hq_size_results.json"
    doc_path = None if args.no_doc else report_dir / "hq_size_report.md"

    clean_result = run_cmd(["make", "clean"], timeout=180)
    if clean_result.returncode != 0:
        raise RuntimeError("make clean failed\n%s" % (clean_result.stderr or clean_result.stdout))

    requested = args.variants.split(",") if args.variants else []
    variants = select_variants(VARIANTS, requested)

    entries = []
    for variant in variants:
        print("=== Building %s ===" % variant["title"])
        entry = build_variant(variant)
        entries.append(entry)

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

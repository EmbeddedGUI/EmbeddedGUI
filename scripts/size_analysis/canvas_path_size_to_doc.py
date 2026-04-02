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
PROBE_CONFIG_PATH = PROJECT_ROOT / "example" / "HelloSizeAnalysis" / "canvas_path_probe" / "size_analysis_probe_config.h"

APP = "HelloSizeAnalysis"
APP_SUB = "canvas_path_probe"
PORT = "qemu"
CPU_ARCH = "cortex-m0plus"
MAKE_JOBS = max(1, multiprocessing.cpu_count())
APP_OBJ_SUFFIX = "size_canvas_path"

VARIANTS = [
    {
        "name": "baseline",
        "title": "BASELINE",
        "suffix": "cpsb",
        "cflags": [],
        "functions": [],
        "description": "不强制链接任何 canvas probe 场景。",
    },
    {
        "name": "rect_stroke",
        "title": "RECT_STROKE",
        "suffix": "cps_rect_s",
        "cflags": ["-DEGUI_SIZE_PROBE_RECT_STROKE_PATH=1"],
        "functions": ["egui_canvas_draw_rectangle"],
        "description": "矩形描边路径。",
    },
    {
        "name": "rect_fill",
        "title": "RECT_FILL",
        "suffix": "cps_rect_f",
        "cflags": ["-DEGUI_SIZE_PROBE_RECT_FILL_PATH=1"],
        "functions": ["egui_canvas_draw_rectangle_fill"],
        "description": "矩形填充路径。",
    },
    {
        "name": "round_rect_stroke",
        "title": "ROUND_RECT_STROKE",
        "suffix": "cps_rr_s",
        "cflags": ["-DEGUI_SIZE_PROBE_ROUND_RECT_STROKE_PATH=1"],
        "functions": ["egui_canvas_draw_round_rectangle"],
        "description": "圆角矩形描边路径。",
    },
    {
        "name": "round_rect_fill",
        "title": "ROUND_RECT_FILL",
        "suffix": "cps_rr_f",
        "cflags": ["-DEGUI_SIZE_PROBE_ROUND_RECT_FILL_PATH=1"],
        "functions": ["egui_canvas_draw_round_rectangle_fill"],
        "description": "圆角矩形填充路径。",
    },
    {
        "name": "triangle_stroke",
        "title": "TRIANGLE_STROKE",
        "suffix": "cps_tri_s",
        "cflags": ["-DEGUI_SIZE_PROBE_TRIANGLE_STROKE_PATH=1"],
        "functions": ["egui_canvas_draw_triangle"],
        "description": "三角形描边路径。",
    },
    {
        "name": "triangle_fill",
        "title": "TRIANGLE_FILL",
        "suffix": "cps_tri_f",
        "cflags": ["-DEGUI_SIZE_PROBE_TRIANGLE_FILL_PATH=1"],
        "functions": ["egui_canvas_draw_triangle_fill"],
        "description": "三角形填充路径。",
    },
    {
        "name": "circle_basic_stroke",
        "title": "CIRCLE_BASIC_STROKE",
        "suffix": "cps_cir_s",
        "cflags": ["-DEGUI_SIZE_PROBE_CIRCLE_BASIC_STROKE_PATH=1"],
        "functions": ["egui_canvas_draw_circle_basic"],
        "description": "basic 圆描边路径。",
    },
    {
        "name": "circle_basic_fill",
        "title": "CIRCLE_BASIC_FILL",
        "suffix": "cps_cir_f",
        "cflags": ["-DEGUI_SIZE_PROBE_CIRCLE_BASIC_FILL_PATH=1"],
        "functions": ["egui_canvas_draw_circle_fill_basic"],
        "description": "basic 圆填充路径，包含 circle LUT 成本。",
    },
    {
        "name": "arc_basic_stroke",
        "title": "ARC_BASIC_STROKE",
        "suffix": "cps_arc_s",
        "cflags": ["-DEGUI_SIZE_PROBE_ARC_BASIC_STROKE_PATH=1"],
        "functions": ["egui_canvas_draw_arc_basic"],
        "description": "basic 弧描边路径。",
    },
    {
        "name": "arc_basic_fill",
        "title": "ARC_BASIC_FILL",
        "suffix": "cps_arc_f",
        "cflags": ["-DEGUI_SIZE_PROBE_ARC_BASIC_FILL_PATH=1"],
        "functions": ["egui_canvas_draw_arc_fill_basic"],
        "description": "basic 弧填充路径。",
    },
    {
        "name": "line_path",
        "title": "LINE",
        "suffix": "cps_line",
        "cflags": ["-DEGUI_SIZE_PROBE_LINE_PATH=1"],
        "functions": ["egui_canvas_draw_line"],
        "description": "基础直线路径。",
    },
    {
        "name": "polyline_path",
        "title": "POLYLINE",
        "suffix": "cps_poly",
        "cflags": ["-DEGUI_SIZE_PROBE_POLYLINE_PATH=1"],
        "functions": ["egui_canvas_draw_polyline"],
        "description": "基础折线路径。",
    },
    {
        "name": "gradient_rect",
        "title": "GRADIENT_RECT",
        "suffix": "cps_grad_rect",
        "cflags": ["-DEGUI_SIZE_PROBE_GRADIENT_RECT_PATH=1"],
        "functions": ["egui_canvas_draw_rectangle_fill_gradient"],
        "description": "矩形渐变填充路径。",
    },
    {
        "name": "gradient_round_rect",
        "title": "GRADIENT_ROUND_RECT",
        "suffix": "cps_grad_rr",
        "cflags": ["-DEGUI_SIZE_PROBE_GRADIENT_ROUND_RECT_PATH=1"],
        "functions": ["egui_canvas_draw_round_rectangle_fill_gradient"],
        "description": "圆角矩形渐变填充路径。",
    },
    {
        "name": "gradient_circle",
        "title": "GRADIENT_CIRCLE",
        "suffix": "cps_grad_circle",
        "cflags": ["-DEGUI_SIZE_PROBE_GRADIENT_CIRCLE_PATH=1"],
        "functions": ["egui_canvas_draw_circle_fill_gradient"],
        "description": "圆形渐变填充路径。",
    },
    {
        "name": "mask_circle",
        "title": "MASK_CIRCLE",
        "suffix": "cps_mask_circle",
        "cflags": ["-DEGUI_SIZE_PROBE_MASK_CIRCLE_PATH=1"],
        "functions": ["egui_mask_circle_init", "egui_canvas_set_mask"],
        "description": "circle mask 场景。",
    },
    {
        "name": "mask_round_rect",
        "title": "MASK_ROUND_RECT",
        "suffix": "cps_mask_rr",
        "cflags": ["-DEGUI_SIZE_PROBE_MASK_ROUND_RECT_PATH=1"],
        "functions": ["egui_mask_round_rectangle_init", "egui_canvas_set_mask"],
        "description": "round-rect mask 场景。",
    },
    {
        "name": "mask_image",
        "title": "MASK_IMAGE",
        "suffix": "cps_mask_img",
        "cflags": ["-DEGUI_SIZE_PROBE_MASK_IMAGE_PATH=1"],
        "functions": ["egui_mask_image_init", "egui_mask_image_set_image", "egui_canvas_set_mask"],
        "description": "image mask 场景。",
    },
    {
        "name": "image_draw",
        "title": "IMAGE_DRAW",
        "suffix": "cps_img_draw",
        "cflags": ["-DEGUI_SIZE_PROBE_IMAGE_DRAW_PATH=1"],
        "functions": ["egui_canvas_draw_image"],
        "description": "标准图片直绘路径。",
    },
    {
        "name": "image_resize",
        "title": "IMAGE_RESIZE",
        "suffix": "cps_img_resize",
        "cflags": ["-DEGUI_SIZE_PROBE_IMAGE_RESIZE_PATH=1"],
        "functions": ["egui_canvas_draw_image_resize"],
        "description": "标准图片缩放路径。",
    },
    {
        "name": "image_tint",
        "title": "IMAGE_TINT",
        "suffix": "cps_img_tint",
        "cflags": ["-DEGUI_SIZE_PROBE_IMAGE_TINT_PATH=1"],
        "functions": ["egui_canvas_draw_image_color"],
        "description": "标准图片着色路径。",
    },
    {
        "name": "image_rotate",
        "title": "IMAGE_ROTATE",
        "suffix": "cps_img_rot",
        "cflags": ["-DEGUI_SIZE_PROBE_IMAGE_ROTATE_PATH=1"],
        "functions": ["egui_canvas_draw_image_rotate"],
        "description": "图片旋转/transform 场景。",
    },
    {
        "name": "text_rotate",
        "title": "TEXT_ROTATE",
        "suffix": "cps_txt_rot",
        "cflags": ["-DEGUI_SIZE_PROBE_TEXT_ROTATE_PATH=1"],
        "functions": ["egui_canvas_draw_text_rotate"],
        "description": "文字旋转场景。",
    },
    {
        "name": "rle_draw",
        "title": "RLE_DRAW",
        "suffix": "cps_rle",
        "cflags": ["-DEGUI_SIZE_PROBE_RLE_DRAW_PATH=1"],
        "functions": ["egui_canvas_draw_image(rle)"],
        "description": "RLE codec 绘制路径。",
    },
    {
        "name": "qoi_draw",
        "title": "QOI_DRAW",
        "suffix": "cps_qoi",
        "cflags": ["-DEGUI_SIZE_PROBE_QOI_DRAW_PATH=1"],
        "functions": ["egui_canvas_draw_image(qoi)"],
        "description": "QOI codec 绘制路径。",
    },
    {
        "name": "all_canvas_paths",
        "title": "ALL_CANVAS_PATHS",
        "suffix": "cps_all",
        "cflags": [
            "-DEGUI_SIZE_PROBE_RECT_STROKE_PATH=1",
            "-DEGUI_SIZE_PROBE_RECT_FILL_PATH=1",
            "-DEGUI_SIZE_PROBE_ROUND_RECT_STROKE_PATH=1",
            "-DEGUI_SIZE_PROBE_ROUND_RECT_FILL_PATH=1",
            "-DEGUI_SIZE_PROBE_TRIANGLE_STROKE_PATH=1",
            "-DEGUI_SIZE_PROBE_TRIANGLE_FILL_PATH=1",
            "-DEGUI_SIZE_PROBE_CIRCLE_BASIC_STROKE_PATH=1",
            "-DEGUI_SIZE_PROBE_CIRCLE_BASIC_FILL_PATH=1",
            "-DEGUI_SIZE_PROBE_ARC_BASIC_STROKE_PATH=1",
            "-DEGUI_SIZE_PROBE_ARC_BASIC_FILL_PATH=1",
            "-DEGUI_SIZE_PROBE_LINE_PATH=1",
            "-DEGUI_SIZE_PROBE_POLYLINE_PATH=1",
            "-DEGUI_SIZE_PROBE_GRADIENT_RECT_PATH=1",
            "-DEGUI_SIZE_PROBE_GRADIENT_ROUND_RECT_PATH=1",
            "-DEGUI_SIZE_PROBE_GRADIENT_CIRCLE_PATH=1",
            "-DEGUI_SIZE_PROBE_MASK_CIRCLE_PATH=1",
            "-DEGUI_SIZE_PROBE_MASK_ROUND_RECT_PATH=1",
            "-DEGUI_SIZE_PROBE_MASK_IMAGE_PATH=1",
            "-DEGUI_SIZE_PROBE_IMAGE_DRAW_PATH=1",
            "-DEGUI_SIZE_PROBE_IMAGE_RESIZE_PATH=1",
            "-DEGUI_SIZE_PROBE_IMAGE_TINT_PATH=1",
            "-DEGUI_SIZE_PROBE_IMAGE_ROTATE_PATH=1",
            "-DEGUI_SIZE_PROBE_TEXT_ROTATE_PATH=1",
            "-DEGUI_SIZE_PROBE_RLE_DRAW_PATH=1",
            "-DEGUI_SIZE_PROBE_QOI_DRAW_PATH=1",
        ],
        "functions": ["all representative probe functions"],
        "description": "把本报告中的细分 probe 场景一起强制链接。",
    },
]


CANVAS_PATH_DESCRIPTION_OVERRIDES = {
    "baseline": "No canvas probe scene is force-linked.",
    "rect_stroke": "Rectangle stroke path.",
    "rect_fill": "Rectangle fill path.",
    "round_rect_stroke": "Round-rectangle stroke path.",
    "round_rect_fill": "Round-rectangle fill path.",
    "triangle_stroke": "Triangle stroke path.",
    "triangle_fill": "Triangle fill path.",
    "circle_basic_stroke": "Basic circle stroke path.",
    "circle_basic_fill": "Basic circle fill path, including circle LUT cost.",
    "arc_basic_stroke": "Basic arc stroke path.",
    "arc_basic_fill": "Basic arc fill path.",
    "line_path": "Basic line path.",
    "polyline_path": "Basic polyline path.",
    "gradient_rect": "Gradient rectangle fill path.",
    "gradient_round_rect": "Gradient round-rectangle fill path.",
    "gradient_circle": "Gradient circle fill path.",
    "mask_circle": "Circle mask scene.",
    "mask_round_rect": "Round-rectangle mask scene.",
    "mask_image": "Image mask scene.",
    "image_draw": "Standard image draw path.",
    "image_resize": "Standard image resize path.",
    "image_tint": "Standard image tint path.",
    "image_rotate": "Image rotation and transform scene.",
    "text_rotate": "Rotated text draw scene.",
    "rle_draw": "RLE codec draw path.",
    "qoi_draw": "QOI codec draw path.",
    "all_canvas_paths": "All representative canvas path probe scenes enabled together.",
}

CANVAS_PATH_DOC_REPLACEMENTS = {
    "## 璇存槑": "## Notes",
    "- 杩欐槸鈥滄覆鏌撳満鏅紩鍏ユ垚鏈€濇姤鍛婏紝涓嶆槸涓氬姟搴旂敤鏈€缁堟垚鏈姤鍛娿€?": "- This report measures the linked cost of render-path probe scenes. It is not the final size of a business application.",
    "- 鍚勫満鏅箣闂村瓨鍦ㄤ緷璧栦笌閲嶅彔锛屼笉鑳界畝鍗曟妸澧為噺鐩存帴鐩稿姞銆?": "- Scenes share dependencies and overlap. Their deltas must not be summed directly.",
    "- `Delta Text` 鏄富瑕佷唬鐮佷綋绉寚鏍囷紱`Delta ROM = Delta Text + Delta Rodata` 鏇撮€傚悎浣滀负 flash 鎴愭湰鍙ｅ緞銆?": "- `Delta Text` is the main code-size number. `Delta ROM = Delta Text + Delta Rodata` is the more useful flash-cost number.",
    "- 杩欎唤鎶ュ憡鍙鐩栨櫘閫?canvas/render path锛汬Q line/circle/arc 璇风粨鍚?`hq_size_report.md` 涓€璧风湅銆?": "- This report only covers common canvas render paths. Read `hq_size_report.md` for HQ line/circle/arc paths.",
}


def normalize_canvas_path_doc_text(text):
    for source, target in CANVAS_PATH_DOC_REPLACEMENTS.items():
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
        return subprocess.check_output(
            ["git", "rev-parse", "--short", "HEAD"],
            cwd=PROJECT_ROOT,
            stderr=subprocess.DEVNULL,
        ).decode("utf-8").strip()
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
        "description": CANVAS_PATH_DESCRIPTION_OVERRIDES.get(variant["name"], variant["description"]),
        "cflags": variant["cflags"],
        "functions": variant["functions"],
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
        "# Canvas Path QEMU Code Size Report",
        "",
        "- Commit: `%s`" % payload["git_commit"],
        "- Date: %s" % payload["timestamp"],
        "- Build target: `APP=%s APP_SUB=%s PORT=%s CPU_ARCH=%s`" % (APP, APP_SUB, PORT, CPU_ARCH),
        "- Measurement method: compile a dedicated probe app, then rewrite the app-local probe config header to force-link a single canvas scene.",
        "- Scope: static qemu ELF sections only (`.text/.rodata/.data/.bss`). No runtime heap/stack is measured here.",
        "",
        "## 说明",
        "",
        "- 这是“渲染场景引入成本”报告，不是业务应用最终成本报告。",
        "- 各场景之间存在依赖与重叠，不能简单把增量直接相加。",
        "- `Delta Text` 是主要代码体积指标；`Delta ROM = Delta Text + Delta Rodata` 更适合作为 flash 成本口径。",
        "- 这份报告只覆盖普通 canvas/render path；HQ line/circle/arc 请结合 `hq_size_report.md` 一起看。",
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
        "## Increment Summary",
        "",
        "| Scene | Delta Text | Delta Rodata | Delta Data | Delta Bss | Delta ROM |",
        "|-------|-----------:|-------------:|-----------:|----------:|----------:|",
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
            "## Scene Definition",
            "",
            "| Scene | Representative Functions | Description |",
            "|-------|--------------------------|-------------|",
        ]
    )

    for entry in entries:
        if entry["name"] == "baseline":
            continue
        functions = "<br>".join("`%s`" % item for item in entry["functions"])
        lines.append("| %s | %s | %s |" % (entry["title"], functions, entry["description"]))

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
            "python scripts/size_analysis/canvas_path_size_to_doc.py",
            "```",
            "",
            "Raw JSON is written to `output/canvas_path_size_results.json`.",
        ]
    )

    if doc_path is not None:
        doc_path.write_text(normalize_canvas_path_doc_text("\n".join(lines) + "\n"), encoding="utf-8")


def parse_args():
    parser = argparse.ArgumentParser(description="Generate canvas path qemu size report")
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
    json_path = report_dir / "canvas_path_size_results.json"
    doc_path = None if args.no_doc else report_dir / "canvas_path_size_report.md"

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

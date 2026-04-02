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
APP_OVERRIDE_PATH = PROJECT_ROOT / "example" / "HelloSizeAnalysis" / "canvas_path_probe" / "size_analysis_app_config_override.h"

APP = "HelloSizeAnalysis"
APP_SUB = "canvas_path_probe"
PORT = "qemu"
CPU_ARCH = "cortex-m0plus"
MAKE_JOBS = max(1, multiprocessing.cpu_count())
APP_OBJ_SUFFIX = "size_canvas_feature"

VARIANTS = [
    {
        "name": "baseline",
        "title": "BASELINE",
        "suffix": "cpf_base",
        "cflags": [
            "-DEGUI_CONFIG_FUNCTION_SUPPORT_MASK=0",
            "-DEGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE=0",
            "-DEGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE=0",
        ],
        "scenes": [],
        "description": "所有可选 feature 关闭，不强制链接任何 probe。",
    },
    {
        "name": "basic_geometry_feature",
        "title": "BASIC_GEOMETRY",
        "suffix": "cpf_geom",
        "cflags": [
            "-DEGUI_CONFIG_FUNCTION_SUPPORT_MASK=0",
            "-DEGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE=0",
            "-DEGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE=0",
            "-DEGUI_SIZE_PROBE_RECT_STROKE_PATH=1",
            "-DEGUI_SIZE_PROBE_RECT_FILL_PATH=1",
            "-DEGUI_SIZE_PROBE_ROUND_RECT_STROKE_PATH=1",
            "-DEGUI_SIZE_PROBE_ROUND_RECT_FILL_PATH=1",
            "-DEGUI_SIZE_PROBE_TRIANGLE_STROKE_PATH=1",
            "-DEGUI_SIZE_PROBE_TRIANGLE_FILL_PATH=1",
        ],
        "scenes": ["RECT_*", "ROUND_RECT_*", "TRIANGLE_*"],
        "description": "基础几何图元能力集合，不含 circle/arc。",
    },
    {
        "name": "circle_arc_basic_feature",
        "title": "CIRCLE_ARC_BASIC",
        "suffix": "cpf_circle",
        "cflags": [
            "-DEGUI_CONFIG_FUNCTION_SUPPORT_MASK=0",
            "-DEGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE=0",
            "-DEGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE=0",
            "-DEGUI_SIZE_PROBE_CIRCLE_BASIC_STROKE_PATH=1",
            "-DEGUI_SIZE_PROBE_CIRCLE_BASIC_FILL_PATH=1",
            "-DEGUI_SIZE_PROBE_ARC_BASIC_STROKE_PATH=1",
            "-DEGUI_SIZE_PROBE_ARC_BASIC_FILL_PATH=1",
        ],
        "scenes": ["CIRCLE_BASIC_*", "ARC_BASIC_*"],
        "description": "basic circle/arc 能力集合，包含 LUT 依赖。",
    },
    {
        "name": "line_feature",
        "title": "LINE_FAMILY",
        "suffix": "cpf_line",
        "cflags": [
            "-DEGUI_CONFIG_FUNCTION_SUPPORT_MASK=0",
            "-DEGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE=0",
            "-DEGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE=0",
            "-DEGUI_SIZE_PROBE_LINE_PATH=1",
            "-DEGUI_SIZE_PROBE_POLYLINE_PATH=1",
        ],
        "scenes": ["LINE", "POLYLINE"],
        "description": "基础 line/polyline 能力集合。",
    },
    {
        "name": "gradient_feature",
        "title": "GRADIENT_FEATURE",
        "suffix": "cpf_grad",
        "cflags": [
            "-DEGUI_CONFIG_FUNCTION_SUPPORT_MASK=0",
            "-DEGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE=0",
            "-DEGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE=0",
            "-DEGUI_SIZE_PROBE_GRADIENT_RECT_PATH=1",
            "-DEGUI_SIZE_PROBE_GRADIENT_ROUND_RECT_PATH=1",
            "-DEGUI_SIZE_PROBE_GRADIENT_CIRCLE_PATH=1",
        ],
        "scenes": ["GRADIENT_RECT", "GRADIENT_ROUND_RECT", "GRADIENT_CIRCLE"],
        "description": "shape gradient 能力集合。",
    },
    {
        "name": "mask_feature",
        "title": "MASK_FEATURE",
        "suffix": "cpf_mask",
        "cflags": [
            "-DEGUI_CONFIG_FUNCTION_SUPPORT_MASK=1",
            "-DEGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE=0",
            "-DEGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE=0",
            "-DEGUI_SIZE_PROBE_MASK_CIRCLE_PATH=1",
            "-DEGUI_SIZE_PROBE_MASK_ROUND_RECT_PATH=1",
            "-DEGUI_SIZE_PROBE_MASK_IMAGE_PATH=1",
        ],
        "scenes": ["MASK_CIRCLE", "MASK_ROUND_RECT", "MASK_IMAGE"],
        "description": "mask 能力集合。这里的成本口径是开启 mask 支持后再强制链接三个代表场景。",
    },
    {
        "name": "image_std_feature",
        "title": "IMAGE_STD_FEATURE",
        "suffix": "cpf_img",
        "cflags": [
            "-DEGUI_CONFIG_FUNCTION_SUPPORT_MASK=0",
            "-DEGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE=0",
            "-DEGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE=0",
            "-DEGUI_SIZE_PROBE_IMAGE_DRAW_PATH=1",
            "-DEGUI_SIZE_PROBE_IMAGE_RESIZE_PATH=1",
            "-DEGUI_SIZE_PROBE_IMAGE_TINT_PATH=1",
        ],
        "scenes": ["IMAGE_DRAW", "IMAGE_RESIZE", "IMAGE_TINT"],
        "description": "标准图片绘制能力集合。",
    },
    {
        "name": "image_transform_feature",
        "title": "IMAGE_TRANSFORM",
        "suffix": "cpf_img_rot",
        "cflags": [
            "-DEGUI_CONFIG_FUNCTION_SUPPORT_MASK=0",
            "-DEGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE=0",
            "-DEGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE=0",
            "-DEGUI_SIZE_PROBE_IMAGE_ROTATE_PATH=1",
        ],
        "scenes": ["IMAGE_ROTATE"],
        "description": "图片 transform/rotate 能力。",
    },
    {
        "name": "text_transform_feature",
        "title": "TEXT_TRANSFORM",
        "suffix": "cpf_txt_rot",
        "cflags": [
            "-DEGUI_CONFIG_FUNCTION_SUPPORT_MASK=0",
            "-DEGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE=0",
            "-DEGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE=0",
            "-DEGUI_SIZE_PROBE_TEXT_ROTATE_PATH=1",
        ],
        "scenes": ["TEXT_ROTATE"],
        "description": "文字 transform/rotate 能力。",
    },
    {
        "name": "rle_codec_feature",
        "title": "RLE_CODEC",
        "suffix": "cpf_rle",
        "cflags": [
            "-DEGUI_CONFIG_FUNCTION_SUPPORT_MASK=0",
            "-DEGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE=0",
            "-DEGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE=1",
            "-DEGUI_SIZE_PROBE_RLE_DRAW_PATH=1",
        ],
        "scenes": ["RLE_DRAW"],
        "description": "RLE codec 能力，总体成本口径为 codec 打开并强制链接一个绘制场景。",
    },
    {
        "name": "qoi_codec_feature",
        "title": "QOI_CODEC",
        "suffix": "cpf_qoi",
        "cflags": [
            "-DEGUI_CONFIG_FUNCTION_SUPPORT_MASK=0",
            "-DEGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE=1",
            "-DEGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE=0",
            "-DEGUI_SIZE_PROBE_QOI_DRAW_PATH=1",
        ],
        "scenes": ["QOI_DRAW"],
        "description": "QOI codec 能力，总体成本口径为 codec 打开并强制链接一个绘制场景。",
    },
    {
        "name": "all_common_canvas_features",
        "title": "ALL_COMMON_CANVAS_FEATURES",
        "suffix": "cpf_all",
        "cflags": [
            "-DEGUI_CONFIG_FUNCTION_SUPPORT_MASK=1",
            "-DEGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE=1",
            "-DEGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE=1",
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
        "scenes": ["all common non-HQ canvas features"],
        "description": "当前报告内所有常见非 HQ canvas feature 一起打开后的总体成本。",
    },
]


CANVAS_FEATURE_DESCRIPTION_OVERRIDES = {
    "baseline": "All optional feature switches are disabled and no probe scene is force-linked.",
    "basic_geometry_feature": "Basic geometry paths for rect, round-rect and triangle, excluding circle and arc.",
    "circle_arc_basic_feature": "Basic circle and arc feature set, including LUT-related dependency cost.",
    "line_feature": "Basic line and polyline feature set.",
    "gradient_feature": "Shape gradient feature set.",
    "mask_feature": "Mask feature set measured with circle, round-rect and image mask representative scenes.",
    "image_std_feature": "Standard image draw, resize and tint feature set.",
    "image_transform_feature": "Image transform and rotation feature.",
    "text_transform_feature": "Text transform and rotation feature.",
    "rle_codec_feature": "RLE codec enabled, plus one representative draw path to reflect full linked cost.",
    "qoi_codec_feature": "QOI codec enabled, plus one representative draw path to reflect full linked cost.",
    "all_common_canvas_features": "All common non-HQ canvas features enabled together.",
}

CANVAS_FEATURE_DOC_REPLACEMENTS = {
    "## 璇存槑": "## Notes",
    "- 杩欎唤鎶ュ憡瑙ｅ喅鐨勬槸鈥滄暣椤硅兘鍔涘€间笉鍊煎緱寮€鈥濈殑闂锛屼笉鏄崟涓嚱鏁扮殑杈归檯鎴愭湰銆?": "- This report answers whether an entire canvas feature set is worth enabling. It is not the marginal cost of a single public function.",
    "- baseline 鏄庣‘鍏抽棴 `MASK/QOI/RLE` 杩欑被鏈夋€诲紑鍏崇殑 feature锛屽啀涓?feature-on 鍙樹綋姣旇緝銆?": "- The baseline explicitly disables global switches such as `MASK / QOI / RLE`, then compares one feature-on variant at a time.",
    "- 瀵逛簬娌℃湁鍏ㄥ眬寮€鍏崇殑鑳藉姏锛屼緥濡?`IMAGE_TRANSFORM`銆乣TEXT_TRANSFORM`銆乣GRADIENT`锛岃繖閲屼粛浣跨敤 probe 鑱氬悎鍦烘櫙浠ｈ〃璇?feature 鐨勬€讳綋鎴愭湰銆?": "- For abilities without a single global switch, such as `IMAGE_TRANSFORM`, `TEXT_TRANSFORM` and `GRADIENT`, representative probe scenes are used to approximate the linked feature cost.",
    "- HQ line/circle/arc 涓嶅湪杩欎唤鎶ュ憡閲岋紝浠嶈鐪?`hq_size_report.md`銆?": "- HQ `line/circle/arc` paths are not part of this report. Read `hq_size_report.md` for those numbers.",
}


def normalize_canvas_feature_doc_text(text):
    for source, target in CANVAS_FEATURE_DOC_REPLACEMENTS.items():
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


def render_define_config(header_guard, macros):
    lines = [
        "#ifndef %s" % header_guard,
        "#define %s" % header_guard,
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
            "#endif /* %s */" % header_guard,
            "",
        ]
    )
    return "\n".join(lines)


def write_probe_config(macros):
    PROBE_CONFIG_PATH.write_text(render_define_config("_SIZE_ANALYSIS_PROBE_CONFIG_H_", macros), encoding="utf-8")


def write_app_override(macros):
    APP_OVERRIDE_PATH.write_text(render_define_config("_SIZE_ANALYSIS_APP_CONFIG_OVERRIDE_H_", macros), encoding="utf-8")


def split_variant_macros(macros):
    global_macros = []
    probe_macros = []
    for macro in macros:
        token = macro[2:] if macro.startswith("-D") else macro
        if token.startswith("EGUI_CONFIG_"):
            global_macros.append(macro)
        else:
            probe_macros.append(macro)
    return global_macros, probe_macros


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
    global_macros, probe_macros = split_variant_macros(variant["cflags"])
    write_app_override(global_macros)
    write_probe_config(probe_macros)

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
        "description": CANVAS_FEATURE_DESCRIPTION_OVERRIDES.get(variant["name"], variant["description"]),
        "cflags": variant["cflags"],
        "scenes": variant["scenes"],
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
        "# Canvas Feature QEMU Code Size Report",
        "",
        "- Commit: `%s`" % payload["git_commit"],
        "- Date: %s" % payload["timestamp"],
        "- Build target: `APP=%s APP_SUB=%s PORT=%s CPU_ARCH=%s`" % (APP, APP_SUB, PORT, CPU_ARCH),
        "- Measurement method: compile a dedicated probe app, then rewrite app-local feature override + probe config headers to enable one feature set.",
        "- Scope: feature-level static qemu ELF sections only (`.text/.rodata/.data/.bss`).",
        "",
        "## 说明",
        "",
        "- 这份报告解决的是“整项能力值不值得开”的问题，不是单个函数的边际成本。",
        "- baseline 明确关闭 `MASK/QOI/RLE` 这类有总开关的 feature，再与 feature-on 变体比较。",
        "- 对于没有全局开关的能力，例如 `IMAGE_TRANSFORM`、`TEXT_TRANSFORM`、`GRADIENT`，这里仍使用 probe 聚合场景代表该 feature 的总体成本。",
        "- HQ line/circle/arc 不在这份报告里，仍请看 `hq_size_report.md`。",
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
        "## Feature Summary",
        "",
        "| Feature | Delta Text | Delta Rodata | Delta Data | Delta Bss | Delta ROM |",
        "|---------|-----------:|-------------:|-----------:|----------:|----------:|",
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
            "## Feature Definition",
            "",
            "| Feature | Included Scenes | Description |",
            "|---------|-----------------|-------------|",
        ]
    )

    for entry in entries:
        if entry["name"] == "baseline":
            continue
        scenes = "<br>".join("`%s`" % item for item in entry["scenes"])
        lines.append("| %s | %s | %s |" % (entry["title"], scenes, entry["description"]))

    lines.extend(
        [
            "",
            "## Detailed Variants",
            "",
            "| Variant | Feature Config | Text | Rodata | Data | Bss | Total ROM |",
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
            "python scripts/size_analysis/canvas_feature_size_to_doc.py",
            "```",
            "",
            "Raw JSON is written to `output/canvas_feature_size_results.json`.",
        ]
    )

    if doc_path is not None:
        doc_path.write_text(normalize_canvas_feature_doc_text("\n".join(lines) + "\n"), encoding="utf-8")


def parse_args():
    parser = argparse.ArgumentParser(description="Generate canvas feature qemu size report")
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
    json_path = report_dir / "canvas_feature_size_results.json"
    doc_path = None if args.no_doc else report_dir / "canvas_feature_size_report.md"

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

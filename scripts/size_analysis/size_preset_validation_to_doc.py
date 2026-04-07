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
APP_OVERRIDE_PATH = PROJECT_ROOT / "example" / "HelloSizeAnalysis" / "preset_validation" / "size_analysis_app_config_override.h"

APP = "HelloSizeAnalysis"
APP_SUB = "preset_validation"
PORT = "qemu"
CPU_ARCH = "cortex-m0plus"
MAKE_JOBS = max(1, multiprocessing.cpu_count())
APP_OBJ_SUFFIX = "size_preset_validation"

PRESETS = [
    {
        "name": "tiny_rom",
        "title": "tiny_rom",
        "path": "example/HelloSizeAnalysis/ConfigProfiles/tiny_rom/app_egui_config.h",
        "description": "极限 ROM 裁剪模板。",
    },
    {
        "name": "basic_ui",
        "title": "basic_ui",
        "path": "example/HelloSizeAnalysis/ConfigProfiles/basic_ui/app_egui_config.h",
        "description": "常规 UI 模板。",
    },
    {
        "name": "dashboard",
        "title": "dashboard",
        "path": "example/HelloSizeAnalysis/ConfigProfiles/dashboard/app_egui_config.h",
        "description": "仪表盘/看板模板。",
    },
    {
        "name": "full_feature",
        "title": "full_feature",
        "path": "example/HelloSizeAnalysis/ConfigProfiles/full_feature/app_egui_config.h",
        "description": "高能力模板。",
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


def extract_template_body(text):
    kept = []
    for line in text.splitlines():
        stripped = line.strip()
        if stripped in (
            "#ifndef _APP_EGUI_CONFIG_H_",
            "#define _APP_EGUI_CONFIG_H_",
            "#endif /* _APP_EGUI_CONFIG_H_ */",
            "#ifdef __cplusplus",
            "#endif",
            'extern "C" {',
            "}",
        ):
            continue
        kept.append(line)
    body = "\n".join(kept).strip()
    if body:
        return body + "\n"
    return ""


def select_presets(all_presets, requested):
    if not requested:
        return list(all_presets)

    name_map = {}
    for preset in all_presets:
        name_map[preset["name"]] = preset
        name_map[preset["title"].lower()] = preset

    selected = []
    for raw_name in requested:
        key = raw_name.strip().lower()
        if not key:
            continue
        if key not in name_map:
            raise ValueError("Unknown preset: %s" % raw_name)
        preset = name_map[key]
        if preset not in selected:
            selected.append(preset)
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


def build_preset(preset):
    template_content = (PROJECT_ROOT / preset["path"]).read_text(encoding="utf-8")
    APP_OVERRIDE_PATH.write_text(extract_template_body(template_content), encoding="utf-8")

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
    success = build_result.returncode == 0

    entry = {
        "name": preset["name"],
        "title": preset["title"],
        "path": preset["path"],
        "description": preset["description"],
        "build_success": success,
        "build_message": "",
        "sections": {
            "text": 0,
            "rodata": 0,
            "data": 0,
            "bss": 0,
        },
        "rom_total": 0,
    }

    if not success:
        entry["build_message"] = (build_result.stderr or build_result.stdout)[-4000:]
        return entry

    elf_path = OUTPUT_DIR / "main.elf"
    size_result = run_tool_with_retry(["llvm-size", "-A", str(elf_path)], timeout=30)
    if size_result.returncode != 0:
        entry["build_success"] = False
        entry["build_message"] = (size_result.stderr or size_result.stdout)[-4000:]
        return entry

    sections = parse_size_output(size_result.stdout)
    entry["sections"] = {
        "text": sections.get(".text", 0),
        "rodata": sections.get(".rodata", 0),
        "data": sections.get(".data", 0),
        "bss": sections.get(".bss", 0),
    }
    entry["rom_total"] = entry["sections"]["text"] + entry["sections"]["rodata"]
    return entry


def generate_report(entries, json_path, doc_path=None):
    payload = {
        "timestamp": datetime.now().isoformat(),
        "git_commit": get_git_commit(),
        "app": APP,
        "port": PORT,
        "cpu_arch": CPU_ARCH,
        "presets": entries,
    }
    json_path.write_text(json.dumps(payload, indent=2, ensure_ascii=False), encoding="utf-8")

    lines = [
        "# Size Preset Validation",
        "",
        "- Commit: `%s`" % payload["git_commit"],
        "- Date: %s" % payload["timestamp"],
        "- Validation target: `APP=%s APP_SUB=%s PORT=%s CPU_ARCH=%s`" % (APP, APP_SUB, PORT, CPU_ARCH),
        "- Validation method: rewrite the app-local preset override header from each template, then build the qemu target and collect ELF section sizes.",
        "- Purpose: verify that preset template files themselves are syntactically valid and can be used as direct app-local config bodies.",
        "",
        "## Result",
        "",
        "| Preset | Build | Text | Rodata | Data | Bss | Total ROM | Template |",
        "|--------|-------|-----:|-------:|-----:|----:|----------:|----------|",
    ]

    for entry in entries:
        status = "PASS" if entry["build_success"] else "FAIL"
        lines.append(
            "| %s | %s | %d | %d | %d | %d | %d | `%s` |"
            % (
                entry["title"],
                status,
                entry["sections"]["text"],
                entry["sections"]["rodata"],
                entry["sections"]["data"],
                entry["sections"]["bss"],
                entry["rom_total"],
                entry["path"],
            )
        )

    failures = [item for item in entries if not item["build_success"]]
    if failures:
        lines.extend(["", "## Failures", ""])
        for item in failures:
            lines.append("### %s" % item["title"])
            lines.append("")
            lines.append("```text")
            lines.append(item["build_message"].rstrip())
            lines.append("```")
            lines.append("")

    lines.extend(
        [
            "## Reproduce",
            "",
            "```bash",
            "python scripts/size_analysis/main.py size-preset-validation-to-doc",
            "```",
            "",
            "Raw JSON is written to `output/size_preset_validation_results.json`.",
        ]
    )

    if doc_path is not None:
        doc_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def parse_args():
    parser = argparse.ArgumentParser(description="Validate preset config templates")
    parser.add_argument("--variants", help="Comma-separated preset names/titles to validate")
    parser.add_argument("--list-variants", action="store_true", help="List available presets and exit")
    parser.add_argument("--no-doc", action="store_true", help="Skip markdown report generation")
    parser.add_argument("--report-dir", help="Override output directory for generated JSON/Markdown reports")
    return parser.parse_args()


def main():
    args = parse_args()

    if args.list_variants:
        for preset in PRESETS:
            print("%s (%s)" % (preset["name"], preset["title"]))
        return

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    report_dir = Path(args.report_dir) if args.report_dir else DOC_DIR
    if not report_dir.is_absolute():
        report_dir = (PROJECT_ROOT / report_dir).resolve()
    report_dir.mkdir(parents=True, exist_ok=True)
    json_path = report_dir / "size_preset_validation_results.json"
    doc_path = None if args.no_doc else report_dir / "size_preset_validation.md"

    clean_result = run_cmd(["make", "clean"], timeout=180)
    if clean_result.returncode != 0:
        raise RuntimeError("make clean failed\n%s" % (clean_result.stderr or clean_result.stdout))

    requested = args.variants.split(",") if args.variants else []
    presets = select_presets(PRESETS, requested)

    entries = []
    for preset in presets:
        print("=== Validating %s ===" % preset["title"])
        entries.append(build_preset(preset))

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

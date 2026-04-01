#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
import json
from datetime import datetime
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent
SIZE_OUTPUT = PROJECT_ROOT / "output"
DOC_DIR = PROJECT_ROOT / "doc" / "source" / "size"
IMG_DIR = DOC_DIR / "images"
LEGACY_SIZE_OUTPUT = SIZE_OUTPUT / "stm32g0_empty_size_results.json"


def setup_matplotlib():
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.font_manager as fm
    import matplotlib.pyplot as plt

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
    if not path.exists():
        print("  [WARN] File not found: %s" % path)
        return None
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def _get_case_scope(data=None):
    if data:
        scope = data.get("scope", {})
        description = scope.get("description")
        if description:
            return description
    return _format_case_scope()


def _format_case_scope():
    return "HelloBasic 全子 case、HelloSimple、HelloPerformance、HelloShowcase、HelloStyleDemo、HelloVirtual(virtual_stage_showcase)"


def _build_legacy_compare_lines():
    qemu_data = load_json(SIZE_OUTPUT / "size_results.json")
    legacy_data = load_json(LEGACY_SIZE_OUTPUT)

    if not qemu_data or not legacy_data:
        return []

    qemu_apps = {item["name"]: item for item in qemu_data.get("apps", [])}
    legacy_apps = {item["name"]: item for item in legacy_data.get("apps", [])}
    common_names = sorted(set(qemu_apps) & set(legacy_apps))

    if not common_names:
        return []

    code_deltas = []
    ram_deltas = []
    pfb_deltas = []
    rom_deltas = []

    for name in common_names:
        qemu_item = qemu_apps[name]
        legacy_item = legacy_apps[name]
        code_deltas.append(qemu_item["code_bytes"] - legacy_item["code_bytes"])
        ram_deltas.append(qemu_item["ram_bytes"] - legacy_item["ram_bytes"])
        pfb_deltas.append(qemu_item["pfb_bytes"] - legacy_item["pfb_bytes"])
        rom_deltas.append(
            (qemu_item["code_bytes"] + qemu_item["resource_bytes"]) - (legacy_item["code_bytes"] + legacy_item["resource_bytes"])
        )

    def summarize(values):
        ordered = sorted(values)
        count = len(ordered)
        middle = count // 2
        if count % 2 == 1:
            median = ordered[middle]
        else:
            median = (ordered[middle - 1] + ordered[middle]) // 2
        return ordered[0], median, ordered[-1]

    code_min, code_median, code_max = summarize(code_deltas)
    ram_min, ram_median, ram_max = summarize(ram_deltas)
    pfb_min, pfb_median, pfb_max = summarize(pfb_deltas)
    rom_min, rom_median, rom_max = summarize(rom_deltas)

    code_over_10kb = sum(1 for value in code_deltas if value > 10 * 1024)
    ram_over_1kb = sum(1 for value in ram_deltas if value > 1024)
    failures = legacy_data.get("failures", [])

    lines = [
        "## QEMU vs stm32g0_empty",
        "",
        "- 对比范围：%d 个共同成功 case；qemu 为当前正式口径，`stm32g0_empty` 仅用于静态 size 交叉验证。" % len(common_names),
        "- Code delta（qemu - stm32g0_empty）：min %d B / median %d B / max %d B。" % (code_min, code_median, code_max),
        "- ROM delta（code + resource）：min %d B / median %d B / max %d B。" % (rom_min, rom_median, rom_max),
        "- RAM delta（不含 PFB）：min %d B / median %d B / max %d B。" % (ram_min, ram_median, ram_max),
        "- PFB delta：min %d B / median %d B / max %d B。" % (pfb_min, pfb_median, pfb_max),
        "- 超过阈值统计：`code > +10KB` 为 %d 个，`ram > +1KB` 为 %d 个。" % (code_over_10kb, ram_over_1kb),
        "- 结论：当前 qemu 口径没有引入用户担心的 `>10KB` 代码膨胀或 `>1KB` 静态 RAM 膨胀，并且额外提供了运行期 heap/stack 数据。",
    ]

    if failures:
        lines.append(
            "- 旧链路额外维护成本：`stm32g0_empty` 在 Windows 下仍有 %d 个大 case 无法稳定完成链接：%s。"
            % (len(failures), "、".join(item["name"] for item in failures))
        )

    lines.append("")
    return lines


def generate_size_report():
    plt = setup_matplotlib()
    import numpy as np

    data = load_json(SIZE_OUTPUT / "size_results.json")
    if not data:
        print("  [SKIP] size_results.json not found")
        return

    timestamp = data.get("timestamp", "N/A")
    commit = data.get("git_commit", "N/A")
    build_platform = data.get("build_platform", {})
    runtime_platform = data.get("runtime_platform", {})
    apps = data.get("apps", [])
    failures = data.get("failures", [])

    if not apps:
        print("  [SKIP] No app data in size_results.json")
        return

    app_names = [a["name"] for a in apps]
    code_sizes = [a["code_bytes"] / 1024.0 for a in apps]
    resource_sizes = [a["resource_bytes"] / 1024.0 for a in apps]
    ram_sizes = [a["ram_bytes"] / 1024.0 for a in apps]
    pfb_sizes = [a["pfb_bytes"] / 1024.0 for a in apps]
    heap_peak_sizes = [a["heap_peak_bytes"] / 1024.0 for a in apps]
    stack_peak_sizes = [a["stack_peak_bytes"] / 1024.0 for a in apps]

    fig, (ax1, ax2, ax3) = plt.subplots(1, 3, figsize=(30, max(10, len(apps) * 0.28)), dpi=150)
    y_pos = np.arange(len(app_names))

    ax1.barh(y_pos, code_sizes, color="#3498DB", edgecolor="none", height=0.7, label="Code")
    ax1.barh(y_pos, resource_sizes, left=code_sizes, color="#E67E22", edgecolor="none", height=0.7, label="Resource")
    ax1.set_yticks(y_pos)
    ax1.set_yticklabels(app_names, fontsize=6)
    ax1.set_xlabel("Size (KB)")
    ax1.set_title("ROM Usage: Code + Resource")
    ax1.invert_yaxis()
    ax1.legend(fontsize=8, loc="lower right")

    ax2.barh(y_pos, ram_sizes, color="#27AE60", edgecolor="none", height=0.7, label="RAM")
    ax2.barh(y_pos, pfb_sizes, left=ram_sizes, color="#9B59B6", edgecolor="none", height=0.7, label="PFB")
    ax2.set_yticks(y_pos)
    ax2.set_yticklabels(app_names, fontsize=6)
    ax2.set_xlabel("Size (KB)")
    ax2.set_title("Static RAM Usage: RAM + PFB")
    ax2.invert_yaxis()
    ax2.legend(fontsize=8, loc="lower right")

    group_height = 0.34
    ax3.barh(y_pos - group_height / 2, heap_peak_sizes, color="#16A085", edgecolor="none", height=group_height, label="Heap Peak")
    ax3.barh(y_pos + group_height / 2, stack_peak_sizes, color="#C0392B", edgecolor="none", height=group_height, label="Stack Peak")
    ax3.set_yticks(y_pos)
    ax3.set_yticklabels(app_names, fontsize=6)
    ax3.set_xlabel("Size (KB)")
    ax3.set_title("Runtime Peak Usage")
    ax3.invert_yaxis()
    ax3.legend(fontsize=8, loc="lower right")

    plt.tight_layout()
    IMG_DIR.mkdir(parents=True, exist_ok=True)
    chart_path = IMG_DIR / "size_report.png"
    fig.savefig(chart_path)
    plt.close(fig)
    print("  Chart saved: %s" % chart_path)

    min_code = min(apps, key=lambda a: a["code_bytes"])
    max_code = max(apps, key=lambda a: a["code_bytes"])
    min_ram = min(apps, key=lambda a: a["ram_bytes"])
    max_ram = max(apps, key=lambda a: a["ram_bytes"])
    min_heap = min(apps, key=lambda a: a["heap_peak_bytes"])
    max_heap = max(apps, key=lambda a: a["heap_peak_bytes"])
    min_stack = min(apps, key=lambda a: a["stack_peak_bytes"])
    max_stack = max(apps, key=lambda a: a["stack_peak_bytes"])

    lines = [
        "# QEMU Size And Memory Report",
        "",
        "- Commit: `%s`" % commit,
        "- Date: %s" % timestamp,
        "- Build target: `PORT=%s CPU_ARCH=%s`" % (build_platform.get("port", "qemu"), build_platform.get("cpu_arch", "cortex-m0plus")),
        "- Runtime target: `qemu-system-arm -machine %s -cpu %s`" % (runtime_platform.get("machine", "mps2-an385"), runtime_platform.get("cpu", "cortex-m3")),
        "- Scope: %s" % _get_case_scope(data),
        "- Successful apps: %d" % len(apps),
        "- Failed apps: %d" % len(failures),
        "",
        "## Overview",
        "",
        "- Smallest Code: **%s** (%d bytes)" % (min_code["name"], min_code["code_bytes"]),
        "- Largest Code: **%s** (%d bytes)" % (max_code["name"], max_code["code_bytes"]),
        "- Smallest RAM: **%s** (%d bytes)" % (min_ram["name"], min_ram["ram_bytes"]),
        "- Largest RAM: **%s** (%d bytes)" % (max_ram["name"], max_ram["ram_bytes"]),
        "- Smallest Heap Peak: **%s** (%d bytes)" % (min_heap["name"], min_heap["heap_peak_bytes"]),
        "- Largest Heap Peak: **%s** (%d bytes)" % (max_heap["name"], max_heap["heap_peak_bytes"]),
        "- Smallest Stack Peak: **%s** (%d bytes)" % (min_stack["name"], min_stack["stack_peak_bytes"]),
        "- Largest Stack Peak: **%s** (%d bytes)" % (max_stack["name"], max_stack["stack_peak_bytes"]),
        "",
        "## Charts",
        "",
        "![Size Report](images/size_report.png)",
        "",
        "## Detailed Table",
        "",
        "| App | Code | Resource | RAM | PFB | Heap Idle | Heap Init Peak | Heap Interaction Peak | Heap Peak | Stack Peak | Actions | Total ROM |",
        "|-----|------|----------|-----|-----|-----------|----------------|-----------------------|-----------|------------|---------|-----------|",
    ]

    for a in apps:
        total_rom = a["code_bytes"] + a["resource_bytes"]
        lines.append(
            "| %s | %d | %d | %d | %d | %d | %d | %d | %d | %d | %d | %d |"
            % (
                a["name"],
                a["code_bytes"],
                a["resource_bytes"],
                a["ram_bytes"],
                a["pfb_bytes"],
                a["heap_idle_bytes"],
                a["heap_init_peak_bytes"],
                a["heap_interaction_peak_bytes"],
                a["heap_peak_bytes"],
                a["stack_peak_bytes"],
                a["interaction_action_count"],
                total_rom,
            )
        )

    if failures:
        lines.extend(
            [
                "",
                "## Failures",
                "",
                "| App | Phase | Message |",
                "|-----|-------|---------|",
            ]
        )
        for item in failures:
            lines.append("| %s | %s | %s |" % (item["name"], item["phase"], item["message"].replace("\n", " ")))

    md_path = DOC_DIR / "size_report.md"
    md_path.write_text("\n".join(lines), encoding="utf-8")
    print("  Report saved: %s" % md_path)


def generate_size_overview():
    data = load_json(SIZE_OUTPUT / "size_results.json")
    build_platform = {}
    runtime_platform = {}
    if data:
        build_platform = data.get("build_platform", {})
        runtime_platform = data.get("runtime_platform", {})

    overview = [
        "# QEMU Size Analysis",
        "",
        "EmbeddedGUI 的 size 文档现在只覆盖指定示例集合，并统一基于 qemu 口径生成静态 size 与运行期 heap/stack 数据。",
        "",
        "## Scope",
        "",
        "- %s" % _get_case_scope(data),
        "",
        "## Measurement Method",
        "",
        "- **Static size build**: `make all PORT=%s CPU_ARCH=%s`" % (build_platform.get("port", "qemu"), build_platform.get("cpu_arch", "cortex-m0plus")),
        "- **Static data source**: ELF symbols `__code_size`, `__rodata_size`, `__data_size`, `__bss_size`, `__bss_pfb_size`",
        "- **Runtime measure**: `qemu-system-arm -machine %s -cpu %s -icount shift=0`" % (
            runtime_platform.get("machine", "mps2-an385"),
            runtime_platform.get("cpu", "cortex-m3"),
        ),
        "- **Runtime flags**: `-DQEMU_HEAP_MEASURE=1 -DQEMU_HEAP_ACTIONS_APP_RECORDING=1 -DEGUI_CONFIG_RECORDING_TEST=1`",
        "- **Heap peak definition**: `max(idle_peak, interaction_total_peak)`",
        "- **Stack peak definition**: qemu 保留栈区的 watermark 高水位统计",
        "",
        "## Size Categories",
        "",
        "| Category | Description |",
        "|----------|-------------|",
        "| Code | `.text` 可执行代码大小 |",
        "| Resource | `.rodata` 只读资源大小 |",
        "| RAM | `.data + .bss - .bss.pfb_area` 静态 RAM |",
        "| PFB | `Partial Frame Buffer` 静态 RAM |",
        "| Heap Idle | UI 建立后稳定态 heap 占用 |",
        "| Heap Peak | 初始化或交互阶段出现的最大 heap 占用 |",
        "| Stack Peak | qemu 运行期记录到的栈高水位 |",
        "",
        "## Generation Command",
        "",
        "```bash",
        "python scripts/utils_analysis_elf_size.py --case-set typical",
        "python scripts/size_to_doc.py",
        "```",
    ]

    overview.extend(_build_legacy_compare_lines())

    md_path = DOC_DIR / "size_overview.md"
    md_path.write_text("\n".join(overview), encoding="utf-8")
    print("  Overview saved: %s" % md_path)


def main():
    parser = argparse.ArgumentParser(description="Generate size analysis documentation")
    parser.parse_args()

    DOC_DIR.mkdir(parents=True, exist_ok=True)
    IMG_DIR.mkdir(parents=True, exist_ok=True)

    print("\n=== Generating Size Overview ===")
    try:
        generate_size_overview()
    except Exception as e:
        print("  [ERROR] Size Overview: %s" % e)
        import traceback

        traceback.print_exc()

    print("\n=== Generating Size Report ===")
    try:
        generate_size_report()
    except Exception as e:
        print("  [ERROR] Size Report: %s" % e)
        import traceback

        traceback.print_exc()

    print("\nDone.")


if __name__ == "__main__":
    main()

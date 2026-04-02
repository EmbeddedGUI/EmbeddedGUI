#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
import json
import shutil
import subprocess
import sys
from datetime import datetime
from pathlib import Path

try:
    from code_perf_check import find_qemu_executable
except Exception:  # pragma: no cover - fallback only
    def find_qemu_executable():
        return "qemu-system-arm"


SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent.parent
OUTPUT_DIR = PROJECT_ROOT / "perf_output"
ELF_PATH = PROJECT_ROOT / "output" / "main.elf"
RESOURCE_DST = PROJECT_ROOT / "output" / "app_egui_resource_merge.bin"

QEMU_MACHINE = "mps2-an385"
QEMU_CPU = "cortex-m3"
DEFAULT_TIMEOUT = 180

MODE_CFLAGS = {
    "common": "-DQEMU_HEAP_MEASURE=1 -DQEMU_HEAP_ACTIONS_SHOWCASE_COMMON=1",
    "app-recording": "-DQEMU_HEAP_MEASURE=1 -DQEMU_HEAP_ACTIONS_APP_RECORDING=1 -DEGUI_CONFIG_RECORDING_TEST=1",
}

CASES = [
    {
        "name": "HelloShowcase",
        "app": "HelloShowcase",
        "resource": PROJECT_ROOT / "example" / "HelloShowcase" / "resource" / "app_egui_resource_merge.bin",
    },
    {
        "name": "HelloVirtual/virtual_stage_showcase",
        "app": "HelloVirtual",
        "app_sub": "virtual_stage_showcase",
        "resource": PROJECT_ROOT / "example" / "HelloVirtual" / "virtual_stage_showcase" / "resource" / "app_egui_resource_merge.bin",
    },
]


def run_cmd(cmd, timeout=180):
    return subprocess.run(
        cmd,
        cwd=PROJECT_ROOT,
        capture_output=True,
        text=True,
        timeout=timeout,
    )


def get_output_paths(mode):
    safe_mode = mode.replace("-", "_")
    return (
        OUTPUT_DIR / f"virtual_showcase_heap_qemu_{safe_mode}.json",
        OUTPUT_DIR / f"virtual_showcase_heap_qemu_{safe_mode}.md",
    )


def build_case(case, measure_cflags):
    clean = run_cmd(["make", "clean"], timeout=120)
    if clean.returncode != 0:
        raise RuntimeError(f"make clean failed:\n{clean.stderr}")

    cmd = [
        "make",
        "all",
        f"APP={case['app']}",
        "PORT=qemu",
        "CPU_ARCH=cortex-m3",
        f"USER_CFLAGS={measure_cflags}",
    ]
    if "app_sub" in case:
        cmd.append(f"APP_SUB={case['app_sub']}")

    result = run_cmd(cmd, timeout=180)
    if result.returncode != 0:
        raise RuntimeError(f"build failed for {case['name']}:\n{result.stderr}")

    RESOURCE_DST.parent.mkdir(parents=True, exist_ok=True)
    if RESOURCE_DST.exists():
        RESOURCE_DST.unlink()
    if case["resource"].exists():
        shutil.copy(case["resource"], RESOURCE_DST)


def run_qemu(timeout):
    if not ELF_PATH.exists():
        raise RuntimeError(f"ELF not found: {ELF_PATH}")

    qemu_cmd = [
        find_qemu_executable(),
        "-machine",
        QEMU_MACHINE,
        "-cpu",
        QEMU_CPU,
        "-nographic",
        "-semihosting-config",
        "enable=on,target=native",
        "-icount",
        "shift=0",
        "-kernel",
        str(ELF_PATH),
    ]

    proc = subprocess.Popen(
        qemu_cmd,
        cwd=PROJECT_ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    try:
        stdout, _ = proc.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        proc.kill()
        stdout, _ = proc.communicate()
        raise RuntimeError(f"QEMU timeout after {timeout}s\n{stdout[-1000:]}")

    return stdout


def parse_heap_output(output):
    if "[RUNTIME_CHECK_FAIL]" in output:
        raise RuntimeError(f"runtime interaction check failed:\n{output[-2000:]}")

    metrics = {}
    for line in output.splitlines():
        if not line.startswith("HEAP_RESULT:"):
            continue
        payload = line[len("HEAP_RESULT:") :]
        if "=" not in payload:
            continue
        key, value = payload.split("=", 1)
        metrics[key.strip()] = int(value.strip())

    required = [
        "idle_current",
        "idle_peak",
        "interaction_action_count",
        "interaction_delta_peak",
        "interaction_total_peak",
    ]
    missing = [key for key in required if key not in metrics]
    if missing:
        raise RuntimeError(f"missing heap metrics: {', '.join(missing)}\n{output[-1500:]}")

    return metrics


def generate_report(results, mode, measure_cflags):
    lines = []
    lines.append("# Virtual Showcase QEMU Heap Report")
    lines.append("")
    lines.append(f"- Date: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    lines.append(f"- QEMU: `{QEMU_MACHINE}` / `{QEMU_CPU}`")
    lines.append(f"- Mode: `{mode}`")
    lines.append(f"- USER_CFLAGS: `{measure_cflags}`")
    lines.append("")
    lines.append("| Case | Idle Current | Idle Peak | Interaction Delta Peak | Interaction Total Peak | Actions |")
    lines.append("|------|--------------|-----------|------------------------|------------------------|---------|")
    for item in results:
        metrics = item["metrics"]
        lines.append(
            f"| {item['name']} | {metrics['idle_current']} B | {metrics['idle_peak']} B | "
            f"{metrics['interaction_delta_peak']} B | {metrics['interaction_total_peak']} B | "
            f"{metrics['interaction_action_count']} |"
        )
    lines.append("")

    if len(results) == 2:
        base = results[0]["metrics"]
        target = results[1]["metrics"]
        idle_diff = target["idle_current"] - base["idle_current"]
        peak_diff = target["interaction_total_peak"] - base["interaction_total_peak"]
        lines.append("## Difference")
        lines.append("")
        lines.append(f"- Idle current diff: `{idle_diff:+d} B`")
        lines.append(f"- Interaction total peak diff: `{peak_diff:+d} B`")
        lines.append("")

    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description="Compare HelloShowcase and virtual_stage_showcase heap on QEMU")
    parser.add_argument("--timeout", type=int, default=DEFAULT_TIMEOUT, help=f"QEMU timeout seconds (default: {DEFAULT_TIMEOUT})")
    parser.add_argument("--mode", choices=sorted(MODE_CFLAGS.keys()), default="common", help="Interaction source for heap measurement")
    args = parser.parse_args()

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    result_json, report_md = get_output_paths(args.mode)
    measure_cflags = MODE_CFLAGS[args.mode]

    results = []
    for case in CASES:
        print(f"[BUILD] {case['name']}")
        build_case(case, measure_cflags)
        print(f"[RUN] {case['name']}")
        output = run_qemu(args.timeout)
        metrics = parse_heap_output(output)
        results.append(
            {
                "name": case["name"],
                "metrics": metrics,
            }
        )
        print(
            f"  idle_current={metrics['idle_current']}B "
            f"idle_peak={metrics['idle_peak']}B "
            f"interaction_total_peak={metrics['interaction_total_peak']}B"
        )

    payload = {
        "timestamp": datetime.now().isoformat(),
        "qemu_machine": QEMU_MACHINE,
        "qemu_cpu": QEMU_CPU,
        "mode": args.mode,
        "user_cflags": measure_cflags,
        "results": results,
    }
    result_json.write_text(json.dumps(payload, indent=2, ensure_ascii=False), encoding="utf-8")
    report_md.write_text(generate_report(results, args.mode, measure_cflags), encoding="utf-8")

    print(f"\nJSON: {result_json}")
    print(f"Report: {report_md}")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        sys.exit(2)

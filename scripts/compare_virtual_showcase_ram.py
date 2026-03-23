#!/usr/bin/env python3

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SIZE_TOOL = "arm-none-eabi-size"
NM_TOOL = "arm-none-eabi-nm"
RAM_SECTION_PREFIXES = (".data", ".bss")
RAM_SYMBOL_TYPES = {"b", "B", "d", "D", "s", "S", "c", "C"}


@dataclass(frozen=True)
class Target:
    label: str
    object_root: Path
    make_args: tuple[str, ...]


@dataclass
class RamReport:
    total_ram: int
    object_rows: list[tuple[str, int]]
    symbol_rows: list[tuple[str, int]]


TARGETS = (
    Target(
        label="HelloShowcase",
        object_root=ROOT / "output" / "obj" / "HelloShowcase_stm32g0_empty" / "example" / "HelloShowcase",
        make_args=("APP=HelloShowcase", "PORT=stm32g0_empty", "COMPILE_DEBUG=", "COMPILE_OPT_LEVEL=-Os"),
    ),
    Target(
        label="HelloVirtual/virtual_stage_showcase",
        object_root=ROOT
        / "output"
        / "obj"
        / "HelloVirtual_virtual_stage_showcase_stm32g0_empty"
        / "example"
        / "HelloVirtual"
        / "virtual_stage_showcase",
        make_args=("APP=HelloVirtual", "APP_SUB=virtual_stage_showcase", "PORT=stm32g0_empty", "COMPILE_DEBUG=", "COMPILE_OPT_LEVEL=-Os"),
    ),
)


def run_command(command: list[str]) -> str:
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, check=True)
    return result.stdout


def build_target(target: Target) -> None:
    command = ["make", "-j1", "all", *target.make_args]
    subprocess.run(command, cwd=ROOT, check=True)


def parse_size_output(output: str) -> int:
    total = 0
    for line in output.splitlines():
        parts = line.split()
        if len(parts) < 2:
            continue
        section_name = parts[0]
        if not section_name.startswith(RAM_SECTION_PREFIXES):
            continue
        try:
            total += int(parts[1])
        except ValueError:
            continue
    return total


def read_object_ram(obj_path: Path) -> int:
    output = run_command([SIZE_TOOL, "-A", "-d", str(obj_path)])
    return parse_size_output(output)


def read_object_symbols(obj_path: Path) -> list[tuple[str, int]]:
    output = run_command([NM_TOOL, "-S", "--size-sort", "--radix=d", str(obj_path)])
    rows: list[tuple[str, int]] = []
    pattern = re.compile(r"^\S+\s+(\d+)\s+([A-Za-z])\s+(.+)$")

    for line in output.splitlines():
        match = pattern.match(line.strip())
        if match is None:
            continue
        size = int(match.group(1))
        symbol_type = match.group(2)
        symbol_name = match.group(3).strip()
        if symbol_type not in RAM_SYMBOL_TYPES or size <= 0:
            continue
        rows.append((symbol_name, size))

    return rows


def collect_report(target: Target, top_n: int) -> RamReport:
    if not target.object_root.exists():
        raise FileNotFoundError(f"Object directory not found: {target.object_root}")

    object_rows: list[tuple[str, int]] = []
    symbol_rows: list[tuple[str, int]] = []

    for obj_path in sorted(target.object_root.rglob("*.o")):
        ram_size = read_object_ram(obj_path)
        if ram_size > 0:
            object_rows.append((obj_path.relative_to(ROOT).as_posix(), ram_size))
        symbol_rows.extend(read_object_symbols(obj_path))

    object_rows.sort(key=lambda item: item[1], reverse=True)
    symbol_rows.sort(key=lambda item: item[1], reverse=True)

    return RamReport(
        total_ram=sum(size for _, size in object_rows),
        object_rows=object_rows,
        symbol_rows=symbol_rows[:top_n],
    )


def format_bytes(value: int) -> str:
    return f"{value} B"


def print_markdown(reports: dict[str, RamReport]) -> None:
    showcase = reports["HelloShowcase"]
    stage = reports["HelloVirtual/virtual_stage_showcase"]
    delta = showcase.total_ram - stage.total_ram
    delta_percent = (delta * 100.0 / showcase.total_ram) if showcase.total_ram else 0.0

    print("# Virtual Showcase SRAM Comparison")
    print()
    print("口径：仅统计示例自身对象文件的 `.data* + .bss*`。")
    print("排除项：PFB、平台移植层 SRAM、框架公共 SRAM、启动文件公共数据，以及运行时栈/堆。")
    print()
    print("| 示例 | 示例自身静态 SRAM |")
    print("| --- | ---: |")
    print(f"| `HelloShowcase` | {format_bytes(showcase.total_ram)} |")
    print(f"| `HelloVirtual/virtual_stage_showcase` | {format_bytes(stage.total_ram)} |")
    print()
    print(f"结论：`virtual_stage_showcase` 少占 {format_bytes(delta)}，降幅约 {delta_percent:.1f}%。")
    print()

    for label, report in reports.items():
        print(f"## {label}")
        print()
        print("对象文件 Top:")
        print()
        print("| 对象文件 | `.data* + .bss*` |")
        print("| --- | ---: |")
        for path_text, size in report.object_rows[:5]:
            print(f"| `{path_text}` | {format_bytes(size)} |")
        print()
        print("符号 Top:")
        print()
        print("| 符号 | 大小 |")
        print("| --- | ---: |")
        for symbol_name, size in report.symbol_rows:
            print(f"| `{symbol_name}` | {format_bytes(size)} |")
        print()


def main() -> int:
    parser = argparse.ArgumentParser(description="Compare app-owned static SRAM between HelloShowcase and virtual_stage_showcase.")
    parser.add_argument("--skip-build", action="store_true", help="Use existing build outputs and skip make.")
    parser.add_argument("--top", type=int, default=5, help="Number of top RAM symbols to print for each target.")
    args = parser.parse_args()

    if args.top <= 0:
        parser.error("--top must be greater than 0")

    try:
        if not args.skip_build:
            for target in TARGETS:
                build_target(target)

        reports = {target.label: collect_report(target, args.top) for target in TARGETS}
    except (subprocess.CalledProcessError, FileNotFoundError) as exc:
        print(str(exc), file=sys.stderr)
        return 1

    print_markdown(reports)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

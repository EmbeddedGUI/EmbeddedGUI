#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Unified launcher for perf_analysis scripts.

Default behavior forwards arguments to code_perf_check.py.

Examples:
    python scripts/perf_analysis/main.py
    python scripts/perf_analysis/main.py --profile cortex-m3
    python scripts/perf_analysis/main.py perf-to-doc
    python scripts/perf_analysis/main.py scene-capture --filter RECTANGLE,TEXT
    python scripts/perf_analysis/main.py help
"""

from __future__ import annotations

import importlib
import sys
from dataclasses import dataclass
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
LAUNCHER_PATH = Path("scripts") / "perf_analysis" / "main.py"
DEFAULT_COMMAND = "code-perf-check"
LAUNCHER_HELP_ALIASES = {"help", "launcher-help", "--launcher-help"}


@dataclass(frozen=True)
class CommandEntry:
    module_name: str
    description: str


COMMANDS = {
    "code-perf-check": CommandEntry("code_perf_check", "Run the QEMU performance check (default)."),
    "perf-to-doc": CommandEntry("perf_to_doc", "Generate performance docs and charts from perf_output."),
    "scene-capture": CommandEntry("perf_scene_capture", "Capture HelloPerformance scenes into perf_scenes.png."),
    "perf-compare": CommandEntry("perf_compare", "Compare two perf_results.json files."),
    "dirty-region-report": CommandEntry("dirty_region_stats_report", "Generate dirty region Markdown/CSV reports."),
    "compare-virtual-heap-qemu": CommandEntry("compare_virtual_showcase_heap_qemu", "Compare QEMU heap peaks for virtual showcase variants."),
    "compare-virtual-ram": CommandEntry("compare_virtual_showcase_ram", "Compare static RAM usage for showcase variants."),
}


def _build_aliases() -> dict[str, str]:
    aliases: dict[str, str] = {}
    for command_name, entry in COMMANDS.items():
        aliases[command_name] = command_name
        aliases[command_name.replace("-", "_")] = command_name
        aliases[entry.module_name] = command_name
        aliases[f"{entry.module_name}.py"] = command_name
    return aliases


ALIASES = _build_aliases()


def _launcher_help_text() -> str:
    lines = [
        "perf_analysis unified launcher",
        "",
        f"Default command: python {LAUNCHER_PATH} [code_perf_check args]",
        "",
        "Available commands:",
    ]
    for command_name, entry in COMMANDS.items():
        lines.append(f"  {command_name:<27} {entry.description}")
    lines.extend(
        [
            "",
            "Examples:",
            f"  python {LAUNCHER_PATH} --profile cortex-m3",
            f"  python {LAUNCHER_PATH} --full-check --doc",
            f"  python {LAUNCHER_PATH} perf-to-doc",
            f"  python {LAUNCHER_PATH} scene-capture --filter RECTANGLE,TEXT",
            f"  python {LAUNCHER_PATH} help",
        ]
    )
    return "\n".join(lines)


def _resolve_command(argv: list[str]) -> tuple[str | None, list[str]]:
    if not argv:
        return DEFAULT_COMMAND, []

    first_arg = argv[0]
    if first_arg in LAUNCHER_HELP_ALIASES:
        return None, argv[1:]

    command_name = ALIASES.get(first_arg)
    if command_name:
        return command_name, argv[1:]

    if first_arg.startswith("-"):
        return DEFAULT_COMMAND, argv

    return "", argv


def _run_command(command_name: str, forwarded_args: list[str]) -> int:
    entry = COMMANDS[command_name]

    if str(SCRIPT_DIR) not in sys.path:
        sys.path.insert(0, str(SCRIPT_DIR))

    module = importlib.import_module(entry.module_name)

    if command_name == DEFAULT_COMMAND:
        program_name = str(LAUNCHER_PATH)
    else:
        program_name = f"{LAUNCHER_PATH} {command_name}"

    old_argv = sys.argv[:]
    sys.argv = [program_name, *forwarded_args]
    try:
        result = module.main()
    except SystemExit as exc:
        code = exc.code
        if code is None:
            return 0
        if isinstance(code, int):
            return code
        print(code, file=sys.stderr)
        return 1
    finally:
        sys.argv = old_argv

    if isinstance(result, int):
        return result
    return 0


def main() -> int:
    command_name, forwarded_args = _resolve_command(sys.argv[1:])

    if command_name is None:
        print(_launcher_help_text())
        return 0

    if command_name == "":
        print(f"Unknown perf_analysis command: {forwarded_args[0]}", file=sys.stderr)
        print("Run `python scripts/perf_analysis/main.py help` to list available commands.", file=sys.stderr)
        return 2

    return _run_command(command_name, forwarded_args)


if __name__ == "__main__":
    sys.exit(main())

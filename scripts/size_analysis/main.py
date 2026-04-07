#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Unified launcher for size_analysis scripts.

Default behavior forwards arguments to utils_analysis_elf_size.py.

Examples:
    python scripts/size_analysis/main.py
    python scripts/size_analysis/main.py --case-set typical
    python scripts/size_analysis/main.py run-size-suite --quick
    python scripts/size_analysis/main.py size-to-doc
    python scripts/size_analysis/main.py help
"""

from __future__ import annotations

import importlib
import sys
from dataclasses import dataclass
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
LAUNCHER_PATH = Path("scripts") / "size_analysis" / "main.py"
DEFAULT_COMMAND = "utils-analysis-elf-size"
LAUNCHER_HELP_ALIASES = {"help", "launcher-help", "--launcher-help"}


@dataclass(frozen=True)
class CommandEntry:
    module_name: str
    description: str


COMMANDS = {
    "utils-analysis-elf-size": CommandEntry("utils_analysis_elf_size", "Run the default ELF size analysis (default)."),
    "size-to-doc": CommandEntry("size_to_doc", "Generate the summary size documentation from output/size_results.json."),
    "run-size-suite": CommandEntry("run_size_suite", "Refresh the dedicated HQ/canvas/widget/preset reports."),
    "hq-size-to-doc": CommandEntry("hq_size_to_doc", "Generate the HQ path size report."),
    "canvas-path-size-to-doc": CommandEntry("canvas_path_size_to_doc", "Generate the canvas path size report."),
    "canvas-feature-size-to-doc": CommandEntry("canvas_feature_size_to_doc", "Generate the canvas feature size report."),
    "widget-feature-size-to-doc": CommandEntry("widget_feature_size_to_doc", "Generate the widget feature size report."),
    "size-preset-validation-to-doc": CommandEntry("size_preset_validation_to_doc", "Generate the preset validation size report."),
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
        "size_analysis unified launcher",
        "",
        f"Default command: python {LAUNCHER_PATH} [utils_analysis_elf_size args]",
        "",
        "Available commands:",
    ]
    for command_name, entry in COMMANDS.items():
        lines.append(f"  {command_name:<31} {entry.description}")
    lines.extend(
        [
            "",
            "Examples:",
            f"  python {LAUNCHER_PATH} --case-set typical",
            f"  python {LAUNCHER_PATH} --case-set typical --doc",
            f"  python {LAUNCHER_PATH} run-size-suite --quick",
            f"  python {LAUNCHER_PATH} size-to-doc",
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
        print(f"Unknown size_analysis command: {forwarded_args[0]}", file=sys.stderr)
        print("Run `python scripts/size_analysis/main.py help` to list available commands.", file=sys.stderr)
        return 2

    return _run_command(command_name, forwarded_args)


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
"""Synchronize and build the Visual Studio PC simulator project."""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
import xml.etree.ElementTree as ET
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parents[1]
PROJECT_DIR = PROJECT_ROOT / "porting" / "pc"
SOLUTION_PATH = PROJECT_DIR / "EmbeddedGUI_PC_Simulator.sln"
PROJECT_PATH = PROJECT_DIR / "EmbeddedGUI_PC_Simulator.vcxproj"
FILTERS_PATH = PROJECT_DIR / "EmbeddedGUI_PC_Simulator.vcxproj.filters"
UPDATE_SCRIPT = SCRIPT_DIR / "update_visual_studio_sln.py"
DYNAMIC_INCLUDE_TOKENS = ("*", "$(", ";", "@(")
DEFAULT_CONFIGURATIONS = ("HelloSimple_Debug",)


def format_command(cmd: list[str]) -> str:
    return subprocess.list2cmdline([str(part) for part in cmd])


def run_command(cmd: list[str]) -> int:
    print(f"Command: {format_command(cmd)}", flush=True)
    result = subprocess.run(cmd, cwd=PROJECT_ROOT)
    return result.returncode


def run_sync(check_only: bool) -> int:
    cmd = [sys.executable, str(UPDATE_SCRIPT)]
    if check_only:
        cmd.append("--check")
    return run_command(cmd)


def validate_xml(path: Path) -> None:
    ET.parse(path)


def validate_project_xml() -> None:
    for path in (PROJECT_PATH, FILTERS_PATH):
        validate_xml(path)
        print(f"XML OK: {path.name}", flush=True)


def validate_item_includes() -> None:
    bad: list[str] = []
    for path in (PROJECT_PATH, FILTERS_PATH):
        text = path.read_text(encoding="utf-8")
        for line_number, line in enumerate(text.splitlines(), 1):
            match = re.search(r'\bInclude="([^"]*)"', line)
            if not match:
                continue
            include = match.group(1)
            if any(token in include for token in DYNAMIC_INCLUDE_TOKENS):
                bad.append(f"{path.name}:{line_number}: {include}")

    if bad:
        raise RuntimeError("Visual Studio project item Include contains unsupported wildcard/property/list syntax:\n" + "\n".join(bad))
    print("Project item Include attributes are explicit.", flush=True)


def validate_required_text() -> None:
    project_text = PROJECT_PATH.read_text(encoding="utf-8")
    required_tokens = {
        "Windows subsystem": "<SubSystem>Windows</SubSystem>",
        "main CRT entry": "<EntryPointSymbol>mainCRTStartup</EntryPointSymbol>",
        "native debugger": "<LocalDebuggerDebuggerType>NativeOnly</LocalDebuggerDebuggerType>",
        "platform printf hook": "EGUI_CONFIG_PLATFORM_CUSTOM_PRINTF=1",
        "VS debug output hook": "EGUI_PC_LOG_TO_DEBUG_OUTPUT=1",
        "default debug log level": "EGUI_PC_DEFAULT_DEBUG_LOG_LEVEL=3",
    }
    missing = [name for name, token in required_tokens.items() if token not in project_text]
    if missing:
        raise RuntimeError("Visual Studio project is missing required settings: " + ", ".join(missing))
    print("Visual Studio debugger and log settings are explicit.", flush=True)


def find_vswhere() -> Path | None:
    candidates = []
    program_files_x86 = os.environ.get("ProgramFiles(x86)")
    if program_files_x86:
        candidates.append(Path(program_files_x86) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe")
    program_files = os.environ.get("ProgramFiles")
    if program_files:
        candidates.append(Path(program_files) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe")
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    return None


def find_msbuild_with_vswhere() -> Path | None:
    vswhere = find_vswhere()
    if vswhere is None:
        return None

    cmd = [
        str(vswhere),
        "-latest",
        "-products",
        "*",
        "-requires",
        "Microsoft.Component.MSBuild",
        "-find",
        r"MSBuild\**\Bin\MSBuild.exe",
    ]
    result = subprocess.run(cmd, cwd=PROJECT_ROOT, capture_output=True, text=True)
    if result.returncode != 0:
        return None
    for line in result.stdout.splitlines():
        path = Path(line.strip())
        if path.is_file():
            return path
    return None


def find_msbuild_by_path() -> Path | None:
    for name in ("MSBuild.exe", "msbuild"):
        found = shutil.which(name)
        if found:
            return Path(found)
    return None


def find_msbuild_by_install_root() -> Path | None:
    roots = []
    for env_name in ("ProgramFiles", "ProgramFiles(x86)"):
        value = os.environ.get(env_name)
        if value:
            roots.append(Path(value) / "Microsoft Visual Studio")

    patterns = (
        r"*\*\MSBuild\Current\Bin\amd64\MSBuild.exe",
        r"*\*\MSBuild\Current\Bin\MSBuild.exe",
        r"*\*\MSBuild\*\Bin\amd64\MSBuild.exe",
        r"*\*\MSBuild\*\Bin\MSBuild.exe",
    )
    candidates: list[Path] = []
    for root in roots:
        if not root.is_dir():
            continue
        for pattern in patterns:
            candidates.extend(path for path in root.glob(pattern) if path.is_file())
    if not candidates:
        return None
    return sorted(candidates, key=lambda path: str(path).casefold(), reverse=True)[0]


def find_msbuild(explicit_path: str) -> Path | None:
    if explicit_path:
        path = Path(explicit_path)
        return path if path.is_file() else None

    env_path = os.environ.get("MSBUILD_EXE")
    if env_path:
        path = Path(env_path)
        if path.is_file():
            return path

    return find_msbuild_by_path() or find_msbuild_with_vswhere() or find_msbuild_by_install_root()


def build_with_msbuild(msbuild: Path, configurations: list[str], platform: str, target: str, verbosity: str) -> int:
    for configuration in configurations:
        cmd = [
            str(msbuild),
            str(SOLUTION_PATH),
            f"/t:{target}",
            "/m",
            f"/p:Configuration={configuration}",
            f"/p:Platform={platform}",
            f"/v:{verbosity}",
        ]
        ret = run_command(cmd)
        if ret != 0:
            return ret
    return 0


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Synchronize and build EmbeddedGUI_PC_Simulator Visual Studio project.")
    parser.add_argument("--check", action="store_true", help="check project sync without modifying .sln/.vcxproj/.filters")
    parser.add_argument("--skip-build", action="store_true", help="only synchronize and validate project files")
    parser.add_argument("--allow-missing-msbuild", action="store_true", help="skip the build if MSBuild cannot be found")
    parser.add_argument("--msbuild", default="", help="explicit MSBuild.exe path")
    parser.add_argument(
        "--configuration",
        action="append",
        default=[],
        help="Visual Studio configuration to build; can be repeated",
    )
    parser.add_argument("--platform", default="x64", help="Visual Studio platform to build")
    parser.add_argument("--target", default="Build", help="MSBuild target")
    parser.add_argument("--verbosity", default="minimal", help="MSBuild verbosity")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    configurations = args.configuration or list(DEFAULT_CONFIGURATIONS)

    ret = run_sync(check_only=args.check)
    if ret != 0:
        if args.check:
            print("Visual Studio project is out of sync. Run this script without --check to update generated project files.", file=sys.stderr)
        return ret

    try:
        validate_project_xml()
        validate_item_includes()
        validate_required_text()
    except Exception as exc:  # noqa: BLE001 - command-line tool should report concise failures.
        print(f"error: {exc}", file=sys.stderr)
        return 2

    if args.skip_build:
        print("Visual Studio project build skipped.", flush=True)
        return 0

    msbuild = find_msbuild(args.msbuild)
    if msbuild is None:
        message = "MSBuild.exe was not found; Visual Studio project build cannot run."
        if args.allow_missing_msbuild or os.name != "nt":
            print(f"Warning: {message}", flush=True)
            return 0
        print(f"error: {message}", file=sys.stderr)
        return 2

    print(f"MSBuild: {msbuild}", flush=True)
    return build_with_msbuild(msbuild, configurations, args.platform, args.target, args.verbosity)


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Build a local EmbeddedGUI Designer package with PyInstaller."""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent
SPEC_PATH = SCRIPT_DIR / "ui_designer" / "ui_designer.spec"
DIST_APP_NAME = "EmbeddedGUI-Designer"
SUPPRESSED_LOG_SNIPPETS = (
    "QFluentWidgets Pro is now released",
    "qfluentwidgets.com/pages/pro",
)


def compute_platform_tag(platform_name: str | None = None, machine_name: str | None = None) -> str:
    """Return a stable package platform tag."""
    platform_name = (platform_name or sys.platform).lower()
    detected_machine = machine_name
    if not detected_machine:
        detected_machine = os.environ.get("PROCESSOR_ARCHITECTURE", "")
    if not detected_machine and hasattr(os, "uname"):
        detected_machine = os.uname().machine
    machine_name = (detected_machine or "").lower()

    if any(token in machine_name for token in ("arm64", "aarch64")):
        arch = "arm64"
    else:
        arch = "x64"

    if platform_name.startswith("win"):
        return f"windows-{arch}"
    if platform_name == "darwin":
        return f"macos-{arch}"
    return f"linux-{arch}"


def sanitize_suffix(suffix: str) -> str:
    """Normalize an optional package suffix."""
    suffix = (suffix or "").strip()
    if not suffix:
        return ""
    suffix = suffix.replace(" ", "-")
    invalid_chars = set('/\\:*?"<>|')
    if any(ch in invalid_chars for ch in suffix):
        raise ValueError("package suffix contains invalid path characters")
    return suffix


def build_archive_base_name(platform_tag: str, package_suffix: str = "") -> str:
    """Return the base archive name without extension."""
    base = f"{DIST_APP_NAME}-{platform_tag}"
    suffix = sanitize_suffix(package_suffix)
    if suffix:
        base += f"-{suffix}"
    return base


def resolve_archive_format(archive_mode: str, platform_name: str | None = None) -> str | None:
    """Map CLI archive mode to shutil archive format."""
    archive_mode = (archive_mode or "auto").lower()
    if archive_mode == "none":
        return None
    if archive_mode == "zip":
        return "zip"
    if archive_mode == "tar.gz":
        return "gztar"
    if archive_mode == "auto":
        platform_name = (platform_name or sys.platform).lower()
        return "zip" if platform_name.startswith("win") else "gztar"
    raise ValueError(f"unsupported archive mode: {archive_mode}")


def build_pyinstaller_command(dist_dir: Path, work_dir: Path, clean: bool = True) -> list[str]:
    """Build the PyInstaller command."""
    cmd = [
        sys.executable,
        "-m",
        "PyInstaller",
        str(SPEC_PATH),
        "--distpath",
        str(dist_dir),
        "--workpath",
        str(work_dir),
        "-y",
    ]
    if clean:
        cmd.append("--clean")
    return cmd


def ensure_pyinstaller_available():
    """Raise with a clear message if PyInstaller is missing."""
    try:
        import PyInstaller  # noqa: F401
    except ImportError as exc:
        raise RuntimeError("PyInstaller is required. Run: python -m pip install pyinstaller") from exc


def should_suppress_build_output(line: str) -> bool:
    """Return True when a build log line is pure third-party promotion noise."""
    text = line or ""
    return any(snippet in text for snippet in SUPPRESSED_LOG_SNIPPETS)


def iter_filtered_build_output(lines):
    """Yield build output while stripping known third-party promotion lines."""
    suppress_blank_lines = False
    pending_blank_lines: list[str] = []
    for line in lines:
        if not line.strip():
            pending_blank_lines.append(line)
            continue
        if should_suppress_build_output(line):
            pending_blank_lines.clear()
            suppress_blank_lines = True
            continue
        if suppress_blank_lines:
            pending_blank_lines.clear()
        suppress_blank_lines = False
        if pending_blank_lines:
            for blank_line in pending_blank_lines:
                yield blank_line
            pending_blank_lines.clear()
        yield line


def run_pyinstaller(dist_dir: Path, work_dir: Path, clean: bool = True):
    """Execute the PyInstaller build."""
    cmd = build_pyinstaller_command(dist_dir, work_dir, clean=clean)
    process = subprocess.Popen(
        cmd,
        cwd=PROJECT_ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
    )

    assert process.stdout is not None
    for line in iter_filtered_build_output(process.stdout):
        print(line, end="")

    returncode = process.wait()
    if returncode != 0:
        raise subprocess.CalledProcessError(returncode, cmd)


def create_archive(dist_dir: Path, archive_dir: Path, archive_format: str, base_name: str) -> Path:
    """Archive the built Designer directory."""
    archive_dir.mkdir(parents=True, exist_ok=True)
    archive_base = archive_dir / base_name
    archive_path = shutil.make_archive(
        str(archive_base),
        archive_format,
        root_dir=str(dist_dir),
        base_dir=DIST_APP_NAME,
    )
    return Path(archive_path)


def package_ui_designer(
    *,
    output_dir: str | Path | None = None,
    work_dir: str | Path | None = None,
    archive_mode: str = "auto",
    package_suffix: str = "",
    clean: bool = True,
) -> dict[str, str]:
    """Build the Designer package and optionally archive it."""
    ensure_pyinstaller_available()

    dist_dir = Path(output_dir or (PROJECT_ROOT / "dist")).resolve()
    build_dir = Path(work_dir or (PROJECT_ROOT / "build" / "pyinstaller")).resolve()

    run_pyinstaller(dist_dir, build_dir, clean=clean)

    app_dir = dist_dir / DIST_APP_NAME
    if not app_dir.is_dir():
        raise FileNotFoundError(f"PyInstaller output missing: {app_dir}")

    archive_format = resolve_archive_format(archive_mode)
    archive_path = None
    if archive_format is not None:
        platform_tag = compute_platform_tag()
        base_name = build_archive_base_name(platform_tag, package_suffix=package_suffix)
        archive_path = create_archive(dist_dir, dist_dir, archive_format, base_name)

    return {
        "dist_dir": str(dist_dir),
        "app_dir": str(app_dir),
        "archive_path": str(archive_path) if archive_path else "",
    }


def parse_args():
    parser = argparse.ArgumentParser(description="Build a local EmbeddedGUI Designer package")
    parser.add_argument(
        "--output-dir",
        default=str(PROJECT_ROOT / "dist"),
        help="PyInstaller dist output directory (default: %(default)s)",
    )
    parser.add_argument(
        "--work-dir",
        default=str(PROJECT_ROOT / "build" / "pyinstaller"),
        help="PyInstaller work directory (default: %(default)s)",
    )
    parser.add_argument(
        "--archive",
        choices=["auto", "zip", "tar.gz", "none"],
        default="auto",
        help="Archive mode for the built package (default: %(default)s)",
    )
    parser.add_argument(
        "--package-suffix",
        default="",
        help="Optional suffix appended to the archive name, such as a version tag",
    )
    parser.add_argument(
        "--no-clean",
        action="store_true",
        help="Do not pass --clean to PyInstaller",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    try:
        result = package_ui_designer(
            output_dir=args.output_dir,
            work_dir=args.work_dir,
            archive_mode=args.archive,
            package_suffix=args.package_suffix,
            clean=not args.no_clean,
        )
    except Exception as exc:
        print(f"[FAIL] {exc}")
        sys.exit(1)

    print(f"[OK] app_dir: {result['app_dir']}")
    if result["archive_path"]:
        print(f"[OK] archive: {result['archive_path']}")
    else:
        print("[OK] archive: skipped")


if __name__ == "__main__":
    main()

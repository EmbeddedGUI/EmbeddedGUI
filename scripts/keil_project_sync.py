#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Synchronize Keil .uvprojx project file with actual source files.

Scans configured directories for .c files and updates corresponding groups
in the Keil project file to match. Adds missing files and removes stale entries.
Uses string-level replacement to preserve original XML formatting.

Usage:
    python scripts/keil_project_sync.py
    python scripts/keil_project_sync.py --check   # dry-run, exit 1 if out of sync
"""

import re
import sys
import argparse
from pathlib import Path

SCRIPT_DIR = Path(__file__).parent
PROJECT_ROOT = SCRIPT_DIR.parent

# Keil project file path
UVPROJX_PATH = PROJECT_ROOT / "porting" / "stm32g0" / "MDK-ARM" / "proj_stm32g0.uvprojx"

# Template for a single <File> entry (matches original indentation: 12 spaces)
FILE_ENTRY_TEMPLATE = """\
            <File>
              <FileName>{filename}</FileName>
              <FileType>1</FileType>
              <FilePath>{filepath}</FilePath>
            </File>"""

# ── Group sync definitions ───────────────────────────────────────────────────
# Each group: (group_name, scan_dirs, keil_path_prefix, scan_base_dir)
#   - group_name: <GroupName> in .uvprojx
#   - scan_dirs: list of subdirectories to scan (relative to scan_base_dir),
#                or None to scan scan_base_dir directly (non-recursive)
#   - keil_path_prefix: path prefix in .uvprojx FilePath (relative from MDK-ARM)
#   - scan_base_dir: base directory for scanning (relative from PROJECT_ROOT)

SYNC_GROUPS = [
    {
        "group_name": "EmbeddedGUI",
        "scan_base_dir": "src",
        "scan_subdirs": [
            "anim", "app", "background", "core", "font", "image",
            "mask", "resource", "shadow", "style", "utils", "widget",
        ],
        "keil_prefix": r"..\..\..\src",
        "recursive": True,
    },
    {
        "group_name": "Porting",
        "scan_base_dir": "porting/stm32g0/Porting",
        "scan_subdirs": None,  # scan base dir directly
        "keil_prefix": r"..\Porting",
        "recursive": False,
    },
]


def scan_files(scan_base_dir, scan_subdirs, recursive):
    """Scan directories for .c files, return set of posix paths relative to scan_base_dir."""
    base = PROJECT_ROOT / scan_base_dir
    files = set()

    if scan_subdirs is None:
        # Scan base dir directly
        glob_method = base.rglob if recursive else base.glob
        for c_file in glob_method("*.c"):
            rel = c_file.relative_to(base)
            files.add(rel.as_posix())
    else:
        for subdir in scan_subdirs:
            subdir_path = base / subdir
            if not subdir_path.exists():
                continue
            for c_file in subdir_path.rglob("*.c"):
                rel = c_file.relative_to(base)
                files.add(rel.as_posix())

    return files


def rel_to_keil_path(keil_prefix, rel_posix):
    """Convert a relative posix path to Keil FilePath format."""
    return keil_prefix + "\\" + rel_posix.replace("/", "\\")


def keil_path_to_rel(keil_prefix, keil_path):
    """Convert a Keil FilePath back to relative posix path. Returns None if prefix doesn't match."""
    normalized = keil_path.replace("\\", "/")
    prefix_normalized = keil_prefix.replace("\\", "/")
    if normalized.startswith(prefix_normalized + "/"):
        return normalized[len(prefix_normalized) + 1:]
    return None


def extract_group_files(content, group_name, keil_prefix):
    """Extract relative file paths from a named group in .uvprojx content."""
    group_pattern = rf'<GroupName>{re.escape(group_name)}</GroupName>\s*<Files>(.*?)</Files>'
    match = re.search(group_pattern, content, re.DOTALL)
    if not match:
        return None

    files_content = match.group(1)
    existing = set()
    for fp_match in re.finditer(r'<FilePath>(.*?)</FilePath>', files_content):
        rel = keil_path_to_rel(keil_prefix, fp_match.group(1))
        if rel is not None:
            existing.add(rel)

    return existing


def build_files_block(files_sorted, keil_prefix):
    """Build the <File> entries block."""
    entries = []
    for rel in files_sorted:
        filename = Path(rel).name
        filepath = rel_to_keil_path(keil_prefix, rel)
        entries.append(FILE_ENTRY_TEMPLATE.format(filename=filename, filepath=filepath))
    return "\n".join(entries)


def replace_group_files(content, group_name, new_block):
    """Replace the <Files>...</Files> section of a named group."""
    group_pattern = rf'(<GroupName>{re.escape(group_name)}</GroupName>\s*<Files>)\s*.*?\s*(</Files>)'
    match = re.search(group_pattern, content, re.DOTALL)
    if not match:
        print(f"Error: could not locate {group_name} <Files> block for replacement")
        return None
    start, end = match.start(), match.end()
    new_section = match.group(1) + "\n" + new_block + "\n          " + match.group(2)
    return content[:start] + new_section + content[end:]


def sync_keil_project(check_only=False):
    """Synchronize Keil project file with source directories.

    Returns:
        0 if already in sync
        1 if changes were needed (and applied if not check_only)
    """
    if not UVPROJX_PATH.exists():
        print(f"Warning: Keil project file not found: {UVPROJX_PATH}")
        print("Skipping Keil project sync.")
        return 0

    content = UVPROJX_PATH.read_text(encoding="utf-8")
    has_diff = False
    total_missing = 0
    total_stale = 0

    for group_cfg in SYNC_GROUPS:
        group_name = group_cfg["group_name"]
        keil_prefix = group_cfg["keil_prefix"]

        # Scan actual files
        actual = scan_files(
            group_cfg["scan_base_dir"],
            group_cfg["scan_subdirs"],
            group_cfg["recursive"],
        )

        # Extract existing files from Keil project
        existing = extract_group_files(content, group_name, keil_prefix)
        if existing is None:
            print(f"Warning: Group '{group_name}' not found in .uvprojx, skipping")
            continue

        # Compute differences
        missing = sorted(actual - existing)
        stale = sorted(existing - actual)

        if not missing and not stale:
            print(f"[{group_name}] in sync. ({len(existing)} files)")
            continue

        has_diff = True
        total_missing += len(missing)
        total_stale += len(stale)

        if missing:
            print(f"\n[{group_name}] Missing ({len(missing)} files):")
            for f in missing:
                print(f"  + {f}")

        if stale:
            print(f"\n[{group_name}] Stale ({len(stale)} files):")
            for f in stale:
                print(f"  - {f}")

        if not check_only:
            all_sorted = sorted(actual, key=lambda f: rel_to_keil_path(keil_prefix, f))
            new_block = build_files_block(all_sorted, keil_prefix)
            new_content = replace_group_files(content, group_name, new_block)
            if new_content is None:
                return 1
            content = new_content

    if not has_diff:
        return 0

    if check_only:
        print(f"\nKeil project is OUT OF SYNC. "
              f"({total_missing} missing, {total_stale} stale)")
        return 1

    UVPROJX_PATH.write_text(content, encoding="utf-8")
    print(f"\nKeil project updated. "
          f"({total_missing} added, {total_stale} removed)")
    return 0


def main():
    parser = argparse.ArgumentParser(
        description="Synchronize Keil .uvprojx with source files.")
    parser.add_argument(
        "--check",
        action="store_true",
        help="Dry-run mode: report differences without modifying the file. "
             "Exit code 1 if out of sync.")
    args = parser.parse_args()

    ret = sync_keil_project(check_only=args.check)
    sys.exit(ret)


if __name__ == "__main__":
    main()

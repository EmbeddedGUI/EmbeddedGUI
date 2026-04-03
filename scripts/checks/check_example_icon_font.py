#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Check that example source files explicitly set icon fonts when using icon-capable widgets.

This prevents example demos from silently falling back to widget-internal icon font inference.
By default the checker scans git-tracked example sources only, so local untracked
work-in-progress directories do not break CI-style checks.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
SCRIPTS_ROOT = SCRIPT_DIR.parent
PROJECT_ROOT = SCRIPTS_ROOT.parent
DEFAULT_SCAN_ROOT = PROJECT_ROOT / "example"


@dataclass(frozen=True)
class DirectRule:
    name: str
    icon_pattern: str
    font_pattern: str


@dataclass(frozen=True)
class MaxRule:
    name: str
    icon_patterns: tuple[str, ...]
    font_pattern: str


@dataclass(frozen=True)
class PresenceRule:
    name: str
    icon_pattern: str
    font_pattern: str


DIRECT_RULES = (
    DirectRule("button", r"egui_view_button_set_icon\s*\(", r"egui_view_button_set_icon_font\s*\("),
    DirectRule("image_button", r"egui_view_image_button_set_icon\s*\(", r"egui_view_image_button_set_icon_font\s*\("),
    DirectRule("toggle_button", r"egui_view_toggle_button_set_icon\s*\(", r"egui_view_toggle_button_set_icon_font\s*\("),
    DirectRule("switch", r"egui_view_switch_set_state_icons\s*\(", r"egui_view_switch_set_icon_font\s*\("),
    DirectRule("number_picker", r"egui_view_number_picker_set_button_icons\s*\(", r"egui_view_number_picker_set_icon_font\s*\("),
    DirectRule("menu", r"egui_view_menu_set_navigation_icons\s*\(", r"egui_view_menu_set_icon_font\s*\("),
    DirectRule("button_matrix", r"egui_view_button_matrix_set_icons\s*\(", r"egui_view_button_matrix_set_icon_font\s*\("),
    DirectRule("chips", r"egui_view_chips_set_chip_icons\s*\(", r"egui_view_chips_set_icon_font\s*\("),
    DirectRule("segmented_control", r"egui_view_segmented_control_set_segment_icons\s*\(", r"egui_view_segmented_control_set_icon_font\s*\("),
    DirectRule("notification_badge", r"egui_view_notification_badge_set_icon\s*\(", r"egui_view_notification_badge_set_icon_font\s*\("),
    DirectRule("keyboard", r"egui_view_keyboard_set_special_key_icons\s*\(", r"egui_view_keyboard_set_icon_font\s*\("),
    DirectRule("window", r"egui_view_window_set_close_icon\s*\(", r"egui_view_window_set_close_icon_font\s*\("),
    DirectRule("stepper", r"egui_view_stepper_set_completed_icon\s*\(", r"egui_view_stepper_set_icon_font\s*\("),
    DirectRule("tab_bar", r"egui_view_tab_bar_set_tab_icons\s*\(", r"egui_view_tab_bar_set_icon_font\s*\("),
    DirectRule("page_indicator", r"egui_view_page_indicator_set_icons\s*\(", r"egui_view_page_indicator_set_icon_font\s*\("),
)

MAX_RULES = (
    MaxRule(
        "combobox",
        (r"egui_view_combobox_set_item_icons\s*\(", r"egui_view_combobox_set_arrow_icons\s*\("),
        r"egui_view_combobox_set_icon_font\s*\(",
    ),
    MaxRule(
        "checkbox",
        (r"egui_view_checkbox_set_mark_icon\s*\(", r"EGUI_VIEW_CHECKBOX_MARK_STYLE_ICON"),
        r"egui_view_checkbox_set_icon_font\s*\(",
    ),
    MaxRule(
        "radio_button",
        (r"egui_view_radio_button_set_mark_icon\s*\(", r"EGUI_VIEW_RADIO_BUTTON_MARK_STYLE_ICON"),
        r"egui_view_radio_button_set_icon_font\s*\(",
    ),
)

PRESENCE_RULES = (
    PresenceRule("list", r"egui_view_list_add_item_with_icon\s*\(", r"egui_view_list_set_icon_font\s*\("),
)


def count_matches(text: str, pattern: str) -> int:
    return len(re.findall(pattern, text))


def iter_source_files(scan_root: Path) -> list[Path]:
    tracked_files = iter_tracked_source_files(scan_root)
    if tracked_files is not None:
        return tracked_files

    return iter_source_files_from_fs(scan_root)


def iter_source_files_from_fs(scan_root: Path) -> list[Path]:
    files = []
    for path in scan_root.rglob("*.c"):
        if any(part == "resource" for part in path.parts):
            continue
        files.append(path)
    return sorted(files)


def iter_tracked_source_files(scan_root: Path) -> list[Path] | None:
    try:
        relative_root = scan_root.relative_to(PROJECT_ROOT)
    except ValueError:
        return None

    cmd = ["git", "-C", str(PROJECT_ROOT), "ls-files", "--", relative_root.as_posix()]
    result = subprocess.run(cmd, capture_output=True, text=True, check=False)
    if result.returncode != 0:
        return None

    files = []
    for line in result.stdout.splitlines():
        relative_path = line.strip()
        if not relative_path.endswith(".c"):
            continue

        path = (PROJECT_ROOT / relative_path).resolve()
        if not path.is_file():
            continue
        if not path.is_relative_to(scan_root):
            continue
        if any(part == "resource" for part in path.parts):
            continue

        files.append(path)

    return sorted(files)


def check_file(path: Path) -> list[str]:
    issues: list[str] = []
    text = path.read_text(encoding="utf-8")
    relative_path = path.relative_to(PROJECT_ROOT).as_posix()

    for rule in DIRECT_RULES:
        icon_count = count_matches(text, rule.icon_pattern)
        if icon_count == 0:
            continue
        font_count = count_matches(text, rule.font_pattern)
        if font_count < icon_count:
            issues.append(f"{relative_path}: {rule.name} icon setters={icon_count}, icon font setters={font_count}")

    for rule in MAX_RULES:
        icon_count = max(count_matches(text, pattern) for pattern in rule.icon_patterns)
        if icon_count == 0:
            continue
        font_count = count_matches(text, rule.font_pattern)
        if font_count < icon_count:
            issues.append(f"{relative_path}: {rule.name} icon setters={icon_count}, icon font setters={font_count}")

    for rule in PRESENCE_RULES:
        icon_count = count_matches(text, rule.icon_pattern)
        if icon_count == 0:
            continue
        font_count = count_matches(text, rule.font_pattern)
        if font_count == 0:
            issues.append(f"{relative_path}: {rule.name} uses icons but has no explicit icon font setter")

    return issues


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Check example icon font usage in git-tracked sources by default.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=(
            "Examples:\n"
            "  python scripts/checks/check_example_icon_font.py\n"
            "  python scripts/checks/check_example_icon_font.py --include-untracked\n"
            "  python scripts/checks/check_example_icon_font.py --root example/HelloBasic\n"
        ),
    )
    parser.add_argument(
        "--root",
        type=str,
        default=str(DEFAULT_SCAN_ROOT),
        help="Root directory to scan (default: example).",
    )
    parser.add_argument(
        "--include-untracked",
        action="store_true",
        help="Scan all files from the filesystem instead of only tracked git files.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    scan_root = Path(args.root).resolve()
    if not scan_root.exists():
        print(f"Scan root does not exist: {scan_root}")
        return 1

    issues: list[str] = []
    if args.include_untracked:
        source_files = iter_source_files_from_fs(scan_root)
    else:
        source_files = iter_source_files(scan_root)

    for path in source_files:
        issues.extend(check_file(path))

    if issues:
        print("Example icon font check FAILED:")
        for issue in issues:
            print(f"  - {issue}")
        return 1

    print("Example icon font check PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())

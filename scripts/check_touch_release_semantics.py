"""Audit widget touch release semantics."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parent
CUSTOM_WIDGET_ROOT = REPO_ROOT / "example" / "HelloCustomWidgets"
CORE_WIDGET_ROOT = REPO_ROOT / "src" / "widget"

ALLOWLIST = {
    "custom": {
        "input/rating_control/egui_view_rating_control.c": "rating control supports drag/hover preview semantics",
        "input/color_picker/egui_view_color_picker.c": "color picker updates continuously while dragging",
        "input/scroll_bar/egui_view_scroll_bar.c": "scroll bar thumb is a drag interaction",
        "input/swipe_control/egui_view_swipe_control.c": "swipe control is a continuous gesture widget",
        "navigation/annotated_scroll_bar/egui_view_annotated_scroll_bar.c": "annotated scroll bar uses drag semantics",
    },
    "core": {},
}

TOUCH_HANDLER_RE = re.compile(r"static\s+int\s+\w+_on_touch_event\s*\([^)]*\)\s*\{", re.MULTILINE)
PRESSED_ASSIGN_RE = re.compile(r"\blocal->(pressed_[A-Za-z0-9_]+)\s*(?<![=!<>])=(?!=)")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Check widget touch handlers for release-target drift.")
    parser.add_argument("--scope", choices=("custom", "core", "all"), default="all", help="Audit custom widgets, core widgets, or both.")
    parser.add_argument("--category", type=str, default=None, help="Only audit a specific HelloCustomWidgets category.")
    return parser.parse_args()


def iter_custom_widget_files(category: str | None) -> list[Path]:
    if not CUSTOM_WIDGET_ROOT.is_dir():
        return []

    if category is None:
        category_dirs = sorted(path for path in CUSTOM_WIDGET_ROOT.iterdir() if path.is_dir())
    else:
        category_dir = CUSTOM_WIDGET_ROOT / category
        category_dirs = [category_dir] if category_dir.is_dir() else []

    result = []
    for category_dir in category_dirs:
        for source in sorted(category_dir.glob("*/egui_view_*.c")):
            result.append(source)
    return result


def iter_core_widget_files() -> list[Path]:
    if not CORE_WIDGET_ROOT.is_dir():
        return []
    return sorted(CORE_WIDGET_ROOT.glob("egui_view_*.c"))


def get_relative_widget_path(path: Path, scope: str) -> str:
    if scope == "custom":
        return path.relative_to(CUSTOM_WIDGET_ROOT).as_posix()
    if scope == "core":
        return path.relative_to(REPO_ROOT).as_posix()
    return path.relative_to(CUSTOM_WIDGET_ROOT).as_posix()


def extract_function_body(text: str, match: re.Match[str]) -> tuple[int, int, str]:
    brace_start = text.find("{", match.start())
    if brace_start < 0:
        raise ValueError("touch handler missing opening brace")

    depth = 0
    index = brace_start
    while index < len(text):
        char = text[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return brace_start, index, text[brace_start + 1:index]
        index += 1

    raise ValueError("touch handler missing closing brace")


def extract_top_level_cases(function_body: str, function_start_line: int) -> dict[str, tuple[int, str]]:
    cases: dict[str, tuple[int, str]] = {}
    current_label: str | None = None
    current_lines: list[str] = []
    current_start_line = function_start_line + 1
    depth = 1

    for offset, line in enumerate(function_body.splitlines(), start=1):
        stripped = line.lstrip()
        is_case_label = depth == 2 and (stripped.startswith("case ") or stripped.startswith("default:"))
        if is_case_label:
            if current_label is not None:
                cases[current_label] = (current_start_line, "\n".join(current_lines))
            current_label = stripped.split(":", 1)[0]
            current_start_line = function_start_line + offset
            current_lines = [line]
        elif current_label is not None:
            current_lines.append(line)
        depth += line.count("{") - line.count("}")

    if current_label is not None:
        cases[current_label] = (current_start_line, "\n".join(current_lines))

    return cases


def find_move_assignments(path: Path) -> list[tuple[int, str, str]]:
    text = path.read_text(encoding="utf-8")
    match = TOUCH_HANDLER_RE.search(text)
    if match is None:
        return []

    brace_start, _, function_body = extract_function_body(text, match)
    function_start_line = text.count("\n", 0, brace_start) + 1
    cases = extract_top_level_cases(function_body, function_start_line)
    move_case = cases.get("case EGUI_MOTION_EVENT_ACTION_MOVE")
    if move_case is None:
        return []

    move_start_line, move_body = move_case
    violations = []
    for offset, line in enumerate(move_body.splitlines(), start=0):
        match_assign = PRESSED_ASSIGN_RE.search(line)
        if match_assign is None:
            continue
        violations.append((move_start_line + offset, match_assign.group(1), line.strip()))

    return violations


def main() -> int:
    args = parse_args()
    sources: list[tuple[str, Path]] = []

    if args.scope in ("custom", "all"):
        sources.extend(("custom", path) for path in iter_custom_widget_files(args.category))
    if args.scope in ("core", "all"):
        sources.extend(("core", path) for path in iter_core_widget_files())

    if not sources:
        category_info = args.category if args.category is not None else "all"
        print(f"[touch-release-audit] no widget sources found for scope={args.scope}, category={category_info}")
        return 0

    audited = {"custom": 0, "core": 0}
    skipped = {"custom": 0, "core": 0}
    failures: list[tuple[str, str, int, str, str]] = []

    for scope, source in sources:
        relative_path = get_relative_widget_path(source, scope)
        if relative_path in ALLOWLIST[scope]:
            skipped[scope] += 1
            continue

        audited[scope] += 1
        for line_number, field_name, line_text in find_move_assignments(source):
            failures.append((scope, relative_path, line_number, field_name, line_text))

    if failures:
        print("[touch-release-audit] FAIL")
        print("ACTION_MOVE must not rewrite pressed_* state for non-drag widgets.")
        print("If a widget intentionally supports drag or continuous interaction, add it to the matching allowlist with rationale.")
        for scope, relative_path, line_number, field_name, line_text in failures:
            print(f"  [{scope}] {relative_path}:{line_number}: rewrites {field_name} in ACTION_MOVE")
            print(f"    {line_text}")
        return 1

    print(
        "[touch-release-audit] PASS "
        f"custom_audited={audited['custom']} custom_skipped_allowlist={skipped['custom']} "
        f"core_audited={audited['core']} core_skipped_allowlist={skipped['core']}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())

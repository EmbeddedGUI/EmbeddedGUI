#!/usr/bin/env python3
"""Gate checker for widget readme rationale and differentiation sections."""

import argparse
import json
import re
from datetime import datetime
from pathlib import Path


RUNTIME_ROOT = Path("runtime_check_output")
CJK_PATTERN = re.compile(r"[\u4e00-\u9fff]")
MIN_CHINESE_CHARS = 20

WHY_KEYWORDS = (
    "why this widget",
    "design motivation",
    "problem value",
    "design rationale",
    "why this control",
    "why this component",
    "为什么设计这个控件",
    "为什么要这个控件",
    "设计动机",
    "设计价值",
    "必要性",
)

WHY_NOT_KEYWORDS = (
    "why not existing",
    "why not",
    "not existing widgets",
    "alternative",
    "substitute",
    "why existing widgets are not enough",
    "为什么不用现有控件",
    "为什么不直接用现有控件",
    "不可替代",
    "替代方案不足",
)

DIFF_KEYWORDS = (
    "differentiation",
    "difference",
    "boundary",
    "overlap",
    "差异化",
    "边界",
    "重叠",
    "重合",
    "功能边界",
)


def normalize_widget_name(name: str) -> str:
    return name.strip().lower().replace("-", "_").replace(" ", "_")


def read_utf8_text(path: Path) -> tuple[str, bool]:
    data = path.read_bytes()
    try:
        return data.decode("utf-8"), True
    except UnicodeDecodeError:
        return "", False


def count_chinese_chars(text: str) -> int:
    return len(CJK_PATTERN.findall(text))


def contains_any(text: str, keywords: tuple[str, ...]) -> bool:
    lower_text = text.lower()
    for keyword in keywords:
        if keyword in lower_text:
            return True
    return False


def collect_widget_names() -> list[str]:
    names = []
    widget_dir = Path("src/widget")
    for header in widget_dir.glob("egui_view_*.h"):
        name = header.stem.replace("egui_view_", "", 1)
        if name:
            names.append(name.lower())
    names = sorted(set(names), key=lambda x: len(x), reverse=True)
    return names


def detect_wrapped_widgets(source_text: str, widget: str, widget_names: list[str]) -> list[str]:
    pattern = re.compile(r"\begui_view_([a-z0-9_]+)\s*\(")
    wrapped = set()
    for symbol in pattern.findall(source_text.lower()):
        if symbol.startswith(widget + "_"):
            continue
        matched_widget = None
        for candidate in widget_names:
            if symbol.startswith(candidate + "_"):
                matched_widget = candidate
                break
        if matched_widget and matched_widget != widget:
            wrapped.add(matched_widget)
    return sorted(wrapped)


def evaluate_readme(widget: str, readme_text: str, wrapped_widgets: list[str], utf8_ok: bool) -> dict:
    missing = []
    chinese_char_count = count_chinese_chars(readme_text)
    has_chinese_content = chinese_char_count >= MIN_CHINESE_CHARS

    if not utf8_ok:
        missing.append("encoding_not_utf8")
    if utf8_ok and not has_chinese_content:
        missing.append("readme_not_chinese")

    has_why = contains_any(readme_text, WHY_KEYWORDS)
    has_why_not = contains_any(readme_text, WHY_NOT_KEYWORDS)
    has_diff = contains_any(readme_text, DIFF_KEYWORDS)

    if not has_why:
        missing.append("why_this_widget")
    if not has_why_not:
        missing.append("why_not_existing_widgets")
    if not has_diff:
        missing.append("differentiation_boundary")

    missing_wrapper_refs = []
    lower_text = readme_text.lower()
    for wrapped in wrapped_widgets:
        if wrapped not in lower_text:
            missing_wrapper_refs.append(wrapped)

    if missing_wrapper_refs:
        missing.append("wrapped_widget_reference")

    passed = len(missing) == 0
    return {
        "passed": passed,
        "missing_items": missing,
        "utf8_ok": utf8_ok,
        "has_chinese_content": has_chinese_content,
        "chinese_char_count": chinese_char_count,
        "min_chinese_chars": MIN_CHINESE_CHARS,
        "has_why_section": has_why,
        "has_why_not_section": has_why_not,
        "has_differentiation_section": has_diff,
        "wrapped_widgets": wrapped_widgets,
        "missing_wrapped_widget_refs": missing_wrapper_refs,
    }


def write_report(report_path: Path, widget: str, readme_path: Path, result: dict) -> None:
    payload = {
        "generated_at": datetime.now().isoformat(timespec="seconds"),
        "widget": widget,
        "readme_path": str(readme_path).replace("\\", "/"),
        "result": result,
    }
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate widget readme rationale sections")
    parser.add_argument("--widget", required=True, help="Widget name")
    parser.add_argument("--readme", default="", help="Readme path (default: example/HelloBasic/<widget>/readme.md)")
    parser.add_argument(
        "--report",
        default="",
        help="Report path (default: runtime_check_output/HelloBasic_<widget>/readme_review_report.json)",
    )
    args = parser.parse_args()

    widget = normalize_widget_name(args.widget)
    readme_path = Path(args.readme) if args.readme.strip() else Path("example/HelloBasic") / widget / "readme.md"
    report_path = (
        Path(args.report)
        if args.report.strip()
        else RUNTIME_ROOT / f"HelloBasic_{widget}" / "readme_review_report.json"
    )

    if not readme_path.exists():
        result = {"passed": False, "missing_items": ["readme_missing"]}
        write_report(report_path, widget, readme_path, result)
        print(f"[INFO] Readme review report: {report_path}")
        print(f"[FAIL] readme file missing: {readme_path}")
        return 1

    readme_text, utf8_ok = read_utf8_text(readme_path)
    if not utf8_ok:
        result = evaluate_readme(widget, "", [], utf8_ok=False)
        write_report(report_path, widget, readme_path, result)
        print(f"[INFO] Readme review report: {report_path}")
        print("[FAIL] readme rationale gate failed.")
        print("Missing items: encoding_not_utf8")
        return 1

    source_path = Path("src/widget") / f"egui_view_{widget}.c"
    wrapped_widgets = []
    if source_path.exists():
        wrapped_widgets = detect_wrapped_widgets(source_path.read_text(encoding="utf-8"), widget, collect_widget_names())

    result = evaluate_readme(widget, readme_text, wrapped_widgets, utf8_ok=True)
    write_report(report_path, widget, readme_path, result)
    print(f"[INFO] Readme review report: {report_path}")

    if result["passed"]:
        print("[PASS] readme rationale gate passed.")
        return 0

    print("[FAIL] readme rationale gate failed.")
    if result["missing_items"]:
        print("Missing items: " + ", ".join(result["missing_items"]))
    if result["missing_wrapped_widget_refs"]:
        print("Missing wrapped widget refs: " + ", ".join(result["missing_wrapped_widget_refs"]))
    return 1


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Check demo/catalog consistency across filesystem, web manifest, and docs.

This script derives counts from the real example directories, then verifies:
1. The curated HelloCustomWidgets list in wasm_build_demos.py is still valid.
2. web/demos/demos.json matches the default full-site publishing layout.
3. Key documentation files use the same live counts and publishing policy.
"""

from __future__ import annotations

import argparse
import importlib.util
import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
SCRIPTS_ROOT = SCRIPT_DIR.parent
PROJECT_ROOT = SCRIPTS_ROOT.parent
WASM_BUILD_PATH = SCRIPTS_ROOT / "web" / "wasm_build_demos.py"


@dataclass(frozen=True)
class RegexExpectation:
    path: Path
    label: str
    pattern: str


def load_wasm_build_module():
    spec = importlib.util.spec_from_file_location("embeddedgui_wasm_build_demos", WASM_BUILD_PATH)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Unable to load module from {WASM_BUILD_PATH}")

    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def collect_live_counts(module) -> dict[str, int]:
    return {
        "basic": len(module.get_example_basic_list()),
        "virtual": len(module.get_example_virtual_list()),
        "custom_total": len(module.get_custom_widgets_list()),
        "custom_curated": len(module.DEFAULT_CUSTOM_WIDGET_DEMOS),
    }


def build_expected_demo_sets(module) -> dict[str, set[str]]:
    standalone_apps = {
        app
        for app in module.get_example_list()
        if app not in {"HelloBasic", "HelloVirtual", "HelloCustomWidgets"} | set(module.WASM_SKIP_APPS)
    }
    basic = {module.format_demo_name("HelloBasic", sub) for sub in module.get_example_basic_list()}
    virtual = {module.format_demo_name("HelloVirtual", sub) for sub in module.get_example_virtual_list()}
    custom = {module.format_demo_name("HelloCustomWidgets", app_sub) for app_sub in module.DEFAULT_CUSTOM_WIDGET_DEMOS}
    standalone = standalone_apps | virtual
    return {
        "HelloBasic": basic,
        "HelloCustomWidgets": custom,
        "Standalone": standalone,
    }


def check_curated_custom_widget_sources(module) -> list[str]:
    issues: list[str] = []
    available = {f"{cat}/{widget}" for cat, widget in module.get_custom_widgets_list()}
    for app_sub in module.DEFAULT_CUSTOM_WIDGET_DEMOS:
        if app_sub not in available:
            issues.append(f"Missing curated HelloCustomWidgets demo source: example/HelloCustomWidgets/{app_sub}")
    return issues


def check_demos_json(module, expected_sets: dict[str, set[str]], require_manifest: bool) -> tuple[list[str], bool]:
    issues: list[str] = []
    demos_path = PROJECT_ROOT / "web" / "demos" / "demos.json"
    if not demos_path.exists():
        if require_manifest:
            return [f"Missing generated manifest: {demos_path.relative_to(PROJECT_ROOT).as_posix()}"], False
        return [], True

    demos = json.loads(demos_path.read_text(encoding="utf-8"))
    names = [demo.get("name", "") for demo in demos]
    duplicate_names = sorted({name for name in names if names.count(name) > 1 and name})
    if duplicate_names:
        issues.append("Duplicate demos.json names: " + ", ".join(duplicate_names))

    actual_sets = {
        "HelloBasic": {demo["name"] for demo in demos if demo.get("category") == "HelloBasic"},
        "HelloCustomWidgets": {demo["name"] for demo in demos if demo.get("category") == "HelloCustomWidgets"},
        "Standalone": {demo["name"] for demo in demos if demo.get("category") == "Standalone"},
    }

    for category, expected in expected_sets.items():
        actual = actual_sets[category]
        missing = sorted(expected - actual)
        extra = sorted(actual - expected)
        if missing:
            issues.append(f"demos.json missing {category} entries: {', '.join(missing)}")
        if extra:
            issues.append(f"demos.json has unexpected {category} entries: {', '.join(extra)}")

    expected_total = sum(len(items) for items in expected_sets.values())
    if len(demos) != expected_total:
        issues.append(f"demos.json total mismatch: expected {expected_total}, got {len(demos)}")

    return issues, False


def build_doc_expectations(counts: dict[str, int]) -> list[RegexExpectation]:
    basic = counts["basic"]
    virtual = counts["virtual"]
    custom_total = counts["custom_total"]
    custom_curated = counts["custom_curated"]

    return [
        RegexExpectation(
            PROJECT_ROOT / "README.md",
            "README HelloBasic heading count",
            rf"(?m)^## .*HelloBasic.*\b{basic}\b.*$",
        ),
        RegexExpectation(
            PROJECT_ROOT / "README.md",
            "README size note counts",
            rf"(?m)^> .*HelloBasic.*\b{basic}\b.*HelloVirtual.*\b{virtual}\b.*$",
        ),
        RegexExpectation(
            PROJECT_ROOT / "README.md",
            "README example table HelloBasic count",
            rf"(?m)^\| `HelloBasic` \| \*\*.*\b{basic}\b.*\*\* \|$",
        ),
        RegexExpectation(
            PROJECT_ROOT / "doc" / "source" / "getting_started" / "build_system.md",
            "build_system APP table HelloBasic count",
            rf"(?m)^\| `HelloBasic` \| .*?\b{basic}\b.*`APP_SUB`.*\|$",
        ),
        RegexExpectation(
            PROJECT_ROOT / "doc" / "source" / "getting_started" / "build_system.md",
            "build_system APP_SUB HelloBasic count",
            rf"(?m)^- `HelloBasic`: .*\b{basic}\b.*$",
        ),
        RegexExpectation(
            PROJECT_ROOT / "doc" / "source" / "getting_started" / "project_structure.md",
            "project_structure HelloBasic count",
            rf"(?m)^\| `HelloBasic` \| .*?\b{basic}\b.*\|$",
        ),
        RegexExpectation(
            PROJECT_ROOT / "doc" / "source" / "porting" / "porting_wasm.md",
            "porting_wasm curated HelloCustomWidgets note",
            rf"(?m)^1\. .*HelloUnitTest.*HelloCustomWidgets.*\b{custom_curated}\b.*$",
        ),
        RegexExpectation(
            PROJECT_ROOT / "example" / "HelloCustomWidgets" / "HelloCustomWidgets_plan.md",
            "HelloCustomWidgets plan live count",
            rf"(?m)^- .*当前目录总数 `{custom_total}`.*$",
        ),
        RegexExpectation(
            PROJECT_ROOT / "example" / "HelloCustomWidgets" / "HelloCustomWidgets_plan.md",
            "HelloCustomWidgets plan tracker reference",
            r"(?m)^\d+\. .*widget_progress_tracker\.md.*$",
        ),
    ]


def check_docs(expectations: list[RegexExpectation]) -> list[str]:
    issues: list[str] = []
    cache: dict[Path, str] = {}

    for expectation in expectations:
        text = cache.get(expectation.path)
        if text is None:
            if not expectation.path.exists():
                issues.append(f"Missing documentation file: {expectation.path.relative_to(PROJECT_ROOT).as_posix()}")
                continue
            text = expectation.path.read_text(encoding="utf-8")
            cache[expectation.path] = text

        if not re.search(expectation.pattern, text):
            relative = expectation.path.relative_to(PROJECT_ROOT).as_posix()
            issues.append(f"{relative}: {expectation.label} does not match live data")

    plan_path = PROJECT_ROOT / "example" / "HelloCustomWidgets" / "HelloCustomWidgets_plan.md"
    if plan_path.exists():
        plan_text = cache.get(plan_path) or plan_path.read_text(encoding="utf-8")
        if ".claude/2026-03-14-hcw-fluent-reference-plan.md" in plan_text:
            issues.append(
                "example/HelloCustomWidgets/HelloCustomWidgets_plan.md still references missing "
                ".claude/2026-03-14-hcw-fluent-reference-plan.md"
            )

    return issues


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Check demo/catalog consistency across filesystem, docs, and generated web manifest.",
    )
    parser.add_argument(
        "--require-generated-manifest",
        action="store_true",
        help="Fail if web/demos/demos.json is missing instead of skipping manifest validation.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    module = load_wasm_build_module()
    counts = collect_live_counts(module)
    expected_sets = build_expected_demo_sets(module)

    issues: list[str] = []
    issues.extend(check_curated_custom_widget_sources(module))
    demos_json_issues, manifest_skipped = check_demos_json(module, expected_sets, args.require_generated_manifest)
    issues.extend(demos_json_issues)
    issues.extend(check_docs(build_doc_expectations(counts)))

    if issues:
        print("Demo catalog consistency check FAILED:")
        for issue in issues:
            print(f"  - {issue}")
        return 1

    expected_total = sum(len(items) for items in expected_sets.values())
    status_suffix = " (manifest skipped)" if manifest_skipped else ""
    print(
        "Demo catalog consistency check PASSED"
        f"{status_suffix} "
        f"(HelloBasic={counts['basic']}, HelloVirtual={counts['virtual']}, "
        f"HelloCustomWidgets={counts['custom_total']}, curated_web={counts['custom_curated']}, "
        f"default_web_total={expected_total})"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())

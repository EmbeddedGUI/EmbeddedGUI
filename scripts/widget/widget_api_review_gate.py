#!/usr/bin/env python3
"""Review widget public API and init params usability."""

import argparse
import json
import re
from datetime import datetime
from pathlib import Path

RUNTIME_ROOT = Path("runtime_check_output")


def normalize_widget_name(name: str) -> str:
    return name.strip().lower().replace("-", "_").replace(" ", "_")


def default_report_path(widget: str) -> Path:
    return RUNTIME_ROOT / f"HelloBasic_{widget}" / "api_review_report.json"


def extract_struct_body(text: str, struct_name: str) -> str:
    pattern = re.compile(rf"struct\s+{re.escape(struct_name)}\s*\{{(.*?)\}};", re.S)
    match = pattern.search(text)
    return match.group(1) if match else ""


def extract_function_body(text: str, signature_prefix: str) -> str:
    index = text.find(signature_prefix)
    if index < 0:
        return ""
    brace_start = text.find("{", index)
    if brace_start < 0:
        return ""
    depth = 0
    pos = brace_start
    while pos < len(text):
        char = text[pos]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return text[brace_start : pos + 1]
        pos += 1
    return ""


def has_decl(text: str, pattern: str) -> bool:
    return re.search(pattern, text, re.S) is not None


def run_review(widget: str, header_path: Path, source_path: Path) -> dict:
    checks: list[dict] = []
    warnings: list[str] = []

    header_exists = header_path.exists()
    source_exists = source_path.exists()

    checks.append({"id": "header_exists", "required": True, "passed": header_exists, "detail": str(header_path)})
    checks.append({"id": "source_exists", "required": True, "passed": source_exists, "detail": str(source_path)})

    if not header_exists or not source_exists:
        return {
            "status": "FAIL",
            "passed": False,
            "widget": widget,
            "checks": checks,
            "warnings": warnings,
            "failed_required": [item["id"] for item in checks if item["required"] and not item["passed"]],
        }

    header_text = header_path.read_text(encoding="utf-8")
    source_text = source_path.read_text(encoding="utf-8")

    params_type = f"egui_view_{widget}_params_t"
    params_struct = f"egui_view_{widget}_params"
    macro_name = f"EGUI_VIEW_{widget.upper()}_PARAMS_INIT"
    func_prefix = f"egui_view_{widget}_"

    check_items = [
        (
            "decl_init",
            True,
            has_decl(
                header_text,
                rf"void\s+{re.escape(func_prefix)}init\s*\(\s*egui_view_t\s*\*\s*self\s*\)\s*;",
            ),
            "init declaration",
        ),
        (
            "decl_apply_params",
            True,
            has_decl(
                header_text,
                rf"void\s+{re.escape(func_prefix)}apply_params\s*\(\s*egui_view_t\s*\*\s*self\s*,\s*const\s+{re.escape(params_type)}\s*\*\s*params\s*\)\s*;",
            ),
            "apply_params declaration",
        ),
        (
            "decl_init_with_params",
            True,
            has_decl(
                header_text,
                rf"void\s+{re.escape(func_prefix)}init_with_params\s*\(\s*egui_view_t\s*\*\s*self\s*,\s*const\s+{re.escape(params_type)}\s*\*\s*params\s*\)\s*;",
            ),
            "init_with_params declaration",
        ),
        (
            "params_typedef_exists",
            True,
            has_decl(header_text, rf"typedef\s+struct\s+{re.escape(params_struct)}\s+{re.escape(params_type)}\s*;"),
            params_type,
        ),
        (
            "params_macro_exists",
            True,
            macro_name in header_text,
            macro_name,
        ),
    ]

    for check_id, required, passed, detail in check_items:
        checks.append({"id": check_id, "required": required, "passed": bool(passed), "detail": detail})

    params_body = extract_struct_body(header_text, params_struct)
    has_region_field = "egui_region_t region;" in params_body
    checks.append({"id": "params_has_region", "required": True, "passed": has_region_field, "detail": "egui_region_t region;"})

    params_fields = [line.strip() for line in params_body.splitlines() if line.strip().endswith(";")]
    widget_specific_fields = [line for line in params_fields if "egui_region_t region;" not in line]
    checks.append(
        {
            "id": "params_has_widget_fields",
            "required": False,
            "passed": len(widget_specific_fields) > 0,
            "detail": f"widget_fields={len(widget_specific_fields)}",
        }
    )
    if len(widget_specific_fields) == 0:
        warnings.append("init params has no widget-specific fields; consider exposing key initial values in params.")

    macro_args_match = re.search(rf"#define\s+{re.escape(macro_name)}\s*\(([^)]*)\)", header_text)
    if macro_args_match:
        raw_args = [item.strip() for item in macro_args_match.group(1).split(",") if item.strip()]
        has_x = any(arg.endswith("x") or arg.endswith("_x") for arg in raw_args)
        has_y = any(arg.endswith("y") or arg.endswith("_y") for arg in raw_args)
        has_w = any(arg.endswith("w") or arg.endswith("_w") for arg in raw_args)
        has_h = any(arg.endswith("h") or arg.endswith("_h") for arg in raw_args)
        dims_ok = has_x and has_y and has_w and has_h
    else:
        dims_ok = False
    checks.append(
        {
            "id": "params_macro_dimensions",
            "required": True,
            "passed": dims_ok,
            "detail": "macro args should include x/y/w/h style dimension args",
        }
    )

    apply_body = extract_function_body(source_text, f"void {func_prefix}apply_params(")
    init_with_body = extract_function_body(source_text, f"void {func_prefix}init_with_params(")
    init_body = extract_function_body(source_text, f"void {func_prefix}init(")

    checks.append({"id": "def_init", "required": True, "passed": bool(init_body), "detail": "init definition"})
    checks.append({"id": "def_apply_params", "required": True, "passed": bool(apply_body), "detail": "apply_params definition"})
    checks.append({"id": "def_init_with_params", "required": True, "passed": bool(init_with_body), "detail": "init_with_params definition"})

    apply_region_ok = bool(
        re.search(r"self->region\s*=\s*params->region\s*;", apply_body)
        or re.search(r"egui_region_copy\s*\(\s*&self->region\s*,\s*&params->region\s*\)\s*;", apply_body)
        or re.search(r"egui_view_[a-z0-9_]+_apply_params\s*\(\s*self\s*,", apply_body)
    )
    checks.append(
        {
            "id": "apply_params_assign_region",
            "required": True,
            "passed": apply_region_ok,
            "detail": "apply_params should assign self->region from params->region",
        }
    )

    init_with_calls_init = bool(re.search(rf"{re.escape(func_prefix)}init\s*\(\s*self\s*\)\s*;", init_with_body))
    init_with_calls_apply = bool(re.search(rf"{re.escape(func_prefix)}apply_params\s*\(\s*self\s*,\s*params\s*\)\s*;", init_with_body))
    checks.append(
        {
            "id": "init_with_params_calls_init_apply",
            "required": True,
            "passed": init_with_calls_init and init_with_calls_apply,
            "detail": "init_with_params should call init() then apply_params()",
        }
    )

    all_decl_functions = re.findall(rf"void\s+{re.escape(func_prefix)}(set_[a-zA-Z0-9_]+)\s*\(", header_text)
    unique_setters = sorted(set(all_decl_functions))
    checks.append(
        {
            "id": "public_setter_count",
            "required": False,
            "passed": len(unique_setters) > 0,
            "detail": f"setter_count={len(unique_setters)}",
        }
    )
    if len(unique_setters) == 0:
        warnings.append("no public setter detected; ensure runtime update API is sufficient.")

    missing_setter_defs = []
    no_invalidate_setters = []
    no_guard_setters = []
    for setter in unique_setters:
        body = extract_function_body(source_text, f"void {func_prefix}{setter}(")
        if not body:
            missing_setter_defs.append(setter)
            continue
        is_listener_setter = setter.startswith("set_on_") or setter.endswith("_listener")
        forwards_setter = bool(re.search(r"egui_view_[a-z0-9_]+_set_[a-z0-9_]+\s*\(", body))
        if (not is_listener_setter) and ("egui_view_invalidate(self);" not in body) and (not forwards_setter):
            no_invalidate_setters.append(setter)
        if not re.search(r"if\s*\([^)]*\)\s*\{[^{}]*return\s*;\s*\}", body, re.S):
            no_guard_setters.append(setter)

    checks.append(
        {
            "id": "setter_defs_exist",
            "required": True,
            "passed": len(missing_setter_defs) == 0,
            "detail": ",".join(missing_setter_defs) if missing_setter_defs else "ok",
        }
    )
    checks.append(
        {
            "id": "setter_invalidate",
            "required": True,
            "passed": len(no_invalidate_setters) == 0,
            "detail": ",".join(no_invalidate_setters) if no_invalidate_setters else "ok",
        }
    )

    checks.append(
        {
            "id": "setter_guard_recommended",
            "required": False,
            "passed": len(no_guard_setters) == 0,
            "detail": ",".join(no_guard_setters) if no_guard_setters else "ok",
        }
    )
    if no_guard_setters:
        warnings.append("some setters do not have an early-return guard; consider reducing redundant invalidation.")

    required_failed = [item["id"] for item in checks if item["required"] and not item["passed"]]
    passed = len(required_failed) == 0

    return {
        "status": "PASS" if passed else "FAIL",
        "passed": passed,
        "widget": widget,
        "checks": checks,
        "warnings": warnings,
        "failed_required": required_failed,
        "header": str(header_path).replace("\\", "/"),
        "source": str(source_path).replace("\\", "/"),
    }


def write_report(path: Path, payload: dict) -> None:
    report = {
        "generated_at": datetime.now().isoformat(timespec="seconds"),
        "widget": payload.get("widget"),
        "result": payload,
    }
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Review widget API and init params quality gate")
    parser.add_argument("--widget", required=True, help="Widget name")
    parser.add_argument("--header", default="", help="Override widget header path")
    parser.add_argument("--source", default="", help="Override widget source path")
    parser.add_argument("--report", default="", help="Report json path")
    args = parser.parse_args()

    widget = normalize_widget_name(args.widget)
    header_path = Path(args.header) if args.header.strip() else Path("src/widget") / f"egui_view_{widget}.h"
    source_path = Path(args.source) if args.source.strip() else Path("src/widget") / f"egui_view_{widget}.c"
    report_path = Path(args.report) if args.report.strip() else default_report_path(widget)

    result = run_review(widget, header_path, source_path)
    write_report(report_path, result)
    print(f"[INFO] API review report: {report_path}")

    if result["passed"]:
        print("[PASS] API review gate passed.")
        if result["warnings"]:
            print("[WARN] " + " | ".join(result["warnings"]))
        return 0

    print("[FAIL] API review gate failed.")
    print("Required failures: " + ", ".join(result["failed_required"]))
    return 1


if __name__ == "__main__":
    raise SystemExit(main())

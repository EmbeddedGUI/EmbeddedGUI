#!/usr/bin/env python3
"""Discover widget gaps, assess duplication risk, and generate design briefs."""

import argparse
import json
from datetime import datetime
from pathlib import Path

RUNTIME_ROOT = Path("runtime_check_output")
LEGACY_CATALOG = Path(".claude/workflow/widget_candidate_catalog.json")
DEFAULT_CATALOG = RUNTIME_ROOT / "widget_candidate_catalog.json"
DEFAULT_REPORT = RUNTIME_ROOT / "widget_gap_report.json"
CUSTOM_WIDGET_DIR = Path("scripts/ui_designer/custom_widgets")
HELLOBASIC_DIR = Path("example/HelloBasic")

PRIORITY_ORDER = {"high": 0, "medium": 1, "low": 2}
DUPLICATION_RISK_ORDER = {"low": 0, "medium": 1, "high": 2}
DUPLICATION_HIGH_SCORE = 75
DUPLICATION_MEDIUM_SCORE = 45
MAX_SIMILAR_WIDGETS = 5

SCORE_CRITERIA = [
    {
        "id": "problem_value",
        "name": "Problem clarity",
        "description": "Is the target scenario and user value clearly defined?",
        "weight": 0.17,
        "critical": True,
    },
    {
        "id": "interaction_model",
        "name": "Interaction model",
        "description": "Is the interaction path natural and aligned with existing framework habits?",
        "weight": 0.17,
        "critical": True,
    },
    {
        "id": "visual_spec",
        "name": "Visual spec quality",
        "description": "Are size, spacing, hierarchy, and color constraints actionable?",
        "weight": 0.12,
        "critical": False,
    },
    {
        "id": "state_coverage",
        "name": "State coverage",
        "description": "Are key states (normal/active/disabled/error) covered?",
        "weight": 0.12,
        "critical": True,
    },
    {
        "id": "resource_cost",
        "name": "Resource cost",
        "description": "Does the design fit RAM/ROM/CPU limits for embedded targets?",
        "weight": 0.12,
        "critical": True,
    },
    {
        "id": "implementation_feasibility",
        "name": "Implementation feasibility",
        "description": "Can it be mapped quickly to C layer, UI Designer registration, and HelloBasic acceptance?",
        "weight": 0.15,
        "critical": True,
    },
    {
        "id": "differentiation",
        "name": "Differentiation",
        "description": "Is there enough functional difference vs existing widgets?",
        "weight": 0.15,
        "critical": True,
    },
]
DEFAULT_REVIEW_THRESHOLD = {"overall_min": 80, "critical_min": 3}
DEFAULT_MIN_ITERATION_CYCLES = 10


def resolve_catalog_path(catalog_path: Path) -> Path:
    if catalog_path.exists():
        return catalog_path

    if catalog_path == DEFAULT_CATALOG and LEGACY_CATALOG.exists():
        catalog_path.parent.mkdir(parents=True, exist_ok=True)
        catalog_path.write_text(LEGACY_CATALOG.read_text(encoding="utf-8"), encoding="utf-8")
        print(f"[MIGRATE] catalog moved to runtime: {catalog_path}")
        return catalog_path

    return catalog_path


def normalize_name(name: str) -> str:
    return name.strip().lower().replace("-", "_").replace(" ", "_")


def default_iteration_dir(widget: str) -> Path:
    return Path("runtime_check_output") / f"HelloBasic_{widget}" / "iteration_records"


def default_design_dir(widget: str) -> Path:
    return Path("runtime_check_output") / f"HelloBasic_{widget}" / "widget_designs"


def default_review_dir(widget: str) -> Path:
    return Path("runtime_check_output") / f"HelloBasic_{widget}" / "widget_design_reviews"


def normalize_str_list(values) -> list[str]:
    if not values:
        return []
    return [str(x).strip() for x in values if str(x).strip()]


def load_catalog(catalog_path: Path) -> list[dict]:
    items = json.loads(catalog_path.read_text(encoding="utf-8"))
    normalized = []
    for item in items:
        name = normalize_name(item["name"])
        priority = normalize_name(item.get("priority", "medium"))
        if priority not in PRIORITY_ORDER:
            priority = "medium"
        normalized.append(
            {
                "name": name,
                "display_name": item.get("display_name", name),
                "priority": priority,
                "category": item.get("category", "general"),
                "description": item.get("description", ""),
                "design_focus": normalize_str_list(item.get("design_focus", [])),
                "states": normalize_str_list(item.get("states", [])),
                "interactions": normalize_str_list(item.get("interactions", [])),
                "similar_widgets": [normalize_name(x) for x in item.get("similar_widgets", []) if str(x).strip()],
                "differentiation_goals": normalize_str_list(item.get("differentiation_goals", [])),
            }
        )
    return normalized


def get_registered_widgets() -> set[str]:
    names = set()
    if not CUSTOM_WIDGET_DIR.exists():
        return names
    for path in CUSTOM_WIDGET_DIR.glob("*.py"):
        if path.stem.startswith("__"):
            continue
        names.add(normalize_name(path.stem))
    return names


def get_hellobasic_subapps() -> set[str]:
    names = set()
    if not HELLOBASIC_DIR.exists():
        return names
    for path in HELLOBASIC_DIR.iterdir():
        if path.is_dir():
            names.add(normalize_name(path.name))
    return names


def collect_token_overlap(target_name: str, registered: set[str]) -> list[str]:
    tokens = {x for x in target_name.split("_") if x}
    if not tokens:
        return []

    overlaps = []
    for widget in sorted(registered):
        widget_tokens = {x for x in widget.split("_") if x}
        if tokens.intersection(widget_tokens):
            overlaps.append(widget)
    return overlaps[:MAX_SIMILAR_WIDGETS]


def build_differentiation_actions(item: dict, similar_hits: list[str]) -> list[str]:
    actions = list(item.get("differentiation_goals", []))
    if actions:
        return actions

    auto_actions = []
    if similar_hits:
        for widget in similar_hits[:3]:
            auto_actions.append(f"Must provide at least one behavior not covered by `{widget}`.")
        auto_actions.append("Document explicit scenario boundaries to avoid replacing existing widgets.")
    else:
        auto_actions.append("Define unique scenario boundaries and avoid broad overlap with generic controls.")

    return auto_actions[:4]


def evaluate_duplication(item: dict, registered: set[str]) -> dict:
    similar_hits = [x for x in item.get("similar_widgets", []) if x in registered]
    token_hits = [x for x in collect_token_overlap(item["name"], registered) if x not in similar_hits]

    score = 0
    score += len(similar_hits) * 22
    score += min(len(token_hits), 3) * 8
    score = min(score, 100)

    if score >= DUPLICATION_HIGH_SCORE:
        risk = "high"
    elif score >= DUPLICATION_MEDIUM_SCORE:
        risk = "medium"
    else:
        risk = "low"

    return {
        "risk": risk,
        "score": score,
        "similar_existing": (similar_hits + token_hits)[:MAX_SIMILAR_WIDGETS],
        "differentiation_actions": build_differentiation_actions(item, similar_hits),
    }


def rank_missing(catalog: list[dict], registered: set[str]) -> list[dict]:
    missing = []
    for item in catalog:
        if item["name"] in registered:
            continue
        copied = dict(item)
        copied["duplication"] = evaluate_duplication(copied, registered)
        missing.append(copied)

    missing.sort(
        key=lambda x: (
            PRIORITY_ORDER.get(x["priority"], 9),
            DUPLICATION_RISK_ORDER.get(x["duplication"]["risk"], 9),
            x["duplication"]["score"],
            x["name"],
        )
    )
    return missing


def pick_target(missing: list[dict], requested_widget: str | None, allow_high_duplication: bool) -> tuple[dict | None, str | None]:
    if not missing:
        return None, None

    if requested_widget:
        req = normalize_name(requested_widget)
        for item in missing:
            if item["name"] != req:
                continue
            if item["duplication"]["risk"] == "high" and not allow_high_duplication:
                return None, f"requested_widget_high_duplication:{item['name']}"
            return item, None
        return None, f"requested_widget_not_missing:{req}"

    for item in missing:
        if item["duplication"]["risk"] != "high":
            return item, None

    if allow_high_duplication:
        return missing[0], None
    return None, "all_candidates_high_duplication"


def suggest_similar_existing(target: dict, registered: set[str]) -> list[str]:
    similar = []
    for name in target.get("similar_widgets", []):
        if name in registered and name not in similar:
            similar.append(name)

    for name in collect_token_overlap(target["name"], registered):
        if name not in similar:
            similar.append(name)
        if len(similar) >= MAX_SIMILAR_WIDGETS:
            break

    return similar[:MAX_SIMILAR_WIDGETS]


def render_design_brief(target: dict, similar_existing: list[str], has_hello_subapp: bool) -> str:
    now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    focus = "\n".join(f"- {x}" for x in target.get("design_focus", [])) or "- TODO"
    states = "\n".join(f"- {x}" for x in target.get("states", [])) or "- normal"
    interactions = "\n".join(f"- {x}" for x in target.get("interactions", [])) or "- click"
    similar = "\n".join(f"- `{x}`" for x in similar_existing) or "- none"

    duplication = target.get("duplication", {})
    duplication_actions = "\n".join(f"- {x}" for x in duplication.get("differentiation_actions", [])) or "- TODO"

    hello_note = (
        "A same-name demo directory already exists and can be reused."
        if has_hello_subapp
        else f"Need to create `example/HelloBasic/{target['name']}`."
    )

    return f"""# Widget Design Brief: {target['display_name']} (`{target['name']}`)

Generated at: {now}
Category: {target['category']}
Priority: {target['priority']}

## 1. Goal and value
{target['description'] or 'Fill a missing foundational interaction capability.'}

## 2. Existing widget overlap check

Duplication risk: `{duplication.get('risk', 'unknown')}` (score={duplication.get('score', 0)})

Potentially overlapping widgets:
{similar}

Differentiation requirements:
{duplication_actions}

## 3. Visual and interaction notes
{focus}

Key states:
{states}

Key interactions:
{interactions}

## 4. Draft C API

- `egui_view_{target['name']}_t`
- `egui_view_{target['name']}_params_t`
- `egui_view_{target['name']}_init_with_params()`
- `egui_view_{target['name']}_set_xxx()` (define setters per design)
- event callback: click/value-change/selection-change as needed

## 5. UI Designer registration draft

- `type_name`: `{target['name']}`
- `xml_tag`: `{target['display_name'].replace(' ', '')}`
- `c_type`: `egui_view_{target['name']}_t`
- `init_func`: `egui_view_{target['name']}_init_with_params`
- properties should include appearance/value/state/behavior groups

## 6. HelloBasic demo plan

- demo dir: `example/HelloBasic/{target['name']}`
- status: {hello_note}
- recommended minimum coverage:
  - default state
  - selected/active state
  - disabled state
  - boundary input handling

## 7. Recording and acceptance

- implement `egui_port_get_recording_action()` to cover key state transitions
- recommended recorder profile:
  - `clock-scale=6`
  - `snapshot-settle-ms=0`
  - `snapshot-stable-cycles=1`
  - `snapshot-max-wait-ms=1500`
- acceptance gate:
  - runtime pass
  - non-blank frame pass
  - baseline regression SSIM pass

## 8. Execution split

1. C widget skeleton + drawing behavior
2. UI Designer registration + property mapping
3. HelloBasic demo + recording actions
4. baseline promotion + regression acceptance
"""


def write_report(report_path: Path, catalog: list[dict], registered: set[str], missing: list[dict], target: dict | None, blocked_reason: str | None) -> None:
    report_path.parent.mkdir(parents=True, exist_ok=True)
    payload = {
        "generated_at": datetime.now().isoformat(timespec="seconds"),
        "registered_count": len(registered),
        "catalog_count": len(catalog),
        "missing_count": len(missing),
        "registered_widgets": sorted(registered),
        "missing_widgets": [
            {
                "name": x["name"],
                "priority": x["priority"],
                "category": x["category"],
                "display_name": x["display_name"],
                "duplication": x.get("duplication", {}),
            }
            for x in missing
        ],
        "recommended_next_widget": target["name"] if target else None,
        "selection_blocked_reason": blocked_reason,
    }
    report_path.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")


def build_review_template(target: dict, design_path: Path) -> dict:
    duplication = target.get("duplication", {})
    return {
        "widget": target["name"],
        "display_name": target["display_name"],
        "status": "PENDING_REVIEW",
        "generated_at": datetime.now().isoformat(timespec="seconds"),
        "design_doc": str(design_path).replace("\\", "/"),
        "threshold": DEFAULT_REVIEW_THRESHOLD,
        "duplication_check": {
            "risk": duplication.get("risk", "unknown"),
            "score": duplication.get("score", 0),
            "similar_existing": duplication.get("similar_existing", []),
            "differentiation_actions": duplication.get("differentiation_actions", []),
        },
        "criteria": [
            {
                "id": item["id"],
                "name": item["name"],
                "description": item["description"],
                "weight": item["weight"],
                "critical": item["critical"],
                "score": None,
                "comment": "",
            }
            for item in SCORE_CRITERIA
        ],
        "reviewer": "",
        "reviewed_at": "",
        "notes": "",
    }


def write_review_template(review_dir: Path, target: dict, design_path: Path, overwrite: bool) -> Path | None:
    review_dir.mkdir(parents=True, exist_ok=True)
    review_path = review_dir / f"{target['name']}.json"
    if review_path.exists() and not overwrite:
        return None
    template = build_review_template(target, design_path)
    review_path.write_text(json.dumps(template, ensure_ascii=False, indent=2), encoding="utf-8")
    return review_path


def build_iteration_template(target: dict, min_cycles: int) -> dict:
    return {
        "widget": target["name"],
        "required_min_cycles": int(min_cycles),
        "generated_at": datetime.now().isoformat(timespec="seconds"),
        "rules": {
            "visual_validation_required": True,
            "visual_validation_note": "Each iteration must explicitly record visual checks and outcomes.",
            "interaction_validation_required": True,
            "interaction_validation_note": "Each iteration must explicitly record interaction checks and expected behavior.",
        },
        "cycles": [],
        "notes": "",
    }


def write_iteration_template(iteration_dir: Path, target: dict, min_cycles: int, overwrite: bool) -> Path | None:
    iteration_dir.mkdir(parents=True, exist_ok=True)
    iteration_path = iteration_dir / f"{target['name']}.json"
    if iteration_path.exists() and not overwrite:
        return None
    template = build_iteration_template(target, min_cycles)
    iteration_path.write_text(json.dumps(template, ensure_ascii=False, indent=2), encoding="utf-8")
    return iteration_path


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate widget gap report and design brief")
    parser.add_argument("--widget", default="", help="Target missing widget to generate brief for")
    parser.add_argument("--catalog", default=str(DEFAULT_CATALOG), help="Widget candidate catalog json path")
    parser.add_argument("--report-out", default=str(DEFAULT_REPORT), help="Gap report output path")
    parser.add_argument(
        "--design-dir",
        default="",
        help="Design brief output directory (default: runtime_check_output/HelloBasic_<widget>/widget_designs)",
    )
    parser.add_argument(
        "--review-dir",
        default="",
        help="Design review template output directory (default: runtime_check_output/HelloBasic_<widget>/widget_design_reviews)",
    )
    parser.add_argument(
        "--iteration-dir",
        default="",
        help="Iteration tracker template output directory (default: runtime_check_output/HelloBasic_<widget>/iteration_records)",
    )
    parser.add_argument("--min-iteration-cycles", type=int, default=DEFAULT_MIN_ITERATION_CYCLES, help="Required minimum quality iteration cycles")
    parser.add_argument("--overwrite", action="store_true", help="Overwrite existing design brief")
    parser.add_argument(
        "--allow-high-duplication",
        action="store_true",
        help="Allow selecting high-duplication widgets (not recommended)",
    )
    args = parser.parse_args()

    catalog_path = resolve_catalog_path(Path(args.catalog))
    if not catalog_path.exists():
        print(f"[FAIL] catalog not found: {catalog_path}")
        if catalog_path == DEFAULT_CATALOG:
            print(f"[INFO] expected default catalog path: {DEFAULT_CATALOG}")
        return 1

    catalog = load_catalog(catalog_path)
    registered = get_registered_widgets()
    hello_subapps = get_hellobasic_subapps()
    missing = rank_missing(catalog, registered)
    target, blocked_reason = pick_target(missing, args.widget if args.widget else None, args.allow_high_duplication)

    report_path = Path(args.report_out)
    write_report(report_path, catalog, registered, missing, target, blocked_reason)
    print(f"[OK] Gap report: {report_path}")
    print(f"[INFO] Registered={len(registered)} Catalog={len(catalog)} Missing={len(missing)}")

    if target is None:
        if blocked_reason and blocked_reason.startswith("requested_widget_not_missing:"):
            print(f"[FAIL] requested widget not found in missing list: {blocked_reason.split(':', 1)[1]}")
            return 1

        if blocked_reason and blocked_reason.startswith("requested_widget_high_duplication:"):
            widget = blocked_reason.split(":", 1)[1]
            print(f"[FAIL] requested widget has high duplication risk: {widget}")
            print("[INFO] Use --allow-high-duplication only when there is a strong business reason.")
            return 1

        if blocked_reason == "all_candidates_high_duplication":
            print("[HOLD] all missing candidates are high duplication; no recommendation generated.")
            print("[INFO] refine catalog differentiators first, or rerun with --allow-high-duplication.")
            return 2

        print("[INFO] no missing widget in catalog")
        return 0

    iteration_dir = Path(args.iteration_dir) if args.iteration_dir.strip() else default_iteration_dir(target["name"])
    design_dir = Path(args.design_dir) if args.design_dir.strip() else default_design_dir(target["name"])
    review_dir = Path(args.review_dir) if args.review_dir.strip() else default_review_dir(target["name"])
    design_dir.mkdir(parents=True, exist_ok=True)
    design_path = design_dir / f"{target['name']}.md"
    if design_path.exists() and not args.overwrite:
        print(f"[SKIP] design brief already exists: {design_path}")
        print(f"[INFO] recommended widget: {target['name']}")
        print(
            "[INFO] duplication risk: "
            f"{target['duplication']['risk']} (score={target['duplication']['score']})"
        )
        review_path = write_review_template(review_dir, target, design_path, overwrite=False)
        if review_path is None:
            print(f"[SKIP] design review template already exists: {review_dir / (target['name'] + '.json')}")
        else:
            print(f"[OK] Design review template: {review_path}")
        iteration_path = write_iteration_template(iteration_dir, target, args.min_iteration_cycles, overwrite=False)
        if iteration_path is None:
            print(f"[SKIP] iteration template already exists: {iteration_dir / (target['name'] + '.json')}")
        else:
            print(f"[OK] Iteration template: {iteration_path}")
        return 0

    similar_existing = suggest_similar_existing(target, registered)
    content = render_design_brief(target, similar_existing, target["name"] in hello_subapps)
    design_path.write_text(content, encoding="utf-8")
    print(f"[OK] Design brief: {design_path}")
    print(
        "[INFO] duplication risk: "
        f"{target['duplication']['risk']} (score={target['duplication']['score']})"
    )
    if target["duplication"]["similar_existing"]:
        print("[INFO] overlap with: " + ", ".join(target["duplication"]["similar_existing"]))

    review_path = write_review_template(review_dir, target, design_path, overwrite=args.overwrite)
    if review_path is None:
        print(f"[SKIP] design review template already exists: {review_dir / (target['name'] + '.json')}")
    else:
        print(f"[OK] Design review template: {review_path}")
    iteration_path = write_iteration_template(iteration_dir, target, args.min_iteration_cycles, overwrite=args.overwrite)
    if iteration_path is None:
        print(f"[SKIP] iteration template already exists: {iteration_dir / (target['name'] + '.json')}")
    else:
        print(f"[OK] Iteration template: {iteration_path}")

    print(f"[INFO] recommended widget: {target['name']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

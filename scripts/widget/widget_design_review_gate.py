#!/usr/bin/env python3
"""Gate checker for widget design review scorecard."""

import argparse
import json
from datetime import datetime
from pathlib import Path

WORKFLOW_DIR = Path(".claude/workflow")


def default_review_dir(widget: str) -> Path:
    return Path("runtime_check_output") / f"HelloBasic_{widget}" / "widget_design_reviews"


def default_report_dir(widget: str) -> Path:
    return Path("runtime_check_output") / f"HelloBasic_{widget}" / "widget_design_gate_reports"


def load_review(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def validate_and_score(review: dict, overall_min: int | None, critical_min: int | None) -> dict:
    criteria = review.get("criteria", [])
    if not criteria:
        return {"ok": False, "reason": "missing criteria", "missing_scores": [], "critical_failed": [], "overall_score": 0.0}

    threshold = review.get("threshold", {})
    if overall_min is None:
        overall_min = int(threshold.get("overall_min", 80))
    if critical_min is None:
        critical_min = int(threshold.get("critical_min", 3))

    total_weight = 0.0
    weighted_sum = 0.0
    missing_scores = []
    critical_failed = []
    scored_items = []

    for item in criteria:
        cid = item.get("id", "")
        score = item.get("score")
        weight = float(item.get("weight", 0.0))
        critical = bool(item.get("critical", False))
        total_weight += weight

        if score is None:
            missing_scores.append(cid)
            continue

        try:
            score_val = float(score)
        except Exception:
            missing_scores.append(cid)
            continue

        if score_val < 0 or score_val > 5:
            missing_scores.append(cid)
            continue

        weighted_sum += (score_val / 5.0) * weight
        scored_items.append({"id": cid, "score": score_val, "weight": weight, "critical": critical})
        if critical and score_val < critical_min:
            critical_failed.append(cid)

    if missing_scores:
        return {
            "ok": False,
            "reason": "missing_or_invalid_scores",
            "missing_scores": sorted(set(missing_scores)),
            "critical_failed": critical_failed,
            "overall_score": 0.0,
            "overall_min": overall_min,
            "critical_min": critical_min,
            "scored_items": scored_items,
        }

    if total_weight <= 0:
        return {"ok": False, "reason": "invalid_weight", "missing_scores": [], "critical_failed": [], "overall_score": 0.0}

    overall_score = (weighted_sum / total_weight) * 100.0
    pass_overall = overall_score >= overall_min
    pass_critical = len(critical_failed) == 0
    ok = pass_overall and pass_critical
    return {
        "ok": ok,
        "reason": "pass" if ok else "threshold_not_met",
        "missing_scores": [],
        "critical_failed": critical_failed,
        "overall_score": round(overall_score, 2),
        "overall_min": overall_min,
        "critical_min": critical_min,
        "pass_overall": pass_overall,
        "pass_critical": pass_critical,
        "scored_items": scored_items,
    }


def write_report(report_dir: Path, widget: str, review_path: Path, result: dict) -> Path:
    report_dir.mkdir(parents=True, exist_ok=True)
    report_path = report_dir / f"{widget}.json"
    payload = {
        "generated_at": datetime.now().isoformat(timespec="seconds"),
        "widget": widget,
        "review_path": str(review_path).replace("\\", "/"),
        "result": result,
    }
    report_path.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")
    return report_path


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate widget design review scorecard gate")
    parser.add_argument("--widget", required=True, help="Widget name")
    parser.add_argument(
        "--review-dir",
        default="",
        help="Design review directory (default: runtime_check_output/HelloBasic_<widget>/widget_design_reviews)",
    )
    parser.add_argument(
        "--report-dir",
        default="",
        help="Gate report directory (default: runtime_check_output/HelloBasic_<widget>/widget_design_gate_reports)",
    )
    parser.add_argument("--overall-min", type=int, default=None, help="Override overall score threshold (0-100)")
    parser.add_argument("--critical-min", type=int, default=None, help="Override critical item minimum score (0-5)")
    args = parser.parse_args()

    widget = args.widget.strip().lower().replace("-", "_").replace(" ", "_")
    review_dir = Path(args.review_dir) if args.review_dir.strip() else default_review_dir(widget)
    report_dir = Path(args.report_dir) if args.report_dir.strip() else default_report_dir(widget)

    review_path = review_dir / f"{widget}.json"
    if not review_path.exists():
        print(f"[FAIL] review file not found: {review_path}")
        return 1

    review = load_review(review_path)
    result = validate_and_score(review, args.overall_min, args.critical_min)
    report_path = write_report(report_dir, widget, review_path, result)

    print(f"[INFO] Gate report: {report_path}")
    if result["ok"]:
        print(f"[PASS] design gate passed: overall={result['overall_score']} (min={result['overall_min']})")
        return 0

    print(f"[FAIL] design gate failed: {result['reason']}")
    if result.get("missing_scores"):
        print(f"Missing/invalid scores: {', '.join(result['missing_scores'])}")
    if result.get("critical_failed"):
        print(f"Critical criteria below threshold: {', '.join(result['critical_failed'])}")
    if "overall_score" in result and "overall_min" in result:
        print(f"Overall score: {result['overall_score']} (min={result['overall_min']})")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())

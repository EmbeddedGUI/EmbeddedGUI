#!/usr/bin/env python3
"""Track and gate recursive quality iterations for new widgets."""

import argparse
import hashlib
import json
from datetime import datetime
from pathlib import Path

RUNTIME_ROOT = Path("runtime_check_output")
DEFAULT_MIN_CYCLES = 10


def normalize_widget_name(name: str) -> str:
    return name.strip().lower().replace("-", "_").replace(" ", "_")


def now_iso() -> str:
    return datetime.now().isoformat(timespec="seconds")


def default_iteration_dir(widget: str) -> Path:
    return RUNTIME_ROOT / f"HelloBasic_{widget}" / "iteration_records"


def default_report_dir(widget: str) -> Path:
    return RUNTIME_ROOT / f"HelloBasic_{widget}" / "iteration_reports"


def normalize_path(path: Path) -> str:
    try:
        return path.resolve().relative_to(Path.cwd().resolve()).as_posix()
    except Exception:
        return path.as_posix()


def parse_path_list(raw: str) -> list[Path]:
    tokens = [token.strip() for token in raw.replace(";", ",").split(",")]
    result = []
    for token in tokens:
        if not token:
            continue
        result.append(Path(token))
    return result


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as fp:
        while True:
            chunk = fp.read(65536)
            if not chunk:
                break
            digest.update(chunk)
    return digest.hexdigest()


def build_files_proof(paths: list[Path]) -> tuple[list[dict], str]:
    proof_items = []
    digest = hashlib.sha256()
    for raw_path in paths:
        path = raw_path if raw_path.is_absolute() else (Path.cwd() / raw_path)
        if not path.exists():
            raise ValueError(f"changed file not found: {raw_path}")
        if not path.is_file():
            raise ValueError(f"changed file is not a regular file: {raw_path}")
        path_norm = normalize_path(path)
        file_hash = sha256_file(path)
        proof_items.append({"path": path_norm, "sha256": file_hash})
        digest.update(path_norm.encode("utf-8"))
        digest.update(b"\0")
        digest.update(file_hash.encode("ascii"))
        digest.update(b"\0")
    return proof_items, digest.hexdigest()


def collect_frame_proof(widget: str) -> dict:
    app_dir = RUNTIME_ROOT / f"HelloBasic_{widget}"
    frames_dir = app_dir / "default"
    frames = sorted(frames_dir.glob("frame_*.png"))
    if not frames:
        raise ValueError(f"no runtime frames found for proof: {frames_dir}")

    digest = hashlib.sha256()
    frame_hashes = []
    for frame in frames:
        file_hash = sha256_file(frame)
        frame_hashes.append({"frame": frame.name, "sha256": file_hash})
        digest.update(frame.name.encode("utf-8"))
        digest.update(b"\0")
        digest.update(file_hash.encode("ascii"))
        digest.update(b"\0")

    return {
        "frames_dir": normalize_path(frames_dir),
        "frame_count": len(frames),
        "frame_hashes": frame_hashes,
        "digest": digest.hexdigest(),
    }


def load_json_optional(path: Path) -> dict | None:
    if not path.exists():
        return None
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception:
        return None


def collect_runtime_report_proof(widget: str) -> dict:
    app_dir = RUNTIME_ROOT / f"HelloBasic_{widget}"
    visual_path = app_dir / "visual_report.json"
    interaction_path = app_dir / "interaction_report.json"
    visual = load_json_optional(visual_path)
    interaction = load_json_optional(interaction_path)

    visual_pass = bool(visual and bool(visual.get("passed")))
    interaction_pass = bool(interaction and bool(interaction.get("passed")))

    return {
        "visual_report": {
            "path": normalize_path(visual_path),
            "exists": bool(visual is not None),
            "pass": visual_pass,
            "mode": (visual or {}).get("mode"),
            "regression_status": (visual or {}).get("regression_status"),
        },
        "interaction_report": {
            "path": normalize_path(interaction_path),
            "exists": bool(interaction is not None),
            "pass": interaction_pass,
            "changed_transitions": (interaction or {}).get("changed_transitions"),
            "min_transitions": (interaction or {}).get("min_transitions"),
        },
    }


def default_min_unique_change_cycles(required_min_cycles: int) -> int:
    return max(3, required_min_cycles // 2)


def default_min_visual_delta_cycles(required_min_cycles: int) -> int:
    return max(3, required_min_cycles // 2)


def build_template(widget: str, required_min_cycles: int) -> dict:
    return {
        "widget": widget,
        "required_min_cycles": required_min_cycles,
        "generated_at": now_iso(),
        "rules": {
            "visual_validation_required": True,
            "visual_validation_note": "Each iteration must include explicit visual check notes.",
            "interaction_validation_required": True,
            "interaction_validation_note": "Each iteration must include explicit interaction verification notes.",
        },
        "cycles": [],
        "notes": "",
    }


def ensure_template(path: Path, widget: str, required_min_cycles: int, overwrite: bool = False) -> tuple[dict, bool]:
    if path.exists() and not overwrite:
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
            if not isinstance(data, dict):
                raise ValueError("iteration file root must be object")
            data.setdefault("widget", widget)
            data.setdefault("required_min_cycles", required_min_cycles)
            data.setdefault("cycles", [])
            data.setdefault(
                "rules",
                {
                    "visual_validation_required": True,
                    "visual_validation_note": "Each iteration must include explicit visual check notes.",
                    "interaction_validation_required": True,
                    "interaction_validation_note": "Each iteration must include explicit interaction verification notes.",
                },
            )
            if isinstance(data["rules"], dict):
                data["rules"].setdefault("interaction_validation_required", True)
                data["rules"].setdefault("interaction_validation_note", "Each iteration must include explicit interaction verification notes.")
            if not isinstance(data["cycles"], list):
                raise ValueError("cycles must be list")
            return data, False
        except Exception as exc:
            raise ValueError(f"invalid iteration file: {path} ({exc})") from exc

    data = build_template(widget, required_min_cycles)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8")
    return data, True


def append_cycle(data: dict, widget: str, goal: str, changes: str, verification: str, visual_validation: str, interaction_validation: str, result: str,
                 changed_files: list[Path], visual_artifacts: list[Path], skip_proof_validation: bool) -> dict:
    goal = goal.strip()
    changes = changes.strip()
    verification = verification.strip()
    visual_validation = visual_validation.strip()
    interaction_validation = interaction_validation.strip()
    result = result.strip().upper()

    if not goal:
        raise ValueError("cycle goal is required")
    if not changes:
        raise ValueError("cycle changes is required")
    if not verification:
        raise ValueError("cycle verification is required")
    if not visual_validation:
        raise ValueError("cycle visual_validation is required")
    if not interaction_validation:
        raise ValueError("cycle interaction_validation is required")
    if result not in {"PASS", "FAIL"}:
        raise ValueError("cycle result must be PASS or FAIL")

    if not changed_files and not skip_proof_validation:
        raise ValueError("changed_files is required unless --skip-proof-validation is set")

    files_proof = []
    changes_digest = ""
    if changed_files:
        files_proof, changes_digest = build_files_proof(changed_files)

    frame_proof = None
    report_proof = None
    if not skip_proof_validation:
        frame_proof = collect_frame_proof(widget)
        report_proof = collect_runtime_report_proof(widget)
        if not report_proof["visual_report"]["pass"]:
            raise ValueError("visual_report is missing or not PASS; run acceptance flow before recording cycle")
        if not report_proof["interaction_report"]["pass"]:
            raise ValueError("interaction_report is missing or not PASS; run acceptance flow before recording cycle")
    else:
        try:
            frame_proof = collect_frame_proof(widget)
        except Exception:
            frame_proof = None
        report_proof = collect_runtime_report_proof(widget)

    normalized_visual_artifacts = []
    for artifact in visual_artifacts:
        full = artifact if artifact.is_absolute() else (Path.cwd() / artifact)
        normalized_visual_artifacts.append(normalize_path(full))

    cycles = data.setdefault("cycles", [])
    index = len(cycles) + 1
    cycle = {
        "index": index,
        "timestamp": now_iso(),
        "goal": goal,
        "changes": changes,
        "verification": verification,
        "visual_validation": visual_validation,
        "interaction_validation": interaction_validation,
        "result": result,
        "changed_files": [item["path"] for item in files_proof],
        "changed_files_sha256": files_proof,
        "changes_digest": changes_digest,
        "frame_digest": (frame_proof or {}).get("digest", ""),
        "frame_count": (frame_proof or {}).get("frame_count", 0),
        "frame_dir": (frame_proof or {}).get("frames_dir", ""),
        "visual_artifacts": normalized_visual_artifacts,
        "runtime_report_proof": report_proof,
        "proof_mode": "relaxed" if skip_proof_validation else "strict",
    }
    cycles.append(cycle)
    return cycle


def evaluate_cycles(data: dict, min_cycles_override: int | None = None, min_unique_change_cycles: int | None = None,
                    min_visual_delta_cycles: int | None = None) -> dict:
    required_min_cycles = int(data.get("required_min_cycles", DEFAULT_MIN_CYCLES))
    if min_cycles_override is not None:
        required_min_cycles = int(min_cycles_override)
    if min_unique_change_cycles is None:
        min_unique_change_cycles = default_min_unique_change_cycles(required_min_cycles)
    if min_visual_delta_cycles is None:
        min_visual_delta_cycles = default_min_visual_delta_cycles(required_min_cycles)

    cycles = data.get("cycles", [])
    completed = []
    invalid_indexes = []
    for idx, cycle in enumerate(cycles, 1):
        goal = str(cycle.get("goal", "")).strip()
        changes = str(cycle.get("changes", "")).strip()
        verification = str(cycle.get("verification", "")).strip()
        visual_validation = str(cycle.get("visual_validation", "")).strip()
        interaction_validation = str(cycle.get("interaction_validation", "")).strip()
        result = str(cycle.get("result", "")).upper().strip()
        changed_files = cycle.get("changed_files", [])
        changes_digest = str(cycle.get("changes_digest", "")).strip()
        frame_digest = str(cycle.get("frame_digest", "")).strip()
        report_proof = cycle.get("runtime_report_proof", {})
        visual_pass = report_proof.get("visual_report", {}).get("pass")
        interaction_pass = report_proof.get("interaction_report", {}).get("pass")

        if (
            goal and changes and verification and visual_validation and interaction_validation and result in {"PASS", "FAIL"}
            and isinstance(changed_files, list) and len(changed_files) > 0
            and bool(changes_digest) and bool(frame_digest)
            and visual_pass is True and interaction_pass is True
        ):
            completed.append(
                {
                    "index": idx,
                    "result": result,
                    "changes_digest": changes_digest,
                    "frame_digest": frame_digest,
                }
            )
        else:
            invalid_indexes.append(idx)

    completed_count = len(completed)
    latest_result = completed[-1]["result"] if completed else None
    unique_change_digests = {item["changes_digest"] for item in completed}
    unique_change_cycles = len(unique_change_digests)
    visual_delta_cycles = 0
    prev_digest = None
    for item in completed:
        frame_digest = item["frame_digest"]
        if prev_digest is not None and frame_digest != prev_digest:
            visual_delta_cycles += 1
        prev_digest = frame_digest

    reasons = []
    if completed_count < required_min_cycles:
        reasons.append(f"insufficient_cycles:{completed_count}/{required_min_cycles}")
    if unique_change_cycles < min_unique_change_cycles:
        reasons.append(f"insufficient_unique_changes:{unique_change_cycles}/{min_unique_change_cycles}")
    if visual_delta_cycles < min_visual_delta_cycles:
        reasons.append(f"insufficient_visual_deltas:{visual_delta_cycles}/{min_visual_delta_cycles}")
    if completed_count > 0 and latest_result != "PASS":
        reasons.append("latest_cycle_not_pass")
    if invalid_indexes:
        reasons.append("invalid_cycle_entries")

    passed = (
        completed_count >= required_min_cycles
        and latest_result == "PASS"
        and unique_change_cycles >= min_unique_change_cycles
        and visual_delta_cycles >= min_visual_delta_cycles
        and not invalid_indexes
    )
    status = "PASS" if passed else "HOLD"

    return {
        "status": status,
        "passed": passed,
        "required_min_cycles": required_min_cycles,
        "required_min_unique_change_cycles": min_unique_change_cycles,
        "required_min_visual_delta_cycles": min_visual_delta_cycles,
        "recorded_cycles": len(cycles),
        "completed_cycles": completed_count,
        "latest_result": latest_result,
        "unique_change_cycles": unique_change_cycles,
        "visual_delta_cycles": visual_delta_cycles,
        "invalid_cycle_indexes": invalid_indexes,
        "reasons": reasons,
    }


def write_iteration_file(path: Path, data: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8")


def write_report(report_path: Path, widget: str, iteration_path: Path, result: dict) -> None:
    payload = {
        "generated_at": now_iso(),
        "widget": widget,
        "iteration_file": str(iteration_path).replace("\\", "/"),
        "result": result,
    }
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Record and gate recursive widget quality iterations")
    parser.add_argument("--widget", required=True, help="Widget name")
    parser.add_argument("--iteration-dir", default="", help="Iteration data directory (default: runtime_check_output/HelloBasic_<widget>/iteration_records)")
    parser.add_argument("--report-dir", default="", help="Gate report directory (default: runtime_check_output/HelloBasic_<widget>/iteration_reports)")
    parser.add_argument("--min-cycles", type=int, default=None, help="Override required minimum cycles")
    parser.add_argument("--init", action="store_true", help="Create iteration template if missing")
    parser.add_argument("--overwrite", action="store_true", help="Overwrite iteration template when --init is set")
    parser.add_argument("--record", action="store_true", help="Append one iteration cycle")
    parser.add_argument("--goal", default="", help="Cycle goal (required with --record)")
    parser.add_argument("--changes", default="", help="Cycle changes summary (required with --record)")
    parser.add_argument("--verification", default="", help="Cycle verification summary (required with --record)")
    parser.add_argument(
        "--visual-validation",
        default="",
        help="Cycle visual validation summary (required with --record)",
    )
    parser.add_argument(
        "--interaction-validation",
        default="",
        help="Cycle interaction validation summary (required with --record)",
    )
    parser.add_argument(
        "--changed-files",
        default="",
        help="Comma-separated changed files for this cycle (required by strict proof mode)",
    )
    parser.add_argument(
        "--visual-artifacts",
        default="",
        help="Comma-separated extra visual artifact paths (diff images, notes, etc.)",
    )
    parser.add_argument(
        "--skip-proof-validation",
        action="store_true",
        help="Allow recording cycles without runtime proof (not recommended)",
    )
    parser.add_argument(
        "--min-unique-change-cycles",
        type=int,
        default=None,
        help="Override required minimum unique code-change digests for PASS",
    )
    parser.add_argument(
        "--min-visual-delta-cycles",
        type=int,
        default=None,
        help="Override required minimum visual digest transitions for PASS",
    )
    parser.add_argument("--result", default="PASS", choices=["PASS", "FAIL", "pass", "fail"], help="Cycle result with --record")
    args = parser.parse_args()

    widget = normalize_widget_name(args.widget)
    iteration_dir = Path(args.iteration_dir) if args.iteration_dir.strip() else default_iteration_dir(widget)
    report_dir = Path(args.report_dir) if args.report_dir.strip() else default_report_dir(widget)
    iteration_path = iteration_dir / f"{widget}.json"
    report_path = report_dir / f"{widget}.json"
    required_min_cycles = args.min_cycles if args.min_cycles is not None else DEFAULT_MIN_CYCLES

    created = False
    try:
        if args.init:
            data, created = ensure_template(iteration_path, widget, required_min_cycles, overwrite=args.overwrite)
        elif iteration_path.exists():
            data, _ = ensure_template(iteration_path, widget, required_min_cycles, overwrite=False)
        else:
            data, created = ensure_template(iteration_path, widget, required_min_cycles, overwrite=False)
            created = True
    except ValueError as exc:
        print(f"[FAIL] {exc}")
        return 1

    if created:
        print(f"[OK] Iteration template: {iteration_path}")

    if args.record:
        try:
            cycle = append_cycle(
                data,
                widget,
                args.goal,
                args.changes,
                args.verification,
                args.visual_validation,
                args.interaction_validation,
                args.result,
                parse_path_list(args.changed_files),
                parse_path_list(args.visual_artifacts),
                args.skip_proof_validation,
            )
        except ValueError as exc:
            print(f"[FAIL] {exc}")
            return 1
        write_iteration_file(iteration_path, data)
        print(f"[OK] Iteration cycle recorded: index={cycle['index']} result={cycle['result']}")
    elif args.init:
        write_iteration_file(iteration_path, data)

    result = evaluate_cycles(
        data,
        min_cycles_override=args.min_cycles,
        min_unique_change_cycles=args.min_unique_change_cycles,
        min_visual_delta_cycles=args.min_visual_delta_cycles,
    )
    write_report(report_path, widget, iteration_path, result)
    print(f"[INFO] Iteration gate report: {report_path}")
    print(
        f"[INFO] Cycles: completed={result['completed_cycles']}/{result['required_min_cycles']} "
        f"latest={result['latest_result']} unique_changes={result['unique_change_cycles']}/{result['required_min_unique_change_cycles']} "
        f"visual_deltas={result['visual_delta_cycles']}/{result['required_min_visual_delta_cycles']}"
    )

    if result["passed"]:
        print("[PASS] Iteration gate passed.")
        return 0

    if result["reasons"]:
        print("[HOLD] Iteration gate blocked: " + ", ".join(result["reasons"]))
    else:
        print("[HOLD] Iteration gate blocked.")
    return 2


if __name__ == "__main__":
    raise SystemExit(main())

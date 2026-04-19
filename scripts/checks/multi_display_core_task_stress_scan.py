#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Scan multi-display core-task capacity/stress combinations."""

import argparse
import json
import re
import subprocess
import sys
import time
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
ROOT_DIR = SCRIPT_DIR.parent.parent


SCAN_PROFILES = {
    "retry-delay": {
        "capacities": "1",
        "bursts": "8",
        "retry_counts": "0,1,2",
        "retry_delay_values": "0,1,2",
        "stress_post_gap_values": "1",
    },
    "delay-gap": {
        "capacities": "1",
        "bursts": "8",
        "retry_counts": "1",
        "retry_delay_values": "0,1,2",
        "stress_post_gap_values": "0,1,2",
    },
    "retry-gap": {
        "capacities": "1",
        "bursts": "8",
        "retry_counts": "0,1,2",
        "retry_delay_values": "1",
        "stress_post_gap_values": "0,1,2",
    },
}

RESULT_APP_PATTERN = re.compile(r"^\[(?:PASS|FAIL)\]\s+([A-Za-z0-9_]+)\s+-")
SHUTDOWN_QUEUE_DETAIL_PATTERN = re.compile(
    r"disp(\d+)\(posted=(\d+),retries=(\d+),max_retry_burst=(\d+),peak=(\d+)/(\d+)(?:,queue_wait=(\d+)@([^,@)]+),exec=(\d+)@([^,@)]+))?\)"
)
FAILURE_DISPLAY_PATTERN = re.compile(r"\bon display\s+(\d+)\b")
SHUTDOWN_QUEUE_WAIT_CONTEXT_PATTERN = re.compile(r"core task queue wait exceeded on display \d+: \d+ms > \d+ms \(([^()]+)\)")
SHUTDOWN_EXEC_TIME_CONTEXT_PATTERN = re.compile(r"core task exec time exceeded on display \d+: \d+ms > \d+ms \(([^()]+)\)")


def parse_int_csv(value, allow_zero=False):
    items = []
    for part in (value or "").split(","):
        part = part.strip()
        if not part:
            continue
        number = int(part)
        if allow_zero:
            if number < 0:
                raise ValueError("values must be >= 0")
        else:
            if number <= 0:
                raise ValueError("values must be > 0")
        items.append(number)
    if not items:
        raise ValueError("empty value list")
    return items


def normalize_scan_values(values):
    return sorted(set(values))


def format_command(cmd):
    return " ".join(cmd)


def build_empty_shutdown_rollup():
    return {
        "shutdown_queue_metrics": {},
        "shutdown_peak_queue": None,
        "shutdown_peak_queue_display": None,
        "shutdown_peak_queue_capacity": None,
        "shutdown_max_queue_wait_ms": None,
        "shutdown_max_queue_wait_display": None,
        "shutdown_max_queue_wait_context": "",
        "shutdown_max_exec_time_ms": None,
        "shutdown_max_exec_time_display": None,
        "shutdown_max_exec_time_context": "",
    }


def build_shutdown_rollup_from_text(detail):
    rollup = build_empty_shutdown_rollup()
    for match in SHUTDOWN_QUEUE_DETAIL_PATTERN.finditer(detail or ""):
        display_id = int(match.group(1))
        metrics = {
            "posted": int(match.group(2)),
            "retries": int(match.group(3)),
            "max_retry_burst": int(match.group(4)),
            "peak_queue": int(match.group(5)),
            "queue_capacity": int(match.group(6)),
            "max_queue_wait_ms": int(match.group(7) or 0),
            "max_queue_wait_context": match.group(8) or "none",
            "max_exec_time_ms": int(match.group(9) or 0),
            "max_exec_time_context": match.group(10) or "none",
        }
        rollup["shutdown_queue_metrics"][display_id] = metrics

        if rollup["shutdown_peak_queue"] is None or metrics["peak_queue"] > rollup["shutdown_peak_queue"]:
            rollup["shutdown_peak_queue"] = metrics["peak_queue"]
            rollup["shutdown_peak_queue_display"] = display_id
            rollup["shutdown_peak_queue_capacity"] = metrics["queue_capacity"]

        if rollup["shutdown_max_queue_wait_ms"] is None or metrics["max_queue_wait_ms"] > rollup["shutdown_max_queue_wait_ms"]:
            rollup["shutdown_max_queue_wait_ms"] = metrics["max_queue_wait_ms"]
            rollup["shutdown_max_queue_wait_display"] = display_id
            rollup["shutdown_max_queue_wait_context"] = metrics["max_queue_wait_context"]

        if rollup["shutdown_max_exec_time_ms"] is None or metrics["max_exec_time_ms"] > rollup["shutdown_max_exec_time_ms"]:
            rollup["shutdown_max_exec_time_ms"] = metrics["max_exec_time_ms"]
            rollup["shutdown_max_exec_time_display"] = display_id
            rollup["shutdown_max_exec_time_context"] = metrics["max_exec_time_context"]
    return rollup


def extract_shutdown_rollups_by_app(detail):
    rollups = {}
    for segment in (detail or "").split(" | "):
        stripped = segment.strip()
        app_match = RESULT_APP_PATTERN.match(stripped)
        if app_match is None:
            continue
        app_name = app_match.group(1)
        rollup = build_shutdown_rollup_from_text(stripped)
        if rollup["shutdown_queue_metrics"]:
            rollups[app_name] = rollup

    if rollups:
        return rollups

    fallback_rollup = build_shutdown_rollup_from_text(detail)
    if fallback_rollup["shutdown_queue_metrics"]:
        rollups[""] = fallback_rollup
    return rollups


def select_shutdown_rollup(rollups_by_app, app_name):
    if app_name and app_name in rollups_by_app:
        return rollups_by_app[app_name]
    if len(rollups_by_app) == 1:
        return next(iter(rollups_by_app.values()))
    return build_empty_shutdown_rollup()


def build_prefixed_shutdown_fields(prefix, rollup):
    return {
        "%s_shutdown_queue_metrics" % prefix: rollup["shutdown_queue_metrics"],
        "%s_shutdown_peak_queue" % prefix: rollup["shutdown_peak_queue"],
        "%s_shutdown_peak_queue_display" % prefix: rollup["shutdown_peak_queue_display"],
        "%s_shutdown_peak_queue_capacity" % prefix: rollup["shutdown_peak_queue_capacity"],
        "%s_shutdown_max_queue_wait_ms" % prefix: rollup["shutdown_max_queue_wait_ms"],
        "%s_shutdown_max_queue_wait_display" % prefix: rollup["shutdown_max_queue_wait_display"],
        "%s_shutdown_max_queue_wait_context" % prefix: rollup["shutdown_max_queue_wait_context"],
        "%s_shutdown_max_exec_time_ms" % prefix: rollup["shutdown_max_exec_time_ms"],
        "%s_shutdown_max_exec_time_display" % prefix: rollup["shutdown_max_exec_time_display"],
        "%s_shutdown_max_exec_time_context" % prefix: rollup["shutdown_max_exec_time_context"],
    }


def collect_runtime_summary(output_text):
    lines = []
    for line in output_text.splitlines():
        stripped = line.strip()
        if stripped.startswith("[PASS] ") or stripped.startswith("[FAIL] "):
            lines.append(stripped)
    return " | ".join(lines)


def normalize_failure_line(stripped):
    if "runtime self-check failed:" in stripped:
        return stripped.replace("runtime self-check failed: ", "")
    return stripped


def extract_failure_guardrail(detail):
    normalized = (detail or "").lower()
    if "core task queue wait exceeded on display" in normalized:
        return "shutdown_queue_wait"
    if "core task exec time exceeded on display" in normalized:
        return "shutdown_exec_time"
    if "core task max single-post retries exceeded on display" in normalized:
        return "max_single_post_retry"
    if "core task retries exceeded on display" in normalized:
        return "max_core_task_retries"
    if "core task queue saturated on display" in normalized:
        return "full_queue_guard"
    return ""


def extract_failure_guardrail_display(detail):
    match = FAILURE_DISPLAY_PATTERN.search(detail or "")
    if match:
        return int(match.group(1))
    return None


def extract_failure_guardrail_context(detail):
    guardrail = extract_failure_guardrail(detail)
    if guardrail == "shutdown_queue_wait":
        match = SHUTDOWN_QUEUE_WAIT_CONTEXT_PATTERN.search(detail or "")
    elif guardrail == "shutdown_exec_time":
        match = SHUTDOWN_EXEC_TIME_CONTEXT_PATTERN.search(detail or "")
    else:
        match = None
    if match:
        return match.group(1)
    return ""


def extract_failure_app(detail):
    match = re.search(r"\[FAIL\]\s+([A-Za-z0-9_]+)\s+-", detail or "")
    if match:
        return match.group(1)
    return ""


def extract_failure_stage(detail):
    match = re.search(r"\bduring\s+([A-Za-z0-9_]+)\s+at\s+\d+/\d+", detail or "")
    if match:
        return match.group(1)
    return ""


def extract_failure_progress(detail):
    match = re.search(r"\bat\s+(\d+/\d+)", detail or "")
    if match:
        return match.group(1)
    return ""


def extract_failure_inflight_context(detail):
    match = re.search(r"\binflight=\d+\(([A-Za-z0-9_]+)\)", detail or "")
    if match:
        return match.group(1)
    return ""


def extract_failure_pending_context(detail):
    match = re.search(r"\bpending_ctx=([A-Za-z0-9_]+)", detail or "")
    if match:
        return match.group(1)
    return ""


def classify_failure_contention(detail):
    stage = extract_failure_stage(detail)
    inflight_context = extract_failure_inflight_context(detail)
    pending_context = extract_failure_pending_context(detail)

    if inflight_context:
        if stage and inflight_context == stage:
            return "self_inflight"
        return "foreign_inflight"
    if pending_context:
        if stage and pending_context == stage:
            return "self_pending"
        return "foreign_pending"
    return ""


def parse_progress_step(progress):
    match = re.fullmatch(r"(\d+)/(\d+)", progress or "")
    if not match:
        return None, None
    return int(match.group(1)), int(match.group(2))


def parse_progress_range(progress_range):
    match = re.fullmatch(r"(\d+)/(\d+)\.\.(\d+)/(\d+)", progress_range or "")
    if not match:
        return None, None, None
    min_step = int(match.group(1))
    first_total = int(match.group(2))
    max_step = int(match.group(3))
    second_total = int(match.group(4))
    if first_total != second_total:
        return None, None, None
    return min_step, max_step, first_total


def progress_matches_range(progress_step, progress_total, progress_range):
    min_step, max_step, total = parse_progress_range(progress_range)
    if min_step is None or max_step is None or total is None:
        return False
    if progress_step is None or progress_total is None:
        return False
    return progress_total == total and min_step <= progress_step <= max_step


def progress_ranges_overlap(min_step, max_step, total_steps, progress_range):
    target_min, target_max, target_total = parse_progress_range(progress_range)
    if target_min is None or target_max is None or target_total is None:
        return False
    if min_step is None or max_step is None or total_steps is None:
        return False
    if total_steps != target_total:
        return False
    return not (max_step < target_min or min_step > target_max)


def validate_progress_text(progress, option_name):
    step, total = parse_progress_step(progress)
    if step is None or total is None:
        raise ValueError("%s must use STEP/TOTAL format, e.g. 5/8" % option_name)
    if step < 1 or total < 1 or step > total:
        raise ValueError("%s must satisfy 1 <= STEP <= TOTAL, got %s" % (option_name, progress))


def validate_progress_range_text(progress_range, option_name):
    min_step, max_step, total = parse_progress_range(progress_range)
    if min_step is None or max_step is None or total is None:
        raise ValueError("%s must use START/TOTAL..END/TOTAL format, e.g. 4/8..6/8" % option_name)
    if min_step < 1 or max_step < 1 or total < 1 or min_step > max_step or max_step > total:
        raise ValueError("%s must satisfy 1 <= START <= END <= TOTAL, got %s" % (option_name, progress_range))


def collect_failure_reason(output_text):
    rejected_lines = []
    retry_lines = []
    max_single_post_retry_lines = []
    saturation_lines = []
    runtime_check_fail_lines = []
    for line in output_text.splitlines():
        stripped = line.strip()
        if "core task rejected on display" in stripped:
            rejected_lines.append(stripped)
        if "core task retries exceeded on display" in stripped:
            retry_lines.append(stripped)
        if "core task max single-post retries exceeded on display" in stripped:
            max_single_post_retry_lines.append(stripped)
        if "core task queue saturated on display" in stripped:
            saturation_lines.append(stripped)
        if "[RUNTIME_CHECK_FAIL]" in stripped:
            runtime_check_fail_lines.append(normalize_failure_line(stripped))
    if max_single_post_retry_lines:
        return max_single_post_retry_lines[-1]
    if retry_lines:
        return retry_lines[-1]
    if saturation_lines:
        return saturation_lines[-1]
    if rejected_lines:
        return rejected_lines[-1]
    if runtime_check_fail_lines:
        return runtime_check_fail_lines[-1]

    for line in reversed(output_text.splitlines()):
        stripped = line.strip()
        if stripped.startswith("[FAIL] "):
            return normalize_failure_line(stripped)
    for line in reversed(output_text.splitlines()):
        stripped = line.strip()
        if stripped.startswith("Result: "):
            return stripped
    return "command failed"


def should_retry_failure(stage, reason):
    normalized = (reason or "").lower()
    transient_runtime_markers = (
        "sub page sync did not settle before",
        "failed to query sub tick before",
        "failed to query sub tick after",
    )

    if stage == "runtime":
        for marker in transient_runtime_markers:
            if marker in normalized:
                return True

    if "core task rejected on display" in normalized:
        return False
    if "core task retries exceeded on display" in normalized:
        return False
    if "core task max single-post retries exceeded on display" in normalized:
        return False
    if "core task queue saturated on display" in normalized:
        return False
    if "[runtime_check_fail]" in normalized:
        return False
    if stage == "runtime" and normalized.startswith("[fail] "):
        return False

    return True


def is_pass(item):
    return item["compile_ok"] and item["runtime_ok"]


def classify_result_failure_kind(item):
    if item["compile_ok"] and item["runtime_ok"]:
        return "none"
    if not item["compile_ok"]:
        return "compile"
    return "runtime"


def find_result(results,
                capacity,
                burst,
                post_retry_count,
                post_retry_delay_ms,
                stress_post_gap_ms,
                max_core_task_retries,
                max_single_post_core_task_retries,
                fail_on_full_core_task_queue,
                max_shutdown_queue_wait_ms=None,
                max_shutdown_exec_time_ms=None):
    return next(
        (
            item
            for item in results
            if item["capacity"] == capacity and item["burst"] == burst and item["post_retry_count"] == post_retry_count
            and item["post_retry_delay_ms"] == post_retry_delay_ms
            and item["stress_post_gap_ms"] == stress_post_gap_ms
            and item["max_core_task_retries"] == max_core_task_retries
            and item["max_single_post_core_task_retries"] == max_single_post_core_task_retries
            and item["fail_on_full_core_task_queue"] == fail_on_full_core_task_queue
            and (max_shutdown_queue_wait_ms is None or item["max_shutdown_queue_wait_ms"] == max_shutdown_queue_wait_ms)
            and (max_shutdown_exec_time_ms is None or item["max_shutdown_exec_time_ms"] == max_shutdown_exec_time_ms)
        ),
        None,
    )


def format_retry_label(post_retry_count):
    if post_retry_count < 0:
        return "default"
    return str(post_retry_count)


def format_retry_delay_label(post_retry_delay_ms):
    if post_retry_delay_ms < 0:
        return "default"
    return str(post_retry_delay_ms)


def format_stress_post_gap_label(stress_post_gap_ms):
    if stress_post_gap_ms < 0:
        return "default"
    return str(stress_post_gap_ms)


def format_max_single_post_retry_limit_label(max_single_post_core_task_retries):
    if max_single_post_core_task_retries < 0:
        return "default"
    return str(max_single_post_core_task_retries)


def format_max_core_task_retry_limit_label(max_core_task_retries):
    if max_core_task_retries < 0:
        return "default"
    return str(max_core_task_retries)


def format_full_queue_guard_label(fail_on_full_core_task_queue):
    return "on" if fail_on_full_core_task_queue else "off"


def format_shutdown_guardrail_label(guardrail_value):
    if guardrail_value < 0:
        return "default"
    return str(guardrail_value)


def format_optional_guardrail_suffix(args):
    parts = []
    if getattr(args, "max_shutdown_queue_wait_ms", -1) >= 0:
        parts.append("max_shutdown_queue_wait_ms=%d" % args.max_shutdown_queue_wait_ms)
    if getattr(args, "max_shutdown_exec_time_ms", -1) >= 0:
        parts.append("max_shutdown_exec_time_ms=%d" % args.max_shutdown_exec_time_ms)
    if not parts:
        return ""
    return " " + " ".join(parts)


def format_runtime_guardrail_config_suffix(max_shutdown_queue_wait_ms, max_shutdown_exec_time_ms):
    return " max_shutdown_queue_wait_ms=%s max_shutdown_exec_time_ms=%s" % (
        format_shutdown_guardrail_label(max_shutdown_queue_wait_ms),
        format_shutdown_guardrail_label(max_shutdown_exec_time_ms),
    )


def has_fixed_runtime_guardrail_values(max_shutdown_queue_wait_values, max_shutdown_exec_time_values):
    return len(max_shutdown_queue_wait_values) == 1 and len(max_shutdown_exec_time_values) == 1


def print_runtime_guardrail_grid_skip(title):
    print("\n=== %s ===" % title)
    print("Skipped: multiple shutdown guardrail values in scan range.")


def build_scan_combination_key(capacity,
                               burst,
                               retry,
                               delay,
                               gap,
                               max_core_task_retries,
                               max_single_post_retry,
                               full_queue_guard,
                               max_shutdown_queue_wait_ms=-1,
                               max_shutdown_exec_time_ms=-1):
    return "capacity=%d burst=%d retry=%s delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s max_shutdown_queue_wait_ms=%s max_shutdown_exec_time_ms=%s" % (
        capacity,
        burst,
        format_retry_label(retry),
        format_retry_delay_label(delay),
        format_stress_post_gap_label(gap),
        format_max_core_task_retry_limit_label(max_core_task_retries),
        format_max_single_post_retry_limit_label(max_single_post_retry),
        format_full_queue_guard_label(full_queue_guard),
        format_shutdown_guardrail_label(max_shutdown_queue_wait_ms),
        format_shutdown_guardrail_label(max_shutdown_exec_time_ms),
    )


def build_per_app_scope_key(app_name, capacity, burst, max_shutdown_queue_wait_ms, max_shutdown_exec_time_ms):
    return "app=%s capacity=%d burst=%d max_shutdown_queue_wait_ms=%s max_shutdown_exec_time_ms=%s" % (
        app_name,
        capacity,
        burst,
        format_shutdown_guardrail_label(max_shutdown_queue_wait_ms),
        format_shutdown_guardrail_label(max_shutdown_exec_time_ms),
    )


def parse_scope_guardrail_value_text(value_text, option_name, field_name):
    normalized_value_text = (value_text or "").strip().lower()
    if normalized_value_text in ("default", "-1"):
        return -1
    try:
        guardrail_value = int(normalized_value_text)
    except ValueError as exc:
        raise ValueError("%s %s must be an integer or 'default'" % (option_name, field_name)) from exc
    if guardrail_value < -1:
        raise ValueError("%s %s must be >= -1" % (option_name, field_name))
    return guardrail_value


def parse_scope_guardrail_key_text(scope_guardrail_key, option_name):
    normalized_scope_guardrail_key = " ".join((scope_guardrail_key or "").strip().split())
    if not normalized_scope_guardrail_key:
        return None
    match = re.fullmatch(
        r"max_shutdown_queue_wait_ms=(\S+)\s+max_shutdown_exec_time_ms=(\S+)",
        normalized_scope_guardrail_key,
    )
    if not match:
        raise ValueError(
            "%s must look like \"max_shutdown_queue_wait_ms=default max_shutdown_exec_time_ms=0\"" % option_name
        )
    max_shutdown_queue_wait_ms = parse_scope_guardrail_value_text(
        match.group(1),
        option_name,
        "max_shutdown_queue_wait_ms",
    )
    max_shutdown_exec_time_ms = parse_scope_guardrail_value_text(
        match.group(2),
        option_name,
        "max_shutdown_exec_time_ms",
    )
    return {
        "max_shutdown_queue_wait_ms": max_shutdown_queue_wait_ms,
        "max_shutdown_exec_time_ms": max_shutdown_exec_time_ms,
        "text": format_runtime_guardrail_config_suffix(
            max_shutdown_queue_wait_ms,
            max_shutdown_exec_time_ms,
        ).strip(),
    }


def parse_per_app_scope_key_text(scope_key, option_name):
    normalized_scope_key = " ".join((scope_key or "").strip().split())
    if not normalized_scope_key:
        return None
    match = re.fullmatch(
        r"app=(\S+)\s+capacity=(\d+)\s+burst=(\d+)\s+max_shutdown_queue_wait_ms=(\S+)\s+max_shutdown_exec_time_ms=(\S+)",
        normalized_scope_key,
    )
    if not match:
        raise ValueError(
            "%s must look like \"app=HelloMultiDisplayHetero capacity=32 burst=1 max_shutdown_queue_wait_ms=default max_shutdown_exec_time_ms=0\"" % option_name
        )
    capacity = int(match.group(2))
    burst = int(match.group(3))
    if capacity <= 0:
        raise ValueError("%s capacity must be > 0" % option_name)
    if burst < 0:
        raise ValueError("%s burst must be >= 0" % option_name)
    max_shutdown_queue_wait_ms = parse_scope_guardrail_value_text(
        match.group(4),
        option_name,
        "max_shutdown_queue_wait_ms",
    )
    max_shutdown_exec_time_ms = parse_scope_guardrail_value_text(
        match.group(5),
        option_name,
        "max_shutdown_exec_time_ms",
    )
    return {
        "app": match.group(1),
        "capacity": capacity,
        "burst": burst,
        "max_shutdown_queue_wait_ms": max_shutdown_queue_wait_ms,
        "max_shutdown_exec_time_ms": max_shutdown_exec_time_ms,
        "text": build_per_app_scope_key(
            match.group(1),
            capacity,
            burst,
            max_shutdown_queue_wait_ms,
            max_shutdown_exec_time_ms,
        ),
    }


def validate_and_normalize_per_app_threshold_scope_filters(args):
    parsed_scope_key = parse_per_app_scope_key_text(
        args.per_app_thresholds_scope_key,
        "--per-app-thresholds-scope-key",
    )
    parsed_scope_guardrail_key = parse_scope_guardrail_key_text(
        args.per_app_thresholds_scope_guardrail_key,
        "--per-app-thresholds-scope-guardrail-key",
    )

    if parsed_scope_key is not None:
        args.per_app_thresholds_scope_key = parsed_scope_key["text"]
    if parsed_scope_guardrail_key is not None:
        args.per_app_thresholds_scope_guardrail_key = parsed_scope_guardrail_key["text"]

    if parsed_scope_key is not None:
        if args.app and parsed_scope_key["app"] != args.app:
            raise ValueError(
                "--per-app-thresholds-scope-key app=%s conflicts with --app %s" % (
                    parsed_scope_key["app"],
                    args.app,
                )
            )
        if args.per_app_thresholds_capacity is not None and parsed_scope_key["capacity"] != args.per_app_thresholds_capacity:
            raise ValueError(
                "--per-app-thresholds-scope-key capacity=%d conflicts with --per-app-thresholds-capacity %d" % (
                    parsed_scope_key["capacity"],
                    args.per_app_thresholds_capacity,
                )
            )
        if args.per_app_thresholds_burst is not None and parsed_scope_key["burst"] != args.per_app_thresholds_burst:
            raise ValueError(
                "--per-app-thresholds-scope-key burst=%d conflicts with --per-app-thresholds-burst %d" % (
                    parsed_scope_key["burst"],
                    args.per_app_thresholds_burst,
                )
            )
        if (args.per_app_thresholds_scope_shutdown_queue_wait_ms is not None
                and parsed_scope_key["max_shutdown_queue_wait_ms"] != args.per_app_thresholds_scope_shutdown_queue_wait_ms):
            raise ValueError(
                "--per-app-thresholds-scope-key max_shutdown_queue_wait_ms=%s conflicts with --per-app-thresholds-scope-shutdown-queue-wait-ms %s" % (
                    format_shutdown_guardrail_label(parsed_scope_key["max_shutdown_queue_wait_ms"]),
                    format_shutdown_guardrail_label(args.per_app_thresholds_scope_shutdown_queue_wait_ms),
                )
            )
        if (args.per_app_thresholds_scope_shutdown_exec_time_ms is not None
                and parsed_scope_key["max_shutdown_exec_time_ms"] != args.per_app_thresholds_scope_shutdown_exec_time_ms):
            raise ValueError(
                "--per-app-thresholds-scope-key max_shutdown_exec_time_ms=%s conflicts with --per-app-thresholds-scope-shutdown-exec-time-ms %s" % (
                    format_shutdown_guardrail_label(parsed_scope_key["max_shutdown_exec_time_ms"]),
                    format_shutdown_guardrail_label(args.per_app_thresholds_scope_shutdown_exec_time_ms),
                )
            )

    if parsed_scope_guardrail_key is not None:
        if (args.per_app_thresholds_scope_shutdown_queue_wait_ms is not None
                and parsed_scope_guardrail_key["max_shutdown_queue_wait_ms"] != args.per_app_thresholds_scope_shutdown_queue_wait_ms):
            raise ValueError(
                "--per-app-thresholds-scope-guardrail-key max_shutdown_queue_wait_ms=%s conflicts with --per-app-thresholds-scope-shutdown-queue-wait-ms %s" % (
                    format_shutdown_guardrail_label(parsed_scope_guardrail_key["max_shutdown_queue_wait_ms"]),
                    format_shutdown_guardrail_label(args.per_app_thresholds_scope_shutdown_queue_wait_ms),
                )
            )
        if (args.per_app_thresholds_scope_shutdown_exec_time_ms is not None
                and parsed_scope_guardrail_key["max_shutdown_exec_time_ms"] != args.per_app_thresholds_scope_shutdown_exec_time_ms):
            raise ValueError(
                "--per-app-thresholds-scope-guardrail-key max_shutdown_exec_time_ms=%s conflicts with --per-app-thresholds-scope-shutdown-exec-time-ms %s" % (
                    format_shutdown_guardrail_label(parsed_scope_guardrail_key["max_shutdown_exec_time_ms"]),
                    format_shutdown_guardrail_label(args.per_app_thresholds_scope_shutdown_exec_time_ms),
                )
            )

    if parsed_scope_key is not None and parsed_scope_guardrail_key is not None:
        if parsed_scope_key["max_shutdown_queue_wait_ms"] != parsed_scope_guardrail_key["max_shutdown_queue_wait_ms"]:
            raise ValueError(
                "--per-app-thresholds-scope-key max_shutdown_queue_wait_ms=%s conflicts with --per-app-thresholds-scope-guardrail-key %s" % (
                    format_shutdown_guardrail_label(parsed_scope_key["max_shutdown_queue_wait_ms"]),
                    parsed_scope_guardrail_key["text"],
                )
            )
        if parsed_scope_key["max_shutdown_exec_time_ms"] != parsed_scope_guardrail_key["max_shutdown_exec_time_ms"]:
            raise ValueError(
                "--per-app-thresholds-scope-key max_shutdown_exec_time_ms=%s conflicts with --per-app-thresholds-scope-guardrail-key %s" % (
                    format_shutdown_guardrail_label(parsed_scope_key["max_shutdown_exec_time_ms"]),
                    parsed_scope_guardrail_key["text"],
                )
            )


def summarize_result_cell(item):
    if item is None:
        return "NA"
    if is_pass(item):
        return "PASS"

    app = extract_failure_app(item["detail"])
    contention = classify_failure_contention(item["detail"])
    stage = extract_failure_stage(item["detail"])
    prefix = "FAIL"
    if app:
        prefix += ":%s" % app
    if contention and stage:
        return "%s:%s@%s" % (prefix, contention, stage)
    if contention:
        return "%s:%s" % (prefix, contention)
    if stage:
        return "%s:%s" % (prefix, stage)
    return prefix


def summarize_result_cell_for_app(item):
    if item is None:
        return "NA"
    if is_pass(item):
        return "PASS"

    contention = classify_failure_contention(item["detail"])
    stage = extract_failure_stage(item["detail"])
    if contention and stage:
        return "FAIL:%s@%s" % (contention, stage)
    if contention:
        return "FAIL:%s" % contention
    if stage:
        return "FAIL:%s" % stage
    return "FAIL"


def build_result_sort_key(item):
    return (
        item["post_retry_count"],
        item["post_retry_delay_ms"],
        item["stress_post_gap_ms"],
        item["max_core_task_retries"],
        item["max_single_post_core_task_retries"],
        1 if item["fail_on_full_core_task_queue"] else 0,
        item["max_shutdown_queue_wait_ms"],
        item["max_shutdown_exec_time_ms"],
    )


def build_result_combination_rank_map(results):
    sorted_keys = sorted({build_result_sort_key(item) for item in results})
    return {sort_key: index for index, sort_key in enumerate(sorted_keys)}


def build_dominant_stage_info(items):
    stage_stats = {}
    for item in items:
        if is_pass(item):
            continue
        stage = extract_failure_stage(item["detail"])
        if not stage:
            continue
        progress = extract_failure_progress(item["detail"])
        current_step, total_steps = parse_progress_step(progress)
        stats = stage_stats.setdefault(stage, {"count": 0, "min_step": None, "max_step": None, "total_steps": None})
        stats["count"] += 1
        if current_step is None or total_steps is None:
            continue
        if stats["min_step"] is None or current_step < stats["min_step"]:
            stats["min_step"] = current_step
        if stats["max_step"] is None or current_step > stats["max_step"]:
            stats["max_step"] = current_step
        if stats["total_steps"] is None:
            stats["total_steps"] = total_steps

    dominant_stage = ""
    dominant_metrics = None
    for stage in sorted(stage_stats):
        stats = stage_stats[stage]
        sort_key = (stats["count"], stats["max_step"] or -1, stats["min_step"] or -1)
        if dominant_metrics is None or sort_key > dominant_metrics:
            dominant_metrics = sort_key
            dominant_stage = stage
    if not dominant_stage:
        return {"name": "", "count": 0, "progress_range": "", "progress_min_step": None, "progress_max_step": None, "progress_total": None}

    stats = stage_stats[dominant_stage]
    progress_text = ""
    if stats["min_step"] is not None and stats["max_step"] is not None and stats["total_steps"] is not None:
        progress_text = "%d/%d..%d/%d" % (
            stats["min_step"],
            stats["total_steps"],
            stats["max_step"],
            stats["total_steps"],
        )
    return {
        "name": dominant_stage,
        "count": stats["count"],
        "progress_range": progress_text,
        "progress_min_step": stats["min_step"],
        "progress_max_step": stats["max_step"],
        "progress_total": stats["total_steps"],
    }


def build_dominant_contention_info(items):
    contention_stats = {}
    for item in items:
        if is_pass(item):
            continue
        contention = classify_failure_contention(item["detail"])
        if not contention:
            continue
        contention_stats[contention] = contention_stats.get(contention, 0) + 1

    dominant_contention = ""
    dominant_count = None
    for contention in sorted(contention_stats):
        count = contention_stats[contention]
        if dominant_count is None or count > dominant_count:
            dominant_count = count
            dominant_contention = contention
    if not dominant_contention:
        return {"name": "", "count": 0}
    return {
        "name": dominant_contention,
        "count": contention_stats[dominant_contention],
    }


def build_dominant_guardrail_info(items):
    guardrail_stats = {}
    for item in items:
        if is_pass(item):
            continue
        guardrail = extract_failure_guardrail(item["detail"])
        if not guardrail:
            continue
        guardrail_stats[guardrail] = guardrail_stats.get(guardrail, 0) + 1

    dominant_guardrail = ""
    dominant_count = None
    for guardrail in sorted(guardrail_stats):
        count = guardrail_stats[guardrail]
        if dominant_count is None or count > dominant_count:
            dominant_count = count
            dominant_guardrail = guardrail
    if not dominant_guardrail:
        return {"name": "", "count": 0}
    return {
        "name": dominant_guardrail,
        "count": guardrail_stats[dominant_guardrail],
    }


def build_per_app_threshold_entries(app_name, results, capacities, bursts, shared_fields=None):
    entries = []
    if not app_name:
        return entries

    combination_rank_map = build_result_combination_rank_map(results)
    scope_tuples = sorted(
        {
            (
                item["capacity"],
                item["burst"],
                item["max_shutdown_queue_wait_ms"],
                item["max_shutdown_exec_time_ms"],
            )
            for item in results
        }
    )
    scope_rank_map = {scope_tuple: index for index, scope_tuple in enumerate(scope_tuples)}
    for capacity_index, capacity in enumerate(capacities):
        for burst_index, burst in enumerate(bursts):
            guardrail_scope_tuples = sorted(
                {
                    (
                        item["max_shutdown_queue_wait_ms"],
                        item["max_shutdown_exec_time_ms"],
                    )
                    for item in results
                    if item["capacity"] == capacity and item["burst"] == burst
                }
            )
            for max_shutdown_queue_wait_ms, max_shutdown_exec_time_ms in guardrail_scope_tuples:
                scoped_items = [
                    item for item in results
                    if item["capacity"] == capacity
                    and item["burst"] == burst
                    and item["max_shutdown_queue_wait_ms"] == max_shutdown_queue_wait_ms
                    and item["max_shutdown_exec_time_ms"] == max_shutdown_exec_time_ms
                ]
                if not scoped_items:
                    continue

                passing_items = sorted((item for item in scoped_items if is_pass(item)), key=build_result_sort_key)
                pass_count = len(passing_items)
                fail_count = len(scoped_items) - pass_count
                result_count = len(scoped_items)
                dominant_stage = build_dominant_stage_info(scoped_items)
                dominant_contention = build_dominant_contention_info(scoped_items)
                dominant_guardrail = build_dominant_guardrail_info(scoped_items)
                entry = {
                    "app": app_name,
                    "capacity": capacity,
                    "burst": burst,
                    "capacity_index": capacity_index,
                    "burst_index": burst_index,
                    "scope_shutdown_queue_wait_ms": max_shutdown_queue_wait_ms,
                    "scope_shutdown_exec_time_ms": max_shutdown_exec_time_ms,
                    "scope_max_shutdown_queue_wait_ms": max_shutdown_queue_wait_ms,
                    "scope_max_shutdown_exec_time_ms": max_shutdown_exec_time_ms,
                    "scope_guardrail_key": format_runtime_guardrail_config_suffix(
                        max_shutdown_queue_wait_ms,
                        max_shutdown_exec_time_ms,
                    ).strip(),
                    "scope_key": build_per_app_scope_key(app_name, capacity, burst, max_shutdown_queue_wait_ms, max_shutdown_exec_time_ms),
                    "scope_rank": scope_rank_map[(capacity, burst, max_shutdown_queue_wait_ms, max_shutdown_exec_time_ms)],
                    "has_passing": False,
                    "result_count": result_count,
                    "pass_count": pass_count,
                    "fail_count": fail_count,
                    "all_passed": pass_count == result_count,
                    "all_failed": fail_count == result_count,
                    "minimum_passing_rank": None,
                    "dominant_stage": dominant_stage["name"],
                    "stage_failures": dominant_stage["count"],
                    "progress_range": dominant_stage["progress_range"],
                    "progress_min_step": dominant_stage["progress_min_step"],
                    "progress_max_step": dominant_stage["progress_max_step"],
                    "progress_total": dominant_stage["progress_total"],
                    "dominant_contention": dominant_contention["name"],
                    "contention_failures": dominant_contention["count"],
                    "dominant_guardrail": dominant_guardrail["name"],
                    "guardrail_failures": dominant_guardrail["count"],
                    "minimum_passing_retry": None,
                    "minimum_passing_delay": None,
                    "minimum_passing_gap": None,
                    "minimum_passing_max_core_task_retries": None,
                    "minimum_passing_max_single_post_retry": None,
                    "minimum_passing_full_queue_guard": None,
                    "minimum_passing_max_shutdown_queue_wait_ms": None,
                    "minimum_passing_max_shutdown_exec_time_ms": None,
                }
                entry.update(build_prefixed_shutdown_fields("minimum_passing", build_empty_shutdown_rollup()))
                if passing_items:
                    minimum_passing = passing_items[0]
                    minimum_passing_shutdown_rollup = select_shutdown_rollup(
                        extract_shutdown_rollups_by_app(minimum_passing["detail"]),
                        app_name,
                    )
                    entry["has_passing"] = True
                    entry["minimum_passing"] = {
                        "retry": minimum_passing["post_retry_count"],
                        "delay": minimum_passing["post_retry_delay_ms"],
                        "gap": minimum_passing["stress_post_gap_ms"],
                        "max_core_task_retries": minimum_passing["max_core_task_retries"],
                        "max_single_post_retry": minimum_passing["max_single_post_core_task_retries"],
                        "full_queue_guard": minimum_passing["fail_on_full_core_task_queue"],
                        "max_shutdown_queue_wait_ms": minimum_passing["max_shutdown_queue_wait_ms"],
                        "max_shutdown_exec_time_ms": minimum_passing["max_shutdown_exec_time_ms"],
                    }
                    entry["minimum_passing_retry"] = minimum_passing["post_retry_count"]
                    entry["minimum_passing_delay"] = minimum_passing["post_retry_delay_ms"]
                    entry["minimum_passing_gap"] = minimum_passing["stress_post_gap_ms"]
                    entry["minimum_passing_max_core_task_retries"] = minimum_passing["max_core_task_retries"]
                    entry["minimum_passing_max_single_post_retry"] = minimum_passing["max_single_post_core_task_retries"]
                    entry["minimum_passing_full_queue_guard"] = minimum_passing["fail_on_full_core_task_queue"]
                    entry["minimum_passing_max_shutdown_queue_wait_ms"] = minimum_passing["max_shutdown_queue_wait_ms"]
                    entry["minimum_passing_max_shutdown_exec_time_ms"] = minimum_passing["max_shutdown_exec_time_ms"]
                    entry["minimum_passing_rank"] = combination_rank_map[build_result_sort_key(minimum_passing)]
                    entry["minimum_passing_key"] = build_scan_combination_key(
                        capacity,
                        burst,
                        minimum_passing["post_retry_count"],
                        minimum_passing["post_retry_delay_ms"],
                        minimum_passing["stress_post_gap_ms"],
                        minimum_passing["max_core_task_retries"],
                        minimum_passing["max_single_post_core_task_retries"],
                        minimum_passing["fail_on_full_core_task_queue"],
                        minimum_passing["max_shutdown_queue_wait_ms"],
                        minimum_passing["max_shutdown_exec_time_ms"],
                    )
                    entry.update(build_prefixed_shutdown_fields("minimum_passing", minimum_passing_shutdown_rollup))
                else:
                    entry["minimum_passing"] = None
                    entry["minimum_passing_key"] = ""
                if shared_fields:
                    entry.update(shared_fields)
                entries.append(entry)
    return entries


def print_per_app_threshold_summary(app_name, results, capacities, bursts):
    if not app_name:
        return

    print("\n=== Per-App Threshold Summary ===")

    for entry in build_per_app_threshold_entries(app_name, results, capacities, bursts):
        scope_guardrail_suffix = format_runtime_guardrail_config_suffix(
            entry["scope_max_shutdown_queue_wait_ms"],
            entry["scope_max_shutdown_exec_time_ms"],
        )
        extra_parts = []
        if entry["dominant_stage"]:
            extra_parts.append("dominant_stage=%s" % entry["dominant_stage"])
            extra_parts.append("stage_failures=%d" % entry["stage_failures"])
            if entry["progress_range"]:
                extra_parts.append("progress_range=%s" % entry["progress_range"])
        if entry["dominant_contention"]:
            extra_parts.append("dominant_contention=%s" % entry["dominant_contention"])
            extra_parts.append("contention_failures=%d" % entry["contention_failures"])
        if entry["dominant_guardrail"]:
            extra_parts.append("dominant_guardrail=%s" % entry["dominant_guardrail"])
            extra_parts.append("guardrail_failures=%d" % entry["guardrail_failures"])
        extra_suffix = " " + " ".join(extra_parts) if extra_parts else ""

        if entry["minimum_passing"] is None:
            print("[app=%s capacity=%d burst=%d%s] no passing retry/delay/gap/max-core/max-single-post/full-queue/shutdown-guard config in scan range%s" % (
                entry["app"],
                entry["capacity"],
                entry["burst"],
                scope_guardrail_suffix,
                extra_suffix,
            ))
            continue

        minimum_passing = entry["minimum_passing"]
        peak_parts = []
        if entry["minimum_passing_shutdown_peak_queue"] is not None and entry["minimum_passing_shutdown_peak_queue_capacity"] is not None:
            peak_parts.append(
                "peak_queue=%d/%d@disp%d" % (
                    entry["minimum_passing_shutdown_peak_queue"],
                    entry["minimum_passing_shutdown_peak_queue_capacity"],
                    entry["minimum_passing_shutdown_peak_queue_display"],
                )
            )
        if entry["minimum_passing_shutdown_max_queue_wait_ms"] is not None:
            peak_parts.append(
                "max_queue_wait_ms=%d@disp%d:%s" % (
                    entry["minimum_passing_shutdown_max_queue_wait_ms"],
                    entry["minimum_passing_shutdown_max_queue_wait_display"],
                    entry["minimum_passing_shutdown_max_queue_wait_context"] or "none",
                )
            )
        if entry["minimum_passing_shutdown_max_exec_time_ms"] is not None:
            peak_parts.append(
                "max_exec_time_ms=%d@disp%d:%s" % (
                    entry["minimum_passing_shutdown_max_exec_time_ms"],
                    entry["minimum_passing_shutdown_max_exec_time_display"],
                    entry["minimum_passing_shutdown_max_exec_time_context"] or "none",
                )
            )
        peak_suffix = (" " + " ".join(peak_parts)) if peak_parts else ""
        print(
            "[app=%s capacity=%d burst=%d%s] minimum passing retry=%s delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s max_shutdown_queue_wait_ms=%s max_shutdown_exec_time_ms=%s%s%s" % (
                entry["app"],
                entry["capacity"],
                entry["burst"],
                scope_guardrail_suffix,
                format_retry_label(minimum_passing["retry"]),
                format_retry_delay_label(minimum_passing["delay"]),
                format_stress_post_gap_label(minimum_passing["gap"]),
                format_max_core_task_retry_limit_label(minimum_passing["max_core_task_retries"]),
                format_max_single_post_retry_limit_label(minimum_passing["max_single_post_retry"]),
                format_full_queue_guard_label(minimum_passing["full_queue_guard"]),
                format_shutdown_guardrail_label(minimum_passing["max_shutdown_queue_wait_ms"]),
                format_shutdown_guardrail_label(minimum_passing["max_shutdown_exec_time_ms"]),
                peak_suffix,
                extra_suffix,
            )
        )


def print_per_app_delay_gap_grid_summary(app_name,
                                         results,
                                         capacities,
                                         bursts,
                                         post_retry_counts,
                                         post_retry_delay_values,
                                         stress_post_gap_values,
                                         max_core_task_retry_limits,
                                         max_single_post_retry_limits,
                                         full_queue_guard_values,
                                         max_shutdown_queue_wait_values,
                                         max_shutdown_exec_time_values):
    if not app_name:
        return
    if len(capacities) != 1 or len(bursts) != 1:
        return
    if len(post_retry_counts) != 1:
        return
    if len(post_retry_delay_values) <= 1 or len(stress_post_gap_values) <= 1:
        return
    if len(max_core_task_retry_limits) != 1 or len(max_single_post_retry_limits) != 1 or len(full_queue_guard_values) != 1:
        return
    if not has_fixed_runtime_guardrail_values(max_shutdown_queue_wait_values, max_shutdown_exec_time_values):
        print_runtime_guardrail_grid_skip("Per-App Delay/Gap Grid Summary")
        return

    capacity = capacities[0]
    burst = bursts[0]
    post_retry_count = post_retry_counts[0]
    max_core_task_retries = max_core_task_retry_limits[0]
    max_single_post_core_task_retries = max_single_post_retry_limits[0]
    fail_on_full_core_task_queue = full_queue_guard_values[0]
    max_shutdown_queue_wait_ms = max_shutdown_queue_wait_values[0]
    max_shutdown_exec_time_ms = max_shutdown_exec_time_values[0]

    print("\n=== Per-App Delay/Gap Grid Summary ===")
    print(
        "[app=%s capacity=%d burst=%d retry=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s]" % (
            app_name,
            capacity,
            burst,
            format_retry_label(post_retry_count),
            format_max_core_task_retry_limit_label(max_core_task_retries),
            format_max_single_post_retry_limit_label(max_single_post_core_task_retries),
            format_full_queue_guard_label(fail_on_full_core_task_queue),
            format_runtime_guardrail_config_suffix(max_shutdown_queue_wait_ms, max_shutdown_exec_time_ms),
        )
    )

    header_parts = ["delay\\gap"]
    for stress_post_gap_ms in stress_post_gap_values:
        header_parts.append(format_stress_post_gap_label(stress_post_gap_ms))
    print(" | ".join(header_parts))

    for post_retry_delay_ms in post_retry_delay_values:
        row_parts = [format_retry_delay_label(post_retry_delay_ms)]
        for stress_post_gap_ms in stress_post_gap_values:
            matched = find_result(
                results,
                capacity,
                burst,
                post_retry_count,
                post_retry_delay_ms,
                stress_post_gap_ms,
                max_core_task_retries,
                max_single_post_core_task_retries,
                fail_on_full_core_task_queue,
                max_shutdown_queue_wait_ms,
                max_shutdown_exec_time_ms,
            )
            row_parts.append(summarize_result_cell_for_app(matched))
        print(" | ".join(row_parts))


def print_per_app_retry_delay_grid_summary(app_name,
                                           results,
                                           capacities,
                                           bursts,
                                           post_retry_counts,
                                           post_retry_delay_values,
                                           stress_post_gap_values,
                                           max_core_task_retry_limits,
                                           max_single_post_retry_limits,
                                           full_queue_guard_values,
                                           max_shutdown_queue_wait_values,
                                           max_shutdown_exec_time_values):
    if not app_name:
        return
    if len(capacities) != 1 or len(bursts) != 1:
        return
    if len(post_retry_counts) <= 1 or len(post_retry_delay_values) <= 1:
        return
    if len(stress_post_gap_values) != 1:
        return
    if len(max_core_task_retry_limits) != 1 or len(max_single_post_retry_limits) != 1 or len(full_queue_guard_values) != 1:
        return
    if not has_fixed_runtime_guardrail_values(max_shutdown_queue_wait_values, max_shutdown_exec_time_values):
        print_runtime_guardrail_grid_skip("Per-App Retry/Delay Grid Summary")
        return

    capacity = capacities[0]
    burst = bursts[0]
    stress_post_gap_ms = stress_post_gap_values[0]
    max_core_task_retries = max_core_task_retry_limits[0]
    max_single_post_core_task_retries = max_single_post_retry_limits[0]
    fail_on_full_core_task_queue = full_queue_guard_values[0]
    max_shutdown_queue_wait_ms = max_shutdown_queue_wait_values[0]
    max_shutdown_exec_time_ms = max_shutdown_exec_time_values[0]

    print("\n=== Per-App Retry/Delay Grid Summary ===")
    print(
        "[app=%s capacity=%d burst=%d gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s]" % (
            app_name,
            capacity,
            burst,
            format_stress_post_gap_label(stress_post_gap_ms),
            format_max_core_task_retry_limit_label(max_core_task_retries),
            format_max_single_post_retry_limit_label(max_single_post_core_task_retries),
            format_full_queue_guard_label(fail_on_full_core_task_queue),
            format_runtime_guardrail_config_suffix(max_shutdown_queue_wait_ms, max_shutdown_exec_time_ms),
        )
    )

    header_parts = ["retry\\delay"]
    for post_retry_delay_ms in post_retry_delay_values:
        header_parts.append(format_retry_delay_label(post_retry_delay_ms))
    print(" | ".join(header_parts))

    for post_retry_count in post_retry_counts:
        row_parts = [format_retry_label(post_retry_count)]
        for post_retry_delay_ms in post_retry_delay_values:
            matched = find_result(
                results,
                capacity,
                burst,
                post_retry_count,
                post_retry_delay_ms,
                stress_post_gap_ms,
                max_core_task_retries,
                max_single_post_core_task_retries,
                fail_on_full_core_task_queue,
                max_shutdown_queue_wait_ms,
                max_shutdown_exec_time_ms,
            )
            row_parts.append(summarize_result_cell_for_app(matched))
        print(" | ".join(row_parts))


def print_per_app_retry_gap_grid_summary(app_name,
                                         results,
                                         capacities,
                                         bursts,
                                         post_retry_counts,
                                         post_retry_delay_values,
                                         stress_post_gap_values,
                                         max_core_task_retry_limits,
                                         max_single_post_retry_limits,
                                         full_queue_guard_values,
                                         max_shutdown_queue_wait_values,
                                         max_shutdown_exec_time_values):
    if not app_name:
        return
    if len(capacities) != 1 or len(bursts) != 1:
        return
    if len(post_retry_counts) <= 1 or len(stress_post_gap_values) <= 1:
        return
    if len(post_retry_delay_values) != 1:
        return
    if len(max_core_task_retry_limits) != 1 or len(max_single_post_retry_limits) != 1 or len(full_queue_guard_values) != 1:
        return
    if not has_fixed_runtime_guardrail_values(max_shutdown_queue_wait_values, max_shutdown_exec_time_values):
        print_runtime_guardrail_grid_skip("Per-App Retry/Gap Grid Summary")
        return

    capacity = capacities[0]
    burst = bursts[0]
    post_retry_delay_ms = post_retry_delay_values[0]
    max_core_task_retries = max_core_task_retry_limits[0]
    max_single_post_core_task_retries = max_single_post_retry_limits[0]
    fail_on_full_core_task_queue = full_queue_guard_values[0]
    max_shutdown_queue_wait_ms = max_shutdown_queue_wait_values[0]
    max_shutdown_exec_time_ms = max_shutdown_exec_time_values[0]

    print("\n=== Per-App Retry/Gap Grid Summary ===")
    print(
        "[app=%s capacity=%d burst=%d delay=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s]" % (
            app_name,
            capacity,
            burst,
            format_retry_delay_label(post_retry_delay_ms),
            format_max_core_task_retry_limit_label(max_core_task_retries),
            format_max_single_post_retry_limit_label(max_single_post_core_task_retries),
            format_full_queue_guard_label(fail_on_full_core_task_queue),
            format_runtime_guardrail_config_suffix(max_shutdown_queue_wait_ms, max_shutdown_exec_time_ms),
        )
    )

    header_parts = ["retry\\gap"]
    for stress_post_gap_ms in stress_post_gap_values:
        header_parts.append(format_stress_post_gap_label(stress_post_gap_ms))
    print(" | ".join(header_parts))

    for post_retry_count in post_retry_counts:
        row_parts = [format_retry_label(post_retry_count)]
        for stress_post_gap_ms in stress_post_gap_values:
            matched = find_result(
                results,
                capacity,
                burst,
                post_retry_count,
                post_retry_delay_ms,
                stress_post_gap_ms,
                max_core_task_retries,
                max_single_post_core_task_retries,
                fail_on_full_core_task_queue,
                max_shutdown_queue_wait_ms,
                max_shutdown_exec_time_ms,
            )
            row_parts.append(summarize_result_cell_for_app(matched))
        print(" | ".join(row_parts))


def print_delay_gap_grid_summary(results,
                                 capacities,
                                 bursts,
                                 post_retry_counts,
                                 post_retry_delay_values,
                                 stress_post_gap_values,
                                 max_core_task_retry_limits,
                                 max_single_post_retry_limits,
                                 full_queue_guard_values,
                                 max_shutdown_queue_wait_values,
                                 max_shutdown_exec_time_values):
    if len(capacities) != 1 or len(bursts) != 1:
        return
    if len(post_retry_counts) != 1:
        return
    if len(post_retry_delay_values) <= 1 or len(stress_post_gap_values) <= 1:
        return
    if len(max_core_task_retry_limits) != 1 or len(max_single_post_retry_limits) != 1 or len(full_queue_guard_values) != 1:
        return
    if not has_fixed_runtime_guardrail_values(max_shutdown_queue_wait_values, max_shutdown_exec_time_values):
        print_runtime_guardrail_grid_skip("Delay/Gap Grid Summary")
        return

    capacity = capacities[0]
    burst = bursts[0]
    post_retry_count = post_retry_counts[0]
    max_core_task_retries = max_core_task_retry_limits[0]
    max_single_post_core_task_retries = max_single_post_retry_limits[0]
    fail_on_full_core_task_queue = full_queue_guard_values[0]
    max_shutdown_queue_wait_ms = max_shutdown_queue_wait_values[0]
    max_shutdown_exec_time_ms = max_shutdown_exec_time_values[0]

    print("\n=== Delay/Gap Grid Summary ===")
    print(
        "[capacity=%d burst=%d retry=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s]" % (
            capacity,
            burst,
            format_retry_label(post_retry_count),
            format_max_core_task_retry_limit_label(max_core_task_retries),
            format_max_single_post_retry_limit_label(max_single_post_core_task_retries),
            format_full_queue_guard_label(fail_on_full_core_task_queue),
            format_runtime_guardrail_config_suffix(max_shutdown_queue_wait_ms, max_shutdown_exec_time_ms),
        )
    )

    header_parts = ["delay\\gap"]
    for stress_post_gap_ms in stress_post_gap_values:
        header_parts.append(format_stress_post_gap_label(stress_post_gap_ms))
    print(" | ".join(header_parts))

    for post_retry_delay_ms in post_retry_delay_values:
        row_parts = [format_retry_delay_label(post_retry_delay_ms)]
        for stress_post_gap_ms in stress_post_gap_values:
            matched = find_result(
                results,
                capacity,
                burst,
                post_retry_count,
                post_retry_delay_ms,
                stress_post_gap_ms,
                max_core_task_retries,
                max_single_post_core_task_retries,
                fail_on_full_core_task_queue,
                max_shutdown_queue_wait_ms,
                max_shutdown_exec_time_ms,
            )
            row_parts.append(summarize_result_cell(matched))
        print(" | ".join(row_parts))


def print_retry_delay_grid_summary(results,
                                   capacities,
                                   bursts,
                                   post_retry_counts,
                                   post_retry_delay_values,
                                   stress_post_gap_values,
                                   max_core_task_retry_limits,
                                   max_single_post_retry_limits,
                                   full_queue_guard_values,
                                   max_shutdown_queue_wait_values,
                                   max_shutdown_exec_time_values):
    if len(capacities) != 1 or len(bursts) != 1:
        return
    if len(post_retry_counts) <= 1 or len(post_retry_delay_values) <= 1:
        return
    if len(stress_post_gap_values) != 1:
        return
    if len(max_core_task_retry_limits) != 1 or len(max_single_post_retry_limits) != 1 or len(full_queue_guard_values) != 1:
        return
    if not has_fixed_runtime_guardrail_values(max_shutdown_queue_wait_values, max_shutdown_exec_time_values):
        print_runtime_guardrail_grid_skip("Retry/Delay Grid Summary")
        return

    capacity = capacities[0]
    burst = bursts[0]
    stress_post_gap_ms = stress_post_gap_values[0]
    max_core_task_retries = max_core_task_retry_limits[0]
    max_single_post_core_task_retries = max_single_post_retry_limits[0]
    fail_on_full_core_task_queue = full_queue_guard_values[0]
    max_shutdown_queue_wait_ms = max_shutdown_queue_wait_values[0]
    max_shutdown_exec_time_ms = max_shutdown_exec_time_values[0]

    print("\n=== Retry/Delay Grid Summary ===")
    print(
        "[capacity=%d burst=%d gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s]" % (
            capacity,
            burst,
            format_stress_post_gap_label(stress_post_gap_ms),
            format_max_core_task_retry_limit_label(max_core_task_retries),
            format_max_single_post_retry_limit_label(max_single_post_core_task_retries),
            format_full_queue_guard_label(fail_on_full_core_task_queue),
            format_runtime_guardrail_config_suffix(max_shutdown_queue_wait_ms, max_shutdown_exec_time_ms),
        )
    )

    header_parts = ["retry\\delay"]
    for post_retry_delay_ms in post_retry_delay_values:
        header_parts.append(format_retry_delay_label(post_retry_delay_ms))
    print(" | ".join(header_parts))

    for post_retry_count in post_retry_counts:
        row_parts = [format_retry_label(post_retry_count)]
        for post_retry_delay_ms in post_retry_delay_values:
            matched = find_result(
                results,
                capacity,
                burst,
                post_retry_count,
                post_retry_delay_ms,
                stress_post_gap_ms,
                max_core_task_retries,
                max_single_post_core_task_retries,
                fail_on_full_core_task_queue,
                max_shutdown_queue_wait_ms,
                max_shutdown_exec_time_ms,
            )
            row_parts.append(summarize_result_cell(matched))
        print(" | ".join(row_parts))


def print_retry_gap_grid_summary(results,
                                 capacities,
                                 bursts,
                                 post_retry_counts,
                                 post_retry_delay_values,
                                 stress_post_gap_values,
                                 max_core_task_retry_limits,
                                 max_single_post_retry_limits,
                                 full_queue_guard_values,
                                 max_shutdown_queue_wait_values,
                                 max_shutdown_exec_time_values):
    if len(capacities) != 1 or len(bursts) != 1:
        return
    if len(post_retry_counts) <= 1 or len(stress_post_gap_values) <= 1:
        return
    if len(post_retry_delay_values) != 1:
        return
    if len(max_core_task_retry_limits) != 1 or len(max_single_post_retry_limits) != 1 or len(full_queue_guard_values) != 1:
        return
    if not has_fixed_runtime_guardrail_values(max_shutdown_queue_wait_values, max_shutdown_exec_time_values):
        print_runtime_guardrail_grid_skip("Retry/Gap Grid Summary")
        return

    capacity = capacities[0]
    burst = bursts[0]
    post_retry_delay_ms = post_retry_delay_values[0]
    max_core_task_retries = max_core_task_retry_limits[0]
    max_single_post_core_task_retries = max_single_post_retry_limits[0]
    fail_on_full_core_task_queue = full_queue_guard_values[0]
    max_shutdown_queue_wait_ms = max_shutdown_queue_wait_values[0]
    max_shutdown_exec_time_ms = max_shutdown_exec_time_values[0]

    print("\n=== Retry/Gap Grid Summary ===")
    print(
        "[capacity=%d burst=%d delay=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s]" % (
            capacity,
            burst,
            format_retry_delay_label(post_retry_delay_ms),
            format_max_core_task_retry_limit_label(max_core_task_retries),
            format_max_single_post_retry_limit_label(max_single_post_core_task_retries),
            format_full_queue_guard_label(fail_on_full_core_task_queue),
            format_runtime_guardrail_config_suffix(max_shutdown_queue_wait_ms, max_shutdown_exec_time_ms),
        )
    )

    header_parts = ["retry\\gap"]
    for stress_post_gap_ms in stress_post_gap_values:
        header_parts.append(format_stress_post_gap_label(stress_post_gap_ms))
    print(" | ".join(header_parts))

    for post_retry_count in post_retry_counts:
        row_parts = [format_retry_label(post_retry_count)]
        for stress_post_gap_ms in stress_post_gap_values:
            matched = find_result(
                results,
                capacity,
                burst,
                post_retry_count,
                post_retry_delay_ms,
                stress_post_gap_ms,
                max_core_task_retries,
                max_single_post_core_task_retries,
                fail_on_full_core_task_queue,
                max_shutdown_queue_wait_ms,
                max_shutdown_exec_time_ms,
            )
            row_parts.append(summarize_result_cell(matched))
        print(" | ".join(row_parts))


def print_threshold_summary(results,
                            capacities,
                            bursts,
                            post_retry_counts,
                            post_retry_delay_values,
                            stress_post_gap_values,
                            max_core_task_retry_limits,
                            max_single_post_retry_limits,
                            full_queue_guard_values,
                            max_shutdown_queue_wait_values,
                            max_shutdown_exec_time_values):
    print("\n=== Threshold Summary ===")

    if not has_fixed_runtime_guardrail_values(max_shutdown_queue_wait_values, max_shutdown_exec_time_values):
        print("Skipped: multiple shutdown guardrail values in scan range.")
        return

    max_shutdown_queue_wait_ms = max_shutdown_queue_wait_values[0]
    max_shutdown_exec_time_ms = max_shutdown_exec_time_values[0]
    runtime_guardrail_suffix = format_runtime_guardrail_config_suffix(max_shutdown_queue_wait_ms, max_shutdown_exec_time_ms)
    print("Using fixed shutdown guardrails:%s" % runtime_guardrail_suffix)

    def find_fixed_result(capacity, burst, post_retry_count, post_retry_delay_ms, stress_post_gap_ms, max_core_task_retries, max_single_post_core_task_retries, fail_on_full_core_task_queue):
        return find_result(
            results,
            capacity,
            burst,
            post_retry_count,
            post_retry_delay_ms,
            stress_post_gap_ms,
            max_core_task_retries,
            max_single_post_core_task_retries,
            fail_on_full_core_task_queue,
            max_shutdown_queue_wait_ms,
            max_shutdown_exec_time_ms,
        )

    if len(post_retry_counts) == 1 and len(post_retry_delay_values) == 1 and len(stress_post_gap_values) == 1 and len(max_core_task_retry_limits) == 1 and len(max_single_post_retry_limits) == 1 and len(full_queue_guard_values) == 1:
        post_retry_count = post_retry_counts[0]
        post_retry_delay_ms = post_retry_delay_values[0]
        stress_post_gap_ms = stress_post_gap_values[0]
        max_core_task_retries = max_core_task_retry_limits[0]
        max_single_post_core_task_retries = max_single_post_retry_limits[0]
        fail_on_full_core_task_queue = full_queue_guard_values[0]
        retry_label = format_retry_label(post_retry_count)
        retry_delay_label = format_retry_delay_label(post_retry_delay_ms)
        stress_post_gap_label = format_stress_post_gap_label(stress_post_gap_ms)
        max_core_task_retry_label = format_max_core_task_retry_limit_label(max_core_task_retries)
        max_single_post_retry_label = format_max_single_post_retry_limit_label(max_single_post_core_task_retries)
        full_queue_guard_label = format_full_queue_guard_label(fail_on_full_core_task_queue)

        for burst in bursts:
            passed_capacity = None
            for capacity in capacities:
                matched = find_fixed_result(capacity, burst, post_retry_count, post_retry_delay_ms, stress_post_gap_ms, max_core_task_retries, max_single_post_core_task_retries, fail_on_full_core_task_queue)
                if matched and is_pass(matched):
                    passed_capacity = capacity
                    break
            if passed_capacity is None:
                print("[retry=%s delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s burst=%d] no passing capacity in scan range" % (
                    retry_label, retry_delay_label, stress_post_gap_label, max_core_task_retry_label, max_single_post_retry_label, full_queue_guard_label, runtime_guardrail_suffix, burst))
            else:
                print("[retry=%s delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s burst=%d] minimum passing capacity=%d" % (
                    retry_label, retry_delay_label, stress_post_gap_label, max_core_task_retry_label, max_single_post_retry_label, full_queue_guard_label, runtime_guardrail_suffix, burst, passed_capacity))

        for capacity in capacities:
            passed_burst = None
            for burst in bursts:
                matched = find_fixed_result(capacity, burst, post_retry_count, post_retry_delay_ms, stress_post_gap_ms, max_core_task_retries, max_single_post_core_task_retries, fail_on_full_core_task_queue)
                if matched and is_pass(matched):
                    passed_burst = burst
            if passed_burst is None:
                print("[retry=%s delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s capacity=%d] no passing burst in scan range" % (
                    retry_label, retry_delay_label, stress_post_gap_label, max_core_task_retry_label, max_single_post_retry_label, full_queue_guard_label, runtime_guardrail_suffix, capacity))
            else:
                    print("[retry=%s delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s capacity=%d] maximum passing burst=%d" % (
                        retry_label, retry_delay_label, stress_post_gap_label, max_core_task_retry_label, max_single_post_retry_label, full_queue_guard_label, runtime_guardrail_suffix, capacity, passed_burst))
        return

    if len(post_retry_delay_values) == 1 and len(stress_post_gap_values) == 1 and len(max_core_task_retry_limits) == 1 and len(max_single_post_retry_limits) == 1 and len(full_queue_guard_values) == 1:
        post_retry_delay_ms = post_retry_delay_values[0]
        stress_post_gap_ms = stress_post_gap_values[0]
        max_core_task_retries = max_core_task_retry_limits[0]
        max_single_post_core_task_retries = max_single_post_retry_limits[0]
        fail_on_full_core_task_queue = full_queue_guard_values[0]
        retry_delay_label = format_retry_delay_label(post_retry_delay_ms)
        stress_post_gap_label = format_stress_post_gap_label(stress_post_gap_ms)
        max_core_task_retry_label = format_max_core_task_retry_limit_label(max_core_task_retries)
        max_single_post_retry_label = format_max_single_post_retry_limit_label(max_single_post_core_task_retries)
        full_queue_guard_label = format_full_queue_guard_label(fail_on_full_core_task_queue)

        for capacity in capacities:
            for burst in bursts:
                passed_retry_count = None
                for post_retry_count in post_retry_counts:
                    matched = find_fixed_result(capacity, burst, post_retry_count, post_retry_delay_ms, stress_post_gap_ms, max_core_task_retries, max_single_post_core_task_retries, fail_on_full_core_task_queue)
                    if matched and is_pass(matched):
                        passed_retry_count = post_retry_count
                        break
                if passed_retry_count is None:
                    print("[delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s capacity=%d burst=%d] no passing retry in scan range" % (
                        retry_delay_label, stress_post_gap_label, max_core_task_retry_label, max_single_post_retry_label, full_queue_guard_label, runtime_guardrail_suffix, capacity, burst))
                else:
                    print("[delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s capacity=%d burst=%d] minimum passing retry=%s" % (
                        retry_delay_label,
                        stress_post_gap_label,
                        max_core_task_retry_label,
                        max_single_post_retry_label,
                        full_queue_guard_label,
                        runtime_guardrail_suffix,
                        capacity,
                        burst,
                        format_retry_label(passed_retry_count),
                    ))

        for post_retry_count in post_retry_counts:
            retry_label = format_retry_label(post_retry_count)
            for capacity in capacities:
                passed_burst = None
                for burst in bursts:
                    matched = find_fixed_result(capacity, burst, post_retry_count, post_retry_delay_ms, stress_post_gap_ms, max_core_task_retries, max_single_post_core_task_retries, fail_on_full_core_task_queue)
                    if matched and is_pass(matched):
                        passed_burst = burst
                if passed_burst is None:
                    print("[retry=%s delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s capacity=%d] no passing burst in scan range" % (
                        retry_label, retry_delay_label, stress_post_gap_label, max_core_task_retry_label, max_single_post_retry_label, full_queue_guard_label, runtime_guardrail_suffix, capacity))
                else:
                    print("[retry=%s delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s capacity=%d] maximum passing burst=%d" % (
                        retry_label, retry_delay_label, stress_post_gap_label, max_core_task_retry_label, max_single_post_retry_label, full_queue_guard_label, runtime_guardrail_suffix, capacity, passed_burst))
        return

    if len(post_retry_counts) == 1 and len(stress_post_gap_values) == 1 and len(max_core_task_retry_limits) == 1 and len(max_single_post_retry_limits) == 1 and len(full_queue_guard_values) == 1:
        post_retry_count = post_retry_counts[0]
        stress_post_gap_ms = stress_post_gap_values[0]
        max_core_task_retries = max_core_task_retry_limits[0]
        max_single_post_core_task_retries = max_single_post_retry_limits[0]
        fail_on_full_core_task_queue = full_queue_guard_values[0]
        retry_label = format_retry_label(post_retry_count)
        stress_post_gap_label = format_stress_post_gap_label(stress_post_gap_ms)
        max_core_task_retry_label = format_max_core_task_retry_limit_label(max_core_task_retries)
        max_single_post_retry_label = format_max_single_post_retry_limit_label(max_single_post_core_task_retries)
        full_queue_guard_label = format_full_queue_guard_label(fail_on_full_core_task_queue)

        for capacity in capacities:
            for burst in bursts:
                passed_retry_delay_ms = None
                for post_retry_delay_ms in post_retry_delay_values:
                    matched = find_fixed_result(capacity, burst, post_retry_count, post_retry_delay_ms, stress_post_gap_ms, max_core_task_retries, max_single_post_core_task_retries, fail_on_full_core_task_queue)
                    if matched and is_pass(matched):
                        passed_retry_delay_ms = post_retry_delay_ms
                        break
                if passed_retry_delay_ms is None:
                    print("[retry=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s capacity=%d burst=%d] no passing delay in scan range" % (
                        retry_label, stress_post_gap_label, max_core_task_retry_label, max_single_post_retry_label, full_queue_guard_label, runtime_guardrail_suffix, capacity, burst))
                else:
                    print("[retry=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s capacity=%d burst=%d] minimum passing delay=%s" % (
                        retry_label,
                        stress_post_gap_label,
                        max_core_task_retry_label,
                        max_single_post_retry_label,
                        full_queue_guard_label,
                        runtime_guardrail_suffix,
                        capacity,
                        burst,
                        format_retry_delay_label(passed_retry_delay_ms),
                    ))

        for post_retry_delay_ms in post_retry_delay_values:
            retry_delay_label = format_retry_delay_label(post_retry_delay_ms)
            for capacity in capacities:
                passed_burst = None
                for burst in bursts:
                    matched = find_fixed_result(capacity, burst, post_retry_count, post_retry_delay_ms, stress_post_gap_ms, max_core_task_retries, max_single_post_core_task_retries, fail_on_full_core_task_queue)
                    if matched and is_pass(matched):
                        passed_burst = burst
                if passed_burst is None:
                    print("[retry=%s delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s capacity=%d] no passing burst in scan range" % (
                        retry_label, retry_delay_label, stress_post_gap_label, max_core_task_retry_label, max_single_post_retry_label, full_queue_guard_label, runtime_guardrail_suffix, capacity))
                else:
                    print("[retry=%s delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s capacity=%d] maximum passing burst=%d" % (
                        retry_label, retry_delay_label, stress_post_gap_label, max_core_task_retry_label, max_single_post_retry_label, full_queue_guard_label, runtime_guardrail_suffix, capacity, passed_burst))
        return

    if len(post_retry_counts) == 1 and len(max_core_task_retry_limits) == 1 and len(max_single_post_retry_limits) == 1 and len(full_queue_guard_values) == 1:
        post_retry_count = post_retry_counts[0]
        max_core_task_retries = max_core_task_retry_limits[0]
        max_single_post_core_task_retries = max_single_post_retry_limits[0]
        fail_on_full_core_task_queue = full_queue_guard_values[0]
        retry_label = format_retry_label(post_retry_count)
        max_core_task_retry_label = format_max_core_task_retry_limit_label(max_core_task_retries)
        max_single_post_retry_label = format_max_single_post_retry_limit_label(max_single_post_core_task_retries)
        full_queue_guard_label = format_full_queue_guard_label(fail_on_full_core_task_queue)

        if len(post_retry_delay_values) > 1 and len(stress_post_gap_values) > 1:
            for capacity in capacities:
                for burst in bursts:
                    for post_retry_delay_ms in post_retry_delay_values:
                        passed_stress_post_gap_ms = None
                        for stress_post_gap_ms in stress_post_gap_values:
                            matched = find_fixed_result(capacity, burst, post_retry_count, post_retry_delay_ms, stress_post_gap_ms, max_core_task_retries, max_single_post_core_task_retries, fail_on_full_core_task_queue)
                            if matched and is_pass(matched):
                                passed_stress_post_gap_ms = stress_post_gap_ms
                                break
                        if passed_stress_post_gap_ms is None:
                            print("[retry=%s delay=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s capacity=%d burst=%d] no passing gap in scan range" % (
                                retry_label, format_retry_delay_label(post_retry_delay_ms), max_core_task_retry_label, max_single_post_retry_label, full_queue_guard_label, runtime_guardrail_suffix, capacity, burst))
                        else:
                            print("[retry=%s delay=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s capacity=%d burst=%d] minimum passing gap=%s" % (
                                retry_label,
                                format_retry_delay_label(post_retry_delay_ms),
                                max_core_task_retry_label,
                                max_single_post_retry_label,
                                full_queue_guard_label,
                                runtime_guardrail_suffix,
                                capacity,
                                burst,
                                format_stress_post_gap_label(passed_stress_post_gap_ms),
                            ))

                    for stress_post_gap_ms in stress_post_gap_values:
                        passed_retry_delay_ms = None
                        for post_retry_delay_ms in post_retry_delay_values:
                            matched = find_fixed_result(capacity, burst, post_retry_count, post_retry_delay_ms, stress_post_gap_ms, max_core_task_retries, max_single_post_core_task_retries, fail_on_full_core_task_queue)
                            if matched and is_pass(matched):
                                passed_retry_delay_ms = post_retry_delay_ms
                                break
                        if passed_retry_delay_ms is None:
                            print("[retry=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s capacity=%d burst=%d] no passing delay in scan range" % (
                                retry_label, format_stress_post_gap_label(stress_post_gap_ms), max_core_task_retry_label, max_single_post_retry_label, full_queue_guard_label, runtime_guardrail_suffix, capacity, burst))
                        else:
                            print("[retry=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s capacity=%d burst=%d] minimum passing delay=%s" % (
                                retry_label,
                                format_stress_post_gap_label(stress_post_gap_ms),
                                max_core_task_retry_label,
                                max_single_post_retry_label,
                                full_queue_guard_label,
                                runtime_guardrail_suffix,
                                capacity,
                                burst,
                                format_retry_delay_label(passed_retry_delay_ms),
                            ))

            for post_retry_delay_ms in post_retry_delay_values:
                retry_delay_label = format_retry_delay_label(post_retry_delay_ms)
                for stress_post_gap_ms in stress_post_gap_values:
                    stress_post_gap_label = format_stress_post_gap_label(stress_post_gap_ms)
                    for capacity in capacities:
                        passed_burst = None
                        for burst in bursts:
                            matched = find_fixed_result(capacity, burst, post_retry_count, post_retry_delay_ms, stress_post_gap_ms, max_core_task_retries, max_single_post_core_task_retries, fail_on_full_core_task_queue)
                            if matched and is_pass(matched):
                                passed_burst = burst
                        if passed_burst is None:
                            print("[retry=%s delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s capacity=%d] no passing burst in scan range" % (
                                retry_label, retry_delay_label, stress_post_gap_label, max_core_task_retry_label, max_single_post_retry_label, full_queue_guard_label, runtime_guardrail_suffix, capacity))
                        else:
                            print("[retry=%s delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s capacity=%d] maximum passing burst=%d" % (
                                retry_label, retry_delay_label, stress_post_gap_label, max_core_task_retry_label, max_single_post_retry_label, full_queue_guard_label, runtime_guardrail_suffix, capacity, passed_burst))
            return

    if len(post_retry_counts) == 1 and len(post_retry_delay_values) == 1 and len(stress_post_gap_values) == 1 and len(max_single_post_retry_limits) == 1 and len(full_queue_guard_values) == 1:
        post_retry_count = post_retry_counts[0]
        post_retry_delay_ms = post_retry_delay_values[0]
        stress_post_gap_ms = stress_post_gap_values[0]
        max_single_post_core_task_retries = max_single_post_retry_limits[0]
        fail_on_full_core_task_queue = full_queue_guard_values[0]
        retry_label = format_retry_label(post_retry_count)
        retry_delay_label = format_retry_delay_label(post_retry_delay_ms)
        stress_post_gap_label = format_stress_post_gap_label(stress_post_gap_ms)
        max_single_post_retry_label = format_max_single_post_retry_limit_label(max_single_post_core_task_retries)
        full_queue_guard_label = format_full_queue_guard_label(fail_on_full_core_task_queue)

        for capacity in capacities:
            for burst in bursts:
                passed_max_core_task_retry_limit = None
                for max_core_task_retries in max_core_task_retry_limits:
                    matched = find_fixed_result(capacity, burst, post_retry_count, post_retry_delay_ms, stress_post_gap_ms, max_core_task_retries, max_single_post_core_task_retries, fail_on_full_core_task_queue)
                    if matched and is_pass(matched):
                        passed_max_core_task_retry_limit = max_core_task_retries
                        break
                if passed_max_core_task_retry_limit is None:
                    print("[retry=%s delay=%s gap=%s max_single_post_retry=%s full_queue_guard=%s%s capacity=%d burst=%d] no passing max_core_task_retries in scan range" % (
                        retry_label, retry_delay_label, stress_post_gap_label, max_single_post_retry_label, full_queue_guard_label, runtime_guardrail_suffix, capacity, burst))
                else:
                    print("[retry=%s delay=%s gap=%s max_single_post_retry=%s full_queue_guard=%s%s capacity=%d burst=%d] minimum passing max_core_task_retries=%s" % (
                        retry_label,
                        retry_delay_label,
                        stress_post_gap_label,
                        max_single_post_retry_label,
                        full_queue_guard_label,
                        runtime_guardrail_suffix,
                        capacity,
                        burst,
                        format_max_core_task_retry_limit_label(passed_max_core_task_retry_limit),
                    ))

        for max_core_task_retries in max_core_task_retry_limits:
            max_core_task_retry_label = format_max_core_task_retry_limit_label(max_core_task_retries)
            for capacity in capacities:
                passed_burst = None
                for burst in bursts:
                    matched = find_fixed_result(capacity, burst, post_retry_count, post_retry_delay_ms, stress_post_gap_ms, max_core_task_retries, max_single_post_core_task_retries, fail_on_full_core_task_queue)
                    if matched and is_pass(matched):
                        passed_burst = burst
                if passed_burst is None:
                    print("[retry=%s delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s capacity=%d] no passing burst in scan range" % (
                        retry_label, retry_delay_label, stress_post_gap_label, max_core_task_retry_label, max_single_post_retry_label, full_queue_guard_label, runtime_guardrail_suffix, capacity))
                else:
                    print("[retry=%s delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s capacity=%d] maximum passing burst=%d" % (
                        retry_label, retry_delay_label, stress_post_gap_label, max_core_task_retry_label, max_single_post_retry_label, full_queue_guard_label, runtime_guardrail_suffix, capacity, passed_burst))
        return

    if len(post_retry_counts) == 1 and len(post_retry_delay_values) == 1 and len(stress_post_gap_values) == 1 and len(max_core_task_retry_limits) == 1 and len(full_queue_guard_values) == 1:
        post_retry_count = post_retry_counts[0]
        post_retry_delay_ms = post_retry_delay_values[0]
        stress_post_gap_ms = stress_post_gap_values[0]
        max_core_task_retries = max_core_task_retry_limits[0]
        fail_on_full_core_task_queue = full_queue_guard_values[0]
        retry_label = format_retry_label(post_retry_count)
        retry_delay_label = format_retry_delay_label(post_retry_delay_ms)
        stress_post_gap_label = format_stress_post_gap_label(stress_post_gap_ms)
        max_core_task_retry_label = format_max_core_task_retry_limit_label(max_core_task_retries)
        full_queue_guard_label = format_full_queue_guard_label(fail_on_full_core_task_queue)

        for capacity in capacities:
            for burst in bursts:
                passed_max_single_post_retry_limit = None
                for max_single_post_core_task_retries in max_single_post_retry_limits:
                    matched = find_fixed_result(capacity, burst, post_retry_count, post_retry_delay_ms, stress_post_gap_ms, max_core_task_retries, max_single_post_core_task_retries, fail_on_full_core_task_queue)
                    if matched and is_pass(matched):
                        passed_max_single_post_retry_limit = max_single_post_core_task_retries
                        break
                if passed_max_single_post_retry_limit is None:
                    print("[retry=%s delay=%s gap=%s max_core_task_retries=%s full_queue_guard=%s%s capacity=%d burst=%d] no passing max_single_post_retry in scan range" % (
                        retry_label, retry_delay_label, stress_post_gap_label, max_core_task_retry_label, full_queue_guard_label, runtime_guardrail_suffix, capacity, burst))
                else:
                    print("[retry=%s delay=%s gap=%s max_core_task_retries=%s full_queue_guard=%s%s capacity=%d burst=%d] minimum passing max_single_post_retry=%s" % (
                        retry_label,
                        retry_delay_label,
                        stress_post_gap_label,
                        max_core_task_retry_label,
                        full_queue_guard_label,
                        runtime_guardrail_suffix,
                        capacity,
                        burst,
                        format_max_single_post_retry_limit_label(passed_max_single_post_retry_limit),
                    ))

        for max_single_post_core_task_retries in max_single_post_retry_limits:
            max_single_post_retry_label = format_max_single_post_retry_limit_label(max_single_post_core_task_retries)
            for capacity in capacities:
                passed_burst = None
                for burst in bursts:
                    matched = find_fixed_result(capacity, burst, post_retry_count, post_retry_delay_ms, stress_post_gap_ms, max_core_task_retries, max_single_post_core_task_retries, fail_on_full_core_task_queue)
                    if matched and is_pass(matched):
                        passed_burst = burst
                if passed_burst is None:
                    print("[retry=%s delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s capacity=%d] no passing burst in scan range" % (
                        retry_label, retry_delay_label, stress_post_gap_label, max_core_task_retry_label, max_single_post_retry_label, full_queue_guard_label, runtime_guardrail_suffix, capacity))
                else:
                    print("[retry=%s delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s capacity=%d] maximum passing burst=%d" % (
                        retry_label, retry_delay_label, stress_post_gap_label, max_core_task_retry_label, max_single_post_retry_label, full_queue_guard_label, runtime_guardrail_suffix, capacity, passed_burst))
        return

    if len(post_retry_counts) == 1 and len(post_retry_delay_values) == 1 and len(stress_post_gap_values) == 1 and len(max_core_task_retry_limits) == 1 and len(max_single_post_retry_limits) == 1:
        post_retry_count = post_retry_counts[0]
        post_retry_delay_ms = post_retry_delay_values[0]
        stress_post_gap_ms = stress_post_gap_values[0]
        max_core_task_retries = max_core_task_retry_limits[0]
        max_single_post_core_task_retries = max_single_post_retry_limits[0]
        retry_label = format_retry_label(post_retry_count)
        retry_delay_label = format_retry_delay_label(post_retry_delay_ms)
        stress_post_gap_label = format_stress_post_gap_label(stress_post_gap_ms)
        max_core_task_retry_label = format_max_core_task_retry_limit_label(max_core_task_retries)
        max_single_post_retry_label = format_max_single_post_retry_limit_label(max_single_post_core_task_retries)

        for capacity in capacities:
            for burst in bursts:
                passed_full_queue_guard = None
                for fail_on_full_core_task_queue in full_queue_guard_values:
                    matched = find_fixed_result(capacity, burst, post_retry_count, post_retry_delay_ms, stress_post_gap_ms, max_core_task_retries, max_single_post_core_task_retries, fail_on_full_core_task_queue)
                    if matched and is_pass(matched):
                        passed_full_queue_guard = fail_on_full_core_task_queue
                        break
                if passed_full_queue_guard is None:
                    print("[retry=%s delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s%s capacity=%d burst=%d] no passing full_queue_guard in scan range" % (
                        retry_label, retry_delay_label, stress_post_gap_label, max_core_task_retry_label, max_single_post_retry_label, runtime_guardrail_suffix, capacity, burst))
                else:
                    print("[retry=%s delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s%s capacity=%d burst=%d] minimum passing full_queue_guard=%s" % (
                        retry_label,
                        retry_delay_label,
                        stress_post_gap_label,
                        max_core_task_retry_label,
                        max_single_post_retry_label,
                        runtime_guardrail_suffix,
                        capacity,
                        burst,
                        format_full_queue_guard_label(passed_full_queue_guard),
                    ))

        for fail_on_full_core_task_queue in full_queue_guard_values:
            full_queue_guard_label = format_full_queue_guard_label(fail_on_full_core_task_queue)
            for capacity in capacities:
                passed_burst = None
                for burst in bursts:
                    matched = find_fixed_result(capacity, burst, post_retry_count, post_retry_delay_ms, stress_post_gap_ms, max_core_task_retries, max_single_post_core_task_retries, fail_on_full_core_task_queue)
                    if matched and is_pass(matched):
                        passed_burst = burst
                if passed_burst is None:
                    print("[retry=%s delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s capacity=%d] no passing burst in scan range" % (
                        retry_label, retry_delay_label, stress_post_gap_label, max_core_task_retry_label, max_single_post_retry_label, full_queue_guard_label, runtime_guardrail_suffix, capacity))
                else:
                    print("[retry=%s delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s capacity=%d] maximum passing burst=%d" % (
                        retry_label, retry_delay_label, stress_post_gap_label, max_core_task_retry_label, max_single_post_retry_label, full_queue_guard_label, runtime_guardrail_suffix, capacity, passed_burst))
        return

    if len(post_retry_counts) == 1 and len(post_retry_delay_values) == 1 and len(max_core_task_retry_limits) == 1 and len(max_single_post_retry_limits) == 1 and len(full_queue_guard_values) == 1:
        post_retry_count = post_retry_counts[0]
        post_retry_delay_ms = post_retry_delay_values[0]
        max_core_task_retries = max_core_task_retry_limits[0]
        max_single_post_core_task_retries = max_single_post_retry_limits[0]
        fail_on_full_core_task_queue = full_queue_guard_values[0]
        retry_label = format_retry_label(post_retry_count)
        retry_delay_label = format_retry_delay_label(post_retry_delay_ms)
        max_core_task_retry_label = format_max_core_task_retry_limit_label(max_core_task_retries)
        max_single_post_retry_label = format_max_single_post_retry_limit_label(max_single_post_core_task_retries)
        full_queue_guard_label = format_full_queue_guard_label(fail_on_full_core_task_queue)

        for capacity in capacities:
            for burst in bursts:
                passed_stress_post_gap_ms = None
                for stress_post_gap_ms in stress_post_gap_values:
                    matched = find_fixed_result(capacity, burst, post_retry_count, post_retry_delay_ms, stress_post_gap_ms, max_core_task_retries, max_single_post_core_task_retries, fail_on_full_core_task_queue)
                    if matched and is_pass(matched):
                        passed_stress_post_gap_ms = stress_post_gap_ms
                        break
                if passed_stress_post_gap_ms is None:
                    print("[retry=%s delay=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s capacity=%d burst=%d] no passing gap in scan range" % (
                        retry_label, retry_delay_label, max_core_task_retry_label, max_single_post_retry_label, full_queue_guard_label, runtime_guardrail_suffix, capacity, burst))
                else:
                    print("[retry=%s delay=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s capacity=%d burst=%d] minimum passing gap=%s" % (
                        retry_label,
                        retry_delay_label,
                        max_core_task_retry_label,
                        max_single_post_retry_label,
                        full_queue_guard_label,
                        runtime_guardrail_suffix,
                        capacity,
                        burst,
                        format_stress_post_gap_label(passed_stress_post_gap_ms),
                    ))

        for stress_post_gap_ms in stress_post_gap_values:
            stress_post_gap_label = format_stress_post_gap_label(stress_post_gap_ms)
            for capacity in capacities:
                passed_burst = None
                for burst in bursts:
                    matched = find_fixed_result(capacity, burst, post_retry_count, post_retry_delay_ms, stress_post_gap_ms, max_core_task_retries, max_single_post_core_task_retries, fail_on_full_core_task_queue)
                    if matched and is_pass(matched):
                        passed_burst = burst
                if passed_burst is None:
                    print("[retry=%s delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s capacity=%d] no passing burst in scan range" % (
                        retry_label, retry_delay_label, stress_post_gap_label, max_core_task_retry_label, max_single_post_retry_label, full_queue_guard_label, runtime_guardrail_suffix, capacity))
                else:
                    print("[retry=%s delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s capacity=%d] maximum passing burst=%d" % (
                        retry_label, retry_delay_label, stress_post_gap_label, max_core_task_retry_label, max_single_post_retry_label, full_queue_guard_label, runtime_guardrail_suffix, capacity, passed_burst))
        return

    for capacity in capacities:
        for burst in bursts:
            passed_retry_count = None
            passed_retry_delay_ms = None
            passed_stress_post_gap_ms = None
            passed_max_core_task_retry_limit = None
            passed_max_single_post_retry_limit = None
            passed_full_queue_guard = None
            for post_retry_count in post_retry_counts:
                for post_retry_delay_ms in post_retry_delay_values:
                    for stress_post_gap_ms in stress_post_gap_values:
                        for max_core_task_retries in max_core_task_retry_limits:
                            for max_single_post_core_task_retries in max_single_post_retry_limits:
                                for fail_on_full_core_task_queue in full_queue_guard_values:
                                    matched = find_fixed_result(capacity, burst, post_retry_count, post_retry_delay_ms, stress_post_gap_ms, max_core_task_retries, max_single_post_core_task_retries, fail_on_full_core_task_queue)
                                    if matched and is_pass(matched):
                                        passed_retry_count = post_retry_count
                                        passed_retry_delay_ms = post_retry_delay_ms
                                        passed_stress_post_gap_ms = stress_post_gap_ms
                                        passed_max_core_task_retry_limit = max_core_task_retries
                                        passed_max_single_post_retry_limit = max_single_post_core_task_retries
                                        passed_full_queue_guard = fail_on_full_core_task_queue
                                        break
                                if passed_retry_count is not None:
                                    break
                            if passed_retry_count is not None:
                                break
                        if passed_retry_count is not None:
                            break
                    if passed_retry_count is not None:
                        break
                if passed_retry_count is not None:
                    break
            if passed_retry_count is None:
                print("[capacity=%d burst=%d%s] no passing retry/delay/gap/max-core/max-single-post/full-queue/shutdown-guard config in scan range" % (capacity, burst, runtime_guardrail_suffix))
            else:
                print(
                    "[capacity=%d burst=%d%s] minimum passing retry=%s delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s" % (
                        capacity,
                        burst,
                        runtime_guardrail_suffix,
                        format_retry_label(passed_retry_count),
                        format_retry_delay_label(passed_retry_delay_ms),
                        format_stress_post_gap_label(passed_stress_post_gap_ms),
                        format_max_core_task_retry_limit_label(passed_max_core_task_retry_limit),
                        format_max_single_post_retry_limit_label(passed_max_single_post_retry_limit),
                        format_full_queue_guard_label(passed_full_queue_guard),
                    )
                )

    for post_retry_count in post_retry_counts:
        retry_label = format_retry_label(post_retry_count)
        for post_retry_delay_ms in post_retry_delay_values:
            retry_delay_label = format_retry_delay_label(post_retry_delay_ms)
            for stress_post_gap_ms in stress_post_gap_values:
                stress_post_gap_label = format_stress_post_gap_label(stress_post_gap_ms)
                for max_core_task_retries in max_core_task_retry_limits:
                    max_core_task_retry_label = format_max_core_task_retry_limit_label(max_core_task_retries)
                    for max_single_post_core_task_retries in max_single_post_retry_limits:
                        max_single_post_retry_label = format_max_single_post_retry_limit_label(max_single_post_core_task_retries)
                        for fail_on_full_core_task_queue in full_queue_guard_values:
                            full_queue_guard_label = format_full_queue_guard_label(fail_on_full_core_task_queue)
                            for capacity in capacities:
                                passed_burst = None
                                for burst in bursts:
                                    matched = find_fixed_result(capacity, burst, post_retry_count, post_retry_delay_ms, stress_post_gap_ms, max_core_task_retries, max_single_post_core_task_retries, fail_on_full_core_task_queue)
                                    if matched and is_pass(matched):
                                        passed_burst = burst
                                if passed_burst is None:
                                    print("[retry=%s delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s capacity=%d] no passing burst in scan range" % (
                                        retry_label, retry_delay_label, stress_post_gap_label, max_core_task_retry_label, max_single_post_retry_label, full_queue_guard_label, runtime_guardrail_suffix, capacity))
                                else:
                                    print("[retry=%s delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s capacity=%d] maximum passing burst=%d" % (
                                        retry_label, retry_delay_label, stress_post_gap_label, max_core_task_retry_label, max_single_post_retry_label, full_queue_guard_label, runtime_guardrail_suffix, capacity, passed_burst))


def print_failure_stage_summary(results):
    print("\n=== Failure Stage Summary ===")

    any_stage = False
    for item in results:
        if is_pass(item):
            continue
        stage = extract_failure_stage(item["detail"])
        progress = extract_failure_progress(item["detail"])
        if not stage:
            continue
        any_stage = True
        progress_suffix = " progress=%s" % progress if progress else ""
        print(
            "[capacity=%d burst=%d retry=%s delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s] stage=%s%s" % (
                item["capacity"],
                item["burst"],
                format_retry_label(item["post_retry_count"]),
                format_retry_delay_label(item["post_retry_delay_ms"]),
                format_stress_post_gap_label(item["stress_post_gap_ms"]),
                format_max_core_task_retry_limit_label(item["max_core_task_retries"]),
                format_max_single_post_retry_limit_label(item["max_single_post_core_task_retries"]),
                format_full_queue_guard_label(item["fail_on_full_core_task_queue"]),
                format_runtime_guardrail_config_suffix(item["max_shutdown_queue_wait_ms"], item["max_shutdown_exec_time_ms"]),
                stage,
                progress_suffix,
            )
        )

    if not any_stage:
        print("No stage-tagged runtime failures in scan results.")


def print_failure_app_summary(results):
    print("\n=== Failure App Summary ===")

    any_app = False
    for item in results:
        if is_pass(item):
            continue
        app = extract_failure_app(item["detail"])
        if not app:
            continue
        any_app = True
        stage = extract_failure_stage(item["detail"])
        contention = classify_failure_contention(item["detail"])
        extra_parts = []
        if stage:
            extra_parts.append("stage=%s" % stage)
        if contention:
            extra_parts.append("contention=%s" % contention)
        suffix = " " + " ".join(extra_parts) if extra_parts else ""
        print(
            "[capacity=%d burst=%d retry=%s delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s] app=%s%s" % (
                item["capacity"],
                item["burst"],
                format_retry_label(item["post_retry_count"]),
                format_retry_delay_label(item["post_retry_delay_ms"]),
                format_stress_post_gap_label(item["stress_post_gap_ms"]),
                format_max_core_task_retry_limit_label(item["max_core_task_retries"]),
                format_max_single_post_retry_limit_label(item["max_single_post_core_task_retries"]),
                format_full_queue_guard_label(item["fail_on_full_core_task_queue"]),
                format_runtime_guardrail_config_suffix(item["max_shutdown_queue_wait_ms"], item["max_shutdown_exec_time_ms"]),
                app,
                suffix,
            )
        )

    if not any_app:
        print("No app-tagged runtime failures in scan results.")


def print_failure_app_rollup(results):
    print("\n=== Failure App Rollup ===")

    app_stats = {}
    for item in results:
        if is_pass(item):
            continue
        app = extract_failure_app(item["detail"])
        if not app:
            continue
        app_stats[app] = app_stats.get(app, 0) + 1

    if not app_stats:
        print("No app-tagged runtime failures in scan results.")
        return

    for app in sorted(app_stats):
        print("app=%s failures=%d" % (app, app_stats[app]))


def print_failure_app_leaders(results):
    print("\n=== Failure App Leaders ===")

    leader_stats = {}
    for item in results:
        if is_pass(item):
            continue
        app = extract_failure_app(item["detail"])
        if not app:
            continue
        key = (
            item["capacity"],
            item["burst"],
            item["post_retry_count"],
            item["post_retry_delay_ms"],
            item["stress_post_gap_ms"],
            item["max_core_task_retries"],
            item["max_single_post_core_task_retries"],
            item["fail_on_full_core_task_queue"],
            item["max_shutdown_queue_wait_ms"],
            item["max_shutdown_exec_time_ms"],
        )
        app_map = leader_stats.setdefault(key, {})
        app_map[app] = app_map.get(app, 0) + 1

    if not leader_stats:
        print("No app-tagged runtime failures in scan results.")
        return

    for key in sorted(leader_stats):
        capacity, burst, post_retry_count, post_retry_delay_ms, stress_post_gap_ms, max_core_task_retries, max_single_post_core_task_retries, fail_on_full_core_task_queue, max_shutdown_queue_wait_ms, max_shutdown_exec_time_ms = key
        app_map = leader_stats[key]
        leader_app = None
        leader_count = None
        for app in sorted(app_map):
            count = app_map[app]
            if leader_count is None or count > leader_count:
                leader_count = count
                leader_app = app
        print(
            "[capacity=%d burst=%d retry=%s delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s] dominant_app=%s failures=%d" % (
                capacity,
                burst,
                format_retry_label(post_retry_count),
                format_retry_delay_label(post_retry_delay_ms),
                format_stress_post_gap_label(stress_post_gap_ms),
                format_max_core_task_retry_limit_label(max_core_task_retries),
                format_max_single_post_retry_limit_label(max_single_post_core_task_retries),
                format_full_queue_guard_label(fail_on_full_core_task_queue),
                format_runtime_guardrail_config_suffix(max_shutdown_queue_wait_ms, max_shutdown_exec_time_ms),
                leader_app,
                leader_count,
            )
        )


def print_failure_stage_rollup(results):
    print("\n=== Failure Stage Rollup ===")

    stage_stats = {}
    for item in results:
        if is_pass(item):
            continue
        stage = extract_failure_stage(item["detail"])
        progress = extract_failure_progress(item["detail"])
        if not stage:
            continue
        current_step, total_steps = parse_progress_step(progress)
        stats = stage_stats.setdefault(stage, {"count": 0, "min_step": None, "max_step": None, "total_steps": None})
        stats["count"] += 1
        if current_step is None or total_steps is None:
            continue
        if stats["min_step"] is None or current_step < stats["min_step"]:
            stats["min_step"] = current_step
        if stats["max_step"] is None or current_step > stats["max_step"]:
            stats["max_step"] = current_step
        if stats["total_steps"] is None:
            stats["total_steps"] = total_steps

    if not stage_stats:
        print("No stage-tagged runtime failures in scan results.")
        return

    for stage in sorted(stage_stats):
        stats = stage_stats[stage]
        if stats["min_step"] is None or stats["max_step"] is None or stats["total_steps"] is None:
            print("stage=%s failures=%d progress=unknown" % (stage, stats["count"]))
        else:
            print("stage=%s failures=%d progress_range=%d/%d..%d/%d" % (
                stage,
                stats["count"],
                stats["min_step"],
                stats["total_steps"],
                stats["max_step"],
                stats["total_steps"],
            ))


def print_failure_stage_leaders(results):
    print("\n=== Failure Stage Leaders ===")

    leader_stats = {}
    for item in results:
        if is_pass(item):
            continue
        stage = extract_failure_stage(item["detail"])
        if not stage:
            continue
        progress = extract_failure_progress(item["detail"])
        current_step, total_steps = parse_progress_step(progress)
        key = (
            item["capacity"],
            item["burst"],
            item["post_retry_count"],
            item["post_retry_delay_ms"],
            item["stress_post_gap_ms"],
            item["max_core_task_retries"],
            item["max_single_post_core_task_retries"],
            item["fail_on_full_core_task_queue"],
            item["max_shutdown_queue_wait_ms"],
            item["max_shutdown_exec_time_ms"],
        )
        stage_map = leader_stats.setdefault(key, {})
        stats = stage_map.setdefault(stage, {"count": 0, "min_step": None, "max_step": None, "total_steps": None})
        stats["count"] += 1
        if current_step is None or total_steps is None:
            continue
        if stats["min_step"] is None or current_step < stats["min_step"]:
            stats["min_step"] = current_step
        if stats["max_step"] is None or current_step > stats["max_step"]:
            stats["max_step"] = current_step
        if stats["total_steps"] is None:
            stats["total_steps"] = total_steps

    if not leader_stats:
        print("No stage-tagged runtime failures in scan results.")
        return

    for key in sorted(leader_stats):
        capacity, burst, post_retry_count, post_retry_delay_ms, stress_post_gap_ms, max_core_task_retries, max_single_post_core_task_retries, fail_on_full_core_task_queue, max_shutdown_queue_wait_ms, max_shutdown_exec_time_ms = key
        stage_map = leader_stats[key]
        leader_stage = None
        leader_metrics = None
        for stage in sorted(stage_map):
            stats = stage_map[stage]
            sort_key = (stats["count"], stats["max_step"] or -1, stats["min_step"] or -1)
            if leader_metrics is None or sort_key > leader_metrics:
                leader_metrics = sort_key
                leader_stage = stage
        stats = stage_map[leader_stage]
        if stats["min_step"] is None or stats["max_step"] is None or stats["total_steps"] is None:
            progress_text = "unknown"
        else:
            progress_text = "%d/%d..%d/%d" % (
                stats["min_step"],
                stats["total_steps"],
                stats["max_step"],
                stats["total_steps"],
            )
        print(
            "[capacity=%d burst=%d retry=%s delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s] dominant_stage=%s failures=%d progress_range=%s" % (
                capacity,
                burst,
                format_retry_label(post_retry_count),
                format_retry_delay_label(post_retry_delay_ms),
                format_stress_post_gap_label(stress_post_gap_ms),
                format_max_core_task_retry_limit_label(max_core_task_retries),
                format_max_single_post_retry_limit_label(max_single_post_core_task_retries),
                format_full_queue_guard_label(fail_on_full_core_task_queue),
                format_runtime_guardrail_config_suffix(max_shutdown_queue_wait_ms, max_shutdown_exec_time_ms),
                leader_stage,
                stats["count"],
                progress_text,
            )
        )


def print_failure_contention_summary(results):
    print("\n=== Failure Contention Summary ===")

    any_contention = False
    for item in results:
        if is_pass(item):
            continue
        stage = extract_failure_stage(item["detail"])
        inflight_context = extract_failure_inflight_context(item["detail"])
        pending_context = extract_failure_pending_context(item["detail"])
        contention = classify_failure_contention(item["detail"])
        if not contention:
            continue
        any_contention = True
        extra_parts = []
        if stage:
            extra_parts.append("stage=%s" % stage)
        if inflight_context:
            extra_parts.append("inflight=%s" % inflight_context)
        if pending_context:
            extra_parts.append("pending=%s" % pending_context)
        suffix = " " + " ".join(extra_parts) if extra_parts else ""
        print(
            "[capacity=%d burst=%d retry=%s delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s] contention=%s%s" % (
                item["capacity"],
                item["burst"],
                format_retry_label(item["post_retry_count"]),
                format_retry_delay_label(item["post_retry_delay_ms"]),
                format_stress_post_gap_label(item["stress_post_gap_ms"]),
                format_max_core_task_retry_limit_label(item["max_core_task_retries"]),
                format_max_single_post_retry_limit_label(item["max_single_post_core_task_retries"]),
                format_full_queue_guard_label(item["fail_on_full_core_task_queue"]),
                format_runtime_guardrail_config_suffix(item["max_shutdown_queue_wait_ms"], item["max_shutdown_exec_time_ms"]),
                contention,
                suffix,
            )
        )

    if not any_contention:
        print("No inflight/pending contention tags in scan results.")


def print_failure_contention_rollup(results):
    print("\n=== Failure Contention Rollup ===")

    contention_stats = {}
    for item in results:
        if is_pass(item):
            continue
        contention = classify_failure_contention(item["detail"])
        if not contention:
            continue
        contention_stats[contention] = contention_stats.get(contention, 0) + 1

    if not contention_stats:
        print("No inflight/pending contention tags in scan results.")
        return

    for contention in sorted(contention_stats):
        print("contention=%s failures=%d" % (contention, contention_stats[contention]))


def print_failure_contention_leaders(results):
    print("\n=== Failure Contention Leaders ===")

    leader_stats = {}
    for item in results:
        if is_pass(item):
            continue
        contention = classify_failure_contention(item["detail"])
        if not contention:
            continue
        key = (
            item["capacity"],
            item["burst"],
            item["post_retry_count"],
            item["post_retry_delay_ms"],
            item["stress_post_gap_ms"],
            item["max_core_task_retries"],
            item["max_single_post_core_task_retries"],
            item["fail_on_full_core_task_queue"],
            item["max_shutdown_queue_wait_ms"],
            item["max_shutdown_exec_time_ms"],
        )
        contention_map = leader_stats.setdefault(key, {})
        contention_map[contention] = contention_map.get(contention, 0) + 1

    if not leader_stats:
        print("No inflight/pending contention tags in scan results.")
        return

    for key in sorted(leader_stats):
        capacity, burst, post_retry_count, post_retry_delay_ms, stress_post_gap_ms, max_core_task_retries, max_single_post_core_task_retries, fail_on_full_core_task_queue, max_shutdown_queue_wait_ms, max_shutdown_exec_time_ms = key
        contention_map = leader_stats[key]
        leader_contention = None
        leader_count = None
        for contention in sorted(contention_map):
            count = contention_map[contention]
            if leader_count is None or count > leader_count:
                leader_count = count
                leader_contention = contention
        print(
            "[capacity=%d burst=%d retry=%s delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s] dominant_contention=%s failures=%d" % (
                capacity,
                burst,
                format_retry_label(post_retry_count),
                format_retry_delay_label(post_retry_delay_ms),
                format_stress_post_gap_label(stress_post_gap_ms),
                format_max_core_task_retry_limit_label(max_core_task_retries),
                format_max_single_post_retry_limit_label(max_single_post_core_task_retries),
                format_full_queue_guard_label(fail_on_full_core_task_queue),
                format_runtime_guardrail_config_suffix(max_shutdown_queue_wait_ms, max_shutdown_exec_time_ms),
                leader_contention,
                leader_count,
            )
        )


def print_failure_guardrail_summary(results):
    print("\n=== Failure Guardrail Summary ===")

    any_guardrail = False
    for item in results:
        if is_pass(item):
            continue
        guardrail = extract_failure_guardrail(item["detail"])
        if not guardrail:
            continue
        any_guardrail = True
        display_id = extract_failure_guardrail_display(item["detail"])
        context = extract_failure_guardrail_context(item["detail"])
        extra_parts = []
        if display_id is not None:
            extra_parts.append("display=%d" % display_id)
        if context:
            extra_parts.append("context=%s" % context)
        suffix = " " + " ".join(extra_parts) if extra_parts else ""
        print(
            "[capacity=%d burst=%d retry=%s delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s] guardrail=%s%s" % (
                item["capacity"],
                item["burst"],
                format_retry_label(item["post_retry_count"]),
                format_retry_delay_label(item["post_retry_delay_ms"]),
                format_stress_post_gap_label(item["stress_post_gap_ms"]),
                format_max_core_task_retry_limit_label(item["max_core_task_retries"]),
                format_max_single_post_retry_limit_label(item["max_single_post_core_task_retries"]),
                format_full_queue_guard_label(item["fail_on_full_core_task_queue"]),
                format_runtime_guardrail_config_suffix(item["max_shutdown_queue_wait_ms"], item["max_shutdown_exec_time_ms"]),
                guardrail,
                suffix,
            )
        )

    if not any_guardrail:
        print("No guardrail-tagged runtime failures in scan results.")


def print_failure_guardrail_rollup(results):
    print("\n=== Failure Guardrail Rollup ===")

    guardrail_stats = {}
    for item in results:
        if is_pass(item):
            continue
        guardrail = extract_failure_guardrail(item["detail"])
        if not guardrail:
            continue
        guardrail_stats[guardrail] = guardrail_stats.get(guardrail, 0) + 1

    if not guardrail_stats:
        print("No guardrail-tagged runtime failures in scan results.")
        return

    for guardrail in sorted(guardrail_stats):
        print("guardrail=%s failures=%d" % (guardrail, guardrail_stats[guardrail]))


def print_failure_guardrail_leaders(results):
    print("\n=== Failure Guardrail Leaders ===")

    leader_stats = {}
    for item in results:
        if is_pass(item):
            continue
        guardrail = extract_failure_guardrail(item["detail"])
        if not guardrail:
            continue
        key = (
            item["capacity"],
            item["burst"],
            item["post_retry_count"],
            item["post_retry_delay_ms"],
            item["stress_post_gap_ms"],
            item["max_core_task_retries"],
            item["max_single_post_core_task_retries"],
            item["fail_on_full_core_task_queue"],
            item["max_shutdown_queue_wait_ms"],
            item["max_shutdown_exec_time_ms"],
        )
        guardrail_map = leader_stats.setdefault(key, {})
        guardrail_map[guardrail] = guardrail_map.get(guardrail, 0) + 1

    if not leader_stats:
        print("No guardrail-tagged runtime failures in scan results.")
        return

    for key in sorted(leader_stats):
        capacity, burst, post_retry_count, post_retry_delay_ms, stress_post_gap_ms, max_core_task_retries, max_single_post_core_task_retries, fail_on_full_core_task_queue, max_shutdown_queue_wait_ms, max_shutdown_exec_time_ms = key
        guardrail_map = leader_stats[key]
        leader_guardrail = None
        leader_count = None
        for guardrail in sorted(guardrail_map):
            count = guardrail_map[guardrail]
            if leader_count is None or count > leader_count:
                leader_count = count
                leader_guardrail = guardrail
        print(
            "[capacity=%d burst=%d retry=%s delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s] dominant_guardrail=%s failures=%d" % (
                capacity,
                burst,
                format_retry_label(post_retry_count),
                format_retry_delay_label(post_retry_delay_ms),
                format_stress_post_gap_label(stress_post_gap_ms),
                format_max_core_task_retry_limit_label(max_core_task_retries),
                format_max_single_post_retry_limit_label(max_single_post_core_task_retries),
                format_full_queue_guard_label(fail_on_full_core_task_queue),
                format_runtime_guardrail_config_suffix(max_shutdown_queue_wait_ms, max_shutdown_exec_time_ms),
                leader_guardrail,
                leader_count,
            )
        )


def run_command(cmd):
    result = subprocess.run(
        cmd,
        cwd=ROOT_DIR,
        capture_output=True,
        text=True,
    )
    combined_output = (result.stdout or "") + "\n" + (result.stderr or "")
    return result.returncode, combined_output


def run_command_with_retries(cmd, stage, retry_count, retry_delay_ms):
    rc = 0
    output_text = ""
    reason = "command failed"
    attempts = retry_count + 1

    for attempt in range(1, attempts + 1):
        rc, output_text = run_command(cmd)
        if rc == 0:
            return rc, output_text, "", attempt

        reason = collect_failure_reason(output_text)
        if attempt >= attempts or not should_retry_failure(stage, reason):
            return rc, output_text, reason, attempt

        print("%s retry %d/%d: %s" % (stage.capitalize(), attempt, retry_count, reason))
        if retry_delay_ms > 0:
            time.sleep(retry_delay_ms / 1000.0)

    return rc, output_text, reason, attempts


def build_compile_command(py, capacity, burst, case_jobs, bits64, post_retry_count, post_retry_delay_ms, stress_post_gap_ms, app):
    cmd = [
        py,
        str(ROOT_DIR / "scripts" / "code_compile_check.py"),
        "--case-jobs",
        str(case_jobs),
        "--queue-capacity-probe",
        str(capacity),
    ]
    if app:
        cmd.extend(["--app", app])
    else:
        cmd.extend(["--scope", "multi-display"])
    if burst > 0:
        cmd.extend(["--queue-stress-probe", str(burst)])
    if post_retry_count >= 0:
        cmd.extend(["--queue-post-retry-probe", str(post_retry_count)])
    if post_retry_delay_ms >= 0:
        cmd.extend(["--queue-post-retry-delay-probe", str(post_retry_delay_ms)])
    if stress_post_gap_ms >= 0:
        cmd.extend(["--queue-stress-post-gap-probe", str(stress_post_gap_ms)])
    if bits64:
        cmd.append("--bits64")
    return cmd


def build_runtime_command(py,
                          capacity,
                          burst,
                          jobs,
                          timeout,
                          bits64,
                          keep_screenshots,
                          max_core_task_retries,
                          max_single_post_core_task_retries,
                          fail_on_full_core_task_queue,
                          max_shutdown_queue_wait_ms,
                          max_shutdown_exec_time_ms,
                          post_retry_count,
                          post_retry_delay_ms,
                          stress_post_gap_ms,
                          app):
    cmd = [
        py,
        str(ROOT_DIR / "scripts" / "code_runtime_check.py"),
        "--jobs",
        str(jobs),
        "--timeout",
        str(timeout),
        "--queue-capacity-probe",
        str(capacity),
    ]
    if app:
        cmd.extend(["--app", app])
    else:
        cmd.extend(["--scope", "multi-display"])
    if burst > 0:
        cmd.extend(["--queue-stress-probe", str(burst)])
    if post_retry_count >= 0:
        cmd.extend(["--queue-post-retry-probe", str(post_retry_count)])
    if post_retry_delay_ms >= 0:
        cmd.extend(["--queue-post-retry-delay-probe", str(post_retry_delay_ms)])
    if stress_post_gap_ms >= 0:
        cmd.extend(["--queue-stress-post-gap-probe", str(stress_post_gap_ms)])
    if bits64:
        cmd.append("--bits64")
    if keep_screenshots:
        cmd.append("--keep-screenshots")
    if max_core_task_retries >= 0:
        cmd.extend(["--max-core-task-retries", str(max_core_task_retries)])
    if max_single_post_core_task_retries >= 0:
        cmd.extend(["--max-single-post-core-task-retries", str(max_single_post_core_task_retries)])
    if fail_on_full_core_task_queue:
        cmd.append("--fail-on-full-core-task-queue")
    if max_shutdown_queue_wait_ms >= 0:
        cmd.extend(["--max-shutdown-queue-wait-ms", str(max_shutdown_queue_wait_ms)])
    if max_shutdown_exec_time_ms >= 0:
        cmd.extend(["--max-shutdown-exec-time-ms", str(max_shutdown_exec_time_ms)])
    return cmd


def apply_scan_profile(args):
    if not args.profile:
        return False

    profile = SCAN_PROFILES.get(args.profile)
    if profile is None:
        return False

    parser_defaults = {
        "capacities": "1,16",
        "bursts": "0,8",
        "retry_counts": "",
        "retry_delay_values": "",
        "stress_post_gap_values": "",
    }

    applied = False
    for field, default_value in parser_defaults.items():
        current_value = getattr(args, field)
        if current_value != default_value:
            continue
        profile_value = profile.get(field)
        if profile_value is None:
            continue
        setattr(args, field, profile_value)
        applied = True

    return applied


def build_scan_invocation(py,
                          app,
                          compact_app_view,
                          exit_on_first_failure,
                          capacities,
                          bursts,
                          post_retry_counts,
                          post_retry_delay_values,
                          stress_post_gap_values,
                          max_core_task_retry_limits,
                          max_single_post_retry_limits,
                          full_queue_guard_values,
                          max_shutdown_queue_wait_values,
                          max_shutdown_exec_time_values,
                          case_jobs,
                          jobs,
                          timeout,
                          bits64):
    cmd = [
        py,
        str(ROOT_DIR / "scripts" / "checks" / "multi_display_core_task_stress_scan.py"),
        "--capacities",
        ",".join(str(value) for value in capacities),
        "--bursts",
        ",".join(str(value) for value in bursts),
        "--retry-counts",
        ",".join(str(value) for value in post_retry_counts),
        "--retry-delay-values",
        ",".join(str(value) for value in post_retry_delay_values),
        "--stress-post-gap-values",
        ",".join(str(value) for value in stress_post_gap_values),
        "--max-core-task-retry-values",
        ",".join(str(value) for value in max_core_task_retry_limits),
        "--max-single-post-core-task-retry-values",
        ",".join(str(value) for value in max_single_post_retry_limits),
        "--fail-on-full-core-task-queue-values",
        ",".join("1" if value else "0" for value in full_queue_guard_values),
        "--max-shutdown-queue-wait-ms-values",
        ",".join(str(value) for value in max_shutdown_queue_wait_values),
        "--max-shutdown-exec-time-ms-values",
        ",".join(str(value) for value in max_shutdown_exec_time_values),
        "--case-jobs",
        str(case_jobs),
        "--jobs",
        str(jobs),
        "--timeout",
        str(timeout),
    ]
    if app:
        cmd.extend(["--app", app])
    if compact_app_view:
        cmd.append("--compact-app-view")
    if exit_on_first_failure:
        cmd.append("--exit-on-first-failure")
    if bits64:
        cmd.append("--bits64")
    return cmd


def build_suggested_repro_command_map(py,
                                      args,
                                      results,
                                      capacities,
                                      bursts,
                                      post_retry_counts,
                                      post_retry_delay_values,
                                      stress_post_gap_values,
                                      max_core_task_retry_limits,
                                      max_single_post_retry_limits,
                                      full_queue_guard_values,
                                      max_shutdown_queue_wait_values,
                                      max_shutdown_exec_time_values):
    commands = {}
    rerun_cmd = build_scan_invocation(
        py,
        args.app,
        args.compact_app_view,
        args.exit_on_first_failure,
        capacities,
        bursts,
        post_retry_counts,
        post_retry_delay_values,
        stress_post_gap_values,
        max_core_task_retry_limits,
        max_single_post_retry_limits,
        full_queue_guard_values,
        max_shutdown_queue_wait_values,
        max_shutdown_exec_time_values,
        args.case_jobs,
        args.jobs,
        args.timeout,
        args.bits64,
    )
    commands["rerun_current_scan"] = format_command(rerun_cmd)

    if not args.app:
        return commands

    passing_items = sorted((item for item in results if is_pass(item)), key=build_result_sort_key)
    if passing_items:
        minimum_passing = passing_items[0]
        replay_cmd = build_scan_invocation(
            py,
            args.app,
            True,
            args.exit_on_first_failure,
            [minimum_passing["capacity"]],
            [minimum_passing["burst"]],
            [minimum_passing["post_retry_count"]],
            [minimum_passing["post_retry_delay_ms"]],
            [minimum_passing["stress_post_gap_ms"]],
            [minimum_passing["max_core_task_retries"]],
            [minimum_passing["max_single_post_core_task_retries"]],
            [minimum_passing["fail_on_full_core_task_queue"]],
            [minimum_passing["max_shutdown_queue_wait_ms"]],
            [minimum_passing["max_shutdown_exec_time_ms"]],
            args.case_jobs,
            args.jobs,
            args.timeout,
            args.bits64,
        )
        commands["replay_minimum_passing"] = format_command(replay_cmd)

    failing_items = sorted((item for item in results if not is_pass(item)), key=build_result_sort_key)
    if failing_items:
        first_failing = failing_items[0]
        replay_cmd = build_scan_invocation(
            py,
            args.app,
            True,
            args.exit_on_first_failure,
            [first_failing["capacity"]],
            [first_failing["burst"]],
            [first_failing["post_retry_count"]],
            [first_failing["post_retry_delay_ms"]],
            [first_failing["stress_post_gap_ms"]],
            [first_failing["max_core_task_retries"]],
            [first_failing["max_single_post_core_task_retries"]],
            [first_failing["fail_on_full_core_task_queue"]],
            [first_failing["max_shutdown_queue_wait_ms"]],
            [first_failing["max_shutdown_exec_time_ms"]],
            args.case_jobs,
            args.jobs,
            args.timeout,
            args.bits64,
        )
        commands["replay_first_failure"] = format_command(replay_cmd)

    return commands


def build_first_failure_context(args, results):
    if not args.app:
        return {}

    failing_items = sorted((item for item in results if not is_pass(item)), key=build_result_sort_key)
    if not failing_items:
        return {}

    first_failing = failing_items[0]
    return {
        "app": extract_failure_app(first_failing["detail"]) or args.app,
        "stage": extract_failure_stage(first_failing["detail"]),
        "progress": extract_failure_progress(first_failing["detail"]),
        "contention": classify_failure_contention(first_failing["detail"]),
        "guardrail": extract_failure_guardrail(first_failing["detail"]),
        "guardrail_display": extract_failure_guardrail_display(first_failing["detail"]),
        "guardrail_context": extract_failure_guardrail_context(first_failing["detail"]),
    }


def print_suggested_repro_commands(py,
                                   args,
                                   results,
                                   capacities,
                                   bursts,
                                   post_retry_counts,
                                   post_retry_delay_values,
                                   stress_post_gap_values,
                                   max_core_task_retry_limits,
                                   max_single_post_retry_limits,
                                   full_queue_guard_values,
                                   max_shutdown_queue_wait_values,
                                   max_shutdown_exec_time_values):
    print("\n=== Suggested Repro Commands ===")
    for key, value in build_suggested_repro_command_map(
        py,
        args,
        results,
        capacities,
        bursts,
        post_retry_counts,
        post_retry_delay_values,
        stress_post_gap_values,
        max_core_task_retry_limits,
        max_single_post_retry_limits,
        full_queue_guard_values,
        max_shutdown_queue_wait_values,
        max_shutdown_exec_time_values,
    ).items():
        print("%s: %s" % (key, value))
    first_failure_context = build_first_failure_context(args, results)
    if first_failure_context:
        context_parts = []
        for field in ("app", "stage", "progress", "contention", "guardrail", "guardrail_context"):
            value = first_failure_context[field]
            if value:
                context_parts.append("%s=%s" % (field, value))
        if first_failure_context["guardrail_display"] is not None:
            context_parts.append("guardrail_display=%d" % first_failure_context["guardrail_display"])
        if context_parts:
            print("replay_first_failure_context: %s" % " ".join(context_parts))


def build_scan_export_metadata(args,
                               results,
                               capacities,
                               bursts,
                               post_retry_counts,
                               post_retry_delay_values,
                               stress_post_gap_values,
                               max_core_task_retry_limits,
                               max_single_post_retry_limits,
                               full_queue_guard_values,
                               max_shutdown_queue_wait_values,
                               max_shutdown_exec_time_values,
                               stopped_early=False,
                               stop_reason=""):
    planned_combinations = (
        len(capacities)
        * len(bursts)
        * len(post_retry_counts)
        * len(post_retry_delay_values)
        * len(stress_post_gap_values)
        * len(max_core_task_retry_limits)
        * len(max_single_post_retry_limits)
        * len(full_queue_guard_values)
        * len(max_shutdown_queue_wait_values)
        * len(max_shutdown_exec_time_values)
    )
    executed_combinations = len(results)
    return {
        "exit_on_first_failure": args.exit_on_first_failure,
        "stopped_early": stopped_early,
        "stop_reason": stop_reason or "",
        "planned_combinations": planned_combinations,
        "executed_combinations": executed_combinations,
        "remaining_combinations": max(planned_combinations - executed_combinations, 0),
    }


def build_suggested_repro_payload(py,
                                  args,
                                  results,
                                  capacities,
                                  bursts,
                                  post_retry_counts,
                                  post_retry_delay_values,
                                  stress_post_gap_values,
                                  max_core_task_retry_limits,
                                  max_single_post_retry_limits,
                                  full_queue_guard_values,
                                  max_shutdown_queue_wait_values,
                                  max_shutdown_exec_time_values,
                                  stopped_early=False,
                                  stop_reason=""):
    export_metadata = build_scan_export_metadata(
        args,
        results,
        capacities,
        bursts,
        post_retry_counts,
        post_retry_delay_values,
        stress_post_gap_values,
        max_core_task_retry_limits,
        max_single_post_retry_limits,
        full_queue_guard_values,
        max_shutdown_queue_wait_values,
        max_shutdown_exec_time_values,
        stopped_early=stopped_early,
        stop_reason=stop_reason,
    )
    return {
        "app": args.app,
        "profile": args.profile,
        "compact_app_view": args.compact_app_view,
        "exit_on_first_failure": export_metadata["exit_on_first_failure"],
        "stopped_early": export_metadata["stopped_early"],
        "stop_reason": export_metadata["stop_reason"],
        "planned_combinations": export_metadata["planned_combinations"],
        "executed_combinations": export_metadata["executed_combinations"],
        "remaining_combinations": export_metadata["remaining_combinations"],
        "suggested_repro_commands": build_suggested_repro_command_map(
            py,
            args,
            results,
            capacities,
            bursts,
            post_retry_counts,
            post_retry_delay_values,
            stress_post_gap_values,
            max_core_task_retry_limits,
            max_single_post_retry_limits,
            full_queue_guard_values,
            max_shutdown_queue_wait_values,
            max_shutdown_exec_time_values,
        ),
        "replay_first_failure_context": build_first_failure_context(args, results),
    }


def build_suggested_repro_json_text(py,
                                    args,
                                    results,
                                    capacities,
                                    bursts,
                                    post_retry_counts,
                                    post_retry_delay_values,
                                    stress_post_gap_values,
                                    max_core_task_retry_limits,
                                    max_single_post_retry_limits,
                                    full_queue_guard_values,
                                    max_shutdown_queue_wait_values,
                                    max_shutdown_exec_time_values,
                                    stopped_early=False,
                                    stop_reason="",
                                    compact=False):
    return json.dumps(
        build_suggested_repro_payload(
            py,
            args,
            results,
            capacities,
            bursts,
            post_retry_counts,
            post_retry_delay_values,
            stress_post_gap_values,
            max_core_task_retry_limits,
            max_single_post_retry_limits,
            full_queue_guard_values,
            max_shutdown_queue_wait_values,
            max_shutdown_exec_time_values,
            stopped_early=stopped_early,
            stop_reason=stop_reason,
        ),
        ensure_ascii=False,
        indent=None if compact else 2,
        separators=(",", ":") if compact else None,
        sort_keys=True,
    )


def print_suggested_repro_json(py,
                               args,
                               results,
                               capacities,
                               bursts,
                               post_retry_counts,
                               post_retry_delay_values,
                               stress_post_gap_values,
                               max_core_task_retry_limits,
                               max_single_post_retry_limits,
                               full_queue_guard_values,
                               max_shutdown_queue_wait_values,
                               max_shutdown_exec_time_values,
                               stopped_early=False,
                               stop_reason=""):
    print("\n=== Suggested Repro JSON ===")
    print(build_suggested_repro_json_text(
        py,
        args,
        results,
        capacities,
        bursts,
        post_retry_counts,
        post_retry_delay_values,
        stress_post_gap_values,
        max_core_task_retry_limits,
        max_single_post_retry_limits,
        full_queue_guard_values,
        max_shutdown_queue_wait_values,
        max_shutdown_exec_time_values,
        stopped_early=stopped_early,
        stop_reason=stop_reason,
    ))


def print_suggested_repro_json_compact(py,
                                       args,
                                       results,
                                       capacities,
                                       bursts,
                                       post_retry_counts,
                                       post_retry_delay_values,
                                       stress_post_gap_values,
                                       max_core_task_retry_limits,
                                       max_single_post_retry_limits,
                                       full_queue_guard_values,
                                       max_shutdown_queue_wait_values,
                                       max_shutdown_exec_time_values,
                                       stopped_early=False,
                                       stop_reason=""):
    print("\n=== Suggested Repro JSON Compact ===")
    print(build_suggested_repro_json_text(
        py,
        args,
        results,
        capacities,
        bursts,
        post_retry_counts,
        post_retry_delay_values,
        stress_post_gap_values,
        max_core_task_retry_limits,
        max_single_post_retry_limits,
        full_queue_guard_values,
        max_shutdown_queue_wait_values,
        max_shutdown_exec_time_values,
        stopped_early=stopped_early,
        stop_reason=stop_reason,
        compact=True,
    ))


def write_suggested_repro_json_file(py,
                                    args,
                                    results,
                                    capacities,
                                    bursts,
                                    post_retry_counts,
                                    post_retry_delay_values,
                                    stress_post_gap_values,
                                    max_core_task_retry_limits,
                                    max_single_post_retry_limits,
                                    full_queue_guard_values,
                                    max_shutdown_queue_wait_values,
                                    max_shutdown_exec_time_values,
                                    stopped_early=False,
                                    stop_reason=""):
    if not args.suggested_repro_json_file:
        return

    output_path = Path(args.suggested_repro_json_file)
    if not output_path.is_absolute():
        output_path = ROOT_DIR / output_path
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(
        build_suggested_repro_json_text(
            py,
            args,
            results,
            capacities,
            bursts,
            post_retry_counts,
            post_retry_delay_values,
            stress_post_gap_values,
            max_core_task_retry_limits,
            max_single_post_retry_limits,
            full_queue_guard_values,
            max_shutdown_queue_wait_values,
            max_shutdown_exec_time_values,
            stopped_early=stopped_early,
            stop_reason=stop_reason,
        ) + "\n",
        encoding="utf-8",
    )
    print("\nSuggested repro JSON saved to %s" % output_path)


def build_json_summary(py,
                       args,
                       results,
                       capacities,
                       bursts,
                       post_retry_counts,
                       post_retry_delay_values,
                       stress_post_gap_values,
                       max_core_task_retry_limits,
                       max_single_post_retry_limits,
                       full_queue_guard_values,
                       max_shutdown_queue_wait_values,
                       max_shutdown_exec_time_values,
                       stopped_early=False,
                       stop_reason=""):
    export_metadata = build_scan_export_metadata(
        args,
        results,
        capacities,
        bursts,
        post_retry_counts,
        post_retry_delay_values,
        stress_post_gap_values,
        max_core_task_retry_limits,
        max_single_post_retry_limits,
        full_queue_guard_values,
        max_shutdown_queue_wait_values,
        max_shutdown_exec_time_values,
        stopped_early=stopped_early,
        stop_reason=stop_reason,
    )
    result_entries = build_result_entries(args, results)
    pass_count = sum(1 for item in results if is_pass(item))
    fail_count = len(results) - pass_count
    summary = {
        "app": args.app,
        "profile": args.profile,
        "compact_app_view": args.compact_app_view,
        "scan": {
            "capacities": capacities,
            "bursts": bursts,
            "retry_counts": post_retry_counts,
            "retry_delay_values": post_retry_delay_values,
            "stress_post_gap_values": stress_post_gap_values,
            "max_core_task_retry_values": max_core_task_retry_limits,
            "max_single_post_retry_values": max_single_post_retry_limits,
            "full_queue_guard_values": full_queue_guard_values,
            "max_shutdown_queue_wait_values": max_shutdown_queue_wait_values,
            "max_shutdown_exec_time_values": max_shutdown_exec_time_values,
            "case_jobs": args.case_jobs,
            "jobs": args.jobs,
            "timeout": args.timeout,
            "bits64": args.bits64,
            "exit_on_first_failure": export_metadata["exit_on_first_failure"],
            "stopped_early": export_metadata["stopped_early"],
            "stop_reason": export_metadata["stop_reason"],
            "planned_combinations": export_metadata["planned_combinations"],
            "executed_combinations": export_metadata["executed_combinations"],
            "remaining_combinations": export_metadata["remaining_combinations"],
        },
        "result_counts": {
            "total": len(results),
            "pass": pass_count,
            "fail": fail_count,
        },
        "per_app_thresholds": build_per_app_threshold_entries(args.app, results, capacities, bursts),
        "suggested_repro_commands": build_suggested_repro_command_map(
            py,
            args,
            results,
            capacities,
            bursts,
            post_retry_counts,
            post_retry_delay_values,
            stress_post_gap_values,
            max_core_task_retry_limits,
            max_single_post_retry_limits,
            full_queue_guard_values,
            max_shutdown_queue_wait_values,
            max_shutdown_exec_time_values,
        ),
        "replay_first_failure_context": build_first_failure_context(args, results),
        "suggested_repro": build_suggested_repro_payload(
            py,
            args,
            results,
            capacities,
            bursts,
            post_retry_counts,
            post_retry_delay_values,
            stress_post_gap_values,
            max_core_task_retry_limits,
            max_single_post_retry_limits,
            full_queue_guard_values,
            max_shutdown_queue_wait_values,
            max_shutdown_exec_time_values,
            stopped_early=stopped_early,
            stop_reason=stop_reason,
        ),
        "results": result_entries,
    }
    return summary


def build_result_entries(args, results, shared_fields=None):
    entries = []
    combination_rank_map = build_result_combination_rank_map(results)
    for index, item in enumerate(results):
        item_app = extract_failure_app(item["detail"]) or args.app
        progress = extract_failure_progress(item["detail"])
        progress_step, progress_total = parse_progress_step(progress)
        stage = extract_failure_stage(item["detail"])
        contention = classify_failure_contention(item["detail"])
        guardrail = extract_failure_guardrail(item["detail"])
        shutdown_rollups_by_app = extract_shutdown_rollups_by_app(item["detail"])
        shutdown_rollup = select_shutdown_rollup(shutdown_rollups_by_app, item_app)
        entry = {
            "result_index": index,
            "scan_app": args.app,
            "profile": args.profile,
            "compact_app_view": args.compact_app_view,
            "combination_key": build_scan_combination_key(
                item["capacity"],
                item["burst"],
                item["post_retry_count"],
                item["post_retry_delay_ms"],
                item["stress_post_gap_ms"],
                item["max_core_task_retries"],
                item["max_single_post_core_task_retries"],
                item["fail_on_full_core_task_queue"],
                item["max_shutdown_queue_wait_ms"],
                item["max_shutdown_exec_time_ms"],
            ),
            "combination_rank": combination_rank_map[build_result_sort_key(item)],
            "status": "PASS" if is_pass(item) else "FAIL",
            "failure_kind": classify_result_failure_kind(item),
            "compile_ok": item["compile_ok"],
            "runtime_ok": item["runtime_ok"],
            "capacity": item["capacity"],
            "burst": item["burst"],
            "retry": item["post_retry_count"],
            "delay": item["post_retry_delay_ms"],
            "gap": item["stress_post_gap_ms"],
            "max_core_task_retries": item["max_core_task_retries"],
            "max_single_post_retry": item["max_single_post_core_task_retries"],
            "full_queue_guard": item["fail_on_full_core_task_queue"],
            "max_shutdown_queue_wait_ms": item["max_shutdown_queue_wait_ms"],
            "max_shutdown_exec_time_ms": item["max_shutdown_exec_time_ms"],
            "app": item_app,
            "stage": stage,
            "has_stage_tag": bool(stage),
            "progress": progress,
            "progress_step": progress_step,
            "progress_total": progress_total,
            "contention": contention,
            "has_contention_tag": bool(contention),
            "guardrail": guardrail,
            "has_guardrail_tag": bool(guardrail),
            "guardrail_display": extract_failure_guardrail_display(item["detail"]),
            "guardrail_context": extract_failure_guardrail_context(item["detail"]),
            "shutdown_rollups_by_app": shutdown_rollups_by_app,
            "detail": item["detail"],
        }
        entry.update(shutdown_rollup)
        if shared_fields:
            entry.update(shared_fields)
        entries.append(entry)
    return entries


def filter_result_entries(entries,
                          status_filter,
                          capacity_filter=None,
                          burst_filter=None,
                          combination_rank_filter=None,
                          has_stage_tag_filter=None,
                          has_contention_tag_filter=None,
                          has_guardrail_tag_filter=None,
                          stage_filter="",
                          contention_filter="",
                          guardrail_filter="",
                          guardrail_display_filter=None,
                          guardrail_context_filter="",
                          progress_filter="",
                          progress_range_filter="",
                          retry_filter=None,
                          delay_filter=None,
                          gap_filter=None,
                          max_core_retries_filter=None,
                          max_single_post_retry_filter=None,
                          full_queue_guard_filter=None,
                          shutdown_peak_queue_min_filter=None,
                          shutdown_max_queue_wait_ms_min_filter=None,
                          shutdown_max_exec_time_ms_min_filter=None):
    normalized_filter = (status_filter or "all").lower()
    if normalized_filter == "all":
        filtered_entries = list(entries)
    elif normalized_filter == "compile-fail":
        filtered_entries = [entry for entry in entries if entry["failure_kind"] == "compile"]
    elif normalized_filter == "runtime-fail":
        filtered_entries = [entry for entry in entries if entry["failure_kind"] == "runtime"]
    else:
        target_status = normalized_filter.upper()
        filtered_entries = [entry for entry in entries if entry["status"] == target_status]

    if capacity_filter is not None:
        filtered_entries = [entry for entry in filtered_entries if entry["capacity"] == capacity_filter]

    if burst_filter is not None:
        filtered_entries = [entry for entry in filtered_entries if entry["burst"] == burst_filter]

    if combination_rank_filter is not None:
        filtered_entries = [entry for entry in filtered_entries if entry["combination_rank"] == combination_rank_filter]

    if has_stage_tag_filter is not None:
        filtered_entries = [entry for entry in filtered_entries if entry["has_stage_tag"] == has_stage_tag_filter]

    if has_contention_tag_filter is not None:
        filtered_entries = [entry for entry in filtered_entries if entry["has_contention_tag"] == has_contention_tag_filter]

    if has_guardrail_tag_filter is not None:
        filtered_entries = [entry for entry in filtered_entries if entry["has_guardrail_tag"] == has_guardrail_tag_filter]

    normalized_stage_filter = (stage_filter or "").strip()
    if normalized_stage_filter:
        filtered_entries = [entry for entry in filtered_entries if entry["stage"] == normalized_stage_filter]

    normalized_contention_filter = (contention_filter or "").strip().lower()
    if normalized_contention_filter:
        filtered_entries = [
            entry for entry in filtered_entries
            if (entry["contention"] or "").lower() == normalized_contention_filter
        ]

    normalized_guardrail_filter = (guardrail_filter or "").strip().lower()
    if normalized_guardrail_filter:
        filtered_entries = [
            entry for entry in filtered_entries
            if (entry["guardrail"] or "").lower() == normalized_guardrail_filter
        ]

    if guardrail_display_filter is not None:
        filtered_entries = [
            entry for entry in filtered_entries
            if entry["guardrail_display"] == guardrail_display_filter
        ]

    normalized_guardrail_context_filter = (guardrail_context_filter or "").strip()
    if normalized_guardrail_context_filter:
        filtered_entries = [
            entry for entry in filtered_entries
            if entry["guardrail_context"] == normalized_guardrail_context_filter
        ]

    normalized_progress_filter = (progress_filter or "").strip()
    if normalized_progress_filter:
        filtered_entries = [entry for entry in filtered_entries if entry["progress"] == normalized_progress_filter]

    normalized_progress_range_filter = (progress_range_filter or "").strip()
    if normalized_progress_range_filter:
        filtered_entries = [
            entry for entry in filtered_entries
            if progress_matches_range(entry["progress_step"], entry["progress_total"], normalized_progress_range_filter)
        ]

    if retry_filter is not None:
        filtered_entries = [entry for entry in filtered_entries if entry["retry"] == retry_filter]

    if delay_filter is not None:
        filtered_entries = [entry for entry in filtered_entries if entry["delay"] == delay_filter]

    if gap_filter is not None:
        filtered_entries = [entry for entry in filtered_entries if entry["gap"] == gap_filter]

    if max_core_retries_filter is not None:
        filtered_entries = [entry for entry in filtered_entries if entry["max_core_task_retries"] == max_core_retries_filter]

    if max_single_post_retry_filter is not None:
        filtered_entries = [entry for entry in filtered_entries if entry["max_single_post_retry"] == max_single_post_retry_filter]

    if full_queue_guard_filter is not None:
        filtered_entries = [entry for entry in filtered_entries if entry["full_queue_guard"] == full_queue_guard_filter]

    if shutdown_peak_queue_min_filter is not None:
        filtered_entries = [
            entry for entry in filtered_entries
            if entry["shutdown_peak_queue"] is not None and entry["shutdown_peak_queue"] >= shutdown_peak_queue_min_filter
        ]

    if shutdown_max_queue_wait_ms_min_filter is not None:
        filtered_entries = [
            entry for entry in filtered_entries
            if entry["shutdown_max_queue_wait_ms"] is not None and entry["shutdown_max_queue_wait_ms"] >= shutdown_max_queue_wait_ms_min_filter
        ]

    if shutdown_max_exec_time_ms_min_filter is not None:
        filtered_entries = [
            entry for entry in filtered_entries
            if entry["shutdown_max_exec_time_ms"] is not None and entry["shutdown_max_exec_time_ms"] >= shutdown_max_exec_time_ms_min_filter
        ]
    return filtered_entries


def build_results_jsonl_filter_summary(args, count):
    parts = ["status=%s" % args.results_jsonl_status]
    if args.results_jsonl_capacity is not None:
        parts.append("capacity=%s" % args.results_jsonl_capacity)
    if args.results_jsonl_burst is not None:
        parts.append("burst=%s" % args.results_jsonl_burst)
    if args.results_jsonl_combination_rank is not None:
        parts.append("combination_rank=%s" % args.results_jsonl_combination_rank)
    if args.results_jsonl_has_stage_tag is not None:
        parts.append("has_stage_tag=%s" % args.results_jsonl_has_stage_tag)
    if args.results_jsonl_has_contention_tag is not None:
        parts.append("has_contention_tag=%s" % args.results_jsonl_has_contention_tag)
    if args.results_jsonl_has_guardrail_tag is not None:
        parts.append("has_guardrail_tag=%s" % args.results_jsonl_has_guardrail_tag)
    if args.results_jsonl_stage:
        parts.append("stage=%s" % args.results_jsonl_stage)
    if args.results_jsonl_contention:
        parts.append("contention=%s" % args.results_jsonl_contention)
    if args.results_jsonl_guardrail:
        parts.append("guardrail=%s" % args.results_jsonl_guardrail)
    if args.results_jsonl_guardrail_display is not None:
        parts.append("guardrail_display=%s" % args.results_jsonl_guardrail_display)
    if args.results_jsonl_guardrail_context:
        parts.append("guardrail_context=%s" % args.results_jsonl_guardrail_context)
    if args.results_jsonl_progress:
        parts.append("progress=%s" % args.results_jsonl_progress)
    if args.results_jsonl_progress_range:
        parts.append("progress_range=%s" % args.results_jsonl_progress_range)
    if args.results_jsonl_retry is not None:
        parts.append("retry=%s" % args.results_jsonl_retry)
    if args.results_jsonl_delay is not None:
        parts.append("delay=%s" % args.results_jsonl_delay)
    if args.results_jsonl_gap is not None:
        parts.append("gap=%s" % args.results_jsonl_gap)
    if args.results_jsonl_max_core_retries is not None:
        parts.append("max_core_task_retries=%s" % format_max_core_task_retry_limit_label(args.results_jsonl_max_core_retries))
    if args.results_jsonl_max_single_post_retry is not None:
        parts.append("max_single_post_retry=%s" % format_max_single_post_retry_limit_label(args.results_jsonl_max_single_post_retry))
    if args.results_jsonl_full_queue_guard is not None:
        parts.append("full_queue_guard=%s" % format_full_queue_guard_label(args.results_jsonl_full_queue_guard))
    if args.results_jsonl_shutdown_peak_queue_min is not None:
        parts.append("shutdown_peak_queue_min=%s" % args.results_jsonl_shutdown_peak_queue_min)
    if args.results_jsonl_shutdown_max_queue_wait_ms_min is not None:
        parts.append("shutdown_max_queue_wait_ms_min=%s" % args.results_jsonl_shutdown_max_queue_wait_ms_min)
    if args.results_jsonl_shutdown_max_exec_time_ms_min is not None:
        parts.append("shutdown_max_exec_time_ms_min=%s" % args.results_jsonl_shutdown_max_exec_time_ms_min)
    parts.append("count=%d" % count)
    return " ".join(parts)


def build_result_jsonl_lines(args,
                             results,
                             capacities,
                             bursts,
                             post_retry_counts,
                             post_retry_delay_values,
                             stress_post_gap_values,
                             max_core_task_retry_limits,
                             max_single_post_retry_limits,
                             full_queue_guard_values,
                             max_shutdown_queue_wait_values,
                             max_shutdown_exec_time_values,
                             stopped_early=False,
                             stop_reason=""):
    shared_fields = build_scan_export_metadata(
        args,
        results,
        capacities,
        bursts,
        post_retry_counts,
        post_retry_delay_values,
        stress_post_gap_values,
        max_core_task_retry_limits,
        max_single_post_retry_limits,
        full_queue_guard_values,
        max_shutdown_queue_wait_values,
        max_shutdown_exec_time_values,
        stopped_early=stopped_early,
        stop_reason=stop_reason,
    )
    filtered_entries = filter_result_entries(
        build_result_entries(args, results, shared_fields=shared_fields),
        args.results_jsonl_status,
        capacity_filter=args.results_jsonl_capacity,
        burst_filter=args.results_jsonl_burst,
        combination_rank_filter=args.results_jsonl_combination_rank,
        has_stage_tag_filter=bool(args.results_jsonl_has_stage_tag) if args.results_jsonl_has_stage_tag is not None else None,
        has_contention_tag_filter=bool(args.results_jsonl_has_contention_tag) if args.results_jsonl_has_contention_tag is not None else None,
        has_guardrail_tag_filter=bool(args.results_jsonl_has_guardrail_tag) if args.results_jsonl_has_guardrail_tag is not None else None,
        stage_filter=args.results_jsonl_stage,
        contention_filter=args.results_jsonl_contention,
        guardrail_filter=args.results_jsonl_guardrail,
        guardrail_display_filter=args.results_jsonl_guardrail_display,
        guardrail_context_filter=args.results_jsonl_guardrail_context,
        progress_filter=args.results_jsonl_progress,
        progress_range_filter=args.results_jsonl_progress_range,
        retry_filter=args.results_jsonl_retry,
        delay_filter=args.results_jsonl_delay,
        gap_filter=args.results_jsonl_gap,
        max_core_retries_filter=args.results_jsonl_max_core_retries,
        max_single_post_retry_filter=args.results_jsonl_max_single_post_retry,
        full_queue_guard_filter=args.results_jsonl_full_queue_guard,
        shutdown_peak_queue_min_filter=args.results_jsonl_shutdown_peak_queue_min,
        shutdown_max_queue_wait_ms_min_filter=args.results_jsonl_shutdown_max_queue_wait_ms_min,
        shutdown_max_exec_time_ms_min_filter=args.results_jsonl_shutdown_max_exec_time_ms_min,
    )
    lines = [
        json.dumps(entry, ensure_ascii=False, separators=(",", ":"), sort_keys=True)
        for entry in filtered_entries
    ]
    return filtered_entries, lines


def print_json_summary(py,
                       args,
                       results,
                       capacities,
                       bursts,
                       post_retry_counts,
                       post_retry_delay_values,
                       stress_post_gap_values,
                       max_core_task_retry_limits,
                       max_single_post_retry_limits,
                       full_queue_guard_values,
                       max_shutdown_queue_wait_values,
                       max_shutdown_exec_time_values,
                       stopped_early=False,
                       stop_reason=""):
    print("\n=== JSON Summary ===")
    print(build_json_summary_text(
        py,
        args,
        results,
        capacities,
        bursts,
        post_retry_counts,
        post_retry_delay_values,
        stress_post_gap_values,
        max_core_task_retry_limits,
        max_single_post_retry_limits,
        full_queue_guard_values,
        max_shutdown_queue_wait_values,
        max_shutdown_exec_time_values,
        stopped_early=stopped_early,
        stop_reason=stop_reason,
    ))


def print_json_summary_compact(py,
                               args,
                               results,
                               capacities,
                               bursts,
                               post_retry_counts,
                               post_retry_delay_values,
                               stress_post_gap_values,
                               max_core_task_retry_limits,
                               max_single_post_retry_limits,
                               full_queue_guard_values,
                               max_shutdown_queue_wait_values,
                               max_shutdown_exec_time_values,
                               stopped_early=False,
                               stop_reason=""):
    print("\n=== JSON Summary Compact ===")
    print(build_json_summary_text(
        py,
        args,
        results,
        capacities,
        bursts,
        post_retry_counts,
        post_retry_delay_values,
        stress_post_gap_values,
        max_core_task_retry_limits,
        max_single_post_retry_limits,
        full_queue_guard_values,
        max_shutdown_queue_wait_values,
        max_shutdown_exec_time_values,
        stopped_early=stopped_early,
        stop_reason=stop_reason,
        compact=True,
    ))


def build_json_summary_text(py,
                            args,
                            results,
                            capacities,
                            bursts,
                            post_retry_counts,
                            post_retry_delay_values,
                            stress_post_gap_values,
                            max_core_task_retry_limits,
                            max_single_post_retry_limits,
                            full_queue_guard_values,
                            max_shutdown_queue_wait_values,
                            max_shutdown_exec_time_values,
                            stopped_early=False,
                            stop_reason="",
                            compact=False):
    return json.dumps(
        build_json_summary(
            py,
            args,
            results,
            capacities,
            bursts,
            post_retry_counts,
            post_retry_delay_values,
            stress_post_gap_values,
            max_core_task_retry_limits,
            max_single_post_retry_limits,
            full_queue_guard_values,
            max_shutdown_queue_wait_values,
            max_shutdown_exec_time_values,
            stopped_early=stopped_early,
            stop_reason=stop_reason,
        ),
        ensure_ascii=False,
        indent=None if compact else 2,
        separators=(",", ":") if compact else None,
        sort_keys=True,
    )


def write_json_summary_file(py,
                            args,
                            results,
                            capacities,
                            bursts,
                            post_retry_counts,
                            post_retry_delay_values,
                            stress_post_gap_values,
                            max_core_task_retry_limits,
                            max_single_post_retry_limits,
                            full_queue_guard_values,
                            max_shutdown_queue_wait_values,
                            max_shutdown_exec_time_values,
                            stopped_early=False,
                            stop_reason=""):
    if not args.json_summary_file:
        return

    output_path = Path(args.json_summary_file)
    if not output_path.is_absolute():
        output_path = ROOT_DIR / output_path
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(
        build_json_summary_text(
            py,
            args,
            results,
            capacities,
            bursts,
            post_retry_counts,
            post_retry_delay_values,
            stress_post_gap_values,
            max_core_task_retry_limits,
            max_single_post_retry_limits,
            full_queue_guard_values,
            max_shutdown_queue_wait_values,
            max_shutdown_exec_time_values,
            stopped_early=stopped_early,
            stop_reason=stop_reason,
        ) + "\n",
        encoding="utf-8",
    )
    print("\nJSON summary saved to %s" % output_path)


def write_results_jsonl_file(args,
                             results,
                             capacities,
                             bursts,
                             post_retry_counts,
                             post_retry_delay_values,
                             stress_post_gap_values,
                             max_core_task_retry_limits,
                             max_single_post_retry_limits,
                             full_queue_guard_values,
                             max_shutdown_queue_wait_values,
                             max_shutdown_exec_time_values,
                             stopped_early=False,
                             stop_reason=""):
    if not args.results_jsonl_file:
        return

    output_path = Path(args.results_jsonl_file)
    if not output_path.is_absolute():
        output_path = ROOT_DIR / output_path
    output_path.parent.mkdir(parents=True, exist_ok=True)
    filtered_entries, lines = build_result_jsonl_lines(
        args,
        results,
        capacities,
        bursts,
        post_retry_counts,
        post_retry_delay_values,
        stress_post_gap_values,
        max_core_task_retry_limits,
        max_single_post_retry_limits,
        full_queue_guard_values,
        max_shutdown_queue_wait_values,
        max_shutdown_exec_time_values,
        stopped_early=stopped_early,
        stop_reason=stop_reason,
    )
    output_path.write_text("\n".join(lines) + ("\n" if lines else ""), encoding="utf-8")
    print("\nResults JSONL saved to %s (%s)" % (output_path, build_results_jsonl_filter_summary(args, len(filtered_entries))))


def print_results_jsonl(args,
                        results,
                        capacities,
                        bursts,
                        post_retry_counts,
                        post_retry_delay_values,
                        stress_post_gap_values,
                        max_core_task_retry_limits,
                        max_single_post_retry_limits,
                        full_queue_guard_values,
                        max_shutdown_queue_wait_values,
                        max_shutdown_exec_time_values,
                        stopped_early=False,
                        stop_reason=""):
    filtered_entries, lines = build_result_jsonl_lines(
        args,
        results,
        capacities,
        bursts,
        post_retry_counts,
        post_retry_delay_values,
        stress_post_gap_values,
        max_core_task_retry_limits,
        max_single_post_retry_limits,
        full_queue_guard_values,
        max_shutdown_queue_wait_values,
        max_shutdown_exec_time_values,
        stopped_early=stopped_early,
        stop_reason=stop_reason,
    )
    print("\n=== Results JSONL ===")
    print(build_results_jsonl_filter_summary(args, len(filtered_entries)))
    for line in lines:
        print(line)


def filter_per_app_threshold_entries(entries,
                                     status_filter,
                                     capacity_filter=None,
                                     burst_filter=None,
                                     scope_rank_filter=None,
                                     scope_key_filter="",
                                     scope_guardrail_key_filter="",
                                     scope_shutdown_queue_wait_filter=None,
                                     scope_shutdown_exec_time_filter=None,
                                     all_passed_filter=None,
                                     all_failed_filter=None,
                                     stage_filter="",
                                     contention_filter="",
                                     guardrail_filter="",
                                     progress_range_filter="",
                                     progress_overlap_filter="",
                                     minimum_passing_rank_filter=None,
                                     minimum_retry_filter=None,
                                     minimum_delay_filter=None,
                                     minimum_gap_filter=None,
                                     minimum_max_core_retries_filter=None,
                                     minimum_max_single_post_retry_filter=None,
                                     minimum_full_queue_guard_filter=None,
                                     minimum_shutdown_peak_queue_min_filter=None,
                                     minimum_shutdown_max_queue_wait_ms_min_filter=None,
                                     minimum_shutdown_max_exec_time_ms_min_filter=None):
    normalized_filter = (status_filter or "all").lower()
    if normalized_filter == "all":
        filtered_entries = list(entries)
    elif normalized_filter == "has-pass":
        filtered_entries = [entry for entry in entries if entry["minimum_passing"] is not None]
    else:
        filtered_entries = [entry for entry in entries if entry["minimum_passing"] is None]

    if capacity_filter is not None:
        filtered_entries = [entry for entry in filtered_entries if entry["capacity"] == capacity_filter]

    if burst_filter is not None:
        filtered_entries = [entry for entry in filtered_entries if entry["burst"] == burst_filter]

    if scope_rank_filter is not None:
        filtered_entries = [entry for entry in filtered_entries if entry["scope_rank"] == scope_rank_filter]

    normalized_scope_key_filter = (scope_key_filter or "").strip()
    if normalized_scope_key_filter:
        filtered_entries = [
            entry for entry in filtered_entries
            if entry["scope_key"] == normalized_scope_key_filter
        ]

    normalized_scope_guardrail_key_filter = (scope_guardrail_key_filter or "").strip()
    if normalized_scope_guardrail_key_filter:
        filtered_entries = [
            entry for entry in filtered_entries
            if entry["scope_guardrail_key"] == normalized_scope_guardrail_key_filter
        ]

    if scope_shutdown_queue_wait_filter is not None:
        filtered_entries = [
            entry for entry in filtered_entries
            if entry["scope_max_shutdown_queue_wait_ms"] == scope_shutdown_queue_wait_filter
        ]

    if scope_shutdown_exec_time_filter is not None:
        filtered_entries = [
            entry for entry in filtered_entries
            if entry["scope_max_shutdown_exec_time_ms"] == scope_shutdown_exec_time_filter
        ]

    if all_passed_filter is not None:
        filtered_entries = [entry for entry in filtered_entries if entry["all_passed"] == all_passed_filter]

    if all_failed_filter is not None:
        filtered_entries = [entry for entry in filtered_entries if entry["all_failed"] == all_failed_filter]

    normalized_stage_filter = (stage_filter or "").strip()
    if normalized_stage_filter:
        filtered_entries = [entry for entry in filtered_entries if entry["dominant_stage"] == normalized_stage_filter]

    normalized_contention_filter = (contention_filter or "").strip().lower()
    if normalized_contention_filter:
        filtered_entries = [
            entry for entry in filtered_entries
            if (entry["dominant_contention"] or "").lower() == normalized_contention_filter
        ]

    normalized_guardrail_filter = (guardrail_filter or "").strip().lower()
    if normalized_guardrail_filter:
        filtered_entries = [
            entry for entry in filtered_entries
            if (entry["dominant_guardrail"] or "").lower() == normalized_guardrail_filter
        ]

    normalized_progress_range_filter = (progress_range_filter or "").strip()
    if normalized_progress_range_filter:
        filtered_entries = [
            entry for entry in filtered_entries
            if entry["progress_range"] == normalized_progress_range_filter
        ]

    normalized_progress_overlap_filter = (progress_overlap_filter or "").strip()
    if normalized_progress_overlap_filter:
        filtered_entries = [
            entry for entry in filtered_entries
            if progress_ranges_overlap(
                entry["progress_min_step"],
                entry["progress_max_step"],
                entry["progress_total"],
                normalized_progress_overlap_filter,
            )
        ]

    if minimum_passing_rank_filter is not None:
        filtered_entries = [
            entry for entry in filtered_entries
            if entry["minimum_passing_rank"] == minimum_passing_rank_filter
        ]

    if minimum_retry_filter is not None:
        filtered_entries = [
            entry for entry in filtered_entries
            if entry["minimum_passing"] is not None and entry["minimum_passing"]["retry"] == minimum_retry_filter
        ]

    if minimum_delay_filter is not None:
        filtered_entries = [
            entry for entry in filtered_entries
            if entry["minimum_passing"] is not None and entry["minimum_passing"]["delay"] == minimum_delay_filter
        ]

    if minimum_gap_filter is not None:
        filtered_entries = [
            entry for entry in filtered_entries
            if entry["minimum_passing"] is not None and entry["minimum_passing"]["gap"] == minimum_gap_filter
        ]

    if minimum_max_core_retries_filter is not None:
        filtered_entries = [
            entry for entry in filtered_entries
            if entry["minimum_passing"] is not None and entry["minimum_passing"]["max_core_task_retries"] == minimum_max_core_retries_filter
        ]

    if minimum_max_single_post_retry_filter is not None:
        filtered_entries = [
            entry for entry in filtered_entries
            if entry["minimum_passing"] is not None and entry["minimum_passing"]["max_single_post_retry"] == minimum_max_single_post_retry_filter
        ]

    if minimum_full_queue_guard_filter is not None:
        filtered_entries = [
            entry for entry in filtered_entries
            if entry["minimum_passing"] is not None and entry["minimum_passing"]["full_queue_guard"] == minimum_full_queue_guard_filter
        ]

    if minimum_shutdown_peak_queue_min_filter is not None:
        filtered_entries = [
            entry for entry in filtered_entries
            if entry["minimum_passing_shutdown_peak_queue"] is not None
            and entry["minimum_passing_shutdown_peak_queue"] >= minimum_shutdown_peak_queue_min_filter
        ]

    if minimum_shutdown_max_queue_wait_ms_min_filter is not None:
        filtered_entries = [
            entry for entry in filtered_entries
            if entry["minimum_passing_shutdown_max_queue_wait_ms"] is not None
            and entry["minimum_passing_shutdown_max_queue_wait_ms"] >= minimum_shutdown_max_queue_wait_ms_min_filter
        ]

    if minimum_shutdown_max_exec_time_ms_min_filter is not None:
        filtered_entries = [
            entry for entry in filtered_entries
            if entry["minimum_passing_shutdown_max_exec_time_ms"] is not None
            and entry["minimum_passing_shutdown_max_exec_time_ms"] >= minimum_shutdown_max_exec_time_ms_min_filter
        ]
    return filtered_entries


def build_per_app_thresholds_jsonl_filter_summary(args, count):
    parts = ["status=%s" % args.per_app_thresholds_status]
    normalized_scope_key = args.per_app_thresholds_scope_key.strip()
    normalized_scope_guardrail_key = args.per_app_thresholds_scope_guardrail_key.strip()
    parsed_scope_key = parse_per_app_scope_key_text(
        normalized_scope_key,
        "--per-app-thresholds-scope-key",
    )
    parsed_scope_guardrail_key = parse_scope_guardrail_key_text(
        normalized_scope_guardrail_key,
        "--per-app-thresholds-scope-guardrail-key",
    )
    redundant_filters = []
    if args.per_app_thresholds_capacity is not None:
        parts.append("capacity=%s" % args.per_app_thresholds_capacity)
    if args.per_app_thresholds_burst is not None:
        parts.append("burst=%s" % args.per_app_thresholds_burst)
    if args.per_app_thresholds_scope_rank is not None:
        parts.append("scope_rank=%s" % args.per_app_thresholds_scope_rank)
    if normalized_scope_key:
        parts.append("scope_key=%s" % normalized_scope_key)
        parts.append("scope_key_source=explicit")
        if args.app:
            redundant_filters.append("app")
        if args.per_app_thresholds_capacity is not None:
            redundant_filters.append("capacity")
        if args.per_app_thresholds_burst is not None:
            redundant_filters.append("burst")
    if normalized_scope_guardrail_key:
        parts.append("scope_guardrail_key=%s" % normalized_scope_guardrail_key)
        parts.append("scope_guardrail_key_source=explicit")
        if normalized_scope_key:
            redundant_filters.append("scope_guardrail_key")
    if args.per_app_thresholds_scope_shutdown_queue_wait_ms is not None:
        parts.append("scope_shutdown_queue_wait_ms=%s" % format_shutdown_guardrail_label(args.per_app_thresholds_scope_shutdown_queue_wait_ms))
        if normalized_scope_key or normalized_scope_guardrail_key:
            redundant_filters.append("scope_shutdown_queue_wait_ms")
    if args.per_app_thresholds_scope_shutdown_exec_time_ms is not None:
        parts.append("scope_shutdown_exec_time_ms=%s" % format_shutdown_guardrail_label(args.per_app_thresholds_scope_shutdown_exec_time_ms))
        if normalized_scope_key or normalized_scope_guardrail_key:
            redundant_filters.append("scope_shutdown_exec_time_ms")
    if (not normalized_scope_guardrail_key
            and args.per_app_thresholds_scope_shutdown_queue_wait_ms is not None
            and args.per_app_thresholds_scope_shutdown_exec_time_ms is not None):
        parts.append(
            "scope_guardrail_key=%s" % format_runtime_guardrail_config_suffix(
                args.per_app_thresholds_scope_shutdown_queue_wait_ms,
                args.per_app_thresholds_scope_shutdown_exec_time_ms,
            ).strip()
        )
        parts.append("scope_guardrail_key_source=derived(scope_shutdown_*)")
    if (not normalized_scope_key
            and args.app
            and args.per_app_thresholds_capacity is not None
            and args.per_app_thresholds_burst is not None):
        derived_scope_shutdown_queue_wait_ms = args.per_app_thresholds_scope_shutdown_queue_wait_ms
        derived_scope_shutdown_exec_time_ms = args.per_app_thresholds_scope_shutdown_exec_time_ms
        scope_key_source = ""
        if (parsed_scope_guardrail_key is not None
                and derived_scope_shutdown_queue_wait_ms is None
                and derived_scope_shutdown_exec_time_ms is None):
            derived_scope_shutdown_queue_wait_ms = parsed_scope_guardrail_key["max_shutdown_queue_wait_ms"]
            derived_scope_shutdown_exec_time_ms = parsed_scope_guardrail_key["max_shutdown_exec_time_ms"]
            scope_key_source = "derived(app,capacity,burst,scope_guardrail_key)"
        elif derived_scope_shutdown_queue_wait_ms is not None and derived_scope_shutdown_exec_time_ms is not None:
            scope_key_source = "derived(app,capacity,burst,scope_shutdown_*)"
        if derived_scope_shutdown_queue_wait_ms is not None and derived_scope_shutdown_exec_time_ms is not None:
            parts.append(
                "scope_key=%s" % build_per_app_scope_key(
                    args.app,
                    args.per_app_thresholds_capacity,
                    args.per_app_thresholds_burst,
                    derived_scope_shutdown_queue_wait_ms,
                    derived_scope_shutdown_exec_time_ms,
                )
            )
            if scope_key_source:
                parts.append("scope_key_source=%s" % scope_key_source)
    if redundant_filters:
        ordered_redundant_filters = []
        for name in redundant_filters:
            if name not in ordered_redundant_filters:
                ordered_redundant_filters.append(name)
        parts.append("redundant_filters=%s" % ",".join(ordered_redundant_filters))
    if args.per_app_thresholds_all_passed is not None:
        parts.append("all_passed=%s" % args.per_app_thresholds_all_passed)
    if args.per_app_thresholds_all_failed is not None:
        parts.append("all_failed=%s" % args.per_app_thresholds_all_failed)
    if args.per_app_thresholds_stage:
        parts.append("stage=%s" % args.per_app_thresholds_stage)
    if args.per_app_thresholds_contention:
        parts.append("contention=%s" % args.per_app_thresholds_contention)
    if args.per_app_thresholds_guardrail:
        parts.append("guardrail=%s" % args.per_app_thresholds_guardrail)
    if args.per_app_thresholds_progress_range:
        parts.append("progress_range=%s" % args.per_app_thresholds_progress_range)
    if args.per_app_thresholds_progress_overlap:
        parts.append("progress_overlap=%s" % args.per_app_thresholds_progress_overlap)
    if args.per_app_thresholds_min_passing_rank is not None:
        parts.append("minimum_passing_rank=%s" % args.per_app_thresholds_min_passing_rank)
    if args.per_app_thresholds_min_retry is not None:
        parts.append("minimum_retry=%s" % args.per_app_thresholds_min_retry)
    if args.per_app_thresholds_min_delay is not None:
        parts.append("minimum_delay=%s" % args.per_app_thresholds_min_delay)
    if args.per_app_thresholds_min_gap is not None:
        parts.append("minimum_gap=%s" % args.per_app_thresholds_min_gap)
    if args.per_app_thresholds_min_max_core_retries is not None:
        parts.append("minimum_max_core_task_retries=%s" % format_max_core_task_retry_limit_label(args.per_app_thresholds_min_max_core_retries))
    if args.per_app_thresholds_min_max_single_post_retry is not None:
        parts.append("minimum_max_single_post_retry=%s" % format_max_single_post_retry_limit_label(args.per_app_thresholds_min_max_single_post_retry))
    if args.per_app_thresholds_min_full_queue_guard is not None:
        parts.append("minimum_full_queue_guard=%s" % format_full_queue_guard_label(args.per_app_thresholds_min_full_queue_guard))
    if args.per_app_thresholds_min_shutdown_peak_queue_min is not None:
        parts.append("minimum_shutdown_peak_queue_min=%s" % args.per_app_thresholds_min_shutdown_peak_queue_min)
    if args.per_app_thresholds_min_shutdown_max_queue_wait_ms_min is not None:
        parts.append("minimum_shutdown_max_queue_wait_ms_min=%s" % args.per_app_thresholds_min_shutdown_max_queue_wait_ms_min)
    if args.per_app_thresholds_min_shutdown_max_exec_time_ms_min is not None:
        parts.append("minimum_shutdown_max_exec_time_ms_min=%s" % args.per_app_thresholds_min_shutdown_max_exec_time_ms_min)
    parts.append("count=%d" % count)
    return " ".join(parts)


def build_per_app_threshold_jsonl_lines(args,
                                        results,
                                        capacities,
                                        bursts,
                                        post_retry_counts,
                                        post_retry_delay_values,
                                        stress_post_gap_values,
                                        max_core_task_retry_limits,
                                        max_single_post_retry_limits,
                                        full_queue_guard_values,
                                        max_shutdown_queue_wait_values,
                                        max_shutdown_exec_time_values,
                                        stopped_early=False,
                                        stop_reason=""):
    shared_fields = build_scan_export_metadata(
        args,
        results,
        capacities,
        bursts,
        post_retry_counts,
        post_retry_delay_values,
        stress_post_gap_values,
        max_core_task_retry_limits,
        max_single_post_retry_limits,
        full_queue_guard_values,
        max_shutdown_queue_wait_values,
        max_shutdown_exec_time_values,
        stopped_early=stopped_early,
        stop_reason=stop_reason,
    )
    filtered_entries = filter_per_app_threshold_entries(
        build_per_app_threshold_entries(args.app, results, capacities, bursts, shared_fields=shared_fields),
        args.per_app_thresholds_status,
        capacity_filter=args.per_app_thresholds_capacity,
        burst_filter=args.per_app_thresholds_burst,
        scope_rank_filter=args.per_app_thresholds_scope_rank,
        scope_key_filter=args.per_app_thresholds_scope_key,
        scope_guardrail_key_filter=args.per_app_thresholds_scope_guardrail_key,
        scope_shutdown_queue_wait_filter=args.per_app_thresholds_scope_shutdown_queue_wait_ms,
        scope_shutdown_exec_time_filter=args.per_app_thresholds_scope_shutdown_exec_time_ms,
        all_passed_filter=bool(args.per_app_thresholds_all_passed) if args.per_app_thresholds_all_passed is not None else None,
        all_failed_filter=bool(args.per_app_thresholds_all_failed) if args.per_app_thresholds_all_failed is not None else None,
        stage_filter=args.per_app_thresholds_stage,
        contention_filter=args.per_app_thresholds_contention,
        guardrail_filter=args.per_app_thresholds_guardrail,
        progress_range_filter=args.per_app_thresholds_progress_range,
        progress_overlap_filter=args.per_app_thresholds_progress_overlap,
        minimum_passing_rank_filter=args.per_app_thresholds_min_passing_rank,
        minimum_retry_filter=args.per_app_thresholds_min_retry,
        minimum_delay_filter=args.per_app_thresholds_min_delay,
        minimum_gap_filter=args.per_app_thresholds_min_gap,
        minimum_max_core_retries_filter=args.per_app_thresholds_min_max_core_retries,
        minimum_max_single_post_retry_filter=args.per_app_thresholds_min_max_single_post_retry,
        minimum_full_queue_guard_filter=args.per_app_thresholds_min_full_queue_guard,
        minimum_shutdown_peak_queue_min_filter=args.per_app_thresholds_min_shutdown_peak_queue_min,
        minimum_shutdown_max_queue_wait_ms_min_filter=args.per_app_thresholds_min_shutdown_max_queue_wait_ms_min,
        minimum_shutdown_max_exec_time_ms_min_filter=args.per_app_thresholds_min_shutdown_max_exec_time_ms_min,
    )
    lines = [
        json.dumps(entry, ensure_ascii=False, separators=(",", ":"), sort_keys=True)
        for entry in filtered_entries
    ]
    return filtered_entries, lines


def write_per_app_thresholds_jsonl_file(args,
                                        results,
                                        capacities,
                                        bursts,
                                        post_retry_counts,
                                        post_retry_delay_values,
                                        stress_post_gap_values,
                                        max_core_task_retry_limits,
                                        max_single_post_retry_limits,
                                        full_queue_guard_values,
                                        max_shutdown_queue_wait_values,
                                        max_shutdown_exec_time_values,
                                        stopped_early=False,
                                        stop_reason=""):
    if not args.per_app_thresholds_jsonl_file:
        return

    output_path = Path(args.per_app_thresholds_jsonl_file)
    if not output_path.is_absolute():
        output_path = ROOT_DIR / output_path
    output_path.parent.mkdir(parents=True, exist_ok=True)
    entries, lines = build_per_app_threshold_jsonl_lines(
        args,
        results,
        capacities,
        bursts,
        post_retry_counts,
        post_retry_delay_values,
        stress_post_gap_values,
        max_core_task_retry_limits,
        max_single_post_retry_limits,
        full_queue_guard_values,
        max_shutdown_queue_wait_values,
        max_shutdown_exec_time_values,
        stopped_early=stopped_early,
        stop_reason=stop_reason,
    )
    output_path.write_text("\n".join(lines) + ("\n" if lines else ""), encoding="utf-8")
    print("\nPer-app thresholds JSONL saved to %s (%s)" % (
        output_path,
        build_per_app_thresholds_jsonl_filter_summary(args, len(entries)),
    ))


def print_per_app_thresholds_jsonl(args,
                                   results,
                                   capacities,
                                   bursts,
                                   post_retry_counts,
                                   post_retry_delay_values,
                                   stress_post_gap_values,
                                   max_core_task_retry_limits,
                                   max_single_post_retry_limits,
                                   full_queue_guard_values,
                                   max_shutdown_queue_wait_values,
                                   max_shutdown_exec_time_values,
                                   stopped_early=False,
                                   stop_reason=""):
    entries, lines = build_per_app_threshold_jsonl_lines(
        args,
        results,
        capacities,
        bursts,
        post_retry_counts,
        post_retry_delay_values,
        stress_post_gap_values,
        max_core_task_retry_limits,
        max_single_post_retry_limits,
        full_queue_guard_values,
        max_shutdown_queue_wait_values,
        max_shutdown_exec_time_values,
        stopped_early=stopped_early,
        stop_reason=stop_reason,
    )
    print("\n=== Per-App Thresholds JSONL ===")
    print(build_per_app_thresholds_jsonl_filter_summary(args, len(entries)))
    for line in lines:
        print(line)


def main():
    parser = argparse.ArgumentParser(
        description="Scan multi-display core-task queue capacity/stress combinations.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=(
            "Examples:\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --capacities 1,16 --bursts 0,8\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --capacities 1,2,4,8,16 --bursts 4,8,12 --keep-screenshots\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --profile retry-delay\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --profile retry-delay --compact-app-view\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplay --profile delay-gap --compact-app-view\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --capacities 1 --bursts 8 --retry-counts 0,1,2,4\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --capacities 1 --bursts 8 --retry-counts 1 --retry-delay-values 0,1,2\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --capacities 1 --bursts 8 --queue-stress-post-gap-probe 2\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --capacities 1 --bursts 8 --stress-post-gap-values 0,1,2\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --capacities 1 --bursts 8 --retry-counts 1 --retry-delay-values 0,1,2 --stress-post-gap-values 0,1,2\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --capacities 1 --bursts 8 --retry-counts 0,1,2 --retry-delay-values 0,1,2 --stress-post-gap-values 1\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --profile retry-delay --compact-app-view --emit-json-summary-compact\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --capacities 1 --bursts 8 --retry-counts 1 --retry-delay-values 1 --stress-post-gap-values 1 --results-jsonl-file runtime_check_output/multi_display_scan_results.jsonl\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --profile retry-delay --compact-app-view --results-jsonl-file runtime_check_output/multi_display_scan_failures.jsonl --results-jsonl-status fail\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --profile retry-delay --compact-app-view --results-jsonl-file runtime_check_output/multi_display_scan_runtime_failures.jsonl --results-jsonl-status runtime-fail\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --profile retry-delay --compact-app-view --emit-results-jsonl --results-jsonl-status runtime-fail --results-jsonl-contention self_inflight --results-jsonl-stage after_main_drag_1 --results-jsonl-progress 5/8\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --profile retry-delay --compact-app-view --emit-results-jsonl --results-jsonl-status runtime-fail --results-jsonl-stage after_main_drag_1 --results-jsonl-progress-range 4/8..6/8\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --profile retry-delay --compact-app-view --emit-results-jsonl --results-jsonl-status fail --results-jsonl-retry 1 --results-jsonl-delay 1 --results-jsonl-gap 1\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --emit-results-jsonl --results-jsonl-status pass --results-jsonl-capacity 16 --results-jsonl-burst 0\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --emit-results-jsonl --results-jsonl-status pass --results-jsonl-combination-rank 1\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --emit-results-jsonl --results-jsonl-status fail --results-jsonl-has-stage-tag 1 --results-jsonl-has-contention-tag 1\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --emit-results-jsonl --results-jsonl-status runtime-fail --results-jsonl-has-guardrail-tag 1 --results-jsonl-guardrail shutdown_exec_time\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --emit-results-jsonl --results-jsonl-status runtime-fail --results-jsonl-guardrail shutdown_exec_time --results-jsonl-guardrail-display 1 --results-jsonl-guardrail-context after_main_drag_1\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --emit-results-jsonl --results-jsonl-status pass --results-jsonl-max-core-retries -1 --results-jsonl-max-single-post-retry -1 --results-jsonl-full-queue-guard 0\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --emit-results-jsonl --results-jsonl-status pass --results-jsonl-shutdown-max-exec-time-ms-min 1\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --max-shutdown-queue-wait-ms 2 --max-shutdown-exec-time-ms 2\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --profile retry-delay --compact-app-view --emit-results-jsonl --results-jsonl-status fail\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --profile retry-delay --compact-app-view --emit-suggested-repro-json\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --profile retry-delay --compact-app-view --emit-suggested-repro-json-compact\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --profile retry-delay --compact-app-view --suggested-repro-json-file runtime_check_output/multi_display_repro.json\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --profile retry-delay --compact-app-view --per-app-thresholds-jsonl-file runtime_check_output/multi_display_thresholds.jsonl\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --profile retry-delay --compact-app-view --per-app-thresholds-jsonl-file runtime_check_output/multi_display_thresholds_has_pass.jsonl --per-app-thresholds-status has-pass\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --profile retry-delay --compact-app-view --emit-per-app-thresholds-jsonl --per-app-thresholds-contention self_inflight --per-app-thresholds-stage after_main_drag_1 --per-app-thresholds-progress-range 5/8..5/8\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --profile retry-delay --compact-app-view --emit-per-app-thresholds-jsonl --per-app-thresholds-stage after_main_drag_1 --per-app-thresholds-progress-overlap 4/8..6/8\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --emit-per-app-thresholds-jsonl --per-app-thresholds-status no-pass --per-app-thresholds-guardrail shutdown_exec_time\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --emit-per-app-thresholds-jsonl --per-app-thresholds-status all --per-app-thresholds-scope-key \"app=HelloMultiDisplayHetero capacity=32 burst=1 max_shutdown_queue_wait_ms=default max_shutdown_exec_time_ms=0\"\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --emit-per-app-thresholds-jsonl --per-app-thresholds-status all --per-app-thresholds-scope-guardrail-key \"max_shutdown_queue_wait_ms=default max_shutdown_exec_time_ms=0\"\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --emit-per-app-thresholds-jsonl --per-app-thresholds-status no-pass --per-app-thresholds-scope-shutdown-exec-time-ms 0\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --emit-per-app-thresholds-jsonl --per-app-thresholds-status all --per-app-thresholds-scope-shutdown-queue-wait-ms 0\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --profile retry-delay --compact-app-view --emit-per-app-thresholds-jsonl --per-app-thresholds-status has-pass --per-app-thresholds-min-retry 1 --per-app-thresholds-min-delay 1 --per-app-thresholds-min-gap 1\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --emit-per-app-thresholds-jsonl --per-app-thresholds-status has-pass --per-app-thresholds-capacity 16 --per-app-thresholds-burst 0\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --emit-per-app-thresholds-jsonl --per-app-thresholds-status all --per-app-thresholds-scope-rank 1 --per-app-thresholds-min-passing-rank 0\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --emit-per-app-thresholds-jsonl --per-app-thresholds-status all --per-app-thresholds-all-passed 1 --per-app-thresholds-all-failed 0\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --emit-per-app-thresholds-jsonl --per-app-thresholds-status has-pass --per-app-thresholds-min-max-core-retries -1 --per-app-thresholds-min-max-single-post-retry -1 --per-app-thresholds-min-full-queue-guard 0\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --emit-per-app-thresholds-jsonl --per-app-thresholds-status has-pass --per-app-thresholds-min-shutdown-max-exec-time-ms-min 1\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --profile retry-delay --compact-app-view --emit-per-app-thresholds-jsonl\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --app HelloMultiDisplayHetero --capacities 1 --bursts 8 --retry-counts 0,1 --retry-delay-values 0,1 --stress-post-gap-values 1 --compact-app-view --exit-on-first-failure --emit-json-summary-compact\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --capacities 32 --bursts 40 --max-core-task-retry-values -1,0\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --capacities 32 --bursts 40 --fail-on-full-core-task-queue-values 0,1\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --capacities 32 --bursts 34 --max-single-post-core-task-retries 0\n"
            "  python scripts/checks/multi_display_core_task_stress_scan.py --capacities 32 --bursts 34 --max-single-post-core-task-retry-values -1,0\n"
        ),
    )
    parser.add_argument("--app", type=str, default="", help="Scan one target app only, e.g. HelloMultiDisplay or HelloMultiDisplayHetero.")
    parser.add_argument("--profile", type=str, default="", help="Apply a preset scan profile: retry-delay, delay-gap, retry-gap.")
    parser.add_argument("--compact-app-view", action="store_true", help="When used with --app, prefer per-app summaries and suppress duplicated generic app-prefixed sections.")
    parser.add_argument("--emit-json-summary", action="store_true", help="Emit a machine-readable JSON summary after the text reports.")
    parser.add_argument("--emit-json-summary-compact", action="store_true", help="Emit the JSON summary as a compact single line after the text reports.")
    parser.add_argument("--emit-results-jsonl", action="store_true", help="Emit normalized scan results as JSONL after the text reports.")
    parser.add_argument("--emit-suggested-repro-json", action="store_true", help="Emit suggested repro commands/context as JSON after the text reports.")
    parser.add_argument("--emit-suggested-repro-json-compact", action="store_true", help="Emit suggested repro commands/context as a compact single-line JSON after the text reports.")
    parser.add_argument("--emit-per-app-thresholds-jsonl", action="store_true", help="Emit per-app threshold summary rows as JSONL after the text reports.")
    parser.add_argument("--json-summary-file", type=str, default="", help="Write the JSON summary to a file path. Relative paths are resolved from repo root.")
    parser.add_argument("--suggested-repro-json-file", type=str, default="", help="Write suggested repro commands/context to a JSON file path. Relative paths are resolved from repo root.")
    parser.add_argument("--results-jsonl-file", type=str, default="", help="Write normalized scan results to a JSONL file path. Relative paths are resolved from repo root.")
    parser.add_argument("--results-jsonl-status", type=str, default="all", help="Filter JSONL export rows by status: all, pass, fail, compile-fail, runtime-fail.")
    parser.add_argument("--results-jsonl-capacity", type=int, default=None, help="Filter results JSONL rows by exact capacity value after status filtering.")
    parser.add_argument("--results-jsonl-burst", type=int, default=None, help="Filter results JSONL rows by exact burst value after status filtering.")
    parser.add_argument("--results-jsonl-combination-rank", type=int, default=None, help="Filter results JSONL rows by exact combination_rank value after status filtering.")
    parser.add_argument("--results-jsonl-has-stage-tag", type=int, choices=[0, 1], default=None, help="Filter results JSONL rows by has_stage_tag after status filtering: 0=false, 1=true.")
    parser.add_argument("--results-jsonl-has-contention-tag", type=int, choices=[0, 1], default=None, help="Filter results JSONL rows by has_contention_tag after status filtering: 0=false, 1=true.")
    parser.add_argument("--results-jsonl-has-guardrail-tag", type=int, choices=[0, 1], default=None, help="Filter results JSONL rows by has_guardrail_tag after status filtering: 0=false, 1=true.")
    parser.add_argument("--results-jsonl-stage", type=str, default="", help="Filter results JSONL rows by exact stage value after status filtering.")
    parser.add_argument("--results-jsonl-contention", type=str, default="", help="Filter results JSONL rows by exact contention value after status filtering.")
    parser.add_argument("--results-jsonl-guardrail", type=str, default="", help="Filter results JSONL rows by exact guardrail value after status filtering.")
    parser.add_argument("--results-jsonl-guardrail-display", type=int, default=None, help="Filter results JSONL rows by exact guardrail_display value after status filtering.")
    parser.add_argument("--results-jsonl-guardrail-context", type=str, default="", help="Filter results JSONL rows by exact guardrail_context value after status filtering.")
    parser.add_argument("--results-jsonl-progress", type=str, default="", help="Filter results JSONL rows by exact progress value after status filtering, e.g. 5/8.")
    parser.add_argument("--results-jsonl-progress-range", type=str, default="", help="Filter results JSONL rows by numeric progress range after status filtering, e.g. 4/8..6/8.")
    parser.add_argument("--results-jsonl-retry", type=int, default=None, help="Filter results JSONL rows by exact retry value after status filtering.")
    parser.add_argument("--results-jsonl-delay", type=int, default=None, help="Filter results JSONL rows by exact delay value after status filtering.")
    parser.add_argument("--results-jsonl-gap", type=int, default=None, help="Filter results JSONL rows by exact gap value after status filtering.")
    parser.add_argument("--results-jsonl-max-core-retries", type=int, default=None, help="Filter results JSONL rows by exact max_core_task_retries value after status filtering.")
    parser.add_argument("--results-jsonl-max-single-post-retry", type=int, default=None, help="Filter results JSONL rows by exact max_single_post_retry value after status filtering.")
    parser.add_argument("--results-jsonl-full-queue-guard", type=int, choices=[0, 1], default=None, help="Filter results JSONL rows by full_queue_guard value after status filtering: 0=off, 1=on.")
    parser.add_argument("--results-jsonl-shutdown-peak-queue-min", type=int, default=None, help="Filter results JSONL rows by minimum shutdown_peak_queue after status filtering.")
    parser.add_argument("--results-jsonl-shutdown-max-queue-wait-ms-min", type=int, default=None, help="Filter results JSONL rows by minimum shutdown_max_queue_wait_ms after status filtering.")
    parser.add_argument("--results-jsonl-shutdown-max-exec-time-ms-min", type=int, default=None, help="Filter results JSONL rows by minimum shutdown_max_exec_time_ms after status filtering.")
    parser.add_argument("--per-app-thresholds-jsonl-file", type=str, default="", help="Write per-app threshold summary rows to a JSONL file path. Relative paths are resolved from repo root.")
    parser.add_argument("--per-app-thresholds-status", type=str, default="all", help="Filter per-app threshold JSONL rows by status: all, has-pass, no-pass.")
    parser.add_argument("--per-app-thresholds-capacity", type=int, default=None, help="Filter per-app threshold JSONL rows by exact capacity value after status filtering.")
    parser.add_argument("--per-app-thresholds-burst", type=int, default=None, help="Filter per-app threshold JSONL rows by exact burst value after status filtering.")
    parser.add_argument("--per-app-thresholds-scope-rank", type=int, default=None, help="Filter per-app threshold JSONL rows by exact scope_rank value after status filtering.")
    parser.add_argument("--per-app-thresholds-scope-key", type=str, default="", help="Filter per-app threshold JSONL rows by exact scope_key value after status filtering.")
    parser.add_argument("--per-app-thresholds-scope-guardrail-key", type=str, default="", help="Filter per-app threshold JSONL rows by exact scope_guardrail_key value after status filtering.")
    parser.add_argument("--per-app-thresholds-scope-shutdown-queue-wait-ms", type=int, default=None, help="Filter per-app threshold JSONL rows by exact scope_max_shutdown_queue_wait_ms value after status filtering (-1=default/disabled).")
    parser.add_argument("--per-app-thresholds-scope-shutdown-exec-time-ms", type=int, default=None, help="Filter per-app threshold JSONL rows by exact scope_max_shutdown_exec_time_ms value after status filtering (-1=default/disabled).")
    parser.add_argument("--per-app-thresholds-all-passed", type=int, choices=[0, 1], default=None, help="Filter per-app threshold JSONL rows by all_passed after status filtering: 0=false, 1=true.")
    parser.add_argument("--per-app-thresholds-all-failed", type=int, choices=[0, 1], default=None, help="Filter per-app threshold JSONL rows by all_failed after status filtering: 0=false, 1=true.")
    parser.add_argument("--per-app-thresholds-stage", type=str, default="", help="Filter per-app threshold JSONL rows by exact dominant stage value after status filtering.")
    parser.add_argument("--per-app-thresholds-contention", type=str, default="", help="Filter per-app threshold JSONL rows by exact dominant contention value after status filtering.")
    parser.add_argument("--per-app-thresholds-guardrail", type=str, default="", help="Filter per-app threshold JSONL rows by exact dominant guardrail value after status filtering.")
    parser.add_argument("--per-app-thresholds-progress-range", type=str, default="", help="Filter per-app threshold JSONL rows by exact progress_range value after status filtering, e.g. 5/8..5/8.")
    parser.add_argument("--per-app-thresholds-progress-overlap", type=str, default="", help="Filter per-app threshold JSONL rows by overlap with a numeric progress window after status filtering, e.g. 4/8..6/8.")
    parser.add_argument("--per-app-thresholds-min-passing-rank", type=int, default=None, help="Filter per-app threshold JSONL rows by exact minimum_passing_rank value after status filtering.")
    parser.add_argument("--per-app-thresholds-min-retry", type=int, default=None, help="Filter per-app threshold JSONL rows by exact minimum_passing.retry value after status filtering.")
    parser.add_argument("--per-app-thresholds-min-delay", type=int, default=None, help="Filter per-app threshold JSONL rows by exact minimum_passing.delay value after status filtering.")
    parser.add_argument("--per-app-thresholds-min-gap", type=int, default=None, help="Filter per-app threshold JSONL rows by exact minimum_passing.gap value after status filtering.")
    parser.add_argument("--per-app-thresholds-min-max-core-retries", type=int, default=None, help="Filter per-app threshold JSONL rows by exact minimum_passing.max_core_task_retries value after status filtering.")
    parser.add_argument("--per-app-thresholds-min-max-single-post-retry", type=int, default=None, help="Filter per-app threshold JSONL rows by exact minimum_passing.max_single_post_retry value after status filtering.")
    parser.add_argument("--per-app-thresholds-min-full-queue-guard", type=int, choices=[0, 1], default=None, help="Filter per-app threshold JSONL rows by minimum_passing.full_queue_guard after status filtering: 0=off, 1=on.")
    parser.add_argument("--per-app-thresholds-min-shutdown-peak-queue-min", type=int, default=None, help="Filter per-app threshold JSONL rows by minimum minimum_passing_shutdown_peak_queue after status filtering.")
    parser.add_argument("--per-app-thresholds-min-shutdown-max-queue-wait-ms-min", type=int, default=None, help="Filter per-app threshold JSONL rows by minimum minimum_passing_shutdown_max_queue_wait_ms after status filtering.")
    parser.add_argument("--per-app-thresholds-min-shutdown-max-exec-time-ms-min", type=int, default=None, help="Filter per-app threshold JSONL rows by minimum minimum_passing_shutdown_max_exec_time_ms after status filtering.")
    parser.add_argument("--exit-on-first-failure", action="store_true", help="Stop the scan after the first compile/runtime failure, but still emit collected summaries and machine-readable outputs.")
    parser.add_argument("--capacities", type=str, default="1,16", help="Comma-separated queue capacities (>0).")
    parser.add_argument("--bursts", type=str, default="0,8", help="Comma-separated stress burst counts (>=0).")
    parser.add_argument("--case-jobs", type=int, default=2, help="Parallel compile cases passed to code_compile_check.py.")
    parser.add_argument("--jobs", type=int, default=2, help="Parallel runtime jobs passed to code_runtime_check.py.")
    parser.add_argument("--timeout", type=int, default=10, help="Runtime timeout passed to code_runtime_check.py.")
    parser.add_argument("--bits64", action="store_true", help="Forward --bits64 to compile/runtime checks.")
    parser.add_argument("--keep-screenshots", action="store_true", help="Forward --keep-screenshots to runtime checks.")
    parser.add_argument("--retry-counts", type=str, default="", help="Comma-separated post retry counts to scan (-1 means current default).")
    parser.add_argument("--retry-delay-values", type=str, default="", help="Comma-separated post retry delay values in ms to scan (-1 means current default).")
    parser.add_argument("--queue-post-retry-probe", type=int, default=-1, help="Forward post retry count probe (-1=disabled, 0=disable retries).")
    parser.add_argument("--queue-post-retry-delay-probe", type=int, default=-1, help="Forward post retry delay probe in ms (-1=disabled, 0=no delay).")
    parser.add_argument("--queue-stress-post-gap-probe", type=int, default=-1, help="Forward stress post gap probe in ms (-1=disabled, 0=no gap).")
    parser.add_argument("--stress-post-gap-values", type=str, default="", help="Comma-separated stress post gap values in ms to scan (-1 means current default).")
    parser.add_argument("--max-core-task-retries", type=int, default=-1, help="Forward runtime retry ceiling (-1=disabled).")
    parser.add_argument("--max-core-task-retry-values", type=str, default="", help="Comma-separated runtime retry ceilings to scan (-1 means current default guardrail / disabled).")
    parser.add_argument("--max-single-post-core-task-retries", type=int, default=-1, help="Forward runtime single-post retry burst ceiling (-1=disabled).")
    parser.add_argument("--max-single-post-core-task-retry-values", type=str, default="", help="Comma-separated single-post retry burst ceilings to scan (-1 means current default guardrail / disabled).")
    parser.add_argument("--fail-on-full-core-task-queue", action="store_true", help="Forward runtime full-queue saturation failure.")
    parser.add_argument("--fail-on-full-core-task-queue-values", type=str, default="", help="Comma-separated full-queue guard values to scan (0=off, 1=on).")
    parser.add_argument("--max-shutdown-queue-wait-ms", type=int, default=-1, help="Forward runtime shutdown max_queue_wait_ms guardrail (-1=disabled).")
    parser.add_argument("--max-shutdown-exec-time-ms", type=int, default=-1, help="Forward runtime shutdown max_exec_time_ms guardrail (-1=disabled).")
    parser.add_argument("--max-shutdown-queue-wait-ms-values", type=str, default="", help="Comma-separated shutdown max_queue_wait_ms guardrails to scan (-1 means current default guardrail / disabled).")
    parser.add_argument("--max-shutdown-exec-time-ms-values", type=str, default="", help="Comma-separated shutdown max_exec_time_ms guardrails to scan (-1 means current default guardrail / disabled).")
    parser.add_argument("--compile-retries", type=int, default=1, help="Extra retries for non-zero compile command exits.")
    parser.add_argument("--runtime-retries", type=int, default=1, help="Extra retries for transient runtime command exits.")
    parser.add_argument("--retry-delay-ms", type=int, default=500, help="Delay between scan command retries in ms.")
    args = parser.parse_args()

    if args.profile and args.profile not in SCAN_PROFILES:
        print("Error: unknown --profile: %s" % args.profile)
        return 1
    profile_applied = apply_scan_profile(args)

    try:
        if args.results_jsonl_progress:
            validate_progress_text(args.results_jsonl_progress, "--results-jsonl-progress")
        if args.results_jsonl_progress_range:
            validate_progress_range_text(args.results_jsonl_progress_range, "--results-jsonl-progress-range")
        if args.per_app_thresholds_progress_range:
            validate_progress_range_text(args.per_app_thresholds_progress_range, "--per-app-thresholds-progress-range")
        if args.per_app_thresholds_progress_overlap:
            validate_progress_range_text(args.per_app_thresholds_progress_overlap, "--per-app-thresholds-progress-overlap")
    except ValueError as exc:
        print("Error: %s" % exc)
        return 1

    try:
        capacities = normalize_scan_values(parse_int_csv(args.capacities, allow_zero=False))
        bursts = normalize_scan_values(parse_int_csv(args.bursts, allow_zero=True))
    except ValueError as exc:
        print("Error: %s" % exc)
        return 1
    if args.retry_counts.strip():
        try:
            post_retry_counts = []
            for part in args.retry_counts.split(","):
                part = part.strip()
                if not part:
                    continue
                number = int(part)
                if number < -1:
                    raise ValueError("retry counts must be >= -1")
                post_retry_counts.append(number)
            if not post_retry_counts:
                raise ValueError("empty retry count list")
        except ValueError as exc:
            print("Error: %s" % exc)
            return 1
    else:
        post_retry_counts = [args.queue_post_retry_probe]
    post_retry_counts = normalize_scan_values(post_retry_counts)
    if args.retry_delay_values.strip():
        try:
            post_retry_delay_values = []
            for part in args.retry_delay_values.split(","):
                part = part.strip()
                if not part:
                    continue
                number = int(part)
                if number < -1:
                    raise ValueError("retry delay values must be >= -1")
                post_retry_delay_values.append(number)
            if not post_retry_delay_values:
                raise ValueError("empty retry delay list")
        except ValueError as exc:
            print("Error: %s" % exc)
            return 1
    else:
        post_retry_delay_values = [args.queue_post_retry_delay_probe]
    post_retry_delay_values = normalize_scan_values(post_retry_delay_values)
    if args.stress_post_gap_values.strip():
        try:
            stress_post_gap_values = []
            for part in args.stress_post_gap_values.split(","):
                part = part.strip()
                if not part:
                    continue
                number = int(part)
                if number < -1:
                    raise ValueError("stress post gap values must be >= -1")
                stress_post_gap_values.append(number)
            if not stress_post_gap_values:
                raise ValueError("empty stress post gap value list")
        except ValueError as exc:
            print("Error: %s" % exc)
            return 1
    else:
        stress_post_gap_values = [args.queue_stress_post_gap_probe]
    stress_post_gap_values = normalize_scan_values(stress_post_gap_values)
    if args.max_core_task_retry_values.strip():
        try:
            max_core_task_retry_limits = []
            for part in args.max_core_task_retry_values.split(","):
                part = part.strip()
                if not part:
                    continue
                number = int(part)
                if number < -1:
                    raise ValueError("max core task retry values must be >= -1")
                max_core_task_retry_limits.append(number)
            if not max_core_task_retry_limits:
                raise ValueError("empty max core task retry value list")
        except ValueError as exc:
            print("Error: %s" % exc)
            return 1
    else:
        max_core_task_retry_limits = [args.max_core_task_retries]
    max_core_task_retry_limits = normalize_scan_values(max_core_task_retry_limits)
    if args.max_single_post_core_task_retry_values.strip():
        try:
            max_single_post_retry_limits = []
            for part in args.max_single_post_core_task_retry_values.split(","):
                part = part.strip()
                if not part:
                    continue
                number = int(part)
                if number < -1:
                    raise ValueError("max single-post retry values must be >= -1")
                max_single_post_retry_limits.append(number)
            if not max_single_post_retry_limits:
                raise ValueError("empty max single-post retry value list")
        except ValueError as exc:
            print("Error: %s" % exc)
            return 1
    else:
        max_single_post_retry_limits = [args.max_single_post_core_task_retries]
    max_single_post_retry_limits = normalize_scan_values(max_single_post_retry_limits)
    if args.fail_on_full_core_task_queue_values.strip():
        try:
            full_queue_guard_values = []
            for part in args.fail_on_full_core_task_queue_values.split(","):
                part = part.strip()
                if not part:
                    continue
                number = int(part)
                if number not in (0, 1):
                    raise ValueError("full queue guard values must be 0 or 1")
                full_queue_guard_values.append(bool(number))
            if not full_queue_guard_values:
                raise ValueError("empty full queue guard value list")
        except ValueError as exc:
            print("Error: %s" % exc)
            return 1
    else:
        full_queue_guard_values = [args.fail_on_full_core_task_queue]
    full_queue_guard_values = normalize_scan_values(full_queue_guard_values)
    if args.max_shutdown_queue_wait_ms_values.strip():
        try:
            max_shutdown_queue_wait_values = []
            for part in args.max_shutdown_queue_wait_ms_values.split(","):
                part = part.strip()
                if not part:
                    continue
                number = int(part)
                if number < -1:
                    raise ValueError("max shutdown queue wait values must be >= -1")
                max_shutdown_queue_wait_values.append(number)
            if not max_shutdown_queue_wait_values:
                raise ValueError("empty max shutdown queue wait value list")
        except ValueError as exc:
            print("Error: %s" % exc)
            return 1
    else:
        max_shutdown_queue_wait_values = [args.max_shutdown_queue_wait_ms]
    max_shutdown_queue_wait_values = normalize_scan_values(max_shutdown_queue_wait_values)
    if args.max_shutdown_exec_time_ms_values.strip():
        try:
            max_shutdown_exec_time_values = []
            for part in args.max_shutdown_exec_time_ms_values.split(","):
                part = part.strip()
                if not part:
                    continue
                number = int(part)
                if number < -1:
                    raise ValueError("max shutdown exec time values must be >= -1")
                max_shutdown_exec_time_values.append(number)
            if not max_shutdown_exec_time_values:
                raise ValueError("empty max shutdown exec time value list")
        except ValueError as exc:
            print("Error: %s" % exc)
            return 1
    else:
        max_shutdown_exec_time_values = [args.max_shutdown_exec_time_ms]
    max_shutdown_exec_time_values = normalize_scan_values(max_shutdown_exec_time_values)
    if args.max_core_task_retries < -1:
        print("Error: --max-core-task-retries must be >= -1")
        return 1
    if args.max_single_post_core_task_retries < -1:
        print("Error: --max-single-post-core-task-retries must be >= -1")
        return 1
    if args.max_shutdown_queue_wait_ms < -1:
        print("Error: --max-shutdown-queue-wait-ms must be >= -1")
        return 1
    if args.max_shutdown_exec_time_ms < -1:
        print("Error: --max-shutdown-exec-time-ms must be >= -1")
        return 1
    if args.per_app_thresholds_scope_shutdown_queue_wait_ms is not None and args.per_app_thresholds_scope_shutdown_queue_wait_ms < -1:
        print("Error: --per-app-thresholds-scope-shutdown-queue-wait-ms must be >= -1")
        return 1
    if args.per_app_thresholds_scope_shutdown_exec_time_ms is not None and args.per_app_thresholds_scope_shutdown_exec_time_ms < -1:
        print("Error: --per-app-thresholds-scope-shutdown-exec-time-ms must be >= -1")
        return 1
    if args.queue_post_retry_probe < -1:
        print("Error: --queue-post-retry-probe must be >= -1")
        return 1
    if args.queue_post_retry_delay_probe < -1:
        print("Error: --queue-post-retry-delay-probe must be >= -1")
        return 1
    if args.queue_stress_post_gap_probe < -1:
        print("Error: --queue-stress-post-gap-probe must be >= -1")
        return 1
    if args.compile_retries < 0:
        print("Error: --compile-retries must be >= 0")
        return 1
    if args.runtime_retries < 0:
        print("Error: --runtime-retries must be >= 0")
        return 1
    if args.retry_delay_ms < 0:
        print("Error: --retry-delay-ms must be >= 0")
        return 1
    if args.app:
        if args.app not in ("HelloMultiDisplay", "HelloMultiDisplayHetero"):
            print("Error: --app must be HelloMultiDisplay or HelloMultiDisplayHetero")
            return 1
    if args.compact_app_view and not args.app:
        print("Error: --compact-app-view requires --app")
        return 1
    if args.results_jsonl_status not in ("all", "pass", "fail", "compile-fail", "runtime-fail"):
        print("Error: --results-jsonl-status must be all, pass, fail, compile-fail, or runtime-fail")
        return 1
    if args.per_app_thresholds_status not in ("all", "has-pass", "no-pass"):
        print("Error: --per-app-thresholds-status must be all, has-pass, or no-pass")
        return 1
    try:
        validate_and_normalize_per_app_threshold_scope_filters(args)
    except ValueError as exc:
        print("Error: %s" % exc)
        return 1

    py = sys.executable
    results = []
    compile_result_cache = {}
    stopped_early = False
    stop_reason = ""

    if args.profile:
        status = "applied" if profile_applied else "no defaults changed"
        print("Using profile=%s (%s)" % (args.profile, status))

    print(
        "Scanning %d capacity x %d burst x %d retry x %d delay x %d gap x %d max-core-retry x %d max-single-post-retry x %d full-queue-guard x %d shutdown-queue-wait x %d shutdown-exec combinations" % (
            len(capacities),
            len(bursts),
            len(post_retry_counts),
            len(post_retry_delay_values),
            len(stress_post_gap_values),
            len(max_core_task_retry_limits),
            len(max_single_post_retry_limits),
            len(full_queue_guard_values),
            len(max_shutdown_queue_wait_values),
            len(max_shutdown_exec_time_values),
        )
    )
    planned_combinations = (
        len(capacities)
        * len(bursts)
        * len(post_retry_counts)
        * len(post_retry_delay_values)
        * len(stress_post_gap_values)
        * len(max_core_task_retry_limits)
        * len(max_single_post_retry_limits)
        * len(full_queue_guard_values)
        * len(max_shutdown_queue_wait_values)
        * len(max_shutdown_exec_time_values)
    )
    for capacity in capacities:
        for burst in bursts:
            for post_retry_count in post_retry_counts:
                for post_retry_delay_ms in post_retry_delay_values:
                    for stress_post_gap_ms in stress_post_gap_values:
                        for max_core_task_retries in max_core_task_retry_limits:
                            for max_single_post_core_task_retries in max_single_post_retry_limits:
                                for fail_on_full_core_task_queue in full_queue_guard_values:
                                    for max_shutdown_queue_wait_ms in max_shutdown_queue_wait_values:
                                        for max_shutdown_exec_time_ms in max_shutdown_exec_time_values:
                                            label_prefix = "app=%s " % args.app if args.app else ""
                                            label = "%scapacity=%d burst=%d retry=%s delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s max_shutdown_queue_wait_ms=%s max_shutdown_exec_time_ms=%s" % (
                                                label_prefix,
                                                capacity,
                                                burst,
                                                format_retry_label(post_retry_count),
                                                format_retry_delay_label(post_retry_delay_ms),
                                                format_stress_post_gap_label(stress_post_gap_ms),
                                                format_max_core_task_retry_limit_label(max_core_task_retries),
                                                format_max_single_post_retry_limit_label(max_single_post_core_task_retries),
                                                format_full_queue_guard_label(fail_on_full_core_task_queue),
                                                format_shutdown_guardrail_label(max_shutdown_queue_wait_ms),
                                                format_shutdown_guardrail_label(max_shutdown_exec_time_ms),
                                            )
                                            print("\n=== %s ===" % label)

                                            compile_cache_key = (args.app, capacity, burst, post_retry_count, post_retry_delay_ms, stress_post_gap_ms, args.case_jobs, args.bits64)
                                            compile_result = compile_result_cache.get(compile_cache_key)
                                            if compile_result is None:
                                                compile_cmd = build_compile_command(
                                                    py,
                                                    capacity,
                                                    burst,
                                                    args.case_jobs,
                                                    args.bits64,
                                                    post_retry_count,
                                                    post_retry_delay_ms,
                                                    stress_post_gap_ms,
                                                    args.app,
                                                )
                                                print("Compile:", format_command(compile_cmd))
                                                compile_rc, compile_output, compile_reason, compile_attempts = run_command_with_retries(
                                                    compile_cmd,
                                                    "compile",
                                                    args.compile_retries,
                                                    args.retry_delay_ms,
                                                )
                                                compile_result = {
                                                    "rc": compile_rc,
                                                    "output": compile_output,
                                                    "reason": compile_reason,
                                                    "attempts": compile_attempts,
                                                }
                                                compile_result_cache[compile_cache_key] = compile_result
                                            else:
                                                compile_rc = compile_result["rc"]
                                                compile_output = compile_result["output"]
                                                compile_reason = compile_result["reason"]
                                                compile_attempts = compile_result["attempts"]
                                                print("Compile: [cached result] capacity=%d burst=%d retry=%s delay=%s gap=%s" % (
                                                    capacity,
                                                    burst,
                                                    format_retry_label(post_retry_count),
                                                    format_retry_delay_label(post_retry_delay_ms),
                                                    format_stress_post_gap_label(stress_post_gap_ms),
                                                ))

                                            if compile_rc != 0:
                                                reason = compile_reason or collect_failure_reason(compile_output)
                                                results.append({
                                                    "capacity": capacity,
                                                    "burst": burst,
                                                    "post_retry_count": post_retry_count,
                                                    "post_retry_delay_ms": post_retry_delay_ms,
                                                    "stress_post_gap_ms": stress_post_gap_ms,
                                                    "max_core_task_retries": max_core_task_retries,
                                                    "max_single_post_core_task_retries": max_single_post_core_task_retries,
                                                    "fail_on_full_core_task_queue": fail_on_full_core_task_queue,
                                                    "max_shutdown_queue_wait_ms": max_shutdown_queue_wait_ms,
                                                    "max_shutdown_exec_time_ms": max_shutdown_exec_time_ms,
                                                    "compile_ok": False,
                                                    "runtime_ok": False,
                                                    "detail": "%s (attempts=%d)" % (reason, compile_attempts),
                                                })
                                                print("Compile FAIL:", reason)
                                                if args.exit_on_first_failure:
                                                    stopped_early = True
                                                    stop_reason = "compile failure at %s: %s" % (label, reason)
                                                    break
                                                continue

                                            runtime_cmd = build_runtime_command(
                                                py,
                                                capacity,
                                                burst,
                                                args.jobs,
                                                args.timeout,
                                                args.bits64,
                                                args.keep_screenshots,
                                                max_core_task_retries,
                                                max_single_post_core_task_retries,
                                                fail_on_full_core_task_queue,
                                                max_shutdown_queue_wait_ms,
                                                max_shutdown_exec_time_ms,
                                                post_retry_count,
                                                post_retry_delay_ms,
                                                stress_post_gap_ms,
                                                args.app,
                                            )
                                            print("Runtime:", format_command(runtime_cmd))
                                            runtime_rc, runtime_output, runtime_reason, runtime_attempts = run_command_with_retries(
                                                runtime_cmd,
                                                "runtime",
                                                args.runtime_retries,
                                                args.retry_delay_ms,
                                            )
                                            if runtime_rc != 0:
                                                reason = runtime_reason or collect_failure_reason(runtime_output)
                                                results.append({
                                                    "capacity": capacity,
                                                    "burst": burst,
                                                    "post_retry_count": post_retry_count,
                                                    "post_retry_delay_ms": post_retry_delay_ms,
                                                    "stress_post_gap_ms": stress_post_gap_ms,
                                                    "max_core_task_retries": max_core_task_retries,
                                                    "max_single_post_core_task_retries": max_single_post_core_task_retries,
                                                    "fail_on_full_core_task_queue": fail_on_full_core_task_queue,
                                                    "max_shutdown_queue_wait_ms": max_shutdown_queue_wait_ms,
                                                    "max_shutdown_exec_time_ms": max_shutdown_exec_time_ms,
                                                    "compile_ok": True,
                                                    "runtime_ok": False,
                                                    "detail": "%s (attempts=%d)" % (reason, runtime_attempts),
                                                })
                                                print("Runtime FAIL:", reason)
                                                if args.exit_on_first_failure:
                                                    stopped_early = True
                                                    stop_reason = "runtime failure at %s: %s" % (label, reason)
                                                    break
                                                continue

                                            summary = collect_runtime_summary(runtime_output)
                                            results.append({
                                                "capacity": capacity,
                                                "burst": burst,
                                                "post_retry_count": post_retry_count,
                                                "post_retry_delay_ms": post_retry_delay_ms,
                                                "stress_post_gap_ms": stress_post_gap_ms,
                                                "max_core_task_retries": max_core_task_retries,
                                                "max_single_post_core_task_retries": max_single_post_core_task_retries,
                                                "fail_on_full_core_task_queue": fail_on_full_core_task_queue,
                                                "max_shutdown_queue_wait_ms": max_shutdown_queue_wait_ms,
                                                "max_shutdown_exec_time_ms": max_shutdown_exec_time_ms,
                                                "compile_ok": True,
                                                "runtime_ok": True,
                                                "detail": summary or "ALL PASSED",
                                            })
                                            print("Runtime PASS")
                                            if stopped_early:
                                                break
                                        if stopped_early:
                                            break
                                    if stopped_early:
                                        break
                                if stopped_early:
                                    break
                        if stopped_early:
                            break
                    if stopped_early:
                        break
                if stopped_early:
                    break
            if stopped_early:
                break
        if stopped_early:
            break

    print("\n=== Scan Summary ===")
    any_fail = False
    for item in results:
        status = "PASS" if is_pass(item) else "FAIL"
        if status == "FAIL":
            any_fail = True
        print(
            "[%s] capacity=%d burst=%d retry=%s delay=%s gap=%s max_core_task_retries=%s max_single_post_retry=%s full_queue_guard=%s%s | %s" % (
                status,
                item["capacity"],
                item["burst"],
                format_retry_label(item["post_retry_count"]),
                format_retry_delay_label(item["post_retry_delay_ms"]),
                format_stress_post_gap_label(item["stress_post_gap_ms"]),
                format_max_core_task_retry_limit_label(item["max_core_task_retries"]),
                format_max_single_post_retry_limit_label(item["max_single_post_core_task_retries"]),
                format_full_queue_guard_label(item["fail_on_full_core_task_queue"]),
                format_runtime_guardrail_config_suffix(item["max_shutdown_queue_wait_ms"], item["max_shutdown_exec_time_ms"]),
                item["detail"],
            )
        )

    if stopped_early:
        print("\nScan stopped early by --exit-on-first-failure: %s" % stop_reason)
    print("Scan coverage: executed %d/%d combinations" % (len(results), planned_combinations))

    print_threshold_summary(results, capacities, bursts, post_retry_counts, post_retry_delay_values, stress_post_gap_values, max_core_task_retry_limits, max_single_post_retry_limits, full_queue_guard_values, max_shutdown_queue_wait_values, max_shutdown_exec_time_values)
    print_per_app_threshold_summary(args.app, results, capacities, bursts)
    print_per_app_delay_gap_grid_summary(args.app, results, capacities, bursts, post_retry_counts, post_retry_delay_values, stress_post_gap_values, max_core_task_retry_limits, max_single_post_retry_limits, full_queue_guard_values, max_shutdown_queue_wait_values, max_shutdown_exec_time_values)
    print_per_app_retry_delay_grid_summary(args.app, results, capacities, bursts, post_retry_counts, post_retry_delay_values, stress_post_gap_values, max_core_task_retry_limits, max_single_post_retry_limits, full_queue_guard_values, max_shutdown_queue_wait_values, max_shutdown_exec_time_values)
    print_per_app_retry_gap_grid_summary(args.app, results, capacities, bursts, post_retry_counts, post_retry_delay_values, stress_post_gap_values, max_core_task_retry_limits, max_single_post_retry_limits, full_queue_guard_values, max_shutdown_queue_wait_values, max_shutdown_exec_time_values)
    if not args.compact_app_view:
        print_delay_gap_grid_summary(results, capacities, bursts, post_retry_counts, post_retry_delay_values, stress_post_gap_values, max_core_task_retry_limits, max_single_post_retry_limits, full_queue_guard_values, max_shutdown_queue_wait_values, max_shutdown_exec_time_values)
        print_retry_delay_grid_summary(results, capacities, bursts, post_retry_counts, post_retry_delay_values, stress_post_gap_values, max_core_task_retry_limits, max_single_post_retry_limits, full_queue_guard_values, max_shutdown_queue_wait_values, max_shutdown_exec_time_values)
        print_retry_gap_grid_summary(results, capacities, bursts, post_retry_counts, post_retry_delay_values, stress_post_gap_values, max_core_task_retry_limits, max_single_post_retry_limits, full_queue_guard_values, max_shutdown_queue_wait_values, max_shutdown_exec_time_values)
        print_failure_app_summary(results)
        print_failure_app_rollup(results)
        print_failure_app_leaders(results)
    print_failure_stage_summary(results)
    print_failure_stage_rollup(results)
    print_failure_stage_leaders(results)
    print_failure_contention_summary(results)
    print_failure_contention_rollup(results)
    print_failure_contention_leaders(results)
    print_failure_guardrail_summary(results)
    print_failure_guardrail_rollup(results)
    print_failure_guardrail_leaders(results)
    print_suggested_repro_commands(py, args, results, capacities, bursts, post_retry_counts, post_retry_delay_values, stress_post_gap_values, max_core_task_retry_limits, max_single_post_retry_limits, full_queue_guard_values, max_shutdown_queue_wait_values, max_shutdown_exec_time_values)
    write_json_summary_file(py, args, results, capacities, bursts, post_retry_counts, post_retry_delay_values, stress_post_gap_values, max_core_task_retry_limits, max_single_post_retry_limits, full_queue_guard_values, max_shutdown_queue_wait_values, max_shutdown_exec_time_values, stopped_early=stopped_early, stop_reason=stop_reason)
    write_suggested_repro_json_file(py, args, results, capacities, bursts, post_retry_counts, post_retry_delay_values, stress_post_gap_values, max_core_task_retry_limits, max_single_post_retry_limits, full_queue_guard_values, max_shutdown_queue_wait_values, max_shutdown_exec_time_values, stopped_early=stopped_early, stop_reason=stop_reason)
    write_results_jsonl_file(args, results, capacities, bursts, post_retry_counts, post_retry_delay_values, stress_post_gap_values, max_core_task_retry_limits, max_single_post_retry_limits, full_queue_guard_values, max_shutdown_queue_wait_values, max_shutdown_exec_time_values, stopped_early=stopped_early, stop_reason=stop_reason)
    write_per_app_thresholds_jsonl_file(args, results, capacities, bursts, post_retry_counts, post_retry_delay_values, stress_post_gap_values, max_core_task_retry_limits, max_single_post_retry_limits, full_queue_guard_values, max_shutdown_queue_wait_values, max_shutdown_exec_time_values, stopped_early=stopped_early, stop_reason=stop_reason)
    if args.emit_results_jsonl:
        print_results_jsonl(args, results, capacities, bursts, post_retry_counts, post_retry_delay_values, stress_post_gap_values, max_core_task_retry_limits, max_single_post_retry_limits, full_queue_guard_values, max_shutdown_queue_wait_values, max_shutdown_exec_time_values, stopped_early=stopped_early, stop_reason=stop_reason)
    if args.emit_suggested_repro_json:
        print_suggested_repro_json(py, args, results, capacities, bursts, post_retry_counts, post_retry_delay_values, stress_post_gap_values, max_core_task_retry_limits, max_single_post_retry_limits, full_queue_guard_values, max_shutdown_queue_wait_values, max_shutdown_exec_time_values, stopped_early=stopped_early, stop_reason=stop_reason)
    if args.emit_suggested_repro_json_compact:
        print_suggested_repro_json_compact(py, args, results, capacities, bursts, post_retry_counts, post_retry_delay_values, stress_post_gap_values, max_core_task_retry_limits, max_single_post_retry_limits, full_queue_guard_values, max_shutdown_queue_wait_values, max_shutdown_exec_time_values, stopped_early=stopped_early, stop_reason=stop_reason)
    if args.emit_per_app_thresholds_jsonl:
        print_per_app_thresholds_jsonl(args, results, capacities, bursts, post_retry_counts, post_retry_delay_values, stress_post_gap_values, max_core_task_retry_limits, max_single_post_retry_limits, full_queue_guard_values, max_shutdown_queue_wait_values, max_shutdown_exec_time_values, stopped_early=stopped_early, stop_reason=stop_reason)
    if args.emit_json_summary:
        print_json_summary(py, args, results, capacities, bursts, post_retry_counts, post_retry_delay_values, stress_post_gap_values, max_core_task_retry_limits, max_single_post_retry_limits, full_queue_guard_values, max_shutdown_queue_wait_values, max_shutdown_exec_time_values, stopped_early=stopped_early, stop_reason=stop_reason)
    if args.emit_json_summary_compact:
        print_json_summary_compact(py, args, results, capacities, bursts, post_retry_counts, post_retry_delay_values, stress_post_gap_values, max_core_task_retry_limits, max_single_post_retry_limits, full_queue_guard_values, max_shutdown_queue_wait_values, max_shutdown_exec_time_values, stopped_early=stopped_early, stop_reason=stop_reason)

    return 1 if any_fail else 0


if __name__ == "__main__":
    sys.exit(main())

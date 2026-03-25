"""Helpers for page-level timers in UI Designer."""

from __future__ import annotations

from .page_fields import generated_page_member_names
from .widget_name import is_valid_widget_name, make_unique_widget_name, sanitize_widget_name


def _normalize_bool(value):
    if isinstance(value, bool):
        return value
    text = str(value or "").strip().lower()
    return text in {"1", "true", "yes", "on"}


def normalize_page_timer(timer):
    data = dict(timer or {})
    timer_name = sanitize_widget_name(data.get("name", ""))
    callback_name = sanitize_widget_name(data.get("callback", ""))

    delay_value = data.get("delay_ms", "")
    period_value = data.get("period_ms", "")
    delay_ms = "" if delay_value in ("", None) else str(delay_value).strip()
    period_ms = "" if period_value in ("", None) else str(period_value).strip()
    if not delay_ms:
        delay_ms = "1000"
    if not period_ms:
        period_ms = "1000"

    return {
        "name": timer_name,
        "callback": callback_name,
        "delay_ms": delay_ms,
        "period_ms": period_ms,
        "auto_start": _normalize_bool(data.get("auto_start", False)),
    }


def normalize_page_timers(timers):
    return [normalize_page_timer(timer) for timer in (timers or [])]


def suggest_page_timer_name(page, timers=None, base_name="timer"):
    existing = generated_page_member_names(page, include_user_fields=True, include_timers=False)
    for timer in normalize_page_timers(timers):
        if timer.get("name"):
            existing.add(timer["name"])
    return make_unique_widget_name(base_name, existing)


def suggest_page_timer_callback(page, timer_name):
    page_prefix = getattr(page, "c_prefix", "page")
    callback_base = f"{page_prefix}_{sanitize_widget_name(timer_name or 'timer')}_callback"
    return sanitize_widget_name(callback_base)


def collect_page_timer_issues(page, timers):
    normalized_timers = normalize_page_timers(timers)
    reserved_names = generated_page_member_names(page, include_user_fields=True, include_timers=False)
    name_counts = {}

    for timer in normalized_timers:
        timer_name = timer.get("name", "")
        if timer_name:
            name_counts[timer_name] = name_counts.get(timer_name, 0) + 1

    issues = []
    for index, timer in enumerate(normalized_timers):
        timer_name = timer.get("name", "")
        callback_name = timer.get("callback", "")
        delay_ms = timer.get("delay_ms", "")
        period_ms = timer.get("period_ms", "")

        if not timer_name:
            issues.append(
                {
                    "index": index,
                    "timer": timer,
                    "code": "empty_name",
                    "message": f"Page timer #{index + 1} must define a name.",
                }
            )
            continue

        if not is_valid_widget_name(timer_name):
            issues.append(
                {
                    "index": index,
                    "timer": timer,
                    "code": "invalid_name",
                    "message": (
                        f"Page timer '{timer_name}' must be a valid C identifier using letters, numbers, and underscores, "
                        "and it cannot start with a digit."
                    ),
                }
            )
            continue

        if timer_name in reserved_names:
            issues.append(
                {
                    "index": index,
                    "timer": timer,
                    "code": "conflict",
                    "message": f"Page timer '{timer_name}' conflicts with an auto-generated page member.",
                }
            )
            continue

        if name_counts.get(timer_name, 0) > 1:
            issues.append(
                {
                    "index": index,
                    "timer": timer,
                    "code": "duplicate_name",
                    "message": f"Page timer '{timer_name}' already exists in this page.",
                }
            )
            continue

        if not callback_name:
            issues.append(
                {
                    "index": index,
                    "timer": timer,
                    "code": "missing_callback",
                    "message": f"Page timer '{timer_name}' must define a callback function name.",
                }
            )
            continue

        if not is_valid_widget_name(callback_name):
            issues.append(
                {
                    "index": index,
                    "timer": timer,
                    "code": "invalid_callback",
                    "message": f"Page timer '{timer_name}' callback '{callback_name}' is not a valid C identifier.",
                }
            )
            continue

        if not delay_ms:
            issues.append(
                {
                    "index": index,
                    "timer": timer,
                    "code": "missing_delay",
                    "message": f"Page timer '{timer_name}' must define a delay expression in milliseconds.",
                }
            )
            continue

        if not period_ms:
            issues.append(
                {
                    "index": index,
                    "timer": timer,
                    "code": "missing_period",
                    "message": f"Page timer '{timer_name}' must define a period expression in milliseconds.",
                }
            )

    return normalized_timers, issues


def validate_page_timers(page, timers):
    normalized_timers, issues = collect_page_timer_issues(page, timers)
    if issues:
        return False, normalized_timers, issues[0]["message"]
    return True, normalized_timers, ""


def valid_page_timers(page, timers):
    normalized_timers, issues = collect_page_timer_issues(page, timers)
    invalid_indexes = {issue["index"] for issue in issues}
    return [timer for index, timer in enumerate(normalized_timers) if index not in invalid_indexes]

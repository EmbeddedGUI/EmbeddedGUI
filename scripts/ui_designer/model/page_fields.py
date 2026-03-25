"""Helpers for page-level user fields in UI Designer."""

from __future__ import annotations

from .widget_name import is_valid_widget_name, make_unique_widget_name, sanitize_widget_name


COMMON_PAGE_FIELD_TYPES = [
    "int",
    "uint8_t",
    "uint16_t",
    "uint32_t",
    "bool",
    "float",
    "double",
    "char *",
]


def animation_member_names(page):
    names = set()
    if page is None:
        return names

    for widget in page.get_all_widgets():
        for index, anim in enumerate(getattr(widget, "animations", [])):
            suffix = f"_{index}" if len(widget.animations) > 1 else ""
            names.add(f"anim_{widget.name}_{anim.anim_type}{suffix}")
    return names


def base_generated_page_member_names(page):
    names = {"base"}
    if page is None:
        return names

    names.update(widget.name for widget in page.get_all_widgets() if getattr(widget, "name", ""))
    names.update(animation_member_names(page))
    return names


def generated_page_member_names(page, include_user_fields=True, include_timers=True):
    names = base_generated_page_member_names(page)
    if page is None:
        return names

    if include_user_fields:
        for field in normalize_page_fields(getattr(page, "user_fields", [])):
            if field.get("name"):
                names.add(field["name"])

    if include_timers:
        for timer in getattr(page, "timers", []) or []:
            timer_name = sanitize_widget_name((timer or {}).get("name", ""))
            if timer_name:
                names.add(timer_name)

    return names


def normalize_page_field(field):
    data = dict(field or {})
    normalized = {
        "name": sanitize_widget_name(data.get("name", "")),
        "type": str(data.get("type", "") or "").strip(),
    }

    default_value = data.get("default", "")
    if default_value not in ("", None):
        normalized["default"] = str(default_value).strip()

    return normalized


def normalize_page_fields(fields):
    return [normalize_page_field(field) for field in (fields or [])]


def collect_page_field_issues(page, fields):
    normalized_fields = normalize_page_fields(fields)
    reserved_names = generated_page_member_names(page, include_user_fields=False, include_timers=True)
    name_counts = {}

    for field in normalized_fields:
        field_name = field.get("name", "")
        if field_name:
            name_counts[field_name] = name_counts.get(field_name, 0) + 1

    issues = []
    for index, field in enumerate(normalized_fields):
        field_name = field.get("name", "")
        field_type = field.get("type", "")

        if not field_name:
            issues.append(
                {
                    "index": index,
                    "field": field,
                    "code": "empty_name",
                    "message": f"Page field #{index + 1} must define a name.",
                }
            )
            continue

        if not is_valid_widget_name(field_name):
            issues.append(
                {
                    "index": index,
                    "field": field,
                    "code": "invalid_name",
                    "message": (
                        f"Page field '{field_name}' must be a valid C identifier using letters, numbers, and underscores, "
                        "and it cannot start with a digit."
                    ),
                }
            )
            continue

        if not field_type:
            issues.append(
                {
                    "index": index,
                    "field": field,
                    "code": "missing_type",
                    "message": f"Page field '{field_name}' must define a C type.",
                }
            )
            continue

        if field_name in reserved_names:
            issues.append(
                {
                    "index": index,
                    "field": field,
                    "code": "conflict",
                    "message": f"Page field '{field_name}' conflicts with an auto-generated page member.",
                }
            )
            continue

        if name_counts.get(field_name, 0) > 1:
            issues.append(
                {
                    "index": index,
                    "field": field,
                    "code": "duplicate_name",
                    "message": f"Page field '{field_name}' already exists in this page.",
                }
            )

    return normalized_fields, issues


def validate_page_fields(page, fields):
    normalized_fields, issues = collect_page_field_issues(page, fields)
    if issues:
        return False, normalized_fields, issues[0]["message"]
    return True, normalized_fields, ""


def valid_page_fields(page, fields):
    normalized_fields, issues = collect_page_field_issues(page, fields)
    invalid_indexes = {issue["index"] for issue in issues}
    return [field for index, field in enumerate(normalized_fields) if index not in invalid_indexes]


def suggest_page_field_name(page, fields=None, base_name="field"):
    existing = generated_page_member_names(page)
    for field in normalize_page_fields(fields):
        if field.get("name"):
            existing.add(field["name"])
    return make_unique_widget_name(base_name, existing)


def page_field_declaration(field):
    normalized = normalize_page_field(field)
    field_type = normalized.get("type", "")
    field_name = normalized.get("name", "")
    if not field_type or not field_name:
        return ""

    if "[" in field_type and field_type.endswith("]"):
        bracket_index = field_type.index("[")
        base_type = field_type[:bracket_index].rstrip()
        suffix = field_type[bracket_index:]
        if base_type:
            return f"{base_type} {field_name}{suffix};"

    return f"{field_type} {field_name};"


def page_field_default_assignment(field, target_prefix="local->"):
    normalized = normalize_page_field(field)
    field_type = normalized.get("type", "")
    field_name = normalized.get("name", "")
    default_value = normalized.get("default", "")
    if not field_name or default_value == "":
        return ""
    if "[" in field_type and "]" in field_type:
        return ""
    return f"{target_prefix}{field_name} = {default_value};"

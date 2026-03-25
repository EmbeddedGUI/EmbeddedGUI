"""Lightweight diagnostics for UI Designer pages and selection."""

from __future__ import annotations

import os
from dataclasses import dataclass

from .page_fields import collect_page_field_issues
from .page_timers import collect_page_timer_issues
from .string_resource import parse_string_ref
from .widget_name import is_valid_widget_name
from .widget_registry import WidgetRegistry


_RESOURCE_PROP_TYPES = {
    "image_file": "image",
    "font_file": "font",
    "text_file": "text",
}

_SEVERITY_ORDER = {
    "error": 0,
    "warning": 1,
    "info": 2,
}


@dataclass
class DiagnosticEntry:
    severity: str
    code: str
    message: str
    page_name: str = ""
    widget_name: str = ""
    resource_type: str = ""
    resource_name: str = ""
    property_name: str = ""


def _is_layout_managed(widget):
    parent = getattr(widget, "parent", None)
    if parent is None:
        return False
    type_info = WidgetRegistry.instance().get(parent.widget_type)
    return type_info.get("layout_func") is not None


def _missing_resource_reason(prop_type, value, resource_catalog=None, source_resource_dir=""):
    if not value:
        return ""

    if prop_type == "image_file":
        if resource_catalog is not None and not resource_catalog.has_image(value):
            return "catalog"
        if source_resource_dir and not os.path.isfile(os.path.join(source_resource_dir, "images", value)):
            return "disk"
        return ""

    if prop_type == "font_file":
        if resource_catalog is not None and not resource_catalog.has_font(value):
            return "catalog"
        if source_resource_dir and not os.path.isfile(os.path.join(source_resource_dir, value)):
            return "disk"
        return ""

    if prop_type == "text_file":
        if resource_catalog is not None and not resource_catalog.has_text_file(value):
            return "catalog"
        if source_resource_dir and not os.path.isfile(os.path.join(source_resource_dir, value)):
            return "disk"
        return ""

    return ""


def _duplicate_name_entries(page):
    widgets = page.get_all_widgets()
    name_counts = {}
    for widget in widgets:
        if widget.name:
            name_counts[widget.name] = name_counts.get(widget.name, 0) + 1

    entries = []
    for widget in widgets:
        if widget.name and name_counts.get(widget.name, 0) > 1:
            entries.append(
                DiagnosticEntry(
                    "error",
                    "duplicate_name",
                    f"Widget name '{widget.name}' is duplicated in page '{page.name}'.",
                    page_name=page.name,
                    widget_name=widget.name,
                )
            )
    return entries


def _invalid_name_entries(page):
    entries = []
    for widget in page.get_all_widgets():
        if is_valid_widget_name(widget.name):
            continue
        entries.append(
            DiagnosticEntry(
                "error",
                "invalid_name",
                f"Widget name '{widget.name}' is not a valid C identifier.",
                page_name=page.name,
                widget_name=widget.name,
            )
        )
    return entries


def _bounds_entries(page):
    entries = []
    for widget in page.get_all_widgets():
        parent = getattr(widget, "parent", None)
        if parent is None or _is_layout_managed(widget):
            continue

        issues = []
        if widget.x < 0 or widget.y < 0:
            issues.append("negative origin")
        if widget.x + widget.width > parent.width or widget.y + widget.height > parent.height:
            issues.append(f"exceeds parent '{parent.name}' bounds")
        if not issues:
            continue

        entries.append(
            DiagnosticEntry(
                "warning",
                "bounds",
                f"Widget '{widget.name}' has geometry issues: {', '.join(issues)}.",
                page_name=page.name,
                widget_name=widget.name,
            )
        )
    return entries


def _missing_resource_entries(page, resource_catalog=None, source_resource_dir=""):
    registry = WidgetRegistry.instance()
    entries = []

    for widget in page.get_all_widgets():
        descriptor = registry.get(widget.widget_type)
        for prop_name, prop_info in descriptor.get("properties", {}).items():
            prop_type = prop_info.get("type", "")
            if prop_type not in _RESOURCE_PROP_TYPES:
                continue

            value = widget.properties.get(prop_name, "")
            reason = _missing_resource_reason(prop_type, value, resource_catalog=resource_catalog, source_resource_dir=source_resource_dir)
            if not reason:
                continue

            if reason == "disk":
                message = f"Widget '{widget.name}' references {prop_name}='{value}', but the source file is missing on disk."
            else:
                message = f"Widget '{widget.name}' references {prop_name}='{value}', but it is missing from the resource catalog."

            entries.append(
                DiagnosticEntry(
                    "warning",
                    "missing_resource",
                    message,
                    page_name=page.name,
                    widget_name=widget.name,
                    resource_type=_RESOURCE_PROP_TYPES.get(prop_type, ""),
                    resource_name=value,
                    property_name=prop_name,
                )
            )

    return entries


def _missing_string_reference_entries(page, string_catalog=None):
    entries = []
    available_keys = set(string_catalog.all_keys) if string_catalog is not None else set()

    for widget in page.get_all_widgets():
        string_key = parse_string_ref(widget.properties.get("text", ""))
        if not string_key:
            continue
        if string_key in available_keys:
            continue
        entries.append(
            DiagnosticEntry(
                "warning",
                "missing_string_resource",
                f"Widget '{widget.name}' references text='@string/{string_key}', but that key is missing from the string catalog.",
                page_name=page.name,
                widget_name=widget.name,
                resource_type="string",
                resource_name=string_key,
                property_name="text",
            )
        )

    return entries


def _page_field_entries(page):
    _, issues = collect_page_field_issues(page, getattr(page, "user_fields", []))
    entries = []

    for issue in issues:
        field = issue.get("field", {})
        field_name = field.get("name", "")
        entries.append(
            DiagnosticEntry(
                "error",
                f"page_field_{issue.get('code', 'invalid')}",
                issue.get("message", "Page field metadata is invalid."),
                page_name=page.name,
                widget_name=field_name,
            )
        )

    return entries


def _page_timer_entries(page):
    _, issues = collect_page_timer_issues(page, getattr(page, "timers", []))
    entries = []

    for issue in issues:
        timer = issue.get("timer", {})
        timer_name = timer.get("name", "")
        entries.append(
            DiagnosticEntry(
                "error",
                f"page_timer_{issue.get('code', 'invalid')}",
                issue.get("message", "Page timer metadata is invalid."),
                page_name=page.name,
                widget_name=timer_name,
            )
        )

    return entries


def analyze_page(page, resource_catalog=None, string_catalog=None, source_resource_dir=""):
    """Analyze a single page and return sorted diagnostics."""
    if page is None or page.root_widget is None:
        return []

    entries = []
    entries.extend(_page_field_entries(page))
    entries.extend(_page_timer_entries(page))
    entries.extend(_invalid_name_entries(page))
    entries.extend(_duplicate_name_entries(page))
    entries.extend(_bounds_entries(page))
    entries.extend(_missing_resource_entries(page, resource_catalog=resource_catalog, source_resource_dir=source_resource_dir))
    entries.extend(_missing_string_reference_entries(page, string_catalog=string_catalog))

    return sorted(entries, key=lambda entry: (_SEVERITY_ORDER.get(entry.severity, 99), entry.widget_name, entry.message))


def analyze_selection(widgets):
    """Analyze current selection constraints and return informational diagnostics."""
    widgets = [widget for widget in (widgets or []) if widget is not None]
    if not widgets:
        return []

    entries = []
    locked_count = sum(1 for widget in widgets if getattr(widget, "designer_locked", False))
    hidden_count = sum(1 for widget in widgets if getattr(widget, "designer_hidden", False))
    layout_widgets = [widget for widget in widgets if _is_layout_managed(widget)]

    if locked_count:
        noun = "widget" if locked_count == 1 else "widgets"
        entries.append(
            DiagnosticEntry(
                "info",
                "selection_locked",
                f"{locked_count} selected {noun} are locked; canvas drag and resize are disabled.",
            )
        )

    if hidden_count:
        noun = "widget" if hidden_count == 1 else "widgets"
        entries.append(
            DiagnosticEntry(
                "info",
                "selection_hidden",
                f"{hidden_count} selected {noun} are hidden and skipped by canvas hit testing.",
            )
        )

    if layout_widgets:
        if len(layout_widgets) == 1 and len(widgets) == 1:
            parent = getattr(layout_widgets[0], "parent", None)
            parent_type = parent.widget_type if parent is not None else "container"
            message = f"Selected widget is layout-managed by {parent_type}; x/y edits are constrained by the parent layout."
        else:
            noun = "widget" if len(layout_widgets) == 1 else "widgets"
            message = f"{len(layout_widgets)} selected {noun} use parent-managed layout positioning."
        entries.append(DiagnosticEntry("info", "selection_layout_managed", message))

    return entries

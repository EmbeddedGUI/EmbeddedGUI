"""Widget name validation helpers for UI Designer."""

import re


_WIDGET_NAME_PATTERN = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")


def sanitize_widget_name(value):
    """Normalize a user-entered widget name."""
    return (value or "").strip().replace(" ", "_")


def is_valid_widget_name(name):
    """Return True when name can be used as a generated C identifier."""
    return bool(_WIDGET_NAME_PATTERN.fullmatch(name or ""))


def widget_tree_root(widget):
    """Return the top-most widget in the current widget tree."""
    current = widget
    while current is not None and getattr(current, "parent", None) is not None:
        current = current.parent
    return current


def existing_widget_names(widget, exclude_widget=None):
    """Collect widget names from the same page tree."""
    root = widget_tree_root(widget)
    if root is None:
        return set()

    names = set()
    for current in root.get_all_widgets_flat():
        if current is exclude_widget:
            continue
        if getattr(current, "name", ""):
            names.add(current.name)
    return names


def make_unique_widget_name(base_name, existing_names):
    """Resolve duplicate names by appending an incrementing suffix."""
    candidate = sanitize_widget_name(base_name)
    if not candidate:
        return ""
    if candidate not in existing_names:
        return candidate

    match = re.match(r"^(.*?)(?:_(\d+))?$", candidate)
    stem = candidate
    suffix = 2
    if match:
        stem = match.group(1) or candidate
        if match.group(2):
            suffix = int(match.group(2)) + 1

    while f"{stem}_{suffix}" in existing_names:
        suffix += 1
    return f"{stem}_{suffix}"


def resolve_widget_name(widget, candidate):
    """Validate and deduplicate a widget name.

    Returns a tuple of ``(ok, resolved_name, message)``.
    """
    normalized = sanitize_widget_name(candidate)
    current_name = getattr(widget, "name", "")

    if not normalized:
        return False, current_name, "Widget name cannot be empty."

    if not is_valid_widget_name(normalized):
        return (
            False,
            current_name,
            "Widget name must be a valid C identifier using letters, numbers, and underscores, and it cannot start with a digit.",
        )

    names = existing_widget_names(widget, exclude_widget=widget)
    resolved = make_unique_widget_name(normalized, names)
    if resolved != normalized:
        return True, resolved, f"Widget name '{normalized}' already exists. Renamed to '{resolved}'."

    return True, resolved, ""

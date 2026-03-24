"""Workspace path helpers for EmbeddedGUI Designer."""

from __future__ import annotations

import os
import sys
from typing import Iterable


_COMMON_SDK_CONTAINER_NAMES = ("sdk", "SDK")


def normalize_path(path: str | None) -> str:
    """Return an absolute normalized path, or an empty string."""
    if not path:
        return ""
    return os.path.normpath(os.path.abspath(path))


def is_valid_sdk_root(path: str | None) -> bool:
    """Return True when *path* looks like a valid EmbeddedGUI SDK root."""
    root = normalize_path(path)
    if not root:
        return False
    return (
        os.path.isfile(os.path.join(root, "Makefile"))
        and os.path.isdir(os.path.join(root, "src"))
        and os.path.isdir(os.path.join(root, "porting", "designer"))
    )


def _dedupe_paths(paths: Iterable[str]) -> list[str]:
    seen = set()
    result = []
    for path in paths:
        norm = normalize_path(path)
        if not norm or norm in seen:
            continue
        seen.add(norm)
        result.append(norm)
    return result


def _walk_ancestors(path: str) -> list[str]:
    current = normalize_path(path)
    if not current:
        return []
    if os.path.isfile(current):
        current = os.path.dirname(current)

    result = []
    while current:
        result.append(current)
        parent = os.path.dirname(current)
        if parent == current:
            break
        current = parent
    return result


def _looks_like_sdk_dir_name(name: str) -> bool:
    name = (name or "").strip().lower()
    if not name:
        return False
    return name in {"sdk", "embeddedgui"} or "embeddedgui" in name


def _list_matching_child_dirs(base: str) -> list[str]:
    base = normalize_path(base)
    if not base or not os.path.isdir(base):
        return []

    result = []
    try:
        for entry in sorted(os.listdir(base)):
            child = os.path.join(base, entry)
            if os.path.isdir(child) and _looks_like_sdk_dir_name(entry):
                result.append(child)
    except OSError:
        return []
    return result


def _search_common_sdk_locations(anchor: str) -> list[str]:
    result = []
    for base in _walk_ancestors(anchor):
        result.append(base)
        result.extend(_list_matching_child_dirs(base))
        for container_name in _COMMON_SDK_CONTAINER_NAMES:
            container_dir = os.path.join(base, container_name)
            result.append(container_dir)
            result.extend(_list_matching_child_dirs(container_dir))
    return result


def resolve_sdk_root_candidate(path: str | None) -> str:
    """Resolve *path* to a valid SDK root when possible.

    Accepts exact SDK roots as well as nearby parent/container directories such
    as ``sdk/`` or a project directory living under ``sdk_root/example/app``.
    """
    candidate = normalize_path(path)
    if not candidate:
        return ""

    if is_valid_sdk_root(candidate):
        return candidate

    inferred = infer_sdk_root_from_project_dir(candidate)
    if inferred:
        return inferred

    for nearby in _dedupe_paths(_search_common_sdk_locations(candidate)):
        if is_valid_sdk_root(nearby):
            return nearby
    return ""


def serialize_sdk_root(project_dir: str, sdk_root: str) -> str:
    """Serialize *sdk_root* for storage in a project file."""
    project_dir = normalize_path(project_dir)
    sdk_root = normalize_path(sdk_root)
    if not project_dir or not sdk_root:
        return ""
    try:
        return os.path.relpath(sdk_root, project_dir).replace("\\", "/")
    except ValueError:
        return sdk_root.replace("\\", "/")


def resolve_project_sdk_root(project_dir: str, stored_path: str) -> str:
    """Resolve a stored SDK path relative to *project_dir*."""
    project_dir = normalize_path(project_dir)
    stored_path = (stored_path or "").strip()
    if not project_dir or not stored_path:
        return ""
    if os.path.isabs(stored_path):
        return normalize_path(stored_path)
    return normalize_path(os.path.join(project_dir, stored_path))


def infer_sdk_root_from_project_dir(project_dir: str) -> str:
    """Infer SDK root when the project lives under ``sdk_root/example/app``."""
    project_dir = normalize_path(project_dir)
    if not project_dir:
        return ""
    example_dir = os.path.dirname(project_dir)
    if os.path.basename(example_dir) != "example":
        return ""
    candidate = os.path.dirname(example_dir)
    if is_valid_sdk_root(candidate):
        return candidate
    return ""


def compute_make_app_root_arg(sdk_root: str, app_dir: str, app_name: str) -> str:
    """Compute ``EGUI_APP_ROOT_PATH`` for make."""
    sdk_root = normalize_path(sdk_root)
    app_dir = normalize_path(app_dir)
    if not sdk_root or not app_dir or not app_name:
        raise ValueError("sdk_root, app_dir, and app_name are required")

    example_app_dir = normalize_path(os.path.join(sdk_root, "example", app_name))
    if app_dir == example_app_dir:
        return "example"

    try:
        relative_parent = os.path.relpath(os.path.dirname(app_dir), sdk_root)
    except ValueError as exc:
        raise ValueError("External app must be on the same drive as the SDK root") from exc

    relative_parent = relative_parent.replace("\\", "/")
    return relative_parent or "."


def find_sdk_root(
    *,
    cli_sdk_root: str | None = None,
    configured_sdk_root: str | None = None,
    project_path: str | None = None,
    env: dict[str, str] | None = None,
) -> str:
    """Find the best SDK root candidate."""
    env = env or os.environ
    candidates = []

    if cli_sdk_root:
        candidates.append(cli_sdk_root)

    if project_path:
        project_norm = normalize_path(project_path)
        project_dir = project_norm if os.path.isdir(project_norm) else os.path.dirname(project_norm)
        candidates.extend(_search_common_sdk_locations(project_dir))
        inferred = infer_sdk_root_from_project_dir(project_dir)
        if inferred:
            candidates.append(inferred)

    if configured_sdk_root:
        candidates.append(configured_sdk_root)

    env_sdk_root = env.get("EMBEDDEDGUI_SDK_ROOT", "")
    if env_sdk_root:
        candidates.append(env_sdk_root)

    runtime_anchor = os.path.dirname(sys.executable if getattr(sys, "frozen", False) else __file__)
    candidates.extend(_search_common_sdk_locations(runtime_anchor))
    candidates.extend(_search_common_sdk_locations(os.getcwd()))

    for candidate in _dedupe_paths(candidates):
        resolved = resolve_sdk_root_candidate(candidate)
        if resolved:
            return resolved
    return ""


def describe_sdk_root(path: str | None) -> str:
    """Return a short status string for a candidate SDK root."""
    root = normalize_path(path)
    if not root:
        return "missing"
    if is_valid_sdk_root(root):
        return "ready"
    return "invalid"

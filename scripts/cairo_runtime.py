from __future__ import annotations

import os
from pathlib import Path


_WINDOWS_CAIRO_DLL_NAME = "libcairo-2.dll"
_WINDOWS_MSYS2_BIN_DIRS = (
    ("mingw64", "bin"),
    ("ucrt64", "bin"),
    ("clang64", "bin"),
)
_DLL_DIR_HANDLES = []


def _project_root() -> Path:
    return Path(__file__).resolve().parents[1]


def _split_path_like(value: str) -> list[Path]:
    result: list[Path] = []
    for raw_item in (value or "").split(os.pathsep):
        item = raw_item.strip().strip('"')
        if not item:
            continue
        result.append(Path(item))
    return result


def _iter_windows_cairo_dir_candidates(root_dir: Path | None = None, env: dict[str, str] | None = None) -> list[Path]:
    if os.name != "nt":
        return []

    env_map = env or os.environ
    root_dir = root_dir or _project_root()
    candidates: list[Path] = []

    for path in _split_path_like(env_map.get("CAIROCFFI_DLL_DIRECTORIES", "")):
        candidates.append(path)

    msys2_roots: list[Path] = []
    msys2_location = env_map.get("MSYS2_LOCATION", "").strip()
    if msys2_location:
        msys2_roots.append(Path(msys2_location))
    msys2_roots.append(root_dir / "tools" / "msys64")
    msys2_roots.append(Path("C:/msys64"))

    for msys2_root in msys2_roots:
        for parts in _WINDOWS_MSYS2_BIN_DIRS:
            candidates.append(msys2_root.joinpath(*parts))

    unique: list[Path] = []
    seen: set[str] = set()
    for candidate in candidates:
        try:
            normalized = str(candidate.resolve(strict=False)).lower()
        except OSError:
            normalized = str(candidate).lower()
        if normalized in seen:
            continue
        seen.add(normalized)
        unique.append(candidate)
    return unique


def find_windows_cairo_bin_dir(root_dir: Path | None = None, env: dict[str, str] | None = None) -> Path | None:
    if os.name != "nt":
        return None

    for candidate in _iter_windows_cairo_dir_candidates(root_dir, env):
        dll_path = candidate / _WINDOWS_CAIRO_DLL_NAME
        if dll_path.exists():
            return candidate
    return None


def prepare_cairo_runtime(root_dir: Path | None = None) -> list[Path]:
    if os.name != "nt":
        return []

    added: list[Path] = []
    existing = _split_path_like(os.environ.get("CAIROCFFI_DLL_DIRECTORIES", ""))
    existing_keys = {str(path).lower() for path in existing}

    for candidate in _iter_windows_cairo_dir_candidates(root_dir):
        dll_path = candidate / _WINDOWS_CAIRO_DLL_NAME
        if not dll_path.exists():
            continue

        candidate_text = str(candidate)
        if candidate_text.lower() not in existing_keys:
            existing.append(candidate)
            existing_keys.add(candidate_text.lower())

        if hasattr(os, "add_dll_directory"):
            try:
                handle = os.add_dll_directory(candidate_text)
            except (FileNotFoundError, OSError):
                continue
            _DLL_DIR_HANDLES.append(handle)

        added.append(candidate)

    if existing:
        os.environ["CAIROCFFI_DLL_DIRECTORIES"] = os.pathsep.join(str(path) for path in existing)

    return added

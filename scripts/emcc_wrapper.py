#!/usr/bin/env python3
"""Windows-friendly emcc wrapper for repo-local emsdk."""

from __future__ import annotations

import os
from pathlib import Path
import subprocess
import sys


SCRIPT_DIR = Path(__file__).resolve().parent
ROOT_DIR = SCRIPT_DIR.parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from setup_env import find_emsdk_root, inject_emsdk_paths  # noqa: E402


def _find_tool_executable(root: Path, relative_candidates: list[Path]) -> str | None:
    for candidate in relative_candidates:
        full_path = root / candidate
        if full_path.exists():
            return str(full_path)
    return None


def _resolve_emsdk_root(env: dict[str, str]) -> Path | None:
    override = env.get("EMSDK_WRAPPER_ROOT")
    if override:
        candidate = Path(override).expanduser()
        if not candidate.is_absolute():
            candidate = (ROOT_DIR / candidate).resolve()
        if candidate.exists():
            return candidate

    _, detected = find_emsdk_root(ROOT_DIR, env)
    if detected is not None and detected.exists():
        return detected
    return None


def _build_runtime_env(emsdk_root: Path, env: dict[str, str]) -> dict[str, str]:
    runtime_env = inject_emsdk_paths(env, emsdk_root)

    python_exe = _find_tool_executable(
        emsdk_root,
        [
            Path("python") / child / "python.exe"
            for child in sorted(os.listdir(emsdk_root / "python"))
        ]
        if (emsdk_root / "python").exists()
        else [],
    )
    if python_exe is not None:
        runtime_env["EMSDK_PYTHON"] = python_exe

    node_exe = _find_tool_executable(
        emsdk_root,
        [
            Path("node") / child / "bin" / "node.exe"
            for child in sorted(os.listdir(emsdk_root / "node"))
        ]
        if (emsdk_root / "node").exists()
        else [],
    )
    if node_exe is not None:
        runtime_env["EMSDK_NODE"] = node_exe

    return runtime_env


def main() -> int:
    env = os.environ.copy()
    emsdk_root = _resolve_emsdk_root(env)
    if emsdk_root is None:
        print("[emcc_wrapper] Unable to locate emsdk. Run `python scripts/setup_env.py --install-emsdk` first.", file=sys.stderr)
        return 1

    emcc_py = emsdk_root / "upstream" / "emscripten" / "emcc.py"
    if not emcc_py.exists():
        print(f"[emcc_wrapper] emcc.py not found: {emcc_py}", file=sys.stderr)
        return 1

    runtime_env = _build_runtime_env(emsdk_root, env)
    python_exe = runtime_env.get("EMSDK_PYTHON", sys.executable)
    command = [python_exe, "-E", str(emcc_py), *sys.argv[1:]]
    return subprocess.run(command, cwd=str(Path.cwd()), env=runtime_env).returncode


if __name__ == "__main__":
    raise SystemExit(main())

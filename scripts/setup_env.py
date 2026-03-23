#!/usr/bin/env python3
"""EmbeddedGUI environment setup entrypoint."""

from __future__ import annotations

import argparse
import hashlib
import os
from pathlib import Path
import shutil
import ssl
import subprocess
import sys
import urllib.request


MIN_PYTHON_VERSION = (3, 8)
DEFAULT_VENV_DIR = ".venv"
REQUIREMENTS_BASIC = [Path("requirements.txt")]
REQUIREMENTS_FULL = [
    Path("requirements.txt"),
    Path("scripts/ui_designer/requirements-desktop.txt"),
]
EXTRA_PACKAGES_FULL = ["playwright"]
PIP_INDEX_URLS = [
    ("TUNA mirror", "https://pypi.tuna.tsinghua.edu.cn/simple"),
    ("PyPI", "https://pypi.org/simple"),
]

W64DEVKIT_VERSION = "2.5.0"
W64DEVKIT_FILENAME = f"w64devkit-x64-{W64DEVKIT_VERSION}.7z.exe"
W64DEVKIT_URL_PRIMARY = (
    f"https://github.com/skeeto/w64devkit/releases/download/v{W64DEVKIT_VERSION}/{W64DEVKIT_FILENAME}"
)
W64DEVKIT_URL_MIRROR = (
    f"https://ghfast.top/https://github.com/skeeto/w64devkit/releases/download/v{W64DEVKIT_VERSION}/{W64DEVKIT_FILENAME}"
)
W64DEVKIT_SHA256 = ""


def project_root() -> Path:
    return Path(__file__).resolve().parents[1]


def is_windows() -> bool:
    return os.name == "nt"


def venv_python_path(venv_dir: Path) -> Path:
    if is_windows():
        return venv_dir / "Scripts" / "python.exe"
    return venv_dir / "bin" / "python"


def quoted(path_or_text: object) -> str:
    return f'"{path_or_text}"'


def log_header(title: str) -> None:
    print()
    print("=" * 40)
    print(f"  {title}")
    print("=" * 40)


def run(
    command: list[str],
    *,
    cwd: Path | None = None,
    env: dict[str, str] | None = None,
    capture_output: bool = False,
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        command,
        cwd=str(cwd) if cwd else None,
        env=env,
        text=True,
        capture_output=capture_output,
    )


def find_command(name: str, env: dict[str, str] | None = None) -> str | None:
    search_path = None if env is None else env.get("PATH")
    return shutil.which(name, path=search_path)


def ensure_python_version() -> None:
    version = sys.version_info
    if version < MIN_PYTHON_VERSION:
        print(
            f"[!!] Python {version.major}.{version.minor} is too old. "
            f"EmbeddedGUI requires Python {MIN_PYTHON_VERSION[0]}.{MIN_PYTHON_VERSION[1]}+."
        )
        raise SystemExit(1)

    print(f"[OK] Python {version.major}.{version.minor}.{version.micro}")


def ensure_venv(project_dir: Path, venv_dir: Path) -> Path:
    venv_python = venv_python_path(venv_dir)
    if venv_python.exists():
        result = run([str(venv_python), "--version"], capture_output=True)
        if result.returncode == 0:
            print(f"[OK] Reusing virtual environment: {venv_dir}")
            return venv_python

        print(f"[!!] Broken virtual environment detected, recreating: {venv_dir}")
        shutil.rmtree(venv_dir, ignore_errors=True)

    print(f"Creating virtual environment: {venv_dir}")
    result = run([sys.executable, "-m", "venv", str(venv_dir)], cwd=project_dir)
    if result.returncode != 0:
        print("[!!] Failed to create virtual environment.")
        raise SystemExit(1)

    return venv_python


def _run_pip_install(venv_python: Path, args: list[str], index_url: str) -> subprocess.CompletedProcess[str]:
    command = [
        str(venv_python),
        "-m",
        "pip",
        "install",
        "--disable-pip-version-check",
        "--progress-bar",
        "off",
        "-i",
        index_url,
    ] + args
    return run(command, capture_output=True)


def _print_result_tail(result: subprocess.CompletedProcess[str], max_lines: int = 10) -> None:
    lines: list[str] = []
    for stream in (result.stdout, result.stderr):
        if not stream:
            continue
        for line in stream.splitlines():
            stripped = line.strip()
            if stripped:
                lines.append(stripped)

    if not lines:
        return

    print("     Last pip output:")
    for line in lines[-max_lines:]:
        print(f"       {line}")


def pip_install_with_fallback(venv_python: Path, args: list[str], label: str) -> bool:
    print(f"Installing {label} ...")
    last_result: subprocess.CompletedProcess[str] | None = None
    for index_name, index_url in PIP_INDEX_URLS:
        result = _run_pip_install(venv_python, args, index_url)
        last_result = result
        if result.returncode == 0:
            print(f"[OK] {label}")
            return True
        print(f"[!!] Failed via {index_name}: {' '.join(args)}")

    if last_result is not None:
        _print_result_tail(last_result)
    return False


def print_manual_python_help(venv_python: Path, profile: str, venv_dir: Path) -> None:
    rel_python = os.path.relpath(venv_python, project_root())
    rel_python = rel_python.replace("/", "\\") if is_windows() else rel_python

    print("Manual recovery steps:")
    print(f"  {rel_python} -m pip install -r requirements.txt")
    if profile == "full":
        print(f"  {rel_python} -m pip install -r scripts/ui_designer/requirements-desktop.txt")
        print(f"  {rel_python} -m pip install playwright")
        print(f"  {rel_python} -m playwright install chromium")
    print()
    print("Virtual environment activation:")
    if is_windows():
        print(f"  {venv_dir}\\Scripts\\activate.bat")
    else:
        print(f"  source {venv_dir}/bin/activate")


def verify_python_environment(venv_python: Path, profile: str, root_dir: Path) -> bool:
    imports = [
        "import json5",
        "import numpy",
        "from PIL import Image",
        "import freetype",
        "from elftools.elf.elffile import ELFFile",
    ]
    if profile == "full":
        imports.extend(
            [
                f"root = {str(root_dir)!r}",
                "scripts_dir = os.path.join(root, 'scripts')",
                "if scripts_dir not in sys.path:",
                "    sys.path.insert(0, scripts_dir)",
                "from PyQt5.QtWidgets import QApplication",
                "import qfluentwidgets",
                "import ui_designer.main",
                "from playwright.sync_api import sync_playwright",
            ]
        )

    script = "import os, sys\n" + "\n".join(imports) + "\nprint('ok')\n"
    result = run([str(venv_python), "-c", script], capture_output=True)
    if result.returncode == 0:
        print("[OK] Python dependency verification passed.")
        return True

    print("[!!] Python dependency verification failed.")
    if result.stderr.strip():
        print(result.stderr.strip())
    elif result.stdout.strip():
        print(result.stdout.strip())
    return False


def install_python_environment(root_dir: Path, venv_dir: Path, profile: str) -> bool:
    log_header("Python Setup")
    ensure_python_version()
    venv_python = ensure_venv(root_dir, venv_dir)

    if not pip_install_with_fallback(venv_python, ["--upgrade", "pip"], "pip upgrade"):
        print_manual_python_help(venv_python, profile, venv_dir)
        return False

    requirements = REQUIREMENTS_BASIC if profile == "basic" else REQUIREMENTS_FULL
    for requirement in requirements:
        requirement_path = root_dir / requirement
        if not requirement_path.exists():
            print(f"[!!] Missing requirement file: {requirement}")
            print_manual_python_help(venv_python, profile, venv_dir)
            return False

        if not pip_install_with_fallback(venv_python, ["-r", str(requirement_path)], f"requirements from {requirement}"):
            print_manual_python_help(venv_python, profile, venv_dir)
            return False

    if profile == "full":
        if not pip_install_with_fallback(venv_python, EXTRA_PACKAGES_FULL, "extra Python tooling"):
            print_manual_python_help(venv_python, profile, venv_dir)
            return False

    if not verify_python_environment(venv_python, profile, root_dir):
        print_manual_python_help(venv_python, profile, venv_dir)
        return False

    print()
    print("Virtual environment ready.")
    if is_windows():
        print(f"  Activate with: {venv_dir}\\Scripts\\activate.bat")
    else:
        print(f"  Activate with: source {venv_dir}/bin/activate")
    return True


def verify_checksum(file_path: Path, expected_sha256: str) -> bool:
    sha = hashlib.sha256()
    with file_path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(65536), b""):
            sha.update(chunk)
    return sha.hexdigest() == expected_sha256


def download_file(url: str, destination: Path) -> bool:
    print(f"Downloading: {url}")
    try:
        ssl_context = ssl.create_default_context()
        request = urllib.request.Request(url, headers={"User-Agent": "EmbeddedGUI-Setup/2.0"})
        with urllib.request.urlopen(request, context=ssl_context, timeout=120) as response:
            total = int(response.headers.get("Content-Length", 0))
            downloaded = 0
            chunk_size = 256 * 1024

            with destination.open("wb") as output:
                while True:
                    chunk = response.read(chunk_size)
                    if not chunk:
                        break
                    output.write(chunk)
                    downloaded += len(chunk)
                    if total > 0:
                        percent = downloaded * 100 // total
                        print(
                            f"\r  [{percent:3d}%] {downloaded // 1048576}/{max(total // 1048576, 1)} MB",
                            end="",
                            flush=True,
                        )
        if total > 0:
            print()
        return True
    except Exception as exc:  # pragma: no cover - network path
        print(f"[!!] Download failed: {exc}")
        destination.unlink(missing_ok=True)
        return False


def print_manual_toolchain_help(root_dir: Path) -> None:
    devkit_dir = root_dir / "tools" / "w64devkit"
    print("Manual recovery steps:")
    print(f"  1. Download: {W64DEVKIT_URL_PRIMARY}")
    print(f"  2. Extract to: {devkit_dir}")
    print(f"  3. Add {devkit_dir / 'bin'} to PATH or rerun setup.")


def download_w64devkit(root_dir: Path) -> bool:
    tools_dir = root_dir / "tools"
    devkit_dir = tools_dir / "w64devkit"
    gcc_exe = devkit_dir / "bin" / "gcc.exe"
    if gcc_exe.exists():
        print(f"[OK] Reusing existing w64devkit: {gcc_exe}")
        return True

    tools_dir.mkdir(parents=True, exist_ok=True)
    archive_path = tools_dir / W64DEVKIT_FILENAME
    urls = [W64DEVKIT_URL_MIRROR, W64DEVKIT_URL_PRIMARY]
    for url in urls:
        if archive_path.exists():
            break
        if download_file(url, archive_path):
            break

    if not archive_path.exists():
        print("[!!] Unable to download w64devkit.")
        print_manual_toolchain_help(root_dir)
        return False

    if W64DEVKIT_SHA256:
        print("Verifying download checksum ...")
        if not verify_checksum(archive_path, W64DEVKIT_SHA256):
            print("[!!] Checksum verification failed.")
            archive_path.unlink(missing_ok=True)
            print_manual_toolchain_help(root_dir)
            return False

    print("Extracting w64devkit ...")
    result = run([str(archive_path), f"-o{tools_dir}", "-y"], cwd=root_dir, capture_output=True)
    if result.returncode != 0 or not gcc_exe.exists():
        print("[!!] Failed to extract w64devkit.")
        if result.stderr.strip():
            print(result.stderr.strip())
        print_manual_toolchain_help(root_dir)
        return False

    archive_path.unlink(missing_ok=True)
    print(f"[OK] Installed w64devkit to: {devkit_dir}")
    return True


def local_w64devkit_bin(root_dir: Path) -> Path:
    return root_dir / "tools" / "w64devkit" / "bin"


def prepend_path(env: dict[str, str], path: Path) -> dict[str, str]:
    updated = env.copy()
    updated["PATH"] = str(path) + os.pathsep + updated.get("PATH", "")
    return updated


def ensure_windows_toolchain(root_dir: Path, auto_install: bool) -> tuple[dict[str, str], bool]:
    log_header("Windows Toolchain")
    env = os.environ.copy()
    local_bin = local_w64devkit_bin(root_dir)
    if local_bin.exists():
        env = prepend_path(env, local_bin)

    make_path = find_command("make.exe", env) or find_command("make", env)
    gcc_path = find_command("gcc.exe", env) or find_command("gcc", env)
    if make_path and gcc_path:
        print(f"[OK] make: {make_path}")
        print(f"[OK] gcc : {gcc_path}")
        if local_bin.exists():
            print(f"[OK] Local w64devkit available: {local_bin}")
        return env, True

    print("[!!] make/gcc not found in PATH.")
    if not auto_install:
        print_manual_toolchain_help(root_dir)
        return env, False

    if not download_w64devkit(root_dir):
        return env, False

    env = prepend_path(os.environ.copy(), local_bin)
    make_path = find_command("make.exe", env) or find_command("make", env)
    gcc_path = find_command("gcc.exe", env) or find_command("gcc", env)
    if make_path and gcc_path:
        print(f"[OK] make: {make_path}")
        print(f"[OK] gcc : {gcc_path}")
        print(f"[OK] Add this to PATH if needed: {local_bin}")
        return env, True

    print("[!!] w64devkit was downloaded, but make/gcc are still unavailable.")
    print_manual_toolchain_help(root_dir)
    return env, False


def check_posix_toolchain() -> tuple[dict[str, str], bool]:
    log_header("Build Toolchain")
    env = os.environ.copy()
    make_path = find_command("make", env)
    gcc_path = find_command("gcc", env)
    if make_path and gcc_path:
        print(f"[OK] make: {make_path}")
        print(f"[OK] gcc : {gcc_path}")
        return env, True

    print("[!!] make or gcc not found.")
    print("Install them with your package manager, then rerun setup.")
    print("  Debian/Ubuntu: sudo apt install build-essential")
    print("  Fedora      : sudo dnf install make gcc")
    print("  Arch Linux  : sudo pacman -S base-devel")
    return env, False


def run_build_verification(root_dir: Path, env: dict[str, str]) -> bool:
    log_header("Build Verification")
    make_cmd = find_command("make.exe", env) or find_command("make", env)
    if not make_cmd:
        print("[!!] Skipping build verification because make is unavailable.")
        return False

    command = [
        make_cmd,
        "all",
        "APP=HelloSimple",
        "PORT=pc",
        "COMPILE_DEBUG=",
        "COMPILE_OPT_LEVEL=-O0",
    ]
    print("Running:", " ".join(quoted(part) if " " in part else part for part in command))
    result = run(command, cwd=root_dir, env=env)
    if result.returncode == 0:
        print("[OK] HelloSimple build verification passed.")
        return True

    print("[!!] Build verification failed.")
    return False


def print_summary(root_dir: Path, venv_dir: Path, profile: str, toolchain_ready: bool) -> None:
    log_header("Summary")
    print(f"Project root : {root_dir}")
    print(f"Python mode  : {profile}")
    print(f"Virtual env  : {venv_dir}")
    print(f"Toolchain    : {'ready' if toolchain_ready else 'not ready'}")
    print()
    print("Common commands:")
    print("  make all APP=HelloSimple")
    print("  make run")
    if is_windows():
        print(f"  {venv_dir}\\Scripts\\activate.bat")
    else:
        print(f"  source {venv_dir}/bin/activate")


def normalize_python_mode(args: argparse.Namespace) -> str:
    if args.mode is None:
        return args.python_mode

    legacy_map = {
        "0": "none",
        "1": "basic",
        "2": "full",
        "3": "none",
    }
    return legacy_map[args.mode]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="EmbeddedGUI environment setup")
    parser.add_argument(
        "--python-mode",
        choices=["full", "basic", "none"],
        default="full",
        help="Python dependency profile to install (default: full).",
    )
    parser.add_argument(
        "--mode",
        choices=["0", "1", "2", "3"],
        help=argparse.SUPPRESS,
    )
    parser.add_argument(
        "--venv-dir",
        default=DEFAULT_VENV_DIR,
        help="Virtual environment directory (default: .venv).",
    )
    parser.add_argument(
        "--skip-toolchain",
        action="store_true",
        help="Do not auto-install or validate the native build toolchain.",
    )
    parser.add_argument(
        "--install-toolchain",
        action="store_true",
        help="Install the Windows w64devkit toolchain and exit.",
    )
    parser.add_argument(
        "--skip-build-check",
        action="store_true",
        help="Skip the final HelloSimple build verification step.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    root_dir = project_root()
    venv_dir = (root_dir / args.venv_dir).resolve()
    python_mode = normalize_python_mode(args)

    os.chdir(root_dir)

    if args.install_toolchain:
        if not is_windows():
            print("[!!] --install-toolchain is only supported on Windows.")
            return 1
        _, ready = ensure_windows_toolchain(root_dir, auto_install=True)
        return 0 if ready else 1

    if python_mode != "none":
        if not install_python_environment(root_dir, venv_dir, python_mode):
            return 1
    else:
        print("Skipping Python dependency installation.")

    if args.skip_toolchain:
        toolchain_env = os.environ.copy()
        toolchain_ready = True
        print("Skipping toolchain setup by request.")
    else:
        if is_windows():
            toolchain_env, toolchain_ready = ensure_windows_toolchain(root_dir, auto_install=True)
        else:
            toolchain_env, toolchain_ready = check_posix_toolchain()

    if toolchain_ready and not args.skip_build_check:
        if not run_build_verification(root_dir, toolchain_env):
            return 1
    elif not toolchain_ready:
        print("Build verification skipped because the toolchain is not ready.")

    print_summary(root_dir, venv_dir, python_mode, toolchain_ready)
    if not args.skip_toolchain and not toolchain_ready:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

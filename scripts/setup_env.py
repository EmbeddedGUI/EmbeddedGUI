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
import tempfile
import urllib.request
import zipfile


MIN_PYTHON_VERSION = (3, 8)
DEFAULT_VENV_DIR = ".venv"
REQUIREMENTS_BASIC = [Path("requirements.txt")]
REQUIREMENTS_FULL = [Path("requirements.txt")]
EXTRA_PACKAGES_FULL = []
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

FFMPEG_WINDOWS_FILENAME = "ffmpeg-release-essentials.zip"
FFMPEG_WINDOWS_URL_PRIMARY = "https://www.gyan.dev/ffmpeg/builds/ffmpeg-release-essentials.zip"
EMSDK_ENV_KEYS = ("EMSDK_PATH", "EMSDK")
EMSDK_GIT_URL_PRIMARY = "https://github.com/emscripten-core/emsdk.git"
EMSDK_GIT_URL_MIRROR = "https://ghfast.top/https://github.com/emscripten-core/emsdk.git"
DEFAULT_EMSDK_VERSION = os.environ.get("EMSDK_VERSION", "latest")
EMSDK_COMMAND_TIMEOUT = 3600


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
    timeout: float | None = None,
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        command,
        cwd=str(cwd) if cwd else None,
        env=env,
        text=True,
        capture_output=capture_output,
        timeout=timeout,
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
    print()
    print("Virtual environment activation:")
    if is_windows():
        print(f"  {venv_dir}\\Scripts\\activate.bat")
    else:
        print(f"  source {venv_dir}/bin/activate")


def build_python_verify_script(profile: str, root_dir: Path) -> str:
    imports = [
        "import json5",
        "import numpy",
        "from PIL import Image",
        "import freetype",
        "from elftools.elf.elffile import ELFFile",
    ]
    return "import os, sys\n" + "\n".join(imports) + "\nprint('ok')\n"


def verify_python_environment(venv_python: Path, profile: str, root_dir: Path) -> bool:
    script = build_python_verify_script(profile, root_dir)
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


def ffmpeg_executable_name() -> str:
    return "ffmpeg.exe" if is_windows() else "ffmpeg"


def local_ffmpeg_bin(root_dir: Path) -> Path:
    return root_dir / "tools" / "ffmpeg" / "bin"


def local_emsdk_dir(root_dir: Path) -> Path:
    return root_dir / "tools" / "emsdk"


def print_manual_ffmpeg_help(root_dir: Path) -> None:
    ffmpeg_dir = root_dir / "tools" / "ffmpeg"
    print("Manual recovery steps:")
    if is_windows():
        print(f"  1. Download: {FFMPEG_WINDOWS_URL_PRIMARY}")
        print("  2. Extract it so this file exists:")
        print(f"     {ffmpeg_dir / 'bin' / 'ffmpeg.exe'}")
        print(f"  3. Add {ffmpeg_dir / 'bin'} to PATH or rerun setup.")
    else:
        print("  Install FFmpeg with your package manager, then rerun setup.")
        print("  Debian/Ubuntu: sudo apt install ffmpeg")
        print("  Fedora      : sudo dnf install ffmpeg")
        print("  Arch Linux  : sudo pacman -S ffmpeg")
        print("  macOS       : brew install ffmpeg")
        print(f"  Or place a local build under: {ffmpeg_dir}")
    print("  Use --skip-ffmpeg if you do not need MP4 or GIF workflows.")


def find_tool_dir_by_executable(search_root: Path, executable_name: str) -> Path | None:
    for candidate in search_root.rglob(executable_name):
        if candidate.parent.name == "bin":
            return candidate.parent.parent
    return None


def extract_zip_tool_archive(archive_path: Path, destination_dir: Path, executable_name: str) -> bool:
    destination_dir.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=f"{destination_dir.name}_", dir=str(destination_dir.parent)) as temp_dir_name:
        temp_dir = Path(temp_dir_name)
        try:
            with zipfile.ZipFile(archive_path) as archive:
                archive.extractall(temp_dir)
        except (OSError, zipfile.BadZipFile) as exc:
            print(f"[!!] Failed to extract zip archive: {exc}")
            return False

        extracted_dir = find_tool_dir_by_executable(temp_dir, executable_name)
        if extracted_dir is None:
            print(f"[!!] Extracted archive did not contain {executable_name}.")
            return False

        if destination_dir.exists():
            shutil.rmtree(destination_dir, ignore_errors=True)
        shutil.move(str(extracted_dir), str(destination_dir))

    return True


def validate_ffmpeg(ffmpeg_path: str, env: dict[str, str]) -> bool:
    try:
        result = run([ffmpeg_path, "-version"], env=env, capture_output=True)
    except OSError as exc:
        print(f"[!!] Found ffmpeg at {ffmpeg_path}, but it failed to start: {exc}")
        return False

    if result.returncode == 0:
        return True

    print(f"[!!] Found ffmpeg at {ffmpeg_path}, but it failed to run.")
    if result.stderr.strip():
        print(result.stderr.strip())
    elif result.stdout.strip():
        print(result.stdout.strip())
    return False


def download_ffmpeg_windows(root_dir: Path) -> bool:
    tools_dir = root_dir / "tools"
    ffmpeg_dir = tools_dir / "ffmpeg"
    ffmpeg_exe = ffmpeg_dir / "bin" / "ffmpeg.exe"
    if ffmpeg_exe.exists():
        print(f"[OK] Reusing existing FFmpeg: {ffmpeg_exe}")
        return True

    tools_dir.mkdir(parents=True, exist_ok=True)
    archive_path = tools_dir / FFMPEG_WINDOWS_FILENAME
    if not archive_path.exists() and not download_file(FFMPEG_WINDOWS_URL_PRIMARY, archive_path):
        print("[!!] Unable to download FFmpeg.")
        print_manual_ffmpeg_help(root_dir)
        return False

    print("Extracting FFmpeg ...")
    if not extract_zip_tool_archive(archive_path, ffmpeg_dir, "ffmpeg.exe"):
        archive_path.unlink(missing_ok=True)
        print_manual_ffmpeg_help(root_dir)
        return False

    archive_path.unlink(missing_ok=True)
    if not ffmpeg_exe.exists():
        print("[!!] FFmpeg extraction completed, but ffmpeg.exe was not found.")
        print_manual_ffmpeg_help(root_dir)
        return False

    print(f"[OK] Installed FFmpeg to: {ffmpeg_dir}")
    return True


def ensure_ffmpeg(root_dir: Path, env: dict[str, str], auto_install: bool) -> tuple[dict[str, str], bool]:
    log_header("Media Tools")
    local_bin = local_ffmpeg_bin(root_dir)
    if local_bin.exists():
        env = prepend_path(env, local_bin)

    ffmpeg_path = find_command("ffmpeg.exe", env) or find_command("ffmpeg", env)
    if ffmpeg_path and validate_ffmpeg(ffmpeg_path, env):
        print(f"[OK] ffmpeg: {ffmpeg_path}")
        if local_bin.exists():
            print(f"[OK] Local FFmpeg available: {local_bin}")
        return env, True

    print("[!!] ffmpeg not found in PATH, or the existing binary is unusable.")
    if not is_windows() or not auto_install:
        print_manual_ffmpeg_help(root_dir)
        return env, False

    if not download_ffmpeg_windows(root_dir):
        return env, False

    env = prepend_path(env, local_bin)
    ffmpeg_path = find_command(ffmpeg_executable_name(), env) or find_command("ffmpeg", env)
    if ffmpeg_path and validate_ffmpeg(ffmpeg_path, env):
        print(f"[OK] ffmpeg: {ffmpeg_path}")
        print(f"[OK] Add this to PATH if needed: {local_bin}")
        return env, True

    print("[!!] FFmpeg was downloaded, but ffmpeg is still unavailable.")
    print_manual_ffmpeg_help(root_dir)
    return env, False


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


def display_path(path_or_text: str | Path, root_dir: Path | None = None) -> str:
    path_text = str(path_or_text)
    try:
        path_obj = Path(path_text)
        if root_dir is not None:
            try:
                path_text = str(path_obj.resolve().relative_to(root_dir.resolve()))
            except ValueError:
                path_text = str(path_obj.resolve())
        else:
            path_text = str(path_obj)
    except OSError:
        path_text = str(path_or_text)

    if is_windows():
        return path_text.replace("/", "\\")
    return path_text


def first_non_empty_line(text: str) -> str:
    for line in text.splitlines():
        stripped = line.strip()
        if stripped:
            return stripped
    return ""


def last_non_empty_line(text: str) -> str:
    lines = [line.strip() for line in text.splitlines() if line.strip()]
    return lines[-1] if lines else ""


def summarize_process_error(result: subprocess.CompletedProcess[str]) -> str:
    return last_non_empty_line(result.stderr) or last_non_empty_line(result.stdout) or f"exit code {result.returncode}"


def probe_python_host(root_dir: Path) -> str:
    version = f"{sys.version_info.major}.{sys.version_info.minor}.{sys.version_info.micro}"
    return f"ready ({display_path(sys.executable, root_dir)}; Python {version})"


def probe_virtualenv(venv_dir: Path, profile: str, root_dir: Path) -> tuple[Path | None, str]:
    venv_python = venv_python_path(venv_dir)
    if not venv_python.exists():
        if profile == "none":
            return None, "not requested"
        return None, f"missing ({display_path(venv_dir, root_dir)})"

    result = run([str(venv_python), "--version"], capture_output=True)
    if result.returncode != 0:
        return None, f"broken ({display_path(venv_python, root_dir)}; {summarize_process_error(result)})"

    version_text = first_non_empty_line(result.stdout) or first_non_empty_line(result.stderr) or "Python"
    if profile == "none":
        return venv_python, f"present ({display_path(venv_python, root_dir)}; {version_text})"
    return venv_python, f"ready ({display_path(venv_python, root_dir)}; {version_text})"


def probe_python_dependencies(venv_python: Path | None, profile: str, root_dir: Path) -> str:
    if profile == "none":
        return "skipped (python-mode=none)"
    if venv_python is None:
        return "not ready (virtual environment unavailable)"

    script = build_python_verify_script(profile, root_dir)
    result = run([str(venv_python), "-c", script], capture_output=True)
    if result.returncode == 0:
        return f"ready ({profile})"
    return f"not ready ({summarize_process_error(result)})"


def probe_command_status(command_names: list[str], env: dict[str, str], root_dir: Path, version_args: list[str] | None = None) -> str:
    version_args = version_args or ["--version"]
    command_path = None
    for name in command_names:
        command_path = find_command(name, env)
        if command_path:
            break

    if not command_path:
        return "missing"

    result = run([command_path] + version_args, env=env, capture_output=True)
    if result.returncode != 0:
        return f"broken ({display_path(command_path, root_dir)}; {summarize_process_error(result)})"

    version_line = first_non_empty_line(result.stdout) or first_non_empty_line(result.stderr) or "ok"
    return f"ready ({display_path(command_path, root_dir)}; {version_line})"


def probe_local_tool_status(tool_dir: Path, executable_path: Path, root_dir: Path) -> str:
    if executable_path.exists():
        return f"installed ({display_path(tool_dir, root_dir)})"
    if tool_dir.exists():
        return f"incomplete ({display_path(tool_dir, root_dir)})"
    return "not installed"


def emsdk_script_name() -> str:
    return "emsdk.bat" if is_windows() else "emsdk"


def emsdk_env_script_name() -> str:
    return "emsdk_env.bat" if is_windows() else "emsdk_env.sh"


def emcc_executable_name() -> str:
    return "emcc.bat" if is_windows() else "emcc"


def emsdk_script_path(emsdk_root: Path) -> Path:
    return emsdk_root / emsdk_script_name()


def emsdk_env_script_path(emsdk_root: Path) -> Path:
    return emsdk_root / emsdk_env_script_name()


def local_emcc_path(root_dir: Path) -> Path:
    return local_emsdk_dir(root_dir) / "upstream" / "emscripten" / emcc_executable_name()


def format_shell_command(parts: list[str]) -> str:
    return " ".join(quoted(part) if " " in part else part for part in parts)


def print_manual_emsdk_help(root_dir: Path, version: str = DEFAULT_EMSDK_VERSION) -> None:
    emsdk_dir = local_emsdk_dir(root_dir)
    print("Manual recovery steps:")
    if is_windows():
        print("  1. Ensure git is installed and available in PATH.")
        print(f"  2. Clone: git clone {EMSDK_GIT_URL_PRIMARY} {quoted(display_path(emsdk_dir, root_dir))}")
        print(f"  3. Install: {display_path(emsdk_dir / 'emsdk.bat', root_dir)} install {version}")
        print(f"  4. Activate: {display_path(emsdk_dir / 'emsdk.bat', root_dir)} activate {version}")
        print(f"  5. Enable for a shell: call {display_path(emsdk_dir / 'emsdk_env.bat', root_dir)}")
    else:
        print("  1. Ensure git is installed and available in PATH.")
        print(f"  2. Clone: git clone {EMSDK_GIT_URL_PRIMARY} {quoted(display_path(emsdk_dir, root_dir))}")
        print(f"  3. Install: {display_path(emsdk_dir / 'emsdk', root_dir)} install {version}")
        print(f"  4. Activate: {display_path(emsdk_dir / 'emsdk', root_dir)} activate {version}")
        print(f"  5. Enable for a shell: source {display_path(emsdk_dir / 'emsdk_env.sh', root_dir)}")
    print("  Use --skip-emsdk if you do not need WASM workflows.")


def run_emsdk_command(
    emsdk_root: Path,
    args: list[str],
    env: dict[str, str],
    *,
    capture_output: bool = False,
    timeout: float | None = EMSDK_COMMAND_TIMEOUT,
) -> subprocess.CompletedProcess[str]:
    script = emsdk_script_path(emsdk_root)
    if is_windows():
        command_text = f'call "{script}"'
        for arg in args:
            command_text += " " + (quoted(arg) if " " in arg else arg)
        return subprocess.run(
            command_text,
            cwd=str(emsdk_root),
            env=env,
            text=True,
            capture_output=capture_output,
            timeout=timeout,
            shell=True,
            executable=os.environ.get("COMSPEC", "cmd.exe"),
        )

    return run([str(script)] + args, cwd=emsdk_root, env=env, capture_output=capture_output, timeout=timeout)


def clone_emsdk_repository(root_dir: Path, env: dict[str, str]) -> bool:
    emsdk_dir = local_emsdk_dir(root_dir)
    emsdk_script = emsdk_script_path(emsdk_dir)

    if emsdk_script.exists():
        print(f"[OK] Reusing existing emsdk repository: {emsdk_dir}")
        return True

    if emsdk_dir.exists():
        print(f"[!!] Existing emsdk directory is incomplete: {emsdk_dir}")
        print_manual_emsdk_help(root_dir)
        return False

    git_path = find_command("git.exe", env) or find_command("git", env)
    if not git_path:
        print("[!!] git not found in PATH, cannot clone emsdk.")
        print_manual_emsdk_help(root_dir)
        return False

    emsdk_dir.parent.mkdir(parents=True, exist_ok=True)
    for url in [EMSDK_GIT_URL_MIRROR, EMSDK_GIT_URL_PRIMARY]:
        print(f"Cloning emsdk repository from: {url}")
        result = run([git_path, "clone", "--depth", "1", url, str(emsdk_dir)], cwd=root_dir, env=env, capture_output=True, timeout=EMSDK_COMMAND_TIMEOUT)
        if result.returncode == 0 and emsdk_script.exists():
            print(f"[OK] Cloned emsdk to: {emsdk_dir}")
            return True

        shutil.rmtree(emsdk_dir, ignore_errors=True)

    print("[!!] Unable to clone emsdk repository.")
    if result.stderr.strip():
        print(result.stderr.strip())
    elif result.stdout.strip():
        print(result.stdout.strip())
    print_manual_emsdk_help(root_dir)
    return False


def capture_emsdk_environment(emsdk_root: Path, env: dict[str, str]) -> tuple[dict[str, str] | None, str | None]:
    env_script = emsdk_env_script_path(emsdk_root)
    if not env_script.exists():
        return None, f"{display_path(env_script)} missing"

    activation_env = env.copy()
    activation_env["EMSDK_QUIET"] = "1"

    if is_windows():
        result = subprocess.run(
            f'call "{env_script}" >nul && set',
            cwd=str(emsdk_root),
            env=activation_env,
            text=True,
            capture_output=True,
            timeout=EMSDK_COMMAND_TIMEOUT,
            shell=True,
            executable=os.environ.get("COMSPEC", "cmd.exe"),
        )
    else:
        bash = find_command("bash", env) or "bash"
        result = run(
            [bash, "-lc", f'source "{env_script}" >/dev/null && env'],
            cwd=emsdk_root,
            env=activation_env,
            capture_output=True,
            timeout=EMSDK_COMMAND_TIMEOUT,
        )

    if result.returncode != 0:
        return None, summarize_process_error(result)

    activated_env: dict[str, str] = {}
    for line in result.stdout.splitlines():
        if "=" not in line or line.startswith("="):
            continue
        key, value = line.split("=", 1)
        if key:
            activated_env[key] = value

    if not activated_env:
        return None, "environment capture produced no output"
    return activated_env, None


def inject_emsdk_paths(env: dict[str, str], emsdk_root: Path) -> dict[str, str]:
    updated = env.copy()
    updated["EMSDK"] = str(emsdk_root)
    updated["EMSDK_PATH"] = str(emsdk_root)

    if "EM_CONFIG" not in updated:
        em_config = Path.home() / ".emscripten"
        if em_config.exists():
            updated["EM_CONFIG"] = str(em_config)

    path_entries: list[Path] = [emsdk_root, emsdk_root / "upstream" / "emscripten"]
    for tool_dir_name in ("node", "python"):
        tool_dir = emsdk_root / tool_dir_name
        if not tool_dir.exists():
            continue
        for child in sorted(tool_dir.iterdir()):
            if not child.is_dir():
                continue
            if (child / "bin").exists():
                path_entries.append(child / "bin")
            else:
                path_entries.append(child)

    for path in reversed(path_entries):
        if path.exists():
            updated = prepend_path(updated, path)
    return updated


def build_emsdk_runtime_env(root_dir: Path, env: dict[str, str]) -> dict[str, str]:
    source, emsdk_root = find_emsdk_root(root_dir, env)
    if emsdk_root is None or not emsdk_root.exists():
        return env

    activated_env, error = capture_emsdk_environment(emsdk_root, env)
    if activated_env is not None:
        return activated_env

    updated = inject_emsdk_paths(env, emsdk_root)
    if error:
        updated["_EMSDK_CAPTURE_WARNING"] = error
    return updated


def ensure_emsdk(root_dir: Path, env: dict[str, str], auto_install: bool, version: str) -> tuple[dict[str, str], bool]:
    log_header("Emscripten SDK")
    runtime_env = build_emsdk_runtime_env(root_dir, env.copy())
    emcc_status = probe_emcc_status(root_dir, runtime_env)
    source, emsdk_root = find_emsdk_root(root_dir, runtime_env)

    if emcc_status.startswith("ready"):
        if emsdk_root is not None:
            print(f"[OK] EMSDK: {display_path(emsdk_root, root_dir)}")
        print(f"[OK] emcc : {emcc_status[len('ready ('):-1] if emcc_status.startswith('ready (') and emcc_status.endswith(')') else emcc_status}")
        return runtime_env, True

    print("[!!] emcc not found in PATH, or the existing Emscripten activation is unusable.")
    if source is not None and emsdk_root is not None and not emsdk_root.exists():
        print(f"[!!] {source} points to a missing directory: {display_path(emsdk_root, root_dir)}")

    if not auto_install:
        print_manual_emsdk_help(root_dir, version)
        return env, False

    target_root = emsdk_root if emsdk_root is not None and emsdk_root.exists() else local_emsdk_dir(root_dir)
    if target_root == local_emsdk_dir(root_dir) and not emsdk_script_path(target_root).exists():
        if not clone_emsdk_repository(root_dir, env):
            return env, False

    emsdk_script = emsdk_script_path(target_root)
    if not emsdk_script.exists():
        print(f"[!!] emsdk script not found: {emsdk_script}")
        print_manual_emsdk_help(root_dir, version)
        return env, False

    print(f"Installing Emscripten toolchain: {version}")
    install_result = run_emsdk_command(target_root, ["install", version], env, timeout=EMSDK_COMMAND_TIMEOUT)
    if install_result.returncode != 0:
        print("[!!] Failed to install Emscripten toolchain.")
        print_manual_emsdk_help(root_dir, version)
        return env, False

    print(f"Activating Emscripten toolchain: {version}")
    activate_result = run_emsdk_command(target_root, ["activate", version], env, timeout=EMSDK_COMMAND_TIMEOUT)
    if activate_result.returncode != 0:
        print("[!!] Failed to activate Emscripten toolchain.")
        print_manual_emsdk_help(root_dir, version)
        return env, False

    runtime_env = build_emsdk_runtime_env(root_dir, env.copy())
    emcc_status = probe_emcc_status(root_dir, runtime_env)
    if emcc_status.startswith("ready"):
        print(f"[OK] EMSDK: {display_path(target_root, root_dir)}")
        print(f"[OK] emcc : {emcc_status[len('ready ('):-1] if emcc_status.startswith('ready (') and emcc_status.endswith(')') else emcc_status}")
        env_script = emsdk_env_script_path(target_root)
        if env_script.exists():
            if is_windows():
                print(f"[OK] Activate for a shell if needed: call {display_path(env_script, root_dir)}")
            else:
                print(f"[OK] Activate for a shell if needed: source {display_path(env_script, root_dir)}")
        return runtime_env, True

    print("[!!] emsdk was installed, but emcc is still unavailable.")
    warning = runtime_env.get("_EMSDK_CAPTURE_WARNING")
    if warning:
        print(f"[!!] emsdk_env activation warning: {warning}")
    print_manual_emsdk_help(root_dir, version)
    return env, False


def build_summary_env(root_dir: Path, env: dict[str, str] | None = None) -> dict[str, str]:
    summary_env = (env or os.environ.copy()).copy()
    local_toolchain = local_w64devkit_bin(root_dir)
    local_media = local_ffmpeg_bin(root_dir)
    if local_toolchain.exists():
        summary_env = prepend_path(summary_env, local_toolchain)
    if local_media.exists():
        summary_env = prepend_path(summary_env, local_media)
    summary_env = build_emsdk_runtime_env(root_dir, summary_env)
    return summary_env


def find_emsdk_root(root_dir: Path, env: dict[str, str] | None = None) -> tuple[str | None, Path | None]:
    search_env = env or os.environ
    local_dir = local_emsdk_dir(root_dir)

    for key in EMSDK_ENV_KEYS:
        value = search_env.get(key)
        if not value:
            continue
        candidate = Path(value)
        if candidate.exists() or not local_dir.exists():
            return key, candidate
        return "local", local_dir

    if local_dir.exists():
        return "local", local_dir
    return None, None


def probe_emsdk_status(root_dir: Path, env: dict[str, str] | None = None) -> str:
    source, emsdk_root = find_emsdk_root(root_dir, env)
    if emsdk_root is None:
        return "missing"
    if not emsdk_root.exists():
        return f"broken ({source}={display_path(emsdk_root, root_dir)} missing)"
    return f"present ({source}={display_path(emsdk_root, root_dir)})"


def probe_emcc_status(root_dir: Path, env: dict[str, str]) -> str:
    command_path = find_command("emcc.bat", env) or find_command("emcc", env)
    if command_path:
        return probe_command_status(["emcc.bat", "emcc"], env, root_dir, ["--version"])

    _, emsdk_root = find_emsdk_root(root_dir, env)
    if emsdk_root is None or not emsdk_root.exists():
        return "missing"

    emcc_name = emcc_executable_name()
    candidate = emsdk_root / "upstream" / "emscripten" / emcc_name
    if not candidate.exists():
        return f"broken ({display_path(candidate, root_dir)} missing)"

    result = run([str(candidate), "--version"], env=env, capture_output=True)
    if result.returncode != 0:
        return f"broken ({display_path(candidate, root_dir)}; {summarize_process_error(result)})"
    version_line = first_non_empty_line(result.stdout) or first_non_empty_line(result.stderr) or "ok"
    return f"ready ({display_path(candidate, root_dir)}; {version_line})"


def print_status_line(label: str, value: str) -> None:
    status = "INFO"
    lowered = value.lower()

    if lowered.startswith("ready") or lowered.startswith("passed") or lowered.startswith("installed") or lowered.startswith("present"):
        status = "OK"
    elif lowered.startswith("skipped") or lowered.startswith("not requested"):
        status = "SKIP"
    elif lowered.startswith("missing") or lowered.startswith("broken") or lowered.startswith("not ready") or lowered.startswith("failed") or lowered.startswith("incomplete"):
        status = "FAIL"

    print(f"[{status:<4}] {label:<14}: {value}")


def print_summary(root_dir: Path, venv_dir: Path, profile: str, env: dict[str, str], build_status: str) -> None:
    log_header("Summary")
    summary_env = build_summary_env(root_dir, env)
    venv_python, venv_status = probe_virtualenv(venv_dir, profile, root_dir)

    print_status_line("Project root", display_path(root_dir))
    print_status_line("Platform", f"{sys.platform} ({os.name})")
    print_status_line("Host Python", probe_python_host(root_dir))
    print_status_line("Python mode", profile)
    print_status_line("Virtual env", venv_status)
    print_status_line("Python deps", probe_python_dependencies(venv_python, profile, root_dir))
    print_status_line("make", probe_command_status(["make.exe", "make"], summary_env, root_dir))
    print_status_line("gcc", probe_command_status(["gcc.exe", "gcc"], summary_env, root_dir))
    if is_windows():
        local_toolchain_dir = root_dir / "tools" / "w64devkit"
        local_toolchain_exe = local_toolchain_dir / "bin" / "gcc.exe"
        print_status_line("w64devkit", probe_local_tool_status(local_toolchain_dir, local_toolchain_exe, root_dir))
    print_status_line("FFmpeg", probe_command_status([ffmpeg_executable_name(), "ffmpeg"], summary_env, root_dir, ["-version"]))
    local_ffmpeg_dir = root_dir / "tools" / "ffmpeg"
    local_ffmpeg_exe = local_ffmpeg_dir / "bin" / ffmpeg_executable_name()
    print_status_line("Local FFmpeg", probe_local_tool_status(local_ffmpeg_dir, local_ffmpeg_exe, root_dir))
    local_emsdk = local_emsdk_dir(root_dir)
    print_status_line("EMSDK", probe_emsdk_status(root_dir, summary_env))
    print_status_line("Local EMSDK", probe_local_tool_status(local_emsdk, emsdk_script_path(local_emsdk), root_dir))
    print_status_line("emcc", probe_emcc_status(root_dir, summary_env))
    print_status_line("Build check", build_status)
    print()
    print("Common commands:")
    print("  make all APP=HelloSimple")
    if probe_emcc_status(root_dir, summary_env).startswith("ready"):
        print("  make all APP=HelloSimple PORT=emscripten")
    print("  make run")
    if is_windows():
        print(f"  {venv_dir}\\Scripts\\activate.bat")
        local_emsdk_env = local_emsdk / "emsdk_env.bat"
        if local_emsdk_env.exists():
            print(f"  call {display_path(local_emsdk_env, root_dir)}")
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
        "--skip-ffmpeg",
        action="store_true",
        help="Skip FFmpeg validation or installation for MP4 and GIF workflows.",
    )
    parser.add_argument(
        "--install-ffmpeg",
        action="store_true",
        help="Install the Windows FFmpeg bundle and exit.",
    )
    parser.add_argument(
        "--skip-emsdk",
        action="store_true",
        help="Skip Emscripten validation or installation for WASM workflows.",
    )
    parser.add_argument(
        "--install-emsdk",
        action="store_true",
        help="Install the Emscripten SDK bundle and exit.",
    )
    parser.add_argument(
        "--emsdk-version",
        default=DEFAULT_EMSDK_VERSION,
        help=f"Emscripten SDK version or alias to install/activate (default: {DEFAULT_EMSDK_VERSION}).",
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
    summary_env = os.environ.copy()
    build_status = "not run"
    exit_code = 0

    os.chdir(root_dir)

    install_only_flags = [args.install_toolchain, args.install_ffmpeg, args.install_emsdk]
    if sum(1 for flag in install_only_flags if flag) > 1:
        print("[!!] Use only one of --install-toolchain, --install-ffmpeg, or --install-emsdk.")
        return 1

    if args.install_toolchain:
        if not is_windows():
            print("[!!] --install-toolchain is only supported on Windows.")
            return 1
        summary_env, ready = ensure_windows_toolchain(root_dir, auto_install=True)
        build_status = "skipped (--install-toolchain)"
        exit_code = 0 if ready else 1
        print_summary(root_dir, venv_dir, python_mode, summary_env, build_status)
        return exit_code

    if args.install_ffmpeg:
        if not is_windows():
            print("[!!] --install-ffmpeg is only supported on Windows.")
            return 1
        summary_env, ready = ensure_ffmpeg(root_dir, os.environ.copy(), auto_install=True)
        build_status = "skipped (--install-ffmpeg)"
        exit_code = 0 if ready else 1
        print_summary(root_dir, venv_dir, python_mode, summary_env, build_status)
        return exit_code

    if args.install_emsdk:
        summary_env, ready = ensure_emsdk(root_dir, os.environ.copy(), auto_install=is_windows(), version=args.emsdk_version)
        build_status = "skipped (--install-emsdk)"
        exit_code = 0 if ready else 1
        print_summary(root_dir, venv_dir, python_mode, summary_env, build_status)
        return exit_code

    if python_mode != "none":
        if not install_python_environment(root_dir, venv_dir, python_mode):
            build_status = "skipped (python setup failed)"
            print_summary(root_dir, venv_dir, python_mode, summary_env, build_status)
            return 1
    else:
        print("Skipping Python dependency installation.")

    if args.skip_toolchain:
        toolchain_env = summary_env.copy()
        toolchain_ready = True
        print("Skipping toolchain setup by request.")
    else:
        if is_windows():
            toolchain_env, toolchain_ready = ensure_windows_toolchain(root_dir, auto_install=True)
        else:
            toolchain_env, toolchain_ready = check_posix_toolchain()

    if args.skip_ffmpeg:
        ffmpeg_ready = True
        print("Skipping FFmpeg setup by request.")
    else:
        toolchain_env, ffmpeg_ready = ensure_ffmpeg(root_dir, toolchain_env, auto_install=is_windows())

    if args.skip_emsdk:
        emsdk_ready = True
        print("Skipping Emscripten setup by request.")
    else:
        toolchain_env, emsdk_ready = ensure_emsdk(root_dir, toolchain_env, auto_install=is_windows(), version=args.emsdk_version)

    summary_env = toolchain_env

    if not toolchain_ready:
        build_status = "skipped (toolchain not ready)"
        print("Build verification skipped because the toolchain is not ready.")
    elif args.skip_build_check:
        build_status = "skipped by request"
    elif run_build_verification(root_dir, toolchain_env):
        build_status = "passed"
    else:
        build_status = "failed"
        exit_code = 1

    print_summary(root_dir, venv_dir, python_mode, summary_env, build_status)
    if not args.skip_toolchain and not toolchain_ready:
        return 1
    if not args.skip_ffmpeg and not ffmpeg_ready:
        return 1
    if not args.skip_emsdk and not emsdk_ready:
        return 1
    return exit_code


if __name__ == "__main__":
    raise SystemExit(main())

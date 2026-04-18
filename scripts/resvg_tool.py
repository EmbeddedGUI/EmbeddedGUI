from __future__ import annotations

import os
import platform
import shutil
import subprocess
import tarfile
import tempfile
import urllib.request
import zipfile
from pathlib import Path


RESVG_VERSION = "0.47.0"
RESVG_RELEASE_TAG = f"v{RESVG_VERSION}"
_EXECUTABLE_NAME = "resvg.exe" if os.name == "nt" else "resvg"
_RELEASE_BASE_URL = "https://github.com/linebender/resvg/releases/download/%s" % RESVG_RELEASE_TAG
_RELEASE_MIRROR_BASE_URL = "https://ghfast.top/%s" % _RELEASE_BASE_URL
_ASSET_MAP = {
    ("win32", "x86_64"): ("windows-x86_64", "resvg-win64.zip"),
    ("linux", "x86_64"): ("linux-x86_64", "resvg-linux-x86_64.tar.gz"),
    ("darwin", "x86_64"): ("macos-x86_64", "resvg-macos-x86_64.zip"),
    ("darwin", "arm64"): ("macos-aarch64", "resvg-macos-aarch64.zip"),
}


def project_root() -> Path:
    return Path(__file__).resolve().parents[1]


def normalize_machine(machine: str) -> str:
    normalized = str(machine or "").strip().lower()
    if normalized in ("amd64", "x86_64", "x64"):
        return "x86_64"
    if normalized in ("arm64", "aarch64"):
        return "arm64"
    return normalized


def current_platform_key() -> tuple[str, str]:
    return sys_platform(), normalize_machine(platform.machine())


def sys_platform() -> str:
    if os.name == "nt":
        return "win32"
    return platform.system().lower()


def release_asset_info() -> tuple[str, str]:
    key = current_platform_key()
    if key not in _ASSET_MAP:
        raise RuntimeError("unsupported resvg host platform: %s/%s" % key)
    return _ASSET_MAP[key]


def release_asset_url(asset_name: str) -> str:
    return "%s/%s" % (_RELEASE_BASE_URL, asset_name)


def release_asset_urls(asset_name: str) -> tuple[str, ...]:
    return (
        release_asset_url(asset_name),
        "%s/%s" % (_RELEASE_MIRROR_BASE_URL, asset_name),
    )


def local_install_dir(root_dir: Path | None = None) -> Path:
    root_dir = root_dir or project_root()
    platform_dir, _ = release_asset_info()
    return root_dir / "tools" / "resvg" / RESVG_VERSION / platform_dir


def local_resvg_path(root_dir: Path | None = None) -> Path:
    return local_install_dir(root_dir) / _EXECUTABLE_NAME


def find_resvg_binary(root_dir: Path | None = None, env: dict[str, str] | None = None) -> Path | None:
    root_dir = root_dir or project_root()
    env_map = env or os.environ

    explicit = str(env_map.get("RESVG", "")).strip().strip('"')
    if explicit:
        explicit_path = Path(explicit)
        if explicit_path.exists():
            return explicit_path

    try:
        local_path = local_resvg_path(root_dir)
    except RuntimeError:
        local_path = None
    if local_path is not None and local_path.exists():
        return local_path

    search_path = env_map.get("PATH")
    command_path = shutil.which(_EXECUTABLE_NAME, path=search_path) or shutil.which("resvg", path=search_path)
    if command_path:
        return Path(command_path)
    return None


def describe_resvg_missing(root_dir: Path | None = None, env: dict[str, str] | None = None) -> str:
    root_dir = root_dir or project_root()
    env_map = env or os.environ

    explicit = str(env_map.get("RESVG", "")).strip().strip('"')
    if explicit:
        return "resvg was requested via RESVG, but the file does not exist: %s" % explicit

    try:
        expected_local = local_resvg_path(root_dir)
    except RuntimeError:
        return "resvg is not available for this host platform"

    return (
        "resvg is required for build-time SVG rasterization. "
        "Set RESVG, install `resvg` in PATH, or run `python scripts/setup_resvg.py --install` "
        "to place it at %s" % expected_local
    )


def resolve_resvg_binary(root_dir: Path | None = None, env: dict[str, str] | None = None) -> Path:
    binary = find_resvg_binary(root_dir, env)
    if binary is None:
        raise RuntimeError(describe_resvg_missing(root_dir, env))
    return binary


def _download_file(url: str, destination: Path, timeout: int = 20) -> None:
    request = urllib.request.Request(url, headers={"User-Agent": "EmbeddedGUI-resvg-setup"})
    with urllib.request.urlopen(request, timeout=timeout) as response:
        destination.write_bytes(response.read())


def _extract_archive(archive_path: Path, destination_dir: Path) -> None:
    archive_name = archive_path.name.lower()
    if archive_name.endswith(".zip"):
        with zipfile.ZipFile(archive_path) as archive:
            archive.extractall(destination_dir)
        return
    if archive_name.endswith(".tar.gz"):
        with tarfile.open(archive_path, "r:gz") as archive:
            archive.extractall(destination_dir)
        return
    raise RuntimeError("unsupported resvg archive format: %s" % archive_path.name)


def install_resvg(root_dir: Path | None = None, force: bool = False) -> Path:
    root_dir = root_dir or project_root()
    install_path = local_resvg_path(root_dir)
    if install_path.exists() and not force:
        return install_path

    install_path.parent.mkdir(parents=True, exist_ok=True)
    _, asset_name = release_asset_info()

    with tempfile.TemporaryDirectory(prefix="resvg_", dir=str(root_dir / "tools")) as temp_dir_name:
        temp_dir = Path(temp_dir_name)
        archive_path = temp_dir / asset_name
        extract_dir = temp_dir / "extract"
        extract_dir.mkdir(parents=True, exist_ok=True)

        download_errors: list[str] = []
        for candidate_url in release_asset_urls(asset_name):
            try:
                _download_file(candidate_url, archive_path)
                break
            except Exception as exc:
                archive_path.unlink(missing_ok=True)
                download_errors.append("%s (%s)" % (candidate_url, exc))
        else:
            raise RuntimeError("failed to download resvg: %s" % "; ".join(download_errors))

        _extract_archive(archive_path, extract_dir)

        extracted_binary = next(extract_dir.rglob(_EXECUTABLE_NAME), None)
        if extracted_binary is None:
            extracted_binary = next(extract_dir.rglob("resvg"), None)
        if extracted_binary is None or not extracted_binary.exists():
            raise RuntimeError("downloaded resvg archive did not contain %s" % _EXECUTABLE_NAME)

        if install_path.exists():
            install_path.unlink()
        shutil.copy2(extracted_binary, install_path)

    if os.name != "nt":
        install_path.chmod(0o755)
    return install_path


def render_svg_to_png(
    svg_path: Path,
    output_png_path: Path,
    width_px: int,
    height_px: int,
    root_dir: Path | None = None,
    env: dict[str, str] | None = None,
) -> None:
    if width_px <= 0 or height_px <= 0:
        raise RuntimeError("resvg target size must be positive")

    binary = resolve_resvg_binary(root_dir, env)
    command = [
        str(binary),
        "-w",
        str(width_px),
        "-h",
        str(height_px),
        str(svg_path),
        str(output_png_path),
    ]
    result = subprocess.run(command, capture_output=True, text=True)
    if result.returncode != 0 or not output_png_path.exists():
        detail = (result.stderr or result.stdout or "").strip()
        if not detail:
            detail = "exit code %d" % result.returncode
        raise RuntimeError("resvg failed: %s" % detail)

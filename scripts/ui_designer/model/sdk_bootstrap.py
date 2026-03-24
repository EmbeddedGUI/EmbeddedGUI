"""SDK bootstrap helpers for packaged UI Designer flows."""

from __future__ import annotations

import os
from pathlib import Path
import shutil
import ssl
import subprocess
import tempfile
import time
import urllib.request
import zipfile
from dataclasses import dataclass

from .config import _get_config_dir
from .workspace import is_valid_sdk_root, normalize_path, resolve_sdk_root_candidate


GITHUB_SDK_ARCHIVE_URL = "https://codeload.github.com/EmbeddedGUI/EmbeddedGUI/zip/refs/heads/main"
GITEE_SDK_ARCHIVE_URL = "https://gitee.com/embeddedgui/EmbeddedGUI/repository/archive/main.zip"
GITEE_SDK_GIT_URL = "https://gitee.com/embeddedgui/EmbeddedGUI.git"
DOWNLOAD_USER_AGENT = "EmbeddedGUI-Designer/1.0"
DEFAULT_DOWNLOAD_TOTAL_TIMEOUT_SECONDS = 60
DOWNLOAD_TIMEOUT_ENV = "EMBEDDEDGUI_SDK_ARCHIVE_TIMEOUT_SECONDS"


@dataclass(frozen=True)
class _DownloadResult:
    content_type: str
    final_url: str


def _get_download_timeout_seconds() -> int:
    raw_value = (os.environ.get(DOWNLOAD_TIMEOUT_ENV) or "").strip()
    try:
        timeout_seconds = int(raw_value)
        if timeout_seconds > 0:
            return timeout_seconds
    except (TypeError, ValueError):
        pass
    return DEFAULT_DOWNLOAD_TOTAL_TIMEOUT_SECONDS


def default_sdk_install_dir() -> str:
    """Return the default location for a downloaded SDK copy."""
    return normalize_path(os.path.join(_get_config_dir(), "sdk", "EmbeddedGUI"))


def _emit_progress(progress_callback, message: str, percent: int | None = None) -> None:
    if progress_callback is not None:
        progress_callback(message, percent)


def _download_file(url: str, destination: Path, progress_callback=None) -> _DownloadResult:
    ssl_context = ssl.create_default_context()
    request = urllib.request.Request(url, headers={"User-Agent": DOWNLOAD_USER_AGENT})
    with urllib.request.urlopen(request, context=ssl_context, timeout=120) as response:
        content_type = (response.headers.get("Content-Type") or "").lower()
        final_url = response.geturl()
        total = int(response.headers.get("Content-Length", 0))
        downloaded = 0
        chunk_size = 256 * 1024
        timeout_seconds = _get_download_timeout_seconds()
        started_at = time.monotonic()

        with destination.open("wb") as output:
            while True:
                if time.monotonic() - started_at > timeout_seconds:
                    raise RuntimeError(f"download timed out after {timeout_seconds}s")
                chunk = response.read(chunk_size)
                if not chunk:
                    break
                output.write(chunk)
                downloaded += len(chunk)
                if total > 0:
                    percent = min(downloaded * 100 // total, 100)
                    _emit_progress(progress_callback, f"Downloading SDK... {percent}%", percent)

    return _DownloadResult(content_type=content_type, final_url=final_url)


def _looks_like_html_file(path: Path) -> bool:
    try:
        prefix = path.read_bytes()[:256].lstrip().lower()
    except OSError:
        return False
    return prefix.startswith(b"<!doctype html") or prefix.startswith(b"<html")


def _find_sdk_root_in_directory(root_dir: Path) -> str:
    resolved = resolve_sdk_root_candidate(str(root_dir))
    if resolved:
        return resolved

    for current_root, dirnames, _ in os.walk(root_dir):
        current_root = normalize_path(current_root)
        if is_valid_sdk_root(current_root):
            return current_root

        normalized_children = [Path(current_root) / dirname for dirname in dirnames]
        if not any("embeddedgui" in child.name.lower() or child.name.lower() == "sdk" for child in normalized_children):
            dirnames[:] = []
    return ""


def _replace_directory(source_dir: Path, destination_dir: Path) -> str:
    destination_dir.parent.mkdir(parents=True, exist_ok=True)
    if destination_dir.exists():
        shutil.rmtree(destination_dir, ignore_errors=True)
    shutil.move(str(source_dir), str(destination_dir))
    return normalize_path(str(destination_dir))


def download_and_extract_sdk(url: str, destination_dir: str, progress_callback=None) -> str:
    """Download a zip archive and extract the SDK into *destination_dir*."""
    destination_path = Path(normalize_path(destination_dir))
    with tempfile.TemporaryDirectory(prefix="embeddedgui_sdk_") as temp_dir_name:
        temp_dir = Path(temp_dir_name)
        archive_path = temp_dir / "sdk.zip"
        download_result = _download_file(url, archive_path, progress_callback=progress_callback)
        if not zipfile.is_zipfile(archive_path):
            if "text/html" in download_result.content_type or _looks_like_html_file(archive_path):
                raise RuntimeError(
                    "server returned HTML instead of a zip archive; "
                    "this source may require browser access or block scripted downloads"
                )
            raise RuntimeError(f"downloaded file was not a valid zip archive: {download_result.final_url}")
        _emit_progress(progress_callback, "Extracting SDK archive...", 95)
        with zipfile.ZipFile(archive_path) as archive:
            archive.extractall(temp_dir)

        extracted_sdk_root = _find_sdk_root_in_directory(temp_dir)
        if not extracted_sdk_root:
            raise RuntimeError("downloaded archive did not contain a valid EmbeddedGUI SDK")

        return _replace_directory(Path(extracted_sdk_root), destination_path)


def clone_sdk_from_git(url: str, destination_dir: str, progress_callback=None) -> str:
    """Clone the SDK repository into *destination_dir* using git."""
    git_exe = shutil.which("git")
    if not git_exe:
        raise RuntimeError("git is not available for clone fallback")

    destination_path = Path(normalize_path(destination_dir))
    with tempfile.TemporaryDirectory(prefix="embeddedgui_sdk_clone_") as temp_dir_name:
        temp_dir = Path(temp_dir_name)
        clone_dir = temp_dir / "EmbeddedGUI"
        _emit_progress(progress_callback, "Cloning SDK from Gitee...", 95)
        result = subprocess.run(
            [git_exe, "clone", "--depth", "1", url, str(clone_dir)],
            text=True,
            capture_output=True,
            timeout=300,
        )
        if result.returncode != 0:
            raise RuntimeError(result.stderr.strip() or result.stdout.strip() or f"git clone failed with exit code {result.returncode}")

        resolved = resolve_sdk_root_candidate(str(clone_dir))
        if not resolved:
            raise RuntimeError("cloned repository did not contain a valid EmbeddedGUI SDK")

        return _replace_directory(Path(resolved), destination_path)


def ensure_sdk_downloaded(destination_dir: str | None = None, progress_callback=None) -> str:
    """Ensure a local SDK copy exists, downloading it when necessary."""
    target_dir = normalize_path(destination_dir) or default_sdk_install_dir()
    resolved_existing = resolve_sdk_root_candidate(target_dir)
    if resolved_existing and is_valid_sdk_root(resolved_existing):
        _emit_progress(progress_callback, "Using existing SDK copy.", 100)
        return resolved_existing

    attempts = [
        ("GitHub archive", lambda: download_and_extract_sdk(GITHUB_SDK_ARCHIVE_URL, target_dir, progress_callback=progress_callback)),
        ("Gitee archive", lambda: download_and_extract_sdk(GITEE_SDK_ARCHIVE_URL, target_dir, progress_callback=progress_callback)),
        ("Gitee git clone", lambda: clone_sdk_from_git(GITEE_SDK_GIT_URL, target_dir, progress_callback=progress_callback)),
    ]

    errors: list[str] = []
    for label, action in attempts:
        try:
            _emit_progress(progress_callback, f"Trying {label}...", 5)
            sdk_root = action()
            resolved = resolve_sdk_root_candidate(sdk_root)
            if resolved and is_valid_sdk_root(resolved):
                _emit_progress(progress_callback, "EmbeddedGUI SDK is ready.", 100)
                return resolved
            errors.append(f"{label}: invalid SDK content")
        except Exception as exc:
            errors.append(f"{label}: {exc}")

    raise RuntimeError("Automatic SDK setup failed:\n- " + "\n- ".join(errors))

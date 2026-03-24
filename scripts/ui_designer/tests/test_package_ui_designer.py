"""Tests for the local UI Designer packaging helper."""

from __future__ import annotations

import importlib.util
from pathlib import Path

import pytest


def _load_module():
    module_path = Path(__file__).resolve().parents[2] / "package_ui_designer.py"
    spec = importlib.util.spec_from_file_location("package_ui_designer", str(module_path))
    module = importlib.util.module_from_spec(spec)
    assert spec is not None and spec.loader is not None
    spec.loader.exec_module(module)
    return module


def test_compute_platform_tag_variants():
    module = _load_module()

    assert module.compute_platform_tag("win32", "AMD64") == "windows-x64"
    assert module.compute_platform_tag("linux", "aarch64") == "linux-arm64"
    assert module.compute_platform_tag("darwin", "arm64") == "macos-arm64"


def test_build_archive_base_name_with_suffix():
    module = _load_module()

    assert module.build_archive_base_name("windows-x64", package_suffix="v1.2.3") == "EmbeddedGUI-Designer-windows-x64-v1.2.3"


def test_build_archive_base_name_rejects_invalid_suffix():
    module = _load_module()

    with pytest.raises(ValueError, match="invalid path characters"):
        module.build_archive_base_name("windows-x64", package_suffix="bad/name")


def test_build_pyinstaller_command_includes_expected_paths(tmp_path):
    module = _load_module()

    cmd = module.build_pyinstaller_command(tmp_path / "dist", tmp_path / "build", clean=True)

    assert cmd[:3] == [module.sys.executable, "-m", "PyInstaller"]
    assert "--distpath" in cmd
    assert "--workpath" in cmd
    assert "--clean" in cmd
    assert "-y" in cmd


def test_package_ui_designer_creates_archive_without_running_pyinstaller(tmp_path, monkeypatch):
    module = _load_module()

    dist_dir = tmp_path / "dist"
    app_dir = dist_dir / module.DIST_APP_NAME

    monkeypatch.setattr(module, "ensure_pyinstaller_available", lambda: None)

    def fake_run_pyinstaller(resolved_dist_dir, resolved_work_dir, clean=True):
        app_dir.mkdir(parents=True, exist_ok=True)
        (app_dir / "designer.txt").write_text("ok", encoding="utf-8")

    monkeypatch.setattr(module, "run_pyinstaller", fake_run_pyinstaller)

    result = module.package_ui_designer(
        output_dir=dist_dir,
        work_dir=tmp_path / "build",
        archive_mode="zip",
        package_suffix="dev",
        clean=False,
    )

    assert result["app_dir"] == str(app_dir.resolve())
    archive_path = Path(result["archive_path"])
    assert archive_path.is_file()
    expected_name = f"EmbeddedGUI-Designer-{module.compute_platform_tag()}-dev.zip"
    assert archive_path.name == expected_name


def test_package_ui_designer_can_skip_archive(tmp_path, monkeypatch):
    module = _load_module()

    dist_dir = tmp_path / "dist"
    app_dir = dist_dir / module.DIST_APP_NAME

    monkeypatch.setattr(module, "ensure_pyinstaller_available", lambda: None)
    monkeypatch.setattr(
        module,
        "run_pyinstaller",
        lambda resolved_dist_dir, resolved_work_dir, clean=True: (app_dir.mkdir(parents=True, exist_ok=True), (app_dir / "designer.txt").write_text("ok", encoding="utf-8")),
    )

    result = module.package_ui_designer(
        output_dir=dist_dir,
        work_dir=tmp_path / "build",
        archive_mode="none",
    )

    assert result["archive_path"] == ""

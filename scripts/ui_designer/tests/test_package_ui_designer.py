"""Tests for the local UI Designer packaging helper."""

from __future__ import annotations

import importlib.util
import sys
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


def test_build_preflight_command_uses_preview_smoke_script():
    module = _load_module()

    cmd = module.build_preflight_command()

    assert cmd == [module.sys.executable, str(module.PREFLIGHT_SMOKE_SCRIPT_PATH.resolve())]


def test_iter_filtered_build_output_strips_qfluentwidgets_promotion():
    module = _load_module()

    lines = [
        "\n",
        "Tips: QFluentWidgets Pro is now released. Click https://qfluentwidgets.com/pages/pro to learn more about it.\n",
        "\n",
        "123 INFO: Building EXE\n",
    ]

    assert list(module.iter_filtered_build_output(lines)) == ["123 INFO: Building EXE\n"]


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
        bundle_sdk=False,
        run_preflight=False,
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
        bundle_sdk=False,
        run_preflight=False,
    )

    assert result["archive_path"] == ""


def test_package_ui_designer_bundles_sdk_by_default(tmp_path, monkeypatch):
    module = _load_module()

    dist_dir = tmp_path / "dist"
    app_dir = dist_dir / module.DIST_APP_NAME
    bundled_dir = app_dir / "sdk" / module.SDK_BUNDLE_DIR_NAME
    calls = []

    monkeypatch.setattr(module, "ensure_pyinstaller_available", lambda: None)

    def fake_run_pyinstaller(resolved_dist_dir, resolved_work_dir, clean=True):
        app_dir.mkdir(parents=True, exist_ok=True)
        (app_dir / "designer.txt").write_text("ok", encoding="utf-8")

    monkeypatch.setattr(module, "run_pyinstaller", fake_run_pyinstaller)
    monkeypatch.setattr(
        module,
        "copy_sdk_bundle",
        lambda resolved_app_dir, sdk_root=None: calls.append((resolved_app_dir, sdk_root)) or bundled_dir,
    )

    result = module.package_ui_designer(
        output_dir=dist_dir,
        work_dir=tmp_path / "build",
        archive_mode="none",
        run_preflight=False,
    )

    assert calls == [(app_dir, None)]
    assert result["bundled_sdk_dir"] == str(bundled_dir)


def test_package_ui_designer_runs_preflight_by_default(tmp_path, monkeypatch):
    module = _load_module()

    dist_dir = tmp_path / "dist"
    app_dir = dist_dir / module.DIST_APP_NAME
    calls = []

    monkeypatch.setattr(module, "ensure_pyinstaller_available", lambda: calls.append("ensure"))
    monkeypatch.setattr(module, "run_preflight_check", lambda: calls.append("preflight"))

    def fake_run_pyinstaller(resolved_dist_dir, resolved_work_dir, clean=True):
        calls.append("pyinstaller")
        app_dir.mkdir(parents=True, exist_ok=True)
        (app_dir / "designer.txt").write_text("ok", encoding="utf-8")

    monkeypatch.setattr(module, "run_pyinstaller", fake_run_pyinstaller)

    module.package_ui_designer(
        output_dir=dist_dir,
        work_dir=tmp_path / "build",
        archive_mode="none",
        bundle_sdk=False,
    )

    assert calls == ["ensure", "preflight", "pyinstaller"]


def test_package_ui_designer_can_skip_preflight(tmp_path, monkeypatch):
    module = _load_module()

    dist_dir = tmp_path / "dist"
    app_dir = dist_dir / module.DIST_APP_NAME
    calls = []

    monkeypatch.setattr(module, "ensure_pyinstaller_available", lambda: None)
    monkeypatch.setattr(module, "run_preflight_check", lambda: calls.append("preflight"))

    def fake_run_pyinstaller(resolved_dist_dir, resolved_work_dir, clean=True):
        app_dir.mkdir(parents=True, exist_ok=True)
        (app_dir / "designer.txt").write_text("ok", encoding="utf-8")

    monkeypatch.setattr(module, "run_pyinstaller", fake_run_pyinstaller)

    module.package_ui_designer(
        output_dir=dist_dir,
        work_dir=tmp_path / "build",
        archive_mode="none",
        bundle_sdk=False,
        run_preflight=False,
    )

    assert calls == []


def test_copy_sdk_bundle_copies_sdk_tree_into_app_dir(tmp_path):
    module = _load_module()

    sdk_root = tmp_path / "sdk_root"
    (sdk_root / "src").mkdir(parents=True)
    (sdk_root / "porting" / "designer").mkdir(parents=True)
    (sdk_root / "Makefile").write_text("all:\n", encoding="utf-8")
    (sdk_root / ".git").mkdir()
    (sdk_root / ".git" / "config").write_text("ignored", encoding="utf-8")
    (sdk_root / "README.md").write_text("sdk", encoding="utf-8")

    app_dir = tmp_path / "dist" / module.DIST_APP_NAME
    app_dir.mkdir(parents=True)

    bundled_dir = module.copy_sdk_bundle(app_dir, sdk_root=sdk_root)

    assert bundled_dir == app_dir / "sdk" / module.SDK_BUNDLE_DIR_NAME
    assert (bundled_dir / "Makefile").is_file()
    assert (bundled_dir / "src").is_dir()
    assert (bundled_dir / "porting" / "designer").is_dir()
    assert (bundled_dir / "README.md").is_file()
    assert not (bundled_dir / ".git").exists()
    metadata_path = bundled_dir / module.SDK_BUNDLE_METADATA_NAME
    assert metadata_path.is_file()
    metadata = module.load_sdk_bundle_metadata(bundled_dir)
    assert metadata["source_root"] == str(sdk_root.resolve())
    assert metadata["file_count"] >= 2
    assert metadata["total_size_bytes"] > 0


def test_copy_sdk_bundle_ignores_designer_test_tree(tmp_path):
    module = _load_module()

    sdk_root = tmp_path / "sdk_root"
    (sdk_root / "src").mkdir(parents=True)
    (sdk_root / "porting" / "designer").mkdir(parents=True)
    (sdk_root / "Makefile").write_text("all:\n", encoding="utf-8")
    (sdk_root / "scripts" / "ui_designer" / "tests").mkdir(parents=True)
    (sdk_root / "scripts" / "ui_designer" / "tests" / "test_dummy.py").write_text("x = 1\n", encoding="utf-8")
    (sdk_root / "scripts" / "ui_designer" / "main.py").write_text("print('ok')\n", encoding="utf-8")

    app_dir = tmp_path / "dist" / module.DIST_APP_NAME
    app_dir.mkdir(parents=True)

    bundled_dir = module.copy_sdk_bundle(app_dir, sdk_root=sdk_root)

    assert (bundled_dir / "scripts" / "ui_designer" / "main.py").is_file()
    assert not (bundled_dir / "scripts" / "ui_designer" / "tests").exists()


def test_package_ui_designer_can_bundle_sdk(tmp_path, monkeypatch):
    module = _load_module()

    dist_dir = tmp_path / "dist"
    app_dir = dist_dir / module.DIST_APP_NAME
    sdk_root = tmp_path / "sdk_root"
    (sdk_root / "src").mkdir(parents=True)
    (sdk_root / "porting" / "designer").mkdir(parents=True)
    (sdk_root / "Makefile").write_text("all:\n", encoding="utf-8")

    monkeypatch.setattr(module, "ensure_pyinstaller_available", lambda: None)

    def fake_run_pyinstaller(resolved_dist_dir, resolved_work_dir, clean=True):
        app_dir.mkdir(parents=True, exist_ok=True)
        (app_dir / "designer.txt").write_text("ok", encoding="utf-8")

    monkeypatch.setattr(module, "run_pyinstaller", fake_run_pyinstaller)

    result = module.package_ui_designer(
        output_dir=dist_dir,
        work_dir=tmp_path / "build",
        archive_mode="none",
        bundle_sdk=True,
        sdk_root=sdk_root,
        run_preflight=False,
    )

    bundled_dir = app_dir / "sdk" / module.SDK_BUNDLE_DIR_NAME
    assert result["bundled_sdk_dir"] == str(bundled_dir)
    assert (bundled_dir / "Makefile").is_file()
    assert result["bundled_sdk_source"] == str(sdk_root.resolve())
    assert result["bundled_sdk_file_count"] >= 1
    assert result["bundled_sdk_total_size_bytes"] > 0
    assert Path(result["bundled_sdk_metadata_path"]).is_file()


def test_resolve_sdk_bundle_root_rejects_invalid_directory(tmp_path):
    module = _load_module()

    invalid_root = tmp_path / "invalid_sdk"
    invalid_root.mkdir()

    with pytest.raises(ValueError, match="invalid EmbeddedGUI SDK root"):
        module.resolve_sdk_bundle_root(invalid_root)


def test_parse_args_bundles_sdk_by_default(monkeypatch):
    module = _load_module()

    monkeypatch.setattr(sys, "argv", ["package_ui_designer.py"])

    args = module.parse_args()

    assert args.bundle_sdk is True


def test_parse_args_can_disable_sdk_bundle(monkeypatch):
    module = _load_module()

    monkeypatch.setattr(sys, "argv", ["package_ui_designer.py", "--no-bundle-sdk"])

    args = module.parse_args()

    assert args.bundle_sdk is False


def test_parse_args_can_skip_preflight(monkeypatch):
    module = _load_module()

    monkeypatch.setattr(sys, "argv", ["package_ui_designer.py", "--skip-preflight"])

    args = module.parse_args()

    assert args.skip_preflight is True


def test_format_byte_count_uses_compact_units():
    module = _load_module()

    assert module.format_byte_count(512) == "512 B"
    assert module.format_byte_count(2048) == "2.0 KB"

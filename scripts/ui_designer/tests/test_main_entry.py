"""Tests for UI Designer main entry flow."""

from __future__ import annotations

import argparse
import os

import pytest


class _FakeConfig:
    def __init__(self):
        self.sdk_root = ""
        self.egui_root = ""
        self.last_app = "HelloDesigner"
        self.last_project_path = ""
        self.theme = "dark"
        self.font_size_px = 0
        self.save_calls = 0

    def save(self):
        self.save_calls += 1


class _FakeApp:
    last_instance = None

    def __init__(self, argv):
        self.argv = argv
        self.application_name = ""
        self._style_sheet = "base-style"
        self.exec_calls = 0
        type(self).last_instance = self

    def setApplicationName(self, name):
        self.application_name = name

    def styleSheet(self):
        return self._style_sheet

    def setStyleSheet(self, style):
        self._style_sheet = style

    def exec_(self):
        self.exec_calls += 1
        return 0


class _FakeWindow:
    last_instance = None

    def __init__(self, sdk_root, app_name="HelloDesigner"):
        self.sdk_root = sdk_root
        self.app_name = app_name
        self.open_calls = []
        self.show_called = False
        self.raise_on_open = None
        self.prompt_calls = 0
        type(self).last_instance = self

    def _open_project_path(self, path, preferred_sdk_root="", silent=False):
        self.open_calls.append(
            {
                "path": path,
                "preferred_sdk_root": preferred_sdk_root,
                "silent": silent,
            }
        )
        if self.raise_on_open is not None:
            raise self.raise_on_open

    def show(self):
        self.show_called = True

    def maybe_prompt_initial_sdk_setup(self):
        self.prompt_calls += 1


@pytest.fixture
def main_module():
    import ui_designer.main as designer_main

    return designer_main


def _patch_main_dependencies(monkeypatch, config, sdk_root, main_module, open_error=None):
    import PyQt5.QtWidgets as qtwidgets
    import PyQt5.QtCore as qtcore
    import ui_designer.model.config as config_module
    import ui_designer.model.widget_registry as registry_module
    import ui_designer.model.workspace as workspace_module
    import ui_designer.ui.main_window as main_window_module
    import ui_designer.ui.theme as theme_module

    theme_calls = []
    registry_calls = []
    exit_codes = []
    window_state = {"instance": None}

    class WindowFactory(_FakeWindow):
        def __init__(self, sdk_root_arg, app_name="HelloDesigner"):
            super().__init__(sdk_root_arg, app_name=app_name)
            self.raise_on_open = open_error
            window_state["instance"] = self

    monkeypatch.setattr(config_module, "get_config", lambda: config)
    monkeypatch.setattr(
        workspace_module,
        "find_sdk_root",
        lambda **kwargs: sdk_root,
    )
    monkeypatch.setattr(
        registry_module.WidgetRegistry,
        "instance",
        classmethod(lambda cls: registry_calls.append("instance") or object()),
    )
    monkeypatch.setattr(theme_module, "apply_theme", lambda app, theme: theme_calls.append(theme))
    monkeypatch.setattr(main_window_module, "MainWindow", WindowFactory)
    monkeypatch.setattr(qtcore.QTimer, "singleShot", staticmethod(lambda _msec, callback: callback()))
    monkeypatch.setattr(qtwidgets, "QApplication", _FakeApp)
    monkeypatch.setattr(main_module.sys, "exit", lambda code=0: exit_codes.append(code))

    return theme_calls, registry_calls, exit_codes, window_state


def test_main_opens_cli_project_with_resolved_sdk_root(monkeypatch, tmp_path, main_module):
    project_path = tmp_path / "DemoApp.egui"
    project_path.write_text("demo", encoding="utf-8")
    sdk_root = os.path.normpath(os.path.abspath(tmp_path / "sdk"))
    config = _FakeConfig()

    monkeypatch.setattr(
        main_module,
        "_parse_args",
        lambda: argparse.Namespace(project=str(project_path), app="DemoApp", sdk_root=str(tmp_path / "sdk")),
    )
    theme_calls, registry_calls, exit_codes, window_state = _patch_main_dependencies(monkeypatch, config, sdk_root, main_module)

    main_module.main()

    window = window_state["instance"]
    app = _FakeApp.last_instance
    assert config.sdk_root == sdk_root
    assert config.egui_root == sdk_root
    assert config.save_calls == 1
    assert theme_calls == ["dark"]
    assert registry_calls == ["instance"]
    assert window.sdk_root == sdk_root
    assert window.app_name == "DemoApp"
    assert window.open_calls == [
        {
            "path": os.path.normpath(os.path.abspath(project_path)),
            "preferred_sdk_root": sdk_root,
            "silent": False,
        }
    ]
    assert window.show_called is True
    assert app.application_name == "EmbeddedGUI Designer"
    assert "font-size: 9pt" in app.styleSheet()
    assert window.prompt_calls == 0
    assert exit_codes == [0]


def test_main_reopens_recent_project_directory_silently(monkeypatch, tmp_path, main_module):
    project_dir = tmp_path / "RecentProject"
    project_dir.mkdir()
    sdk_root = os.path.normpath(os.path.abspath(tmp_path / "sdk"))
    config = _FakeConfig()
    config.sdk_root = sdk_root
    config.egui_root = sdk_root
    config.last_app = "RecentApp"
    config.last_project_path = str(project_dir)

    monkeypatch.setattr(
        main_module,
        "_parse_args",
        lambda: argparse.Namespace(project=None, app=None, sdk_root=None),
    )
    _, _, _, window_state = _patch_main_dependencies(monkeypatch, config, sdk_root, main_module)

    main_module.main()

    window = window_state["instance"]
    assert window.app_name == "RecentApp"
    assert window.open_calls == [
        {
            "path": os.path.normpath(os.path.abspath(project_dir)),
            "preferred_sdk_root": sdk_root,
            "silent": True,
        }
    ]
    assert window.prompt_calls == 0
    assert window.show_called is True


def test_main_prints_warning_when_project_open_fails(monkeypatch, tmp_path, capsys, main_module):
    project_path = tmp_path / "Broken.egui"
    project_path.write_text("broken", encoding="utf-8")
    sdk_root = os.path.normpath(os.path.abspath(tmp_path / "sdk"))
    config = _FakeConfig()

    monkeypatch.setattr(
        main_module,
        "_parse_args",
        lambda: argparse.Namespace(project=str(project_path), app=None, sdk_root=None),
    )
    _, _, _, window_state = _patch_main_dependencies(monkeypatch, config, sdk_root, main_module, open_error=RuntimeError("boom"))

    main_module.main()

    window = window_state["instance"]
    out = capsys.readouterr().out
    assert "Warning: Failed to load project: boom" in out
    assert window.show_called is True
    assert window.prompt_calls == 0


def test_main_starts_without_sdk_root_and_keeps_window_usable(monkeypatch, main_module):
    config = _FakeConfig()

    monkeypatch.setattr(
        main_module,
        "_parse_args",
        lambda: argparse.Namespace(project=None, app=None, sdk_root=None),
    )
    theme_calls, registry_calls, exit_codes, window_state = _patch_main_dependencies(monkeypatch, config, "", main_module)

    main_module.main()

    window = window_state["instance"]
    app = _FakeApp.last_instance
    assert config.sdk_root == ""
    assert config.egui_root == ""
    assert config.save_calls == 0
    assert theme_calls == ["dark"]
    assert registry_calls == ["instance"]
    assert window.sdk_root == ""
    assert window.open_calls == []
    assert window.show_called is True
    assert app.application_name == "EmbeddedGUI Designer"
    assert window.prompt_calls == 1
    assert exit_codes == [0]

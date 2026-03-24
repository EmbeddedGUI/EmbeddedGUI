"""Qt UI tests for workspace-related dialogs and welcome page."""

import os

import pytest

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

try:
    from PyQt5.QtCore import Qt
    from PyQt5.QtWidgets import QApplication
    _has_pyqt5 = True
except ImportError:
    _has_pyqt5 = False

_skip_no_qt = pytest.mark.skipif(not _has_pyqt5, reason="PyQt5 not available")


@pytest.fixture
def qapp():
    app = QApplication.instance()
    if app is None:
        app = QApplication([])
    yield app
    app.processEvents()


@pytest.fixture
def isolated_config(tmp_path, monkeypatch):
    from ui_designer.model.config import DesignerConfig

    config_dir = tmp_path / "config"
    config_dir.mkdir()
    config_path = config_dir / "config.json"
    monkeypatch.setattr("ui_designer.model.config._get_config_dir", lambda: str(config_dir))
    monkeypatch.setattr("ui_designer.model.config._get_config_path", lambda: str(config_path))
    DesignerConfig._instance = None
    config = DesignerConfig.instance()
    yield config
    DesignerConfig._instance = None


def _create_sdk_root(root):
    (root / "src").mkdir(parents=True)
    (root / "porting" / "designer").mkdir(parents=True)
    (root / "Makefile").write_text("all:\n")


@_skip_no_qt
class TestAppSelectorDialog:
    def test_filters_legacy_examples_by_default(self, qapp, isolated_config, tmp_path):
        from ui_designer.ui.app_selector import AppSelectorDialog

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        example_dir = sdk_root / "example"
        example_dir.mkdir()

        modern = example_dir / "ModernApp"
        modern.mkdir()
        (modern / "build.mk").write_text("")
        (modern / "ModernApp.egui").write_text("")

        legacy = example_dir / "LegacyApp"
        legacy.mkdir()
        (legacy / "build.mk").write_text("")

        isolated_config.sdk_root = str(sdk_root)
        isolated_config.last_app = "ModernApp"
        isolated_config.show_all_examples = False

        dialog = AppSelectorDialog(egui_root=str(sdk_root))
        assert dialog._app_list.count() == 1
        assert dialog._app_list.item(0).text() == "ModernApp"
        assert dialog._app_list.currentItem().text() == "ModernApp"
        dialog.deleteLater()

    def test_toggle_legacy_updates_list_and_config(self, qapp, isolated_config, tmp_path):
        from ui_designer.ui.app_selector import AppSelectorDialog

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        example_dir = sdk_root / "example"
        example_dir.mkdir()

        modern = example_dir / "ModernApp"
        modern.mkdir()
        (modern / "build.mk").write_text("")
        (modern / "ModernApp.egui").write_text("")

        legacy = example_dir / "LegacyApp"
        legacy.mkdir()
        (legacy / "build.mk").write_text("")

        isolated_config.sdk_root = str(sdk_root)
        dialog = AppSelectorDialog(egui_root=str(sdk_root))
        dialog._show_legacy.setChecked(True)

        assert isolated_config.show_all_examples is True
        assert dialog._app_list.count() == 2
        texts = [dialog._app_list.item(i).text() for i in range(dialog._app_list.count())]
        assert "LegacyApp [Legacy]" in texts
        dialog.deleteLater()

    def test_selection_updates_selected_entry_and_enables_open(self, qapp, isolated_config, tmp_path):
        from ui_designer.ui.app_selector import AppSelectorDialog

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        example_dir = sdk_root / "example"
        example_dir.mkdir()

        modern = example_dir / "ModernApp"
        modern.mkdir()
        (modern / "build.mk").write_text("")
        (modern / "ModernApp.egui").write_text("")

        isolated_config.sdk_root = str(sdk_root)
        dialog = AppSelectorDialog(egui_root=str(sdk_root))
        dialog._app_list.setCurrentRow(0)

        assert dialog.selected_entry["app_name"] == "ModernApp"
        assert dialog._open_btn.isEnabled() is True
        dialog.deleteLater()


@_skip_no_qt
class TestNewProjectDialog:
    def test_accept_requires_valid_sdk_root(self, qapp, isolated_config):
        from ui_designer.ui.new_project_dialog import NewProjectDialog

        warnings = []
        dialog = NewProjectDialog(sdk_root="", default_parent_dir="")
        dialog._app_name_edit.setText("DemoApp")

        with pytest.MonkeyPatch.context() as mp:
            mp.setattr("ui_designer.ui.new_project_dialog.QMessageBox.warning", lambda *args: warnings.append(args[1:]))
            dialog._accept_if_valid()

        assert dialog.result() == 0
        assert warnings
        assert warnings[0][0] == "Invalid SDK Root"
        dialog.deleteLater()

    def test_accept_requires_valid_app_name(self, qapp, isolated_config, tmp_path):
        from ui_designer.ui.new_project_dialog import NewProjectDialog

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        warnings = []
        dialog = NewProjectDialog(sdk_root=str(sdk_root), default_parent_dir=str(tmp_path))
        dialog._app_name_edit.setText("bad name!")

        with pytest.MonkeyPatch.context() as mp:
            mp.setattr("ui_designer.ui.new_project_dialog.QMessageBox.warning", lambda *args: warnings.append(args[1:]))
            dialog._accept_if_valid()

        assert dialog.result() == 0
        assert warnings[0][0] == "App Name"
        dialog.deleteLater()

    def test_accept_succeeds_with_valid_values(self, qapp, isolated_config, tmp_path):
        from ui_designer.ui.new_project_dialog import NewProjectDialog

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)

        dialog = NewProjectDialog(sdk_root=str(sdk_root), default_parent_dir=str(tmp_path))
        dialog._app_name_edit.setText("DemoApp")
        dialog._width_spin.setValue(320)
        dialog._height_spin.setValue(240)
        dialog._accept_if_valid()

        assert dialog.result() == dialog.Accepted
        assert dialog.app_name == "DemoApp"
        assert dialog.screen_width == 320
        assert dialog.screen_height == 240
        dialog.deleteLater()


@_skip_no_qt
class TestWelcomePage:
    def test_refresh_shows_no_recent_state(self, qapp, isolated_config):
        from ui_designer.ui.welcome_page import WelcomePage

        isolated_config.recent_projects = []
        page = WelcomePage()

        assert page._recent_list.count() == 1
        widget = page._recent_list.itemAt(0).widget()
        assert widget is not None
        assert "No recent projects" in widget.text()
        page.deleteLater()

    def test_recent_click_emits_project_path_and_sdk(self, qapp, isolated_config, tmp_path):
        from ui_designer.ui.welcome_page import WelcomePage

        project_path = str(tmp_path / "DemoApp" / "DemoApp.egui")
        sdk_root = str(tmp_path / "sdk")
        isolated_config.recent_projects = []

        page = WelcomePage()
        emitted = []
        page.open_recent.connect(lambda project, sdk: emitted.append((project, sdk)))
        page._on_recent_clicked(project_path, sdk_root)

        assert emitted == [(project_path, sdk_root)]
        page.deleteLater()

    def test_refresh_shows_sdk_status_and_path(self, qapp, isolated_config, tmp_path):
        from ui_designer.ui.welcome_page import WelcomePage

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        isolated_config.sdk_root = str(sdk_root)

        page = WelcomePage()

        assert "Ready" in page._sdk_status_label.text()
        assert str(sdk_root) in page._sdk_path_label.text()
        page.deleteLater()

    def test_quick_action_buttons_emit_signals(self, qapp, isolated_config):
        from ui_designer.ui.welcome_page import WelcomePage

        page = WelcomePage()
        events = []
        page.new_project.connect(lambda: events.append("new"))
        page.open_project.connect(lambda: events.append("open_project"))
        page.open_app.connect(lambda: events.append("open_app"))
        page.set_sdk_root.connect(lambda: events.append("set_sdk"))

        page._new_project_btn.click()
        page._open_project_btn.click()
        page._open_app_btn.click()
        page._set_sdk_root_btn.click()

        assert events == ["new", "open_project", "open_app", "set_sdk"]
        page.deleteLater()

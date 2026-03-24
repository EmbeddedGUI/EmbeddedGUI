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


@pytest.fixture(autouse=True)
def bind_ui_config(isolated_config, monkeypatch):
    import ui_designer.ui.app_selector as app_selector_module
    import ui_designer.ui.new_project_dialog as new_project_dialog_module
    import ui_designer.ui.welcome_page as welcome_page_module

    monkeypatch.setattr(app_selector_module, "get_config", lambda: isolated_config)
    monkeypatch.setattr(new_project_dialog_module, "get_config", lambda: isolated_config)
    monkeypatch.setattr(welcome_page_module, "get_config", lambda: isolated_config)


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

    def test_shows_placeholder_when_sdk_root_missing(self, qapp, isolated_config):
        from ui_designer.ui.app_selector import AppSelectorDialog

        isolated_config.sdk_root = ""
        isolated_config.egui_root = ""
        dialog = AppSelectorDialog(egui_root="")

        assert dialog._app_list.count() == 1
        assert dialog._app_list.item(0).text() == "(Set or download an SDK root first)"
        assert "Missing" in dialog._root_status_label.text()
        assert "GitHub archive" in dialog._root_status_label.text()
        assert dialog._open_btn.isEnabled() is False
        dialog.deleteLater()

    def test_shows_invalid_placeholder_when_sdk_root_is_invalid(self, qapp, isolated_config, tmp_path):
        from ui_designer.ui.app_selector import AppSelectorDialog

        isolated_config.sdk_root = ""
        isolated_config.egui_root = ""
        dialog = AppSelectorDialog(egui_root=str(tmp_path / "not_sdk"))

        assert dialog._app_list.count() == 1
        assert dialog._app_list.item(0).text() == "(Current SDK root is invalid)"
        assert "Invalid" in dialog._root_status_label.text()
        assert dialog._open_btn.isEnabled() is False
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

    def test_browse_root_auto_resolves_parent_directory(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.app_selector import AppSelectorDialog

        sdk_parent = tmp_path / "tools"
        sdk_root = sdk_parent / "EmbeddedGUI-main"
        _create_sdk_root(sdk_root)
        (sdk_root / "example").mkdir()

        dialog = AppSelectorDialog(egui_root="")
        monkeypatch.setattr("ui_designer.ui.app_selector.QFileDialog.getExistingDirectory", lambda *args, **kwargs: str(sdk_parent))

        dialog._browse_root()

        assert dialog.egui_root == os.path.normpath(os.path.abspath(sdk_root))
        assert dialog._root_edit.text() == os.path.normpath(os.path.abspath(sdk_root))
        dialog.deleteLater()

    def test_search_filters_examples_by_name(self, qapp, isolated_config, tmp_path):
        from ui_designer.ui.app_selector import AppSelectorDialog

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        example_dir = sdk_root / "example"
        example_dir.mkdir()

        for name in ("HelloVirtual", "HelloShowcase", "HelloSimple"):
            app_dir = example_dir / name
            app_dir.mkdir()
            (app_dir / "build.mk").write_text("")
            (app_dir / f"{name}.egui").write_text("")

        isolated_config.sdk_root = str(sdk_root)
        dialog = AppSelectorDialog(egui_root=str(sdk_root))
        dialog._search_edit.setText("show")

        assert dialog._app_list.count() == 1
        assert dialog._app_list.item(0).text() == "HelloShowcase"
        assert dialog.selected_entry["app_name"] == "HelloShowcase"
        dialog.deleteLater()

    def test_search_shows_empty_state_when_no_example_matches(self, qapp, isolated_config, tmp_path):
        from ui_designer.ui.app_selector import AppSelectorDialog

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        example_dir = sdk_root / "example"
        example_dir.mkdir()

        app_dir = example_dir / "HelloVirtual"
        app_dir.mkdir()
        (app_dir / "build.mk").write_text("")
        (app_dir / "HelloVirtual.egui").write_text("")

        isolated_config.sdk_root = str(sdk_root)
        dialog = AppSelectorDialog(egui_root=str(sdk_root))
        dialog._search_edit.setText("missing")

        assert dialog._app_list.count() == 1
        assert dialog._app_list.item(0).text() == "(No matching examples)"
        assert dialog._open_btn.isEnabled() is False
        assert dialog.selected_entry is None
        dialog.deleteLater()

    def test_double_click_placeholder_item_does_not_accept_dialog(self, qapp, isolated_config, tmp_path):
        from ui_designer.ui.app_selector import AppSelectorDialog

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        (sdk_root / "example").mkdir()

        dialog = AppSelectorDialog(egui_root=str(sdk_root))
        placeholder_item = dialog._app_list.item(0)
        accepted = []
        dialog.accept = lambda: accepted.append(True)

        dialog._on_item_double_clicked(placeholder_item)

        assert placeholder_item.text() == "(No SDK examples found)"
        assert accepted == []
        assert dialog.selected_entry is None
        dialog.deleteLater()

    def test_shows_empty_state_when_sdk_has_no_examples(self, qapp, isolated_config, tmp_path):
        from ui_designer.ui.app_selector import AppSelectorDialog

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        (sdk_root / "example").mkdir()

        dialog = AppSelectorDialog(egui_root=str(sdk_root))

        assert dialog._app_list.count() == 1
        assert dialog._app_list.item(0).text() == "(No SDK examples found)"
        assert dialog._open_btn.isEnabled() is False
        dialog.deleteLater()

    def test_legacy_selection_updates_open_button_and_hint(self, qapp, isolated_config, tmp_path):
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

        dialog = AppSelectorDialog(egui_root=str(sdk_root))
        dialog._show_legacy.setChecked(True)

        for index in range(dialog._app_list.count()):
            if dialog._app_list.item(index).text() == "LegacyApp [Legacy]":
                dialog._app_list.setCurrentRow(index)
                break

        assert dialog.selected_entry["app_name"] == "LegacyApp"
        assert dialog._open_btn.text() == "Import Legacy Example"
        assert "initialize a Designer project" in dialog._selection_hint_label.text()
        assert str(legacy) in dialog._selection_hint_label.text()
        dialog.deleteLater()

    def test_download_sdk_callback_updates_root_and_examples(self, qapp, isolated_config, tmp_path):
        from ui_designer.ui.app_selector import AppSelectorDialog

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        example_dir = sdk_root / "example"
        example_dir.mkdir()
        app_dir = example_dir / "HelloVirtual"
        app_dir.mkdir()
        (app_dir / "build.mk").write_text("")
        (app_dir / "HelloVirtual.egui").write_text("")

        dialog = AppSelectorDialog(egui_root="", on_download_sdk=lambda: str(sdk_root))

        dialog._download_btn.click()

        assert dialog.egui_root == os.path.normpath(os.path.abspath(sdk_root))
        assert dialog._root_edit.text() == os.path.normpath(os.path.abspath(sdk_root))
        assert dialog._app_list.count() == 1
        assert dialog._app_list.item(0).text() == "HelloVirtual"
        assert "Ready" in dialog._root_status_label.text()
        dialog.deleteLater()

    def test_shows_bundled_sdk_status_when_using_runtime_sdk(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.app_selector import AppSelectorDialog

        runtime_dir = tmp_path / "EmbeddedGUI-Designer"
        sdk_root = runtime_dir / "sdk" / "EmbeddedGUI"
        _create_sdk_root(sdk_root)
        (sdk_root / "example").mkdir()

        monkeypatch.setattr("ui_designer.ui.app_selector.default_sdk_install_dir", lambda: str(sdk_root))
        monkeypatch.setattr("ui_designer.model.sdk_bootstrap.sys.frozen", True, raising=False)
        monkeypatch.setattr("ui_designer.model.sdk_bootstrap.sys.executable", str(runtime_dir / "EmbeddedGUI-Designer.exe"))

        dialog = AppSelectorDialog(egui_root=str(sdk_root))

        assert "bundled SDK" in dialog._root_status_label.text()
        dialog.deleteLater()

    def test_uses_default_sdk_cache_when_configured_root_is_missing(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.app_selector import AppSelectorDialog

        sdk_root = tmp_path / "cache" / "EmbeddedGUI"
        _create_sdk_root(sdk_root)
        example_dir = sdk_root / "example"
        example_dir.mkdir()
        app_dir = example_dir / "HelloShowcase"
        app_dir.mkdir()
        (app_dir / "build.mk").write_text("")
        (app_dir / "HelloShowcase.egui").write_text("")

        isolated_config.sdk_root = str(tmp_path / "missing_sdk")
        isolated_config.egui_root = str(tmp_path / "missing_sdk")
        monkeypatch.setattr("ui_designer.ui.app_selector.default_sdk_install_dir", lambda: str(sdk_root))

        dialog = AppSelectorDialog(egui_root="")

        assert dialog.egui_root == os.path.normpath(os.path.abspath(sdk_root))
        assert dialog._root_edit.text() == os.path.normpath(os.path.abspath(sdk_root))
        assert dialog._app_list.count() == 1
        assert dialog._app_list.item(0).text() == "HelloShowcase"
        assert "Ready" in dialog._root_status_label.text()
        dialog.deleteLater()


@_skip_no_qt
class TestNewProjectDialog:
    def test_accept_requires_parent_directory(self, qapp, isolated_config):
        from ui_designer.ui.new_project_dialog import NewProjectDialog

        warnings = []
        with pytest.MonkeyPatch.context() as mp:
            mp.setattr("ui_designer.ui.new_project_dialog.default_sdk_install_dir", lambda: "")
            dialog = NewProjectDialog(sdk_root="", default_parent_dir="")
        dialog._app_name_edit.setText("DemoApp")

        with pytest.MonkeyPatch.context() as mp:
            mp.setattr("ui_designer.ui.new_project_dialog.QMessageBox.warning", lambda *args: warnings.append(args[1:]))
            dialog._accept_if_valid()

        assert dialog.result() == 0
        assert warnings
        assert warnings[0][0] == "Parent Directory"
        dialog.deleteLater()

    def test_accept_succeeds_without_sdk_root(self, qapp, isolated_config, tmp_path):
        from ui_designer.ui.new_project_dialog import NewProjectDialog

        with pytest.MonkeyPatch.context() as mp:
            mp.setattr("ui_designer.ui.new_project_dialog.default_sdk_install_dir", lambda: "")
            dialog = NewProjectDialog(sdk_root="", default_parent_dir=str(tmp_path))
        dialog._app_name_edit.setText("DemoApp")
        dialog._accept_if_valid()

        assert dialog.result() == dialog.Accepted
        assert dialog.sdk_root == ""
        assert dialog.parent_dir == os.path.normpath(os.path.abspath(tmp_path))
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

    def test_browse_sdk_root_auto_resolves_parent_directory(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.new_project_dialog import NewProjectDialog

        sdk_parent = tmp_path / "tools"
        sdk_root = sdk_parent / "sdk" / "EmbeddedGUI-main"
        _create_sdk_root(sdk_root)

        dialog = NewProjectDialog(sdk_root="", default_parent_dir=str(tmp_path))
        monkeypatch.setattr("ui_designer.ui.new_project_dialog.QFileDialog.getExistingDirectory", lambda *args, **kwargs: str(sdk_parent))

        dialog._browse_sdk_root()

        assert dialog.sdk_root == os.path.normpath(os.path.abspath(sdk_root))
        assert dialog._sdk_edit.text() == os.path.normpath(os.path.abspath(sdk_root))
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

    def test_prefills_default_sdk_cache_when_available(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.new_project_dialog import NewProjectDialog

        sdk_root = tmp_path / "cache" / "EmbeddedGUI"
        _create_sdk_root(sdk_root)
        isolated_config.sdk_root = str(tmp_path / "missing_sdk")
        isolated_config.egui_root = str(tmp_path / "missing_sdk")
        monkeypatch.setattr("ui_designer.ui.new_project_dialog.default_sdk_install_dir", lambda: str(sdk_root))

        dialog = NewProjectDialog(sdk_root="", default_parent_dir="")

        assert dialog.sdk_root == os.path.normpath(os.path.abspath(sdk_root))
        assert dialog._sdk_edit.text() == os.path.normpath(os.path.abspath(sdk_root))
        dialog.deleteLater()

    def test_defaults_parent_dir_to_sdk_example_when_sdk_root_is_set(self, qapp, isolated_config, tmp_path):
        from ui_designer.ui.new_project_dialog import NewProjectDialog

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)

        dialog = NewProjectDialog(sdk_root=str(sdk_root), default_parent_dir="")

        assert dialog.parent_dir == os.path.join(os.path.normpath(os.path.abspath(sdk_root)), "example")
        dialog.deleteLater()

    def test_browse_sdk_root_updates_auto_managed_parent_dir(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.new_project_dialog import NewProjectDialog

        old_sdk_root = tmp_path / "sdk_old"
        new_sdk_root = tmp_path / "sdk_new"
        _create_sdk_root(old_sdk_root)
        _create_sdk_root(new_sdk_root)

        dialog = NewProjectDialog(sdk_root=str(old_sdk_root), default_parent_dir=os.path.join(str(old_sdk_root), "example"))
        monkeypatch.setattr("ui_designer.ui.new_project_dialog.QFileDialog.getExistingDirectory", lambda *args, **kwargs: str(new_sdk_root))

        dialog._browse_sdk_root()

        assert dialog.sdk_root == os.path.normpath(os.path.abspath(new_sdk_root))
        assert dialog.parent_dir == os.path.join(os.path.normpath(os.path.abspath(new_sdk_root)), "example")
        dialog.deleteLater()

    def test_browse_sdk_root_keeps_manual_parent_dir(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.new_project_dialog import NewProjectDialog

        old_sdk_root = tmp_path / "sdk_old"
        new_sdk_root = tmp_path / "sdk_new"
        custom_parent = tmp_path / "workspace"
        _create_sdk_root(old_sdk_root)
        _create_sdk_root(new_sdk_root)
        custom_parent.mkdir()

        dialog = NewProjectDialog(sdk_root=str(old_sdk_root), default_parent_dir=os.path.join(str(old_sdk_root), "example"))
        monkeypatch.setattr("ui_designer.ui.new_project_dialog.QFileDialog.getExistingDirectory", lambda *args, **kwargs: str(custom_parent))
        dialog._browse_parent_dir()

        monkeypatch.setattr("ui_designer.ui.new_project_dialog.QFileDialog.getExistingDirectory", lambda *args, **kwargs: str(new_sdk_root))
        dialog._browse_sdk_root()

        assert dialog.sdk_root == os.path.normpath(os.path.abspath(new_sdk_root))
        assert dialog.parent_dir == os.path.normpath(os.path.abspath(custom_parent))
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

    def test_recent_project_item_uses_cached_sdk_when_saved_sdk_is_invalid(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.welcome_page import WelcomePage

        cache_dir = tmp_path / "cache" / "EmbeddedGUI"
        _create_sdk_root(cache_dir)
        project_path = str(tmp_path / "DemoApp" / "DemoApp.egui")
        isolated_config.recent_projects = [
            {
                "project_path": project_path,
                "sdk_root": str(tmp_path / "missing_sdk"),
                "display_name": "DemoApp",
            }
        ]
        monkeypatch.setattr("ui_designer.ui.welcome_page.default_sdk_install_dir", lambda: str(cache_dir))

        page = WelcomePage()
        widget = page._recent_list.itemAt(0).widget()

        assert widget is not None
        assert widget.sdk_root == os.path.normpath(os.path.abspath(cache_dir))
        assert "ready" in widget._status_label.text().lower()
        page.deleteLater()

    def test_recent_project_item_marks_missing_project_path(self, qapp, isolated_config, tmp_path):
        from ui_designer.ui.welcome_page import WelcomePage

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        missing_project = tmp_path / "MissingApp" / "MissingApp.egui"
        isolated_config.recent_projects = [
            {
                "project_path": str(missing_project),
                "sdk_root": str(sdk_root),
                "display_name": "MissingApp",
            }
        ]

        page = WelcomePage()
        widget = page._recent_list.itemAt(0).widget()

        assert widget is not None
        assert "project: missing" in widget._status_label.text().lower()
        assert "sdk: ready" in widget._status_label.text().lower()
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

    def test_refresh_shows_default_download_cache_when_sdk_missing(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.welcome_page import WelcomePage

        cache_dir = tmp_path / "cache" / "EmbeddedGUI"
        isolated_config.sdk_root = ""
        isolated_config.egui_root = ""
        monkeypatch.setattr("ui_designer.ui.welcome_page.default_sdk_install_dir", lambda: str(cache_dir))

        page = WelcomePage()

        assert "Missing" in page._sdk_status_label.text()
        assert str(cache_dir) in page._sdk_hint_label.text()
        assert "GitHub archive" in page._sdk_hint_label.text()
        page.deleteLater()

    def test_refresh_uses_default_sdk_cache_when_config_is_invalid(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.welcome_page import WelcomePage

        cache_dir = tmp_path / "cache" / "EmbeddedGUI"
        _create_sdk_root(cache_dir)
        isolated_config.sdk_root = str(tmp_path / "missing_sdk")
        isolated_config.egui_root = str(tmp_path / "missing_sdk")
        monkeypatch.setattr("ui_designer.ui.welcome_page.default_sdk_install_dir", lambda: str(cache_dir))

        page = WelcomePage()

        assert "Ready" in page._sdk_status_label.text()
        assert str(cache_dir) in page._sdk_path_label.text()
        page.deleteLater()

    def test_refresh_shows_bundled_sdk_status_when_using_runtime_sdk(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.welcome_page import WelcomePage

        runtime_dir = tmp_path / "EmbeddedGUI-Designer"
        sdk_root = runtime_dir / "sdk" / "EmbeddedGUI"
        _create_sdk_root(sdk_root)
        isolated_config.sdk_root = str(sdk_root)
        isolated_config.egui_root = str(sdk_root)
        monkeypatch.setattr("ui_designer.ui.welcome_page.default_sdk_install_dir", lambda: str(sdk_root))
        monkeypatch.setattr("ui_designer.model.sdk_bootstrap.sys.frozen", True, raising=False)
        monkeypatch.setattr("ui_designer.model.sdk_bootstrap.sys.executable", str(runtime_dir / "EmbeddedGUI-Designer.exe"))

        page = WelcomePage()

        assert "bundled SDK" in page._sdk_status_label.text()
        assert "packaged beside the application" in page._sdk_hint_label.text()
        page.deleteLater()

    def test_quick_action_buttons_emit_signals(self, qapp, isolated_config):
        from ui_designer.ui.welcome_page import WelcomePage

        page = WelcomePage()
        events = []
        page.new_project.connect(lambda: events.append("new"))
        page.open_project.connect(lambda: events.append("open_project"))
        page.open_app.connect(lambda: events.append("open_app"))
        page.set_sdk_root.connect(lambda: events.append("set_sdk"))
        page.download_sdk.connect(lambda: events.append("download_sdk"))

        page._new_project_btn.click()
        page._open_project_btn.click()
        page._open_app_btn.click()
        page._set_sdk_root_btn.click()
        page._download_sdk_btn.click()

        assert events == ["new", "open_project", "open_app", "set_sdk", "download_sdk"]
        page.deleteLater()

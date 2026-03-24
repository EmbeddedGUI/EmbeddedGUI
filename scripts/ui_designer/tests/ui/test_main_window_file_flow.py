"""Qt UI tests for MainWindow project file flows."""

import os

import pytest

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

try:
    from PyQt5.QtWidgets import QApplication
    from PyQt5.QtWidgets import QMessageBox

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
    (root / "Makefile").write_text("all:\n", encoding="utf-8")


def _create_project(project_dir, app_name, sdk_root=""):
    from ui_designer.model.project import Project

    project = Project(screen_width=240, screen_height=320, app_name=app_name)
    project.sdk_root = str(sdk_root)
    project.project_dir = str(project_dir)
    project.create_new_page("main_page")
    project.save(str(project_dir))
    return project


@_skip_no_qt
class TestMainWindowFileFlow:
    def test_open_project_path_accepts_directory(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "OpenDemo"
        _create_project(project_dir, "OpenDemo", sdk_root)

        window = MainWindow(str(sdk_root))
        captured = {}

        def fake_open_loaded_project(project, project_root, preferred_sdk_root="", silent=False):
            captured["app_name"] = project.app_name
            captured["project_dir"] = project_root
            captured["preferred_sdk_root"] = preferred_sdk_root
            captured["silent"] = silent

        monkeypatch.setattr(window, "_open_loaded_project", fake_open_loaded_project)

        window._open_project_path(str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)

        assert captured == {
            "app_name": "OpenDemo",
            "project_dir": os.path.normpath(os.path.abspath(project_dir)),
            "preferred_sdk_root": os.path.normpath(os.path.abspath(sdk_root)),
            "silent": True,
        }
        window.close()
        window.deleteLater()

    def test_save_project_writes_project_and_generated_files(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "SaveDemo"
        project = _create_project(project_dir, "SaveDemo", sdk_root)

        window = MainWindow(str(sdk_root))
        window.project = project
        window.project_root = str(sdk_root)
        window._project_dir = str(project_dir)
        window.app_name = "SaveDemo"
        window._undo_manager.get_stack("main_page").push("<Page />")

        monkeypatch.setattr(window, "_recreate_compiler", lambda: None)
        monkeypatch.setattr("ui_designer.ui.main_window.generate_all_files_preserved", lambda *args, **kwargs: {"generated.c": "// generated\n"})

        window._save_project()

        assert (project_dir / "SaveDemo.egui").is_file()
        assert (project_dir / "generated.c").read_text(encoding="utf-8") == "// generated\n"
        assert (project_dir / "build.mk").is_file()
        assert (project_dir / "app_egui_config.h").is_file()
        assert (project_dir / "resource" / "src" / "app_resource_config.json").is_file()
        assert isolated_config.last_project_path == os.path.join(str(project_dir), "SaveDemo.egui")
        assert isolated_config.recent_projects[0]["project_path"] == os.path.join(str(project_dir), "SaveDemo.egui")
        assert window._undo_manager.is_any_dirty() is False
        assert "Saved:" in window.statusBar().currentMessage()
        window.close()
        window.deleteLater()

    def test_save_project_as_copies_sidecar_files_and_updates_project_dir(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        src_dir = tmp_path / "SrcDemo"
        dst_dir = tmp_path / "DstDemo"
        project = _create_project(src_dir, "SaveAsDemo", sdk_root)

        (src_dir / "build.mk").write_text("# custom build\n", encoding="utf-8")
        (src_dir / "app_egui_config.h").write_text("#define CUSTOM_CFG 1\n", encoding="utf-8")
        images_dir = src_dir / ".eguiproject" / "resources" / "images"
        images_dir.mkdir(parents=True, exist_ok=True)
        (images_dir / "legacy.png").write_bytes(b"PNG")
        mockup_dir = src_dir / ".eguiproject" / "mockup"
        mockup_dir.mkdir(parents=True, exist_ok=True)
        (mockup_dir / "legacy.txt").write_text("mock", encoding="utf-8")

        window = MainWindow(str(sdk_root))
        window.project = project
        window.project_root = str(sdk_root)
        window._project_dir = str(src_dir)
        window.app_name = "SaveAsDemo"

        monkeypatch.setattr("ui_designer.ui.main_window.QFileDialog.getExistingDirectory", lambda *args, **kwargs: str(dst_dir))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: None)
        monkeypatch.setattr("ui_designer.ui.main_window.generate_all_files_preserved", lambda *args, **kwargs: {"generated.c": "// save as\n"})

        window._save_project_as()

        assert window._project_dir == os.path.normpath(os.path.abspath(dst_dir))
        assert window.project.project_dir == os.path.normpath(os.path.abspath(dst_dir))
        assert (dst_dir / "generated.c").read_text(encoding="utf-8") == "// save as\n"
        assert (dst_dir / "build.mk").read_text(encoding="utf-8") == "# custom build\n"
        assert (dst_dir / "app_egui_config.h").read_text(encoding="utf-8") == "#define CUSTOM_CFG 1\n"
        assert (dst_dir / ".eguiproject" / "resources" / "images" / "legacy.png").is_file()
        assert (dst_dir / ".eguiproject" / "mockup" / "legacy.txt").is_file()
        assert (dst_dir / "resource" / "src" / "legacy.png").is_file()
        window.close()
        window.deleteLater()

    def test_save_project_as_warns_on_non_empty_directory(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        src_dir = tmp_path / "SrcDemo"
        dst_dir = tmp_path / "BusyDir"
        dst_dir.mkdir()
        (dst_dir / "existing.txt").write_text("busy", encoding="utf-8")
        project = _create_project(src_dir, "SaveAsDemo", sdk_root)

        window = MainWindow(str(sdk_root))
        window.project = project
        window.project_root = str(sdk_root)
        window._project_dir = str(src_dir)
        window.app_name = "SaveAsDemo"

        warnings = []
        monkeypatch.setattr("ui_designer.ui.main_window.QFileDialog.getExistingDirectory", lambda *args, **kwargs: str(dst_dir))
        monkeypatch.setattr("ui_designer.ui.main_window.QMessageBox.warning", lambda *args: warnings.append(args[1:]))
        monkeypatch.setattr(window, "_save_project_files", lambda *args, **kwargs: pytest.fail("_save_project_files should not be called"))

        window._save_project_as()

        assert warnings
        assert warnings[0][0] == "Directory Conflict"
        window.close()
        window.deleteLater()

    def test_set_sdk_root_updates_current_project_and_rebuilds_compiler(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.project import Project
        from ui_designer.ui.main_window import MainWindow

        old_sdk = tmp_path / "old_sdk"
        new_sdk = tmp_path / "new_sdk"
        _create_sdk_root(old_sdk)
        _create_sdk_root(new_sdk)

        window = MainWindow(str(old_sdk))
        window.project = Project(app_name="DemoApp")
        window.auto_compile = True

        class FakeCompiler:
            def can_build(self):
                return True

            def is_preview_running(self):
                return False

            def stop_exe(self):
                return None

            def cleanup(self):
                return None

        calls = {"recreate": 0, "compile": 0}

        def fake_recreate():
            calls["recreate"] += 1
            window.compiler = FakeCompiler()

        monkeypatch.setattr("ui_designer.ui.main_window.QFileDialog.getExistingDirectory", lambda *args, **kwargs: str(new_sdk))
        monkeypatch.setattr(window, "_recreate_compiler", fake_recreate)
        monkeypatch.setattr(window, "_trigger_compile", lambda: calls.__setitem__("compile", calls["compile"] + 1))

        window._set_sdk_root()

        assert window.project_root == os.path.normpath(os.path.abspath(new_sdk))
        assert window.project.sdk_root == os.path.normpath(os.path.abspath(new_sdk))
        assert isolated_config.sdk_root == os.path.normpath(os.path.abspath(new_sdk))
        assert calls == {"recreate": 1, "compile": 1}
        assert "SDK root set to:" in window.statusBar().currentMessage()
        window.close()
        window.deleteLater()

    def test_import_legacy_example_generates_project_and_uses_existing_dimensions(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.project import Project
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        app_dir = sdk_root / "example" / "LegacyApp"
        app_dir.mkdir(parents=True)
        (app_dir / "build.mk").write_text("# legacy\n", encoding="utf-8")
        (app_dir / "app_egui_config.h").write_text(
            "#define EGUI_CONFIG_SCEEN_WIDTH  480\n#define EGUI_CONFIG_SCEEN_HEIGHT 272\n",
            encoding="utf-8",
        )

        window = MainWindow(str(sdk_root))
        opened = {}

        def fake_open_project_path(path, preferred_sdk_root="", silent=False):
            opened["path"] = path
            opened["preferred_sdk_root"] = preferred_sdk_root
            opened["silent"] = silent

        monkeypatch.setattr(window, "_open_project_path", fake_open_project_path)

        window._import_legacy_example(
            {
                "app_name": "LegacyApp",
                "app_dir": str(app_dir),
            },
            str(sdk_root),
        )

        project_path = app_dir / "LegacyApp.egui"
        saved = Project.load(str(project_path))
        assert project_path.is_file()
        assert saved.screen_width == 480
        assert saved.screen_height == 272
        assert (app_dir / "resource" / "src" / "app_resource_config.json").is_file()
        assert opened == {
            "path": os.path.normpath(os.path.abspath(project_path)),
            "preferred_sdk_root": os.path.normpath(os.path.abspath(sdk_root)),
            "silent": False,
        }
        window.close()
        window.deleteLater()

    def test_import_legacy_example_warns_on_eguiproject_conflict(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        app_dir = sdk_root / "example" / "LegacyConflict"
        (app_dir / ".eguiproject").mkdir(parents=True)
        (app_dir / "build.mk").write_text("# legacy\n", encoding="utf-8")

        window = MainWindow(str(sdk_root))
        warnings = []

        monkeypatch.setattr("ui_designer.ui.main_window.QMessageBox.warning", lambda *args: warnings.append(args[1:]))
        monkeypatch.setattr(window, "_open_project_path", lambda *args, **kwargs: pytest.fail("_open_project_path should not be called"))

        window._import_legacy_example(
            {
                "app_name": "LegacyConflict",
                "app_dir": str(app_dir),
            },
            str(sdk_root),
        )

        assert warnings
        assert warnings[0][0] == "Legacy Example Conflict"
        assert not (app_dir / "LegacyConflict.egui").exists()
        window.close()
        window.deleteLater()

    def test_open_recent_project_can_remove_missing_entry(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        missing_project = tmp_path / "MissingApp" / "MissingApp.egui"
        window = MainWindow("")
        isolated_config.recent_projects = [
            {
                "project_path": str(missing_project),
                "sdk_root": "",
                "display_name": "MissingApp",
            }
        ]
        window._update_recent_menu()

        monkeypatch.setattr("ui_designer.ui.main_window.QMessageBox.question", lambda *args, **kwargs: QMessageBox.Yes)

        window._open_recent_project(str(missing_project))

        assert isolated_config.recent_projects == []
        assert window._recent_menu.actions()[0].text() == "(No recent projects)"
        recent_widget = window._welcome_page._recent_list.itemAt(0).widget()
        assert recent_widget is not None
        assert "No recent projects" in recent_widget.text()
        assert "Removed missing project" in window.statusBar().currentMessage()
        window.close()
        window.deleteLater()

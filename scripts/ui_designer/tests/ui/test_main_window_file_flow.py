"""Qt UI tests for MainWindow project file flows."""

import os
import subprocess
import sys
import textwrap
from pathlib import Path

import pytest

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

try:
    from PyQt5.QtCore import QByteArray, Qt
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
    try:
        app.sendPostedEvents()
    except Exception:
        pass
    app.processEvents()
    for widget in list(QApplication.topLevelWidgets()):
        try:
            if widget.isVisible():
                undo_manager = getattr(widget, "_undo_manager", None)
                if undo_manager is not None:
                    undo_manager.mark_all_saved()
                widget.close()
            widget.deleteLater()
        except Exception:
            pass
    try:
        app.sendPostedEvents()
    except Exception:
        pass
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


def _close_window(window):
    undo_manager = getattr(window, "_undo_manager", None)
    if undo_manager is not None:
        try:
            # Avoid headless test teardown entering the unsaved-changes dialog path.
            undo_manager.mark_all_saved()
        except Exception:
            pass
    window.close()
    window.deleteLater()
    app = QApplication.instance()
    if app is not None:
        try:
            app.sendPostedEvents()
        except Exception:
            pass
        app.processEvents()


class _DisabledCompiler:
    def can_build(self):
        return False

    def is_preview_running(self):
        return False

    def stop_exe(self):
        return None

    def cleanup(self):
        return None

    def get_build_error(self):
        return "preview disabled for test"

    def set_screen_size(self, width, height):
        return None

    def is_exe_ready(self):
        return False


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
        _close_window(window)

    def test_open_project_uses_recovered_cached_sdk_example_as_default_dir(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        cached_sdk = tmp_path / "config" / "sdk" / "EmbeddedGUI"
        _create_sdk_root(cached_sdk)
        isolated_config.sdk_root = str(tmp_path / "missing_sdk")
        isolated_config.egui_root = str(tmp_path / "missing_sdk")
        captured = {}

        def fake_get_open_file_name(parent, title, directory, filters):
            captured["title"] = title
            captured["directory"] = directory
            captured["filters"] = filters
            return "", ""

        window = MainWindow("")
        monkeypatch.setattr("ui_designer.ui.main_window.default_sdk_install_dir", lambda: str(cached_sdk))
        monkeypatch.setattr("ui_designer.ui.main_window.QFileDialog.getOpenFileName", fake_get_open_file_name)

        window._open_project()

        assert captured["title"] == "Open Project"
        assert captured["directory"] == os.path.join(os.path.normpath(os.path.abspath(cached_sdk)), "example")
        assert "EmbeddedGUI Projects" in captured["filters"]
        _close_window(window)

    def test_open_project_uses_nearest_existing_parent_for_missing_last_project(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        recent_parent = tmp_path / "workspace" / "RecentApp"
        recent_parent.mkdir(parents=True)
        isolated_config.last_project_path = str(recent_parent / "Missing.egui")
        captured = {}

        def fake_get_open_file_name(parent, title, directory, filters):
            captured["directory"] = directory
            return "", ""

        window = MainWindow("")
        monkeypatch.setattr("ui_designer.ui.main_window.QFileDialog.getOpenFileName", fake_get_open_file_name)

        window._open_project()

        assert captured["directory"] == os.path.normpath(os.path.abspath(recent_parent))
        _close_window(window)

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
        _close_window(window)

    def test_new_project_can_be_created_without_sdk_root(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        parent_dir = tmp_path / "workspace"
        parent_dir.mkdir()

        class FakeDialog:
            Accepted = 1

            def __init__(self, *args, **kwargs):
                self.sdk_root = ""
                self.parent_dir = str(parent_dir)
                self.app_name = "NoSdkDemo"
                self.screen_width = 240
                self.screen_height = 320

            def exec_(self):
                return self.Accepted

        opened = {}
        window = MainWindow("")

        def fake_open_loaded_project(project, project_dir, preferred_sdk_root="", silent=False):
            opened["project"] = project
            opened["project_dir"] = project_dir
            opened["preferred_sdk_root"] = preferred_sdk_root
            opened["silent"] = silent

        monkeypatch.setattr("ui_designer.ui.main_window.NewProjectDialog", FakeDialog)
        monkeypatch.setattr(window, "_open_loaded_project", fake_open_loaded_project)

        window._new_project()

        project_dir = parent_dir / "NoSdkDemo"
        assert project_dir.is_dir()
        assert (project_dir / "NoSdkDemo.egui").is_file()
        assert (project_dir / "build.mk").is_file()
        assert opened["project"].sdk_root == ""
        assert opened["project_dir"] == os.path.normpath(os.path.abspath(project_dir))
        assert opened["preferred_sdk_root"] == ""
        assert "Created project: NoSdkDemo" in window.statusBar().currentMessage()
        _close_window(window)

    def test_new_project_uses_current_project_parent_as_default_parent_dir(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        workspace_dir = tmp_path / "workspace"
        project_dir = workspace_dir / "CurrentApp"
        project_dir.mkdir(parents=True)
        captured = {}

        class FakeDialog:
            Accepted = 1

            def __init__(self, parent=None, sdk_root="", default_parent_dir=""):
                captured["default_parent_dir"] = default_parent_dir

            def exec_(self):
                return 0

        window = MainWindow("")
        window._project_dir = str(project_dir)
        monkeypatch.setattr("ui_designer.ui.main_window.NewProjectDialog", FakeDialog)

        window._new_project()

        assert captured["default_parent_dir"] == os.path.normpath(os.path.abspath(workspace_dir))
        _close_window(window)

    def test_new_project_warns_when_target_directory_already_exists_even_if_empty(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        parent_dir = tmp_path / "workspace"
        target_dir = parent_dir / "ExistingDemo"
        target_dir.mkdir(parents=True)

        class FakeDialog:
            Accepted = 1

            def __init__(self, *args, **kwargs):
                self.sdk_root = str(sdk_root)
                self.parent_dir = str(parent_dir)
                self.app_name = "ExistingDemo"
                self.screen_width = 240
                self.screen_height = 320

            def exec_(self):
                return self.Accepted

        warnings = []
        window = MainWindow(str(sdk_root))
        monkeypatch.setattr("ui_designer.ui.main_window.NewProjectDialog", FakeDialog)
        monkeypatch.setattr("ui_designer.ui.main_window.QMessageBox.warning", lambda *args: warnings.append(args[1:]))
        monkeypatch.setattr(window, "_open_loaded_project", lambda *args, **kwargs: pytest.fail("_open_loaded_project should not be called"))

        window._new_project()

        assert warnings
        assert warnings[0][0] == "Directory Conflict"
        assert "already exists" in warnings[0][1]
        _close_window(window)

    def test_selection_feedback_status_mentions_locked_hidden_and_layout_managed_widget(self, qapp, isolated_config):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        window = MainWindow("")
        root = WidgetModel("linearlayout", name="root")
        child = WidgetModel("switch", name="child")
        child.designer_locked = True
        child.designer_hidden = True
        root.add_child(child)

        window._set_selection([child], primary=child, sync_tree=False, sync_preview=False)

        message = window.statusBar().currentMessage()
        assert "Selection note:" in message
        assert "child is locked" in message
        assert "hidden" in message
        assert "layout-managed by linearlayout" in message
        _close_window(window)

    def test_selection_feedback_status_summarizes_multi_selection_constraints(self, qapp, isolated_config):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        window = MainWindow("")
        root = WidgetModel("linearlayout", name="root")
        first = WidgetModel("switch", name="first")
        second = WidgetModel("switch", name="second")
        first.designer_locked = True
        second.designer_hidden = True
        root.add_child(first)
        root.add_child(second)

        window._set_selection([first, second], primary=second, sync_tree=False, sync_preview=False)

        message = window.statusBar().currentMessage()
        assert "Selection note: current selection includes" in message
        assert "1 locked widget" in message
        assert "1 hidden widget" in message
        assert "2 layout-managed widgets" in message
        _close_window(window)

    def test_delete_selection_blocks_locked_widgets(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "DeleteLockedDemo"
        project = _create_project(project_dir, "DeleteLockedDemo", sdk_root)
        locked = WidgetModel("switch", name="locked_widget")
        locked.designer_locked = True
        project.get_startup_page().root_widget.add_child(locked)
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        window._set_selection([locked], primary=locked, sync_tree=False, sync_preview=False)

        deleted_count, skipped_locked = window._delete_selection()

        assert deleted_count == 0
        assert skipped_locked == 1
        assert locked in project.get_startup_page().root_widget.children
        assert window.statusBar().currentMessage() == "Cannot delete selection: 1 locked widget."
        _close_window(window)

    def test_delete_selection_skips_locked_widgets(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "DeleteMixedDemo"
        project = _create_project(project_dir, "DeleteMixedDemo", sdk_root)
        removable = WidgetModel("switch", name="removable")
        locked = WidgetModel("switch", name="locked_widget")
        locked.designer_locked = True
        root = project.get_startup_page().root_widget
        root.add_child(removable)
        root.add_child(locked)
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        window._set_selection([removable, locked], primary=removable, sync_tree=False, sync_preview=False)

        deleted_count, skipped_locked = window._delete_selection()

        assert deleted_count == 1
        assert skipped_locked == 1
        assert removable not in root.children
        assert locked in root.children
        assert window.statusBar().currentMessage() == "Deleted 1 widget(s); skipped 1 locked widget"
        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_cut_selection_skips_locked_widgets(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "CutMixedDemo"
        project = _create_project(project_dir, "CutMixedDemo", sdk_root)
        removable = WidgetModel("switch", name="removable")
        locked = WidgetModel("switch", name="locked_widget")
        locked.designer_locked = True
        root = project.get_startup_page().root_widget
        root.add_child(removable)
        root.add_child(locked)
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        window._set_selection([removable, locked], primary=removable, sync_tree=False, sync_preview=False)

        window._cut_selection()

        assert removable not in root.children
        assert locked in root.children
        assert len(window._clipboard_payload["widgets"]) == 1
        assert window._clipboard_payload["widgets"][0]["name"] == "removable"
        assert window.statusBar().currentMessage() == "Cut 1 widget(s); skipped 1 locked widget"
        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_widget_tree_delete_skips_locked_widgets_and_updates_status(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "TreeDeleteMixedDemo"
        project = _create_project(project_dir, "TreeDeleteMixedDemo", sdk_root)
        removable = WidgetModel("switch", name="removable")
        locked = WidgetModel("switch", name="locked_widget")
        locked.designer_locked = True
        root = project.get_startup_page().root_widget
        root.add_child(removable)
        root.add_child(locked)
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        window._set_selection([removable, locked], primary=removable, sync_tree=True, sync_preview=False)

        window.widget_tree._on_delete_clicked()

        assert removable not in root.children
        assert locked in root.children
        assert window.statusBar().currentMessage() == "Deleted 1 widget(s); skipped 1 locked widget"
        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_selection_sync_reveals_widget_tree_path(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "TreeRevealDemo"
        project = _create_project(project_dir, "TreeRevealDemo", sdk_root)
        root = project.get_startup_page().root_widget
        container = WidgetModel("group", name="container")
        nested = WidgetModel("group", name="nested")
        target = WidgetModel("switch", name="target")
        nested.add_child(target)
        container.add_child(nested)
        root.add_child(container)
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        container_item = window.widget_tree._item_map[id(container)]
        nested_item = window.widget_tree._item_map[id(nested)]
        container_item.setExpanded(False)
        nested_item.setExpanded(False)

        window._set_selection([target], primary=target, sync_tree=True, sync_preview=False)

        assert container_item.isExpanded() is True
        assert nested_item.isExpanded() is True
        assert window.widget_tree._get_selected_widget() is target
        _close_window(window)

    def test_widget_tree_rebuild_preserves_manual_collapse_state(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "TreeCollapseStateDemo"
        project = _create_project(project_dir, "TreeCollapseStateDemo", sdk_root)
        root = project.get_startup_page().root_widget
        container = WidgetModel("group", name="container")
        nested = WidgetModel("group", name="nested")
        target = WidgetModel("switch", name="target")
        nested.add_child(target)
        container.add_child(nested)
        root.add_child(container)
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        window._clear_selection(sync_tree=True, sync_preview=False)
        container_item = window.widget_tree._item_map[id(container)]
        nested_item = window.widget_tree._item_map[id(nested)]
        container_item.setExpanded(False)
        nested_item.setExpanded(False)

        window._on_property_changed()

        assert window.widget_tree._item_map[id(container)].isExpanded() is False
        assert window.widget_tree._item_map[id(nested)].isExpanded() is False
        _close_window(window)

    def test_widget_tree_filter_updates_status_bar(self):
        repo_root = Path(__file__).resolve().parents[4]
        script = textwrap.dedent(
            f"""
            import os
            import shutil
            import sys
            import tempfile
            from pathlib import Path

            os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

            repo_root = Path({repr(str(repo_root))})
            sys.path.insert(0, str(repo_root / "scripts"))

            from PyQt5.QtWidgets import QApplication

            from ui_designer.model.project import Project
            from ui_designer.model.widget_model import WidgetModel
            from ui_designer.ui.main_window import MainWindow


            def create_sdk_root(root: Path):
                (root / "src").mkdir(parents=True)
                (root / "porting" / "designer").mkdir(parents=True)
                (root / "Makefile").write_text("all:\\n", encoding="utf-8")


            class DisabledCompiler:
                def can_build(self):
                    return False

                def is_preview_running(self):
                    return False

                def stop_exe(self):
                    return None

                def cleanup(self):
                    return None

                def get_build_error(self):
                    return "preview disabled for test"

                def set_screen_size(self, width, height):
                    return None

                def is_exe_ready(self):
                    return False


            temp_root = Path(tempfile.mkdtemp(prefix="ui_designer_filter_status_", dir=str(repo_root)))
            app = QApplication.instance() or QApplication([])
            try:
                sdk_root = temp_root / "sdk"
                project_dir = temp_root / "TreeFilterStatusDemo"
                create_sdk_root(sdk_root)

                project = Project(screen_width=240, screen_height=320, app_name="TreeFilterStatusDemo")
                project.sdk_root = str(sdk_root)
                project.project_dir = str(project_dir)
                page = project.create_new_page("main_page")
                page.root_widget.add_child(WidgetModel("label", name="field_label"))
                page.root_widget.add_child(WidgetModel("button", name="field_button"))
                project.save(str(project_dir))

                window = MainWindow(str(sdk_root))
                window._recreate_compiler = lambda _window=window: setattr(_window, "compiler", DisabledCompiler())
                window._trigger_compile = lambda: None

                window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)

                window.widget_tree.filter_edit.setText("field")
                assert window.statusBar().currentMessage() == "Widget filter 'field': 2 matches."

                window.widget_tree._select_next_filter_match()
                assert window.statusBar().currentMessage() == "Widget filter 'field': 2 matches (1/2)."

                window.widget_tree.filter_edit.setText("")
                assert window.statusBar().currentMessage() == "Widget filter cleared."

                window._undo_manager.mark_all_saved()
                window.close()
                window.deleteLater()
                app.sendPostedEvents()
                app.processEvents()
            finally:
                shutil.rmtree(temp_root, ignore_errors=True)
            """
        )

        env = os.environ.copy()
        env.setdefault("QT_QPA_PLATFORM", "offscreen")
        result = subprocess.run(
            [sys.executable, "-c", script],
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            env=env,
            timeout=60,
        )

        assert result.returncode == 0, f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}"

    def test_widget_tree_batch_rename_updates_status_and_selection(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "TreeBatchRenameDemo"
        project = _create_project(project_dir, "TreeBatchRenameDemo", sdk_root)
        root = project.get_startup_page().root_widget
        existing = WidgetModel("label", name="field_1")
        first = WidgetModel("label", name="title")
        second = WidgetModel("switch", name="cta")
        root.add_child(existing)
        root.add_child(first)
        root.add_child(second)
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)
        monkeypatch.setattr(
            "ui_designer.ui.widget_tree.QInputDialog.getText",
            lambda *args, **kwargs: ("field", True),
        )

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        window._set_selection([first, second], primary=first, sync_tree=True, sync_preview=False)

        window.widget_tree._on_rename_clicked()

        assert first.name == "field_2"
        assert second.name == "field_3"
        assert window.widget_tree.selected_widgets() == [first, second]
        assert window.widget_tree._get_selected_widget() is first
        assert window.statusBar().currentMessage() == "Renamed 2 widget(s) with prefix 'field'."
        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_align_selection_reports_locked_constraint(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "AlignLockedDemo"
        project = _create_project(project_dir, "AlignLockedDemo", sdk_root)
        first = WidgetModel("switch", name="first", x=10, y=10, width=40, height=20)
        second = WidgetModel("switch", name="second", x=60, y=20, width=40, height=20)
        second.designer_locked = True
        root = project.get_startup_page().root_widget
        root.add_child(first)
        root.add_child(second)
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        window._set_selection([first, second], primary=first, sync_tree=False, sync_preview=False)

        window._align_selection("left")

        assert window.statusBar().currentMessage() == "Cannot align selection: locked widgets leave fewer than 2 editable widgets."
        _close_window(window)

    def test_distribute_selection_reports_mixed_parent_constraint(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "DistributeParentDemo"
        project = _create_project(project_dir, "DistributeParentDemo", sdk_root)
        root = project.get_startup_page().root_widget
        group_a = WidgetModel("group", name="group_a", x=0, y=0, width=120, height=120)
        group_b = WidgetModel("group", name="group_b", x=130, y=0, width=120, height=120)
        first = WidgetModel("switch", name="first", x=10, y=10, width=20, height=20)
        second = WidgetModel("switch", name="second", x=40, y=10, width=20, height=20)
        third = WidgetModel("switch", name="third", x=10, y=10, width=20, height=20)
        group_a.add_child(first)
        group_a.add_child(second)
        group_b.add_child(third)
        root.add_child(group_a)
        root.add_child(group_b)
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        window._set_selection([first, second, third], primary=first, sync_tree=False, sync_preview=False)

        window._distribute_selection("horizontal")

        assert window.statusBar().currentMessage() == "Cannot distribute selection: selected widgets do not share the same free-position parent."
        _close_window(window)

    def test_move_selection_to_front_reports_all_locked(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "FrontLockedDemo"
        project = _create_project(project_dir, "FrontLockedDemo", sdk_root)
        locked = WidgetModel("switch", name="locked_widget")
        locked.designer_locked = True
        project.get_startup_page().root_widget.add_child(locked)
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        window._set_selection([locked], primary=locked, sync_tree=False, sync_preview=False)

        window._move_selection_to_front()

        assert window.statusBar().currentMessage() == "Cannot bring to front: all selected widgets are locked."
        _close_window(window)

    def test_move_selection_to_back_reports_all_locked(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "BackLockedDemo"
        project = _create_project(project_dir, "BackLockedDemo", sdk_root)
        locked = WidgetModel("switch", name="locked_widget")
        locked.designer_locked = True
        project.get_startup_page().root_widget.add_child(locked)
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        window._set_selection([locked], primary=locked, sync_tree=False, sync_preview=False)

        window._move_selection_to_back()

        assert window.statusBar().currentMessage() == "Cannot send to back: all selected widgets are locked."
        _close_window(window)

    def test_property_edit_status_mentions_dirty_source(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "PropertyStatusDemo"
        project = _create_project(project_dir, "PropertyStatusDemo", sdk_root)
        widget = WidgetModel("switch", name="toggle")
        project.get_startup_page().root_widget.add_child(widget)
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        window._set_selection([widget], primary=widget, sync_tree=False, sync_preview=False)
        widget.properties["is_checked"] = True

        window._on_property_changed()

        assert window.statusBar().currentMessage() == "Changed main_page: property edit."
        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_canvas_move_status_mentions_dirty_source(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "CanvasMoveStatusDemo"
        project = _create_project(project_dir, "CanvasMoveStatusDemo", sdk_root)
        widget = WidgetModel("switch", name="toggle", x=10, y=10, width=50, height=24)
        project.get_startup_page().root_widget.add_child(widget)
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        window._set_selection([widget], primary=widget, sync_tree=False, sync_preview=False)
        widget.x = 20
        widget.display_x = 20

        window._on_widget_moved(widget, 20, 10)

        assert window.statusBar().currentMessage() == "Changed main_page: canvas move."
        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_new_project_prefers_recovered_cached_sdk_for_defaults(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        cached_sdk = tmp_path / "config" / "sdk" / "EmbeddedGUI"
        _create_sdk_root(cached_sdk)
        isolated_config.sdk_root = str(tmp_path / "missing_sdk")
        isolated_config.egui_root = str(tmp_path / "missing_sdk")
        captured = {}

        class FakeDialog:
            Accepted = 1

            def __init__(self, parent=None, sdk_root="", default_parent_dir=""):
                captured["sdk_root"] = sdk_root
                captured["default_parent_dir"] = default_parent_dir

            def exec_(self):
                return 0

        window = MainWindow("")
        monkeypatch.setattr("ui_designer.ui.main_window.default_sdk_install_dir", lambda: str(cached_sdk))
        monkeypatch.setattr("ui_designer.ui.main_window.NewProjectDialog", FakeDialog)

        window._new_project()

        assert captured["sdk_root"] == os.path.normpath(os.path.abspath(cached_sdk))
        assert captured["default_parent_dir"] == os.path.join(os.path.normpath(os.path.abspath(cached_sdk)), "example")
        _close_window(window)

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
        _close_window(window)

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
        _close_window(window)

    def test_save_project_as_warns_on_existing_empty_directory(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        src_dir = tmp_path / "SrcDemo"
        dst_dir = tmp_path / "ExistingDir"
        dst_dir.mkdir()
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
        assert "already exists" in warnings[0][1]
        _close_window(window)

    def test_save_project_as_uses_current_project_parent_as_initial_directory(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        workspace_dir = tmp_path / "workspace"
        src_dir = workspace_dir / "SrcDemo"
        project = _create_project(src_dir, "SaveAsDemo", sdk_root)
        captured = {}

        window = MainWindow(str(sdk_root))
        window.project = project
        window.project_root = str(sdk_root)
        window._project_dir = str(src_dir)

        def fake_get_existing_directory(parent, title, directory):
            captured["title"] = title
            captured["directory"] = directory
            return ""

        monkeypatch.setattr("ui_designer.ui.main_window.QFileDialog.getExistingDirectory", fake_get_existing_directory)

        window._save_project_as()

        assert captured["title"] == "Save Project To Directory"
        assert captured["directory"] == os.path.normpath(os.path.abspath(workspace_dir))
        _close_window(window)

    def test_export_code_uses_current_project_dir_as_initial_directory(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "ExportDemo"
        project = _create_project(project_dir, "ExportDemo", sdk_root)
        captured = {}

        window = MainWindow(str(sdk_root))
        window.project = project
        window._project_dir = str(project_dir)

        def fake_get_existing_directory(parent, title, directory):
            captured["title"] = title
            captured["directory"] = directory
            return ""

        monkeypatch.setattr("ui_designer.ui.main_window.QFileDialog.getExistingDirectory", fake_get_existing_directory)

        window._export_code()

        assert captured["title"] == "Export C Code To Directory"
        assert captured["directory"] == os.path.normpath(os.path.abspath(project_dir))
        _close_window(window)

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
        assert "selected SDK root" in window.statusBar().currentMessage()
        _close_window(window)

    def test_set_sdk_root_auto_resolves_parent_directory(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        sdk_parent = tmp_path / "tools"
        sdk_root = sdk_parent / "sdk" / "EmbeddedGUI-main"
        _create_sdk_root(sdk_root)

        window = MainWindow("")
        monkeypatch.setattr("ui_designer.ui.main_window.QFileDialog.getExistingDirectory", lambda *args, **kwargs: str(sdk_parent))

        window._set_sdk_root()

        assert window.project_root == os.path.normpath(os.path.abspath(sdk_root))
        assert isolated_config.sdk_root == os.path.normpath(os.path.abspath(sdk_root))
        assert "SDK root set to:" in window.statusBar().currentMessage()
        assert "selected SDK root" in window.statusBar().currentMessage()
        _close_window(window)

    def test_set_sdk_root_uses_recovered_cached_sdk_as_initial_directory(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        cached_sdk = tmp_path / "config" / "sdk" / "EmbeddedGUI"
        _create_sdk_root(cached_sdk)
        isolated_config.sdk_root = str(tmp_path / "missing_sdk")
        isolated_config.egui_root = str(tmp_path / "missing_sdk")
        captured = {}

        def fake_get_existing_directory(parent, title, directory):
            captured["title"] = title
            captured["directory"] = directory
            return ""

        window = MainWindow("")
        monkeypatch.setattr("ui_designer.ui.main_window.default_sdk_install_dir", lambda: str(cached_sdk))
        monkeypatch.setattr("ui_designer.ui.main_window.QFileDialog.getExistingDirectory", fake_get_existing_directory)

        window._set_sdk_root()

        assert captured["title"] == "Select EmbeddedGUI SDK Root"
        assert captured["directory"] == os.path.normpath(os.path.abspath(cached_sdk))
        _close_window(window)

    def test_open_app_dialog_uses_recovered_cached_sdk_root(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        cached_sdk = tmp_path / "config" / "sdk" / "EmbeddedGUI"
        _create_sdk_root(cached_sdk)
        isolated_config.sdk_root = str(tmp_path / "missing_sdk")
        isolated_config.egui_root = str(tmp_path / "missing_sdk")
        captured = {}

        class FakeDialog:
            Accepted = 1

            def __init__(self, parent=None, egui_root=None, on_download_sdk=None):
                captured["egui_root"] = egui_root
                captured["has_download_handler"] = callable(on_download_sdk)
                self._selected_entry = None
                self._egui_root = egui_root

            def exec_(self):
                return 0

            @property
            def selected_entry(self):
                return self._selected_entry

            @property
            def egui_root(self):
                return self._egui_root

        window = MainWindow("")
        monkeypatch.setattr("ui_designer.ui.main_window.default_sdk_install_dir", lambda: str(cached_sdk))
        monkeypatch.setattr("ui_designer.ui.main_window.AppSelectorDialog", FakeDialog)

        window._open_app_dialog()

        assert captured["egui_root"] == os.path.normpath(os.path.abspath(cached_sdk))
        assert captured["has_download_handler"] is True
        _close_window(window)

    def test_open_loaded_project_discovers_default_sdk_cache_when_config_is_empty(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.project import Project
        from ui_designer.ui.main_window import MainWindow

        project_dir = tmp_path / "CacheDemo"
        project = _create_project(project_dir, "CacheDemo", "")
        sdk_root = tmp_path / "config" / "sdk" / "EmbeddedGUI"
        _create_sdk_root(sdk_root)

        window = MainWindow("")
        monkeypatch.setattr("ui_designer.ui.main_window.default_sdk_install_dir", lambda: str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(Project.load(str(project_dir)), str(project_dir), preferred_sdk_root="", silent=True)

        assert window.project_root == os.path.normpath(os.path.abspath(sdk_root))
        assert window.project.sdk_root == os.path.normpath(os.path.abspath(sdk_root))
        assert isolated_config.sdk_root == os.path.normpath(os.path.abspath(sdk_root))
        _close_window(window)

    def test_download_sdk_updates_config_and_project_root(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "downloaded_sdk"

        def fake_ensure_sdk_downloaded(target_dir, progress_callback=None):
            _create_sdk_root(sdk_root)
            if progress_callback is not None:
                progress_callback("EmbeddedGUI SDK is ready.", 100)
            return str(sdk_root)

        window = MainWindow("")
        monkeypatch.setattr("ui_designer.ui.main_window.default_sdk_install_dir", lambda: str(tmp_path / "sdk_cache"))
        monkeypatch.setattr("ui_designer.ui.main_window.ensure_sdk_downloaded", fake_ensure_sdk_downloaded)

        result = window._download_sdk()

        assert result == os.path.normpath(os.path.abspath(sdk_root))
        assert window.project_root == os.path.normpath(os.path.abspath(sdk_root))
        assert isolated_config.sdk_root == os.path.normpath(os.path.abspath(sdk_root))
        assert isolated_config.sdk_setup_prompted is True
        assert "SDK downloaded to:" in window.statusBar().currentMessage()
        _close_window(window)

    def test_download_sdk_failure_mentions_target_dir(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        target_dir = tmp_path / "sdk_cache"
        warnings = []

        window = MainWindow("")
        monkeypatch.setattr("ui_designer.ui.main_window.default_sdk_install_dir", lambda: str(target_dir))
        monkeypatch.setattr(
            "ui_designer.ui.main_window.ensure_sdk_downloaded",
            lambda target, progress_callback=None: (_ for _ in ()).throw(RuntimeError("network blocked")),
        )
        monkeypatch.setattr("ui_designer.ui.main_window.QMessageBox.warning", lambda *args: warnings.append(args[1:]))

        result = window._download_sdk()

        assert result == ""
        assert warnings
        assert warnings[0][0] == "Download SDK Failed"
        assert str(target_dir) in warnings[0][1]
        assert "GitHub archive" in warnings[0][1]
        assert "install git for clone fallback" in warnings[0][1]
        _close_window(window)

    def test_initial_sdk_prompt_shows_target_dir_and_dispatches_download(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        target_dir = tmp_path / "sdk_cache"
        captured = {}

        class FakeMessageBox:
            Information = QMessageBox.Information
            AcceptRole = QMessageBox.AcceptRole
            ActionRole = QMessageBox.ActionRole
            RejectRole = QMessageBox.RejectRole

            def __init__(self, parent=None):
                self.parent = parent
                self._buttons = []
                self._clicked = None

            def setWindowTitle(self, title):
                captured["title"] = title

            def setIcon(self, icon):
                captured["icon"] = icon

            def setText(self, text):
                captured["text"] = text

            def setInformativeText(self, text):
                captured["info"] = text

            def addButton(self, text, role):
                button = object()
                self._buttons.append((text, role, button))
                if text == "Download SDK Automatically":
                    self._clicked = button
                return button

            def exec_(self):
                return 0

            def clickedButton(self):
                return self._clicked

        download_calls = []

        window = MainWindow("")
        monkeypatch.setattr("ui_designer.ui.main_window.default_sdk_install_dir", lambda: str(target_dir))
        monkeypatch.setattr("ui_designer.ui.main_window.QMessageBox", FakeMessageBox)
        monkeypatch.setattr(window, "_download_sdk", lambda: download_calls.append("download"))

        window.maybe_prompt_initial_sdk_setup()

        assert captured["title"] == "Prepare EmbeddedGUI SDK"
        assert "No EmbeddedGUI SDK was detected." in captured["text"]
        assert str(target_dir) in captured["info"]
        assert "Automatic setup order:" in captured["info"]
        assert "GitHub archive" in captured["info"]
        assert isolated_config.sdk_setup_prompted is True
        assert download_calls == ["download"]
        _close_window(window)

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
        _close_window(window)

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
        _close_window(window)

    def test_open_recent_project_can_remove_missing_entry(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        missing_project = tmp_path / "MissingApp" / "MissingApp.egui"
        window = MainWindow("")
        isolated_config.last_project_path = str(missing_project)
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
        assert isolated_config.last_project_path == ""
        assert window._recent_menu.actions()[0].text() == "(No recent projects)"
        recent_widget = window._welcome_page._recent_list.itemAt(0).widget()
        assert recent_widget is not None
        assert "No recent projects" in recent_widget.text()
        assert "Removed missing project" in window.statusBar().currentMessage()
        _close_window(window)

    def test_recent_menu_action_uses_recovered_cached_sdk_root(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        cached_sdk = tmp_path / "config" / "sdk" / "EmbeddedGUI"
        _create_sdk_root(cached_sdk)
        project_path = tmp_path / "DemoApp" / "DemoApp.egui"
        isolated_config.recent_projects = [
            {
                "project_path": str(project_path),
                "sdk_root": str(tmp_path / "missing_sdk"),
                "display_name": "DemoApp",
            }
        ]
        captured = {}

        window = MainWindow("")
        monkeypatch.setattr("ui_designer.ui.main_window.default_sdk_install_dir", lambda: str(cached_sdk))
        monkeypatch.setattr(window, "_open_recent_project", lambda path, sdk_root="": captured.update({"path": path, "sdk_root": sdk_root}))

        window._update_recent_menu()
        window._recent_menu.actions()[0].trigger()

        assert captured["path"] == str(project_path)
        assert captured["sdk_root"] == os.path.normpath(os.path.abspath(cached_sdk))
        _close_window(window)

    def test_recent_menu_marks_missing_project_entries(self, qapp, isolated_config, tmp_path):
        from ui_designer.ui.main_window import MainWindow

        missing_project = tmp_path / "MissingApp" / "MissingApp.egui"
        isolated_config.recent_projects = [
            {
                "project_path": str(missing_project),
                "sdk_root": "",
                "display_name": "MissingApp",
            }
        ]

        window = MainWindow("")
        window._update_recent_menu()

        action = window._recent_menu.actions()[0]
        assert action.text() == "[Missing] MissingApp"
        assert "Project path is missing" in action.toolTip()
        _close_window(window)

    def test_duplicate_page_copies_existing_page_content(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "DuplicateDemo"
        project = _create_project(project_dir, "DuplicateDemo", sdk_root)
        source_page = project.get_page_by_name("main_page")
        label = WidgetModel("label", name="title", x=12, y=16, width=100, height=24)
        label.properties["text"] = "Original Title"
        source_page.root_widget.add_child(label)
        source_page.user_fields.append({"name": "counter", "type": "int", "default": 7})
        source_page.timers.append(
            {
                "name": "refresh_timer",
                "callback": "tick_refresh",
                "delay_ms": "500",
                "period_ms": "1000",
                "auto_start": True,
            }
        )
        source_page.mockup_image_path = "mockup/main.png"
        source_page.mockup_image_opacity = 0.6
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        window.project_dock._duplicate_page("main_page")

        duplicated = window.project.get_page_by_name("main_page_copy")
        assert duplicated is not None
        assert window._current_page is duplicated
        assert duplicated.root_widget is not source_page.root_widget
        assert len(duplicated.root_widget.children) == 1
        assert duplicated.root_widget.children[0].properties["text"] == "Original Title"
        assert duplicated.user_fields == [{"name": "counter", "type": "int", "default": "7"}]
        assert duplicated.timers == [
            {
                "name": "refresh_timer",
                "callback": "tick_refresh",
                "delay_ms": "500",
                "period_ms": "1000",
                "auto_start": True,
            }
        ]
        assert duplicated.mockup_image_path == "mockup/main.png"
        assert duplicated.mockup_image_opacity == 0.6
        assert window._undo_manager.is_any_dirty() is True
        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_page_fields_panel_edit_updates_page_dirty_state_and_xml(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "PageFieldsDemo"
        project = _create_project(project_dir, "PageFieldsDemo", sdk_root)

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        assert window.page_fields_dock.objectName() == "page_fields_dock"

        window.page_fields_panel._on_add_field()
        qapp.processEvents()

        table = window.page_fields_panel._table
        table.item(0, 0).setText("counter")
        table.item(0, 1).setText("uint32_t")
        table.item(0, 2).setText("7")
        qapp.processEvents()

        assert window._current_page.user_fields == [{"name": "counter", "type": "uint32_t", "default": "7"}]
        assert window._undo_manager.is_any_dirty() is True
        assert window.statusBar().currentMessage() == "Changed main_page: page fields edit."

        xml = window._current_page.to_xml_string()
        assert "<UserFields>" in xml
        assert 'name="counter"' in xml
        assert 'type="uint32_t"' in xml
        assert 'default="7"' in xml

        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_page_fields_panel_tracks_current_page_when_switching_pages(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "PageFieldsSwitchDemo"
        project = _create_project(project_dir, "PageFieldsSwitchDemo", sdk_root)
        project.get_startup_page().user_fields = [{"name": "counter", "type": "int", "default": "0"}]
        detail_page = project.create_new_page("detail_page")
        detail_page.user_fields = [{"name": "state", "type": "bool", "default": "false"}]
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)

        assert window.page_fields_panel._summary_label.text() == "Page Fields: 1 field on main_page"
        assert window.page_fields_panel._table.item(0, 0).text() == "counter"

        window._switch_page("detail_page")

        assert window.page_fields_panel._summary_label.text() == "Page Fields: 1 field on detail_page"
        assert window.page_fields_panel._table.item(0, 0).text() == "state"
        assert window.page_fields_panel._table.item(0, 2).text() == "false"

        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_page_fields_panel_open_init_creates_page_source_and_updates_status(
        self, qapp, isolated_config, tmp_path, monkeypatch
    ):
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "PageFieldsCodeDemo"
        project = _create_project(project_dir, "PageFieldsCodeDemo", sdk_root)

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)
        opened = []
        monkeypatch.setattr(window, "_open_path_in_default_app", lambda path: opened.append(path) or True)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        window._on_page_user_code_section_requested("init")

        source_path = project_dir / "main_page.c"
        assert opened == [str(source_path)]
        assert source_path.exists() is True
        content = source_path.read_text(encoding="utf-8")
        assert "void egui_main_page_init(egui_page_base_t *self)" in content
        assert "// USER CODE BEGIN init" in content
        assert window.statusBar().currentMessage() == "Opened user code: main_page.c (init)."
        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_page_timers_panel_edit_updates_page_dirty_state_and_xml(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "PageTimersDemo"
        project = _create_project(project_dir, "PageTimersDemo", sdk_root)

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        assert window.page_timers_dock.objectName() == "page_timers_dock"

        window.page_timers_panel._on_add_timer()
        qapp.processEvents()

        table = window.page_timers_panel._table
        table.item(0, 0).setText("refresh_timer")
        table.item(0, 1).setText("tick_refresh")
        table.item(0, 2).setText("500")
        table.item(0, 3).setText("1000")
        table.item(0, 4).setText("true")
        qapp.processEvents()

        assert window._current_page.timers == [
            {
                "name": "refresh_timer",
                "callback": "tick_refresh",
                "delay_ms": "500",
                "period_ms": "1000",
                "auto_start": True,
            }
        ]
        assert window._undo_manager.is_any_dirty() is True
        assert window.statusBar().currentMessage() == "Changed main_page: page timers edit."

        xml = window._current_page.to_xml_string()
        assert "<Timers>" in xml
        assert 'name="refresh_timer"' in xml
        assert 'callback="tick_refresh"' in xml
        assert 'auto_start="true"' in xml

        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_page_timers_panel_tracks_current_page_when_switching_pages(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "PageTimersSwitchDemo"
        project = _create_project(project_dir, "PageTimersSwitchDemo", sdk_root)
        project.get_startup_page().timers = [
            {"name": "refresh_timer", "callback": "tick_refresh", "delay_ms": "500", "period_ms": "1000", "auto_start": True}
        ]
        detail_page = project.create_new_page("detail_page")
        detail_page.timers = [
            {"name": "poll_timer", "callback": "tick_poll", "delay_ms": "250", "period_ms": "250", "auto_start": False}
        ]
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)

        assert window.page_timers_panel._summary_label.text() == "Page Timers: 1 timer on main_page"
        assert window.page_timers_panel._table.item(0, 0).text() == "refresh_timer"

        window._switch_page("detail_page")

        assert window.page_timers_panel._summary_label.text() == "Page Timers: 1 timer on detail_page"
        assert window.page_timers_panel._table.item(0, 0).text() == "poll_timer"
        assert window.page_timers_panel._table.item(0, 4).text() == "false"

        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_animations_panel_edit_updates_widget_dirty_state_and_xml(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "AnimationsDemo"
        project = _create_project(project_dir, "AnimationsDemo", sdk_root)
        card = WidgetModel("group", name="card", x=12, y=16, width=100, height=60)
        project.get_startup_page().root_widget.add_child(card)
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        window._set_selection([card], primary=card, sync_tree=True, sync_preview=False)

        assert window.animations_dock.objectName() == "animations_dock"
        window.animations_panel._on_add_animation()
        window.animations_panel._on_type_changed(0, "translate")
        window.animations_panel._on_interpolator_changed(0, "bounce")
        window.animations_panel._on_duration_changed(0, 900)
        window.animations_panel._on_repeat_mode_changed(0, "reverse")
        window.animations_panel._on_param_changed(0, "to_y", "64")
        qapp.processEvents()

        assert len(card.animations) == 1
        assert card.animations[0].anim_type == "translate"
        assert card.animations[0].interpolator == "bounce"
        assert card.animations[0].duration == 900
        assert card.animations[0].repeat_mode == "reverse"
        assert card.animations[0].params["to_y"] == "64"
        assert window._undo_manager.is_any_dirty() is True
        assert window.statusBar().currentMessage() == "Changed main_page: widget animations edit."

        xml = window._current_page.to_xml_string()
        assert '<Animation type="translate"' in xml
        assert 'duration="900"' in xml
        assert 'interpolator="bounce"' in xml
        assert 'repeat_mode="reverse"' in xml
        assert 'to_y="64"' in xml

        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_animations_panel_tracks_primary_selection(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.widget_animations import create_default_animation
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "AnimationsSelectionDemo"
        project = _create_project(project_dir, "AnimationsSelectionDemo", sdk_root)
        page = project.get_startup_page()
        card = WidgetModel("group", name="card", x=12, y=16, width=100, height=60)
        badge = WidgetModel("group", name="badge", x=12, y=88, width=80, height=40)
        card.animations = [create_default_animation("alpha")]
        badge.animations = [create_default_animation("color")]
        page.root_widget.add_child(card)
        page.root_widget.add_child(badge)
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)

        window._set_selection([card], primary=card, sync_tree=True, sync_preview=False)
        assert window.animations_panel._summary_label.text() == "Animations: 1 animation on group card"
        assert window.animations_panel._table.item(0, 0).text() == "alpha"

        window._set_selection([badge], primary=badge, sync_tree=True, sync_preview=False)
        assert window.animations_panel._summary_label.text() == "Animations: 1 animation on group badge"
        assert window.animations_panel._table.item(0, 0).text() == "color"

        window._set_selection([card, badge], primary=card, sync_tree=True, sync_preview=False)
        assert "select a single widget" in window.animations_panel._summary_label.text().lower()

        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_property_panel_resource_imported_signal_triggers_resource_refresh_flow(self, qapp, isolated_config, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        window = MainWindow("")
        monkeypatch.setattr(window, "_refresh_project_watch_snapshot", lambda: None)

        window.property_panel.resource_imported.emit()

        assert window._resources_need_regen is True
        assert window._regen_timer.isActive() is True
        assert "Resources changed, will regenerate..." in window.statusBar().currentMessage()
        window._regen_timer.stop()
        _close_window(window)

    def test_property_panel_callback_edit_updates_widget_and_dirty_state(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "EventCallbackDemo"
        project = _create_project(project_dir, "EventCallbackDemo", sdk_root)
        slider = WidgetModel("slider", name="volume_slider", x=16, y=16, width=160, height=24)
        project.get_startup_page().root_widget.add_child(slider)
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        window._set_selection([slider], primary=slider, sync_tree=True, sync_preview=False)

        editor = window.property_panel._editors["callback_onValueChanged"]
        editor.setText("on_volume_changed")
        editor.editingFinished.emit()

        assert slider.events["onValueChanged"] == "on_volume_changed"
        assert window._undo_manager.is_any_dirty() is True
        assert window.statusBar().currentMessage() == "Changed main_page: property edit."
        assert 'onValueChanged="on_volume_changed"' in window._current_page.to_xml_string()
        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_user_code_request_creates_page_source_and_opens_callback_stub(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "UserCodeCreateDemo"
        project = _create_project(project_dir, "UserCodeCreateDemo", sdk_root)
        slider = WidgetModel("slider", name="volume_slider", x=16, y=16, width=160, height=24)
        slider.events["onValueChanged"] = "on_volume_changed"
        project.get_startup_page().root_widget.add_child(slider)
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)
        opened = []
        monkeypatch.setattr(window, "_open_path_in_default_app", lambda path: opened.append(path) or True)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        window._on_user_code_requested("on_volume_changed", "void {func_name}(egui_view_t *self, uint8_t value)")

        source_path = project_dir / "main_page.c"
        assert opened == [str(source_path)]
        assert source_path.exists() is True
        content = source_path.read_text(encoding="utf-8")
        assert "void on_volume_changed(egui_view_t *self, uint8_t value)" in content
        assert window.statusBar().currentMessage() == "Opened user code: main_page.c (on_volume_changed)."
        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_user_code_request_updates_existing_page_source_with_missing_callback_stub(
        self, qapp, isolated_config, tmp_path, monkeypatch
    ):
        from ui_designer.generator.code_generator import generate_page_user_source
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "UserCodeUpdateDemo"
        project = _create_project(project_dir, "UserCodeUpdateDemo", sdk_root)
        page = project.get_startup_page()
        source_path = project_dir / "main_page.c"
        source_path.write_text(generate_page_user_source(page, project), encoding="utf-8")

        timer = {"name": "refresh_timer", "callback": "tick_refresh", "delay_ms": "500", "period_ms": "1000", "auto_start": True}
        page.timers = [timer]
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)
        refresh_calls = []
        monkeypatch.setattr(window, "_refresh_project_watch_snapshot", lambda: refresh_calls.append("refreshed"))
        monkeypatch.setattr(window, "_open_path_in_default_app", lambda path: True)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        refresh_calls.clear()
        window._on_user_code_requested("tick_refresh", "void {func_name}(egui_timer_t *timer)")

        content = source_path.read_text(encoding="utf-8")
        assert "void tick_refresh(egui_timer_t *timer)" in content
        assert "EGUI_UNUSED(local);" in content
        assert refresh_calls
        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_property_panel_multi_selection_callback_edit_updates_widgets_and_dirty_state(
        self, qapp, isolated_config, tmp_path, monkeypatch
    ):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "BatchEventCallbackDemo"
        project = _create_project(project_dir, "BatchEventCallbackDemo", sdk_root)
        first = WidgetModel("slider", name="volume_slider_a", x=16, y=16, width=160, height=24)
        second = WidgetModel("slider", name="volume_slider_b", x=16, y=48, width=160, height=24)
        project.get_startup_page().root_widget.add_child(first)
        project.get_startup_page().root_widget.add_child(second)
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        window._set_selection([first, second], primary=second, sync_tree=True, sync_preview=False)

        editor = window.property_panel._editors["callback_onValueChanged"]
        editor.setText("on_shared_volume_changed")
        editor.editingFinished.emit()

        assert first.events["onValueChanged"] == "on_shared_volume_changed"
        assert second.events["onValueChanged"] == "on_shared_volume_changed"
        assert window._undo_manager.is_any_dirty() is True
        assert window.statusBar().currentMessage() == "Changed main_page: property edit."
        assert window._current_page.to_xml_string().count('onValueChanged="on_shared_volume_changed"') == 2
        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_resource_panel_feedback_signal_updates_status_bar(self, qapp, isolated_config):
        from ui_designer.ui.main_window import MainWindow

        window = MainWindow("")

        window.res_panel.feedback_message.emit("Restored image resources: 2 restored.")

        assert window.statusBar().currentMessage() == "Restored image resources: 2 restored."
        _close_window(window)

    def test_focus_missing_resource_updates_main_window_status(self, qapp, isolated_config, tmp_path):
        from ui_designer.model.resource_catalog import ResourceCatalog
        from ui_designer.ui.main_window import MainWindow

        resource_dir = tmp_path / "project" / ".eguiproject" / "resources"
        images_dir = resource_dir / "images"
        images_dir.mkdir(parents=True)
        (images_dir / "present.png").write_bytes(b"PNG")

        catalog = ResourceCatalog()
        catalog.add_image("missing.png")
        catalog.add_image("present.png")

        window = MainWindow("")
        window.res_panel.set_resource_dir(str(resource_dir))
        window.res_panel.set_resource_catalog(catalog)

        focused = window.res_panel._focus_missing_resource("image")

        assert focused == "missing.png"
        assert window.statusBar().currentMessage() == "Focused missing image resource 1/1: missing.png."
        _close_window(window)

    def test_load_background_image_uses_existing_mockup_dir_as_initial_directory(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "MockupDemo"
        project = _create_project(project_dir, "MockupDemo", sdk_root)
        mockup_dir = project_dir / ".eguiproject" / "mockup"
        mockup_dir.mkdir(parents=True, exist_ok=True)
        (mockup_dir / "existing.png").write_bytes(b"PNG")
        project.get_startup_page().mockup_image_path = "mockup/existing.png"
        captured = {}

        window = MainWindow(str(sdk_root))
        window.project = project
        window._project_dir = str(project_dir)
        window._current_page = project.get_startup_page()

        def fake_get_open_file_name(parent, title, directory, filters):
            captured["title"] = title
            captured["directory"] = directory
            captured["filters"] = filters
            return "", ""

        monkeypatch.setattr("ui_designer.ui.main_window.QFileDialog.getOpenFileName", fake_get_open_file_name)

        window._load_background_image()

        assert captured["title"] == "Load Mockup Image"
        assert captured["directory"] == os.path.normpath(os.path.abspath(mockup_dir))
        assert "Images" in captured["filters"]
        _close_window(window)

    def test_load_background_image_falls_back_to_project_dir_when_mockup_dir_missing(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "MockupDemo"
        project = _create_project(project_dir, "MockupDemo", sdk_root)
        captured = {}

        window = MainWindow(str(sdk_root))
        window.project = project
        window._project_dir = str(project_dir)
        window._current_page = project.get_startup_page()

        def fake_get_open_file_name(parent, title, directory, filters):
            captured["directory"] = directory
            return "", ""

        monkeypatch.setattr("ui_designer.ui.main_window.QFileDialog.getOpenFileName", fake_get_open_file_name)

        window._load_background_image()

        assert captured["directory"] == os.path.normpath(os.path.abspath(project_dir))
        _close_window(window)

    def test_xml_edit_updates_page_mockup_metadata(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "XmlMockupDemo"
        project = _create_project(project_dir, "XmlMockupDemo", sdk_root)
        xml_page = project.get_startup_page()
        xml_page.mockup_image_path = "mockup/design.png"
        xml_page.mockup_image_visible = False
        xml_page.mockup_image_opacity = 0.45
        xml_text = xml_page.to_xml_string()
        xml_page.mockup_image_path = ""
        xml_page.mockup_image_visible = True
        xml_page.mockup_image_opacity = 0.3
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        window._on_xml_changed(xml_text)

        assert window._current_page.mockup_image_path == "mockup/design.png"
        assert window._current_page.mockup_image_visible is False
        assert window._current_page.mockup_image_opacity == 0.45
        assert window._undo_manager.is_any_dirty() is True
        assert window.history_panel._source_value.text() == "Source: xml edit"
        history_items = [window.history_panel._history_list.item(i).text() for i in range(window.history_panel._history_list.count())]
        assert any("xml edit" in item and "Current" in item for item in history_items)
        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_toggle_background_image_marks_dirty_and_supports_undo(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "MockupUndoDemo"
        project = _create_project(project_dir, "MockupUndoDemo", sdk_root)
        page = project.get_startup_page()
        page.mockup_image_path = "mockup/design.png"
        page.mockup_image_visible = True
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        assert window._undo_manager.is_any_dirty() is False

        window._toggle_background_image(False)

        assert window._current_page.mockup_image_visible is False
        assert window._undo_manager.is_any_dirty() is True
        assert window.history_panel._dirty_value.text() == "Dirty: Yes"
        assert window.history_panel._source_value.text() == "Source: mockup visibility"

        window._undo()

        assert window._current_page.mockup_image_visible is True
        assert window._undo_manager.is_any_dirty() is False
        assert window.history_panel._dirty_value.text() == "Dirty: No"
        assert window.history_panel._source_value.text() == "Source: Saved state"
        _close_window(window)

    def test_resource_rename_updates_widget_references_across_pages(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "RenameResourceDemo"
        project = _create_project(project_dir, "RenameResourceDemo", sdk_root)
        detail_page = project.create_new_page("detail_page")

        image_a = WidgetModel("image", name="image_a")
        image_a.properties["image_file"] = "star.png"
        project.get_page_by_name("main_page").root_widget.add_child(image_a)

        image_b = WidgetModel("image", name="image_b")
        image_b.properties["image_file"] = "star.png"
        detail_page.root_widget.add_child(image_b)

        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        window._selected_widget = image_a
        window.property_panel.set_widget(image_a)

        window._on_resource_renamed("image", "star.png", "star_new.png")

        assert image_a.properties["image_file"] == "star_new.png"
        assert image_b.properties["image_file"] == "star_new.png"
        assert window._undo_manager.get_stack("main_page").is_dirty() is True
        assert window._undo_manager.get_stack("detail_page").is_dirty() is True
        assert window.statusBar().currentMessage() == "Updated resources in 2 pages: image resource rename."
        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_replace_missing_resource_updates_widget_references_across_pages(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "ReplaceMissingResourceDemo"
        project = _create_project(project_dir, "ReplaceMissingResourceDemo", sdk_root)
        detail_page = project.create_new_page("detail_page")
        project.resource_catalog.add_image("star.png")

        image_a = WidgetModel("image", name="image_a")
        image_a.properties["image_file"] = "star.png"
        project.get_page_by_name("main_page").root_widget.add_child(image_a)

        image_b = WidgetModel("image", name="image_b")
        image_b.properties["image_file"] = "star.png"
        detail_page.root_widget.add_child(image_b)

        project.save(str(project_dir))

        replacement_dir = tmp_path / "external_images"
        replacement_dir.mkdir()
        replacement_path = replacement_dir / "star_new.png"
        replacement_path.write_bytes(b"PNG")

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        window._selected_widget = image_a
        window.property_panel.set_widget(image_a)

        restored, renamed, failures = window.res_panel._replace_missing_resources_from_mapping(
            "image",
            {"star.png": str(replacement_path)},
        )

        assert restored == []
        assert renamed == [("star.png", "star_new.png")]
        assert failures == []
        assert image_a.properties["image_file"] == "star_new.png"
        assert image_b.properties["image_file"] == "star_new.png"
        assert window.project.resource_catalog.has_image("star_new.png") is True
        assert window.project.resource_catalog.has_image("star.png") is False
        assert window._undo_manager.get_stack("main_page").is_dirty() is True
        assert window._undo_manager.get_stack("detail_page").is_dirty() is True
        assert window.statusBar().currentMessage() == "Replaced image resources: 1 renamed."
        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_replace_missing_resources_batch_preview_confirmation_updates_widget_references_across_pages(
        self, qapp, isolated_config, tmp_path, monkeypatch
    ):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "ReplaceMissingBatchPreviewDemo"
        project = _create_project(project_dir, "ReplaceMissingBatchPreviewDemo", sdk_root)
        detail_page = project.create_new_page("detail_page")
        project.resource_catalog.add_image("star.png")

        image_a = WidgetModel("image", name="image_a")
        image_a.properties["image_file"] = "star.png"
        project.get_page_by_name("main_page").root_widget.add_child(image_a)

        image_b = WidgetModel("image", name="image_b")
        image_b.properties["image_file"] = "star.png"
        detail_page.root_widget.add_child(image_b)

        project.save(str(project_dir))

        replacement_dir = tmp_path / "external_images"
        replacement_dir.mkdir()
        replacement_path = replacement_dir / "star_new.png"
        replacement_path.write_bytes(b"PNG")

        preview_calls = []

        class FakeDialog:
            def __init__(self, missing_names, source_paths, parent=None):
                assert missing_names == ["star.png"]
                assert source_paths == [str(replacement_path)]

            def exec_(self):
                return 1

            def selected_mapping(self):
                return {"star.png": str(replacement_path)}

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)
        monkeypatch.setattr(
            "ui_designer.ui.resource_panel.QFileDialog.getOpenFileNames",
            lambda *args, **kwargs: ([str(replacement_path)], "Images (*.png *.bmp *.jpg *.jpeg)"),
        )
        monkeypatch.setattr("ui_designer.ui.resource_panel._MissingResourceReplaceDialog", FakeDialog)
        monkeypatch.setattr(
            window.res_panel,
            "_confirm_batch_replace_impact",
            lambda resource_type, impacts, total_rename_count: preview_calls.append((resource_type, impacts, total_rename_count)) or True,
        )

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        assert window.res_panel._missing_resource_names("image") == ["star.png"]
        window._selected_widget = image_a
        window.property_panel.set_widget(image_a)

        window.res_panel._replace_missing_resources("image")

        assert len(preview_calls) == 1
        assert preview_calls[0][0] == "image"
        assert preview_calls[0][2] == 1
        assert preview_calls[0][1][0]["old_name"] == "star.png"
        assert preview_calls[0][1][0]["new_name"] == "star_new.png"
        assert image_a.properties["image_file"] == "star_new.png"
        assert image_b.properties["image_file"] == "star_new.png"
        assert window.project.resource_catalog.has_image("star_new.png") is True
        assert window.project.resource_catalog.has_image("star.png") is False
        assert window._undo_manager.get_stack("main_page").is_dirty() is True
        assert window._undo_manager.get_stack("detail_page").is_dirty() is True
        assert window.statusBar().currentMessage() == "Replaced image resources: 1 renamed."
        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_resource_panel_rename_preserves_specific_status_message(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "RenameResourceSignalDemo"
        project = _create_project(project_dir, "RenameResourceSignalDemo", sdk_root)
        detail_page = project.create_new_page("detail_page")
        project.resource_catalog.add_image("star.png")

        images_dir = project_dir / ".eguiproject" / "resources" / "images"
        images_dir.mkdir(parents=True, exist_ok=True)
        (images_dir / "star.png").write_bytes(b"PNG")

        image_a = WidgetModel("image", name="image_a")
        image_a.properties["image_file"] = "star.png"
        project.get_page_by_name("main_page").root_widget.add_child(image_a)

        image_b = WidgetModel("image", name="image_b")
        image_b.properties["image_file"] = "star.png"
        detail_page.root_widget.add_child(image_b)

        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)
        monkeypatch.setattr(
            "ui_designer.ui.resource_panel.QInputDialog.getText",
            lambda *args, **kwargs: ("star_new.png", True),
        )
        monkeypatch.setattr(window.res_panel, "_confirm_reference_impact", lambda *args: True)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        window._selected_widget = image_a
        window.property_panel.set_widget(image_a)

        window.res_panel._rename_resource("star.png", "image")

        assert image_a.properties["image_file"] == "star_new.png"
        assert image_b.properties["image_file"] == "star_new.png"
        assert window.statusBar().currentMessage() == "Updated resources in 2 pages: image resource rename."
        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_resource_delete_clears_widget_references(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "DeleteResourceDemo"
        project = _create_project(project_dir, "DeleteResourceDemo", sdk_root)
        label = WidgetModel("label", name="title")
        label.properties["font_file"] = "demo.ttf"
        project.get_page_by_name("main_page").root_widget.add_child(label)
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)
        monkeypatch.setattr(window.property_panel, "set_selection", lambda *args, **kwargs: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        window._selected_widget = label
        window.property_panel.set_widget(label)

        window._on_resource_deleted("font", "demo.ttf")

        assert label.properties["font_file"] == ""
        assert window._undo_manager.get_stack("main_page").is_dirty() is True
        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_resource_selected_assigns_text_file_to_selected_widget(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "AssignTextResourceDemo"
        project = _create_project(project_dir, "AssignTextResourceDemo", sdk_root)
        label = WidgetModel("label", name="title")
        label.properties["font_file"] = "demo.ttf"
        project.get_page_by_name("main_page").root_widget.add_child(label)
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        window._selected_widget = label
        window.property_panel.set_widget(label)

        window._on_resource_selected("text", "chars.txt")

        assert label.properties["font_text_file"] == "chars.txt"
        assert window._undo_manager.get_stack("main_page").is_dirty() is True
        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_resource_rename_updates_text_references_across_pages(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "RenameTextResourceDemo"
        project = _create_project(project_dir, "RenameTextResourceDemo", sdk_root)
        detail_page = project.create_new_page("detail_page")

        label_a = WidgetModel("label", name="label_a")
        label_a.properties["font_file"] = "demo.ttf"
        label_a.properties["font_text_file"] = "chars.txt"
        project.get_page_by_name("main_page").root_widget.add_child(label_a)

        label_b = WidgetModel("label", name="label_b")
        label_b.properties["font_file"] = "demo.ttf"
        label_b.properties["font_text_file"] = "chars.txt"
        detail_page.root_widget.add_child(label_b)

        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        window._selected_widget = label_a
        window.property_panel.set_widget(label_a)

        window._on_resource_renamed("text", "chars.txt", "chars_new.txt")

        assert label_a.properties["font_text_file"] == "chars_new.txt"
        assert label_b.properties["font_text_file"] == "chars_new.txt"
        assert window._undo_manager.get_stack("main_page").is_dirty() is True
        assert window._undo_manager.get_stack("detail_page").is_dirty() is True
        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_resource_delete_clears_text_references(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "DeleteTextResourceDemo"
        project = _create_project(project_dir, "DeleteTextResourceDemo", sdk_root)
        label = WidgetModel("label", name="title")
        label.properties["font_file"] = "demo.ttf"
        label.properties["font_text_file"] = "chars.txt"
        project.get_page_by_name("main_page").root_widget.add_child(label)
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        window._selected_widget = label
        window.property_panel.set_widget(label)

        window._on_resource_deleted("text", "chars.txt")

        assert label.properties["font_text_file"] == ""
        assert window._undo_manager.get_stack("main_page").is_dirty() is True
        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_string_key_delete_rewrites_widget_text_refs_to_default_literal(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.string_resource import DEFAULT_LOCALE
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "DeleteStringKeyDemo"
        project = _create_project(project_dir, "DeleteStringKeyDemo", sdk_root)
        detail_page = project.create_new_page("detail_page")
        project.string_catalog.set("greeting", "Hello", DEFAULT_LOCALE)
        project.string_catalog.set("greeting", "Ni Hao", "zh")

        title = WidgetModel("label", name="title")
        title.properties["text"] = "@string/greeting"
        project.get_page_by_name("main_page").root_widget.add_child(title)

        subtitle = WidgetModel("label", name="subtitle")
        subtitle.properties["text"] = "@string/greeting"
        detail_page.root_widget.add_child(subtitle)
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)
        monkeypatch.setattr(window.res_panel, "_confirm_reference_impact", lambda *args: True)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        window.res_panel._select_resource_item("string", "greeting")

        window.res_panel._on_remove_string_key()

        assert title.properties["text"] == "Hello"
        assert subtitle.properties["text"] == "Hello"
        assert "greeting" not in window.project.string_catalog.all_keys
        assert window._undo_manager.get_stack("main_page").is_dirty() is True
        assert window._undo_manager.get_stack("detail_page").is_dirty() is True
        assert window.statusBar().currentMessage() == "Updated resources in 2 pages: string key delete."
        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_string_key_rename_updates_widget_text_refs_across_pages(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.string_resource import DEFAULT_LOCALE
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "RenameStringKeyDemo"
        project = _create_project(project_dir, "RenameStringKeyDemo", sdk_root)
        detail_page = project.create_new_page("detail_page")
        project.string_catalog.set("greeting", "Hello", DEFAULT_LOCALE)
        project.string_catalog.set("greeting", "Ni Hao", "zh")

        title = WidgetModel("label", name="title")
        title.properties["text"] = "@string/greeting"
        project.get_page_by_name("main_page").root_widget.add_child(title)

        subtitle = WidgetModel("label", name="subtitle")
        subtitle.properties["text"] = "@string/greeting"
        detail_page.root_widget.add_child(subtitle)
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)
        monkeypatch.setattr(
            "ui_designer.ui.resource_panel.QInputDialog.getText",
            lambda *args, **kwargs: ("salutation", True),
        )
        monkeypatch.setattr(window.res_panel, "_confirm_reference_impact", lambda *args: True)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        window.res_panel._select_resource_item("string", "greeting")

        window.res_panel._on_rename_string_key()

        assert title.properties["text"] == "@string/salutation"
        assert subtitle.properties["text"] == "@string/salutation"
        assert window.project.string_catalog.get("salutation", DEFAULT_LOCALE) == "Hello"
        assert "greeting" not in window.project.string_catalog.all_keys
        assert window._undo_manager.get_stack("main_page").is_dirty() is True
        assert window._undo_manager.get_stack("detail_page").is_dirty() is True
        assert window.statusBar().currentMessage() == "Updated resources in 2 pages: string key rename."
        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_string_key_delete_can_focus_usage_without_deleting(self, tmp_path):
        repo_root = Path(__file__).resolve().parents[4]
        script = textwrap.dedent(
            f"""
            import os
            import shutil
            import sys
            import tempfile
            from pathlib import Path

            os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

            repo_root = Path({repr(str(repo_root))})
            sys.path.insert(0, str(repo_root / "scripts"))

            from PyQt5.QtWidgets import QApplication

            from ui_designer.model.project import Project
            from ui_designer.model.string_resource import DEFAULT_LOCALE
            from ui_designer.model.widget_model import WidgetModel
            from ui_designer.ui.main_window import MainWindow


            def create_sdk_root(root: Path):
                (root / "src").mkdir(parents=True)
                (root / "porting" / "designer").mkdir(parents=True)
                (root / "Makefile").write_text("all:\\n", encoding="utf-8")


            def create_project(project_dir: Path, app_name: str, sdk_root: Path):
                project = Project(screen_width=240, screen_height=320, app_name=app_name)
                project.sdk_root = str(sdk_root)
                project.project_dir = str(project_dir)
                project.create_new_page("main_page")
                project.save(str(project_dir))
                return project


            class DisabledCompiler:
                def can_build(self):
                    return False

                def is_preview_running(self):
                    return False

                def stop_exe(self):
                    return None

                def cleanup(self):
                    return None

                def get_build_error(self):
                    return "preview disabled for test"

                def set_screen_size(self, width, height):
                    return None

                def is_exe_ready(self):
                    return False


            temp_root = Path(tempfile.mkdtemp(prefix="ui_designer_string_delete_inspect_", dir=str(repo_root)))
            app = QApplication.instance() or QApplication([])
            try:
                sdk_root = temp_root / "sdk"
                create_sdk_root(sdk_root)
                project_dir = temp_root / "DeleteStringKeyInspectDemo"
                project = create_project(project_dir, "DeleteStringKeyInspectDemo", sdk_root)
                detail_page = project.create_new_page("detail_page")
                project.string_catalog.set("greeting", "Hello", DEFAULT_LOCALE)

                subtitle = WidgetModel("label", name="subtitle")
                subtitle.properties["text"] = "@string/greeting"
                detail_page.root_widget.add_child(subtitle)
                project.save(str(project_dir))

                window = MainWindow(str(sdk_root))
                window._recreate_compiler = lambda _window=window: setattr(_window, "compiler", DisabledCompiler())
                window._trigger_compile = lambda: None

                def inspect_usage(*args):
                    window.res_panel.usage_activated.emit("detail_page", "subtitle")
                    return False

                window.res_panel._confirm_reference_impact = inspect_usage

                window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
                assert window._current_page.name == "main_page"
                window.res_panel._select_resource_item("string", "greeting")

                window.res_panel._on_remove_string_key()

                assert window._current_page.name == "detail_page"
                assert window._selection_state.primary is subtitle
                assert window.project.string_catalog.get("greeting", DEFAULT_LOCALE) == "Hello"
                assert subtitle.properties["text"] == "@string/greeting"
                assert window.statusBar().currentMessage() == "Focused resource usage: detail_page/subtitle."
                assert window._undo_manager.is_any_dirty() is False

                window._undo_manager.mark_all_saved()
                window.close()
                window.deleteLater()
                app.sendPostedEvents()
                app.processEvents()
            finally:
                shutil.rmtree(temp_root, ignore_errors=True)
            """
        )

        env = os.environ.copy()
        env.setdefault("QT_QPA_PLATFORM", "offscreen")
        result = subprocess.run(
            [sys.executable, "-c", script],
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            env=env,
            timeout=60,
        )

        assert result.returncode == 0, f"stdout:\\n{result.stdout}\\n\\nstderr:\\n{result.stderr}"

    def test_string_key_usage_activation_switches_page_and_selects_widget(self, tmp_path):
        repo_root = Path(__file__).resolve().parents[4]
        script = textwrap.dedent(
            f"""
            import os
            import shutil
            import sys
            import tempfile
            from pathlib import Path

            os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

            repo_root = Path({repr(str(repo_root))})
            sys.path.insert(0, str(repo_root / "scripts"))

            from PyQt5.QtWidgets import QApplication

            from ui_designer.model.project import Project
            from ui_designer.model.string_resource import DEFAULT_LOCALE
            from ui_designer.model.widget_model import WidgetModel
            from ui_designer.ui.main_window import MainWindow


            def create_sdk_root(root: Path):
                (root / "src").mkdir(parents=True)
                (root / "porting" / "designer").mkdir(parents=True)
                (root / "Makefile").write_text("all:\\n", encoding="utf-8")


            def create_project(project_dir: Path, app_name: str, sdk_root: Path):
                project = Project(screen_width=240, screen_height=320, app_name=app_name)
                project.sdk_root = str(sdk_root)
                project.project_dir = str(project_dir)
                project.create_new_page("main_page")
                project.save(str(project_dir))
                return project


            class DisabledCompiler:
                def can_build(self):
                    return False

                def is_preview_running(self):
                    return False

                def stop_exe(self):
                    return None

                def cleanup(self):
                    return None

                def get_build_error(self):
                    return "preview disabled for test"

                def set_screen_size(self, width, height):
                    return None

                def is_exe_ready(self):
                    return False


            temp_root = Path(tempfile.mkdtemp(prefix="ui_designer_string_usage_", dir=str(repo_root)))
            app = QApplication.instance() or QApplication([])
            try:
                sdk_root = temp_root / "sdk"
                create_sdk_root(sdk_root)
                project_dir = temp_root / "StringUsageNavigationDemo"
                project = create_project(project_dir, "StringUsageNavigationDemo", sdk_root)
                detail_page = project.create_new_page("detail_page")
                project.string_catalog.set("greeting", "Hello", DEFAULT_LOCALE)

                subtitle = WidgetModel("label", name="subtitle")
                subtitle.properties["text"] = "@string/greeting"
                detail_page.root_widget.add_child(subtitle)
                project.save(str(project_dir))

                window = MainWindow(str(sdk_root))
                window._recreate_compiler = lambda _window=window: setattr(_window, "compiler", DisabledCompiler())
                window._trigger_compile = lambda: None

                window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
                assert window._current_page.name == "main_page"

                window.res_panel._select_resource_item("string", "greeting")
                window.res_panel._on_usage_item_activated(window.res_panel._usage_table.item(0, 0))

                assert window._current_page.name == "detail_page"
                assert window._selection_state.primary is subtitle
                assert window.statusBar().currentMessage() == "Focused resource usage: detail_page/subtitle."

                window._undo_manager.mark_all_saved()
                window.close()
                window.deleteLater()
                app.sendPostedEvents()
                app.processEvents()
            finally:
                shutil.rmtree(temp_root, ignore_errors=True)
            """
        )

        env = os.environ.copy()
        env.setdefault("QT_QPA_PLATFORM", "offscreen")
        result = subprocess.run(
            [sys.executable, "-c", script],
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            env=env,
            timeout=60,
        )

        assert result.returncode == 0, f"stdout:\\n{result.stdout}\\n\\nstderr:\\n{result.stderr}"

    def test_resource_usage_activation_switches_page_and_selects_widget(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "ResourceUsageNavigationDemo"
        project = _create_project(project_dir, "ResourceUsageNavigationDemo", sdk_root)
        detail_page = project.create_new_page("detail_page")
        project.resource_catalog.add_image("star.png")

        hero = WidgetModel("image", name="hero")
        hero.properties["image_file"] = "star.png"
        detail_page.root_widget.add_child(hero)
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)

        assert window._current_page.name == "main_page"

        window.res_panel._select_resource_item("image", "star.png")
        window.res_panel._on_usage_item_activated(window.res_panel._usage_table.item(0, 0))

        assert window._current_page.name == "detail_page"
        assert window._selection_state.primary is hero
        assert window._selection_state.widgets == [hero]
        assert window.statusBar().currentMessage() == "Focused resource usage: detail_page/hero."
        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_resource_usage_filter_tracks_current_page(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "ResourceUsageFilterDemo"
        project = _create_project(project_dir, "ResourceUsageFilterDemo", sdk_root)
        detail_page = project.create_new_page("detail_page")
        project.resource_catalog.add_image("star.png")

        hero_main = WidgetModel("image", name="hero_main")
        hero_main.properties["image_file"] = "star.png"
        project.get_page_by_name("main_page").root_widget.add_child(hero_main)

        hero_detail = WidgetModel("image", name="hero_detail")
        hero_detail.properties["image_file"] = "star.png"
        detail_page.root_widget.add_child(hero_detail)
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        window.res_panel._usage_current_page_only.setChecked(True)
        window.res_panel._select_resource_item("image", "star.png")

        assert window._current_page.name == "main_page"
        assert window.res_panel._usage_table.rowCount() == 1
        assert window.res_panel._usage_table.item(0, 0).text() == "main_page"

        window._switch_page("detail_page")

        assert window.res_panel._usage_table.rowCount() == 1
        assert window.res_panel._usage_table.item(0, 0).text() == "detail_page"
        assert window.res_panel._usage_summary.text() == "'star.png' is used by 1 widget on the current page (2 total across 2 pages)."
        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_poll_project_files_auto_reloads_clean_project(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "WatchDemo"
        project = _create_project(project_dir, "WatchDemo", sdk_root)

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)

        reload_calls = []
        monkeypatch.setattr(
            window,
            "_reload_project_from_disk",
            lambda *args, **kwargs: reload_calls.append(kwargs) or True,
        )

        layout_file = project_dir / ".eguiproject" / "layout" / "main_page.xml"
        layout_file.write_text(layout_file.read_text(encoding="utf-8") + "\n<!-- external -->\n", encoding="utf-8")

        window._poll_project_files()

        assert len(reload_calls) == 1
        assert reload_calls[0]["auto"] is True
        assert os.path.normpath(os.path.abspath(layout_file)) in reload_calls[0]["changed_paths"]
        _close_window(window)

    def test_poll_project_files_marks_pending_when_project_is_dirty(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "DirtyWatchDemo"
        project = _create_project(project_dir, "DirtyWatchDemo", sdk_root)

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        window._undo_manager.get_stack("main_page").push("<Page dirty='1' />")

        reload_calls = []
        monkeypatch.setattr(
            window,
            "_reload_project_from_disk",
            lambda *args, **kwargs: reload_calls.append(kwargs) or True,
        )

        layout_file = project_dir / ".eguiproject" / "layout" / "main_page.xml"
        layout_file.write_text(layout_file.read_text(encoding="utf-8") + "\n<!-- dirty external -->\n", encoding="utf-8")

        window._poll_project_files()

        assert reload_calls == []
        assert window._external_reload_pending is True
        assert "External project changes detected" in window.statusBar().currentMessage()
        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_reload_project_from_disk_preserves_current_page(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.project import Project
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "ReloadDemo"

        project = Project(screen_width=240, screen_height=320, app_name="ReloadDemo")
        project.sdk_root = str(sdk_root)
        project.project_dir = str(project_dir)
        project.create_new_page("main_page")
        project.create_new_page("detail_page")
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        window._switch_page("detail_page")

        reloaded = Project.load(str(project_dir))
        reloaded.startup_page = "main_page"
        reloaded.create_new_page("summary_page")
        reloaded.save(str(project_dir))

        assert window._reload_project_from_disk() is True
        assert window._current_page is not None
        assert window._current_page.name == "detail_page"
        assert window.project.get_page_by_name("summary_page") is not None
        _close_window(window)

    def test_page_navigator_is_populated_and_tracks_current_page(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "NavigatorDemo"
        project = _create_project(project_dir, "NavigatorDemo", sdk_root)
        project.create_new_page("detail_page")
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)

        assert set(window.page_navigator._pages.keys()) == {"main_page", "detail_page"}
        assert window.page_navigator._current_page == "main_page"

        window._switch_page("detail_page")

        assert window.page_navigator._current_page == "detail_page"
        _close_window(window)

    def test_page_navigator_copy_and_template_add_keep_pages_in_sync(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "NavigatorActionsDemo"
        project = _create_project(project_dir, "NavigatorActionsDemo", sdk_root)

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)

        window._duplicate_page_from_navigator("main_page")
        assert window.project.get_page_by_name("main_page_copy") is not None
        assert "main_page_copy" in window.page_navigator._pages
        assert window._current_page.name == "main_page_copy"

        window._on_page_add_from_template("detail", "main_page")
        template_page = window.project.get_page_by_name("detail_page")
        assert template_page is not None
        assert "detail_page" in window.page_navigator._pages
        assert window._current_page.name == "detail_page"
        assert [child.name for child in template_page.root_widget.children] == ["title", "hero_image", "description"]
        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_dirty_page_indicators_sync_across_tabs_navigator_and_project_tree(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "DirtyPagesDemo"
        project = _create_project(project_dir, "DirtyPagesDemo", sdk_root)
        project.create_new_page("detail_page")
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)

        window._undo_manager.get_stack("main_page").push("<Page dirty='main' />")
        window._undo_manager.get_stack("detail_page").push("<Page dirty='detail' />")
        window._update_window_title()

        assert window.page_tab_bar.tabText(0) == "main_page*"
        assert window.page_navigator._thumbnails["main_page"]._name_label.text() == "main_page*"
        assert window.page_navigator._thumbnails["detail_page"]._name_label.text() == "detail_page*"

        texts_by_page = {}
        for i in range(window.project_dock._page_tree.topLevelItemCount()):
            item = window.project_dock._page_tree.topLevelItem(i)
            texts_by_page[item.data(0, Qt.UserRole)] = item.text(0)
        assert texts_by_page["main_page"].endswith("main_page*")
        assert texts_by_page["detail_page"].endswith("detail_page*")

        window._switch_page("detail_page")
        assert window._current_page.name == "detail_page"
        assert any(window.page_tab_bar.tabText(i) == "detail_page*" for i in range(window.page_tab_bar.count()))

        window._undo_manager.mark_all_saved()
        window._update_window_title()

        assert all(not window.page_tab_bar.tabText(i).endswith("*") for i in range(window.page_tab_bar.count()))
        assert window.page_navigator._thumbnails["main_page"]._name_label.text() == "main_page"
        assert window.page_navigator._thumbnails["detail_page"]._name_label.text() == "detail_page"
        _close_window(window)

    def test_copy_and_paste_selection_creates_unique_widget_names(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "ClipboardDemo"
        project = _create_project(project_dir, "ClipboardDemo", sdk_root)
        page = project.get_startup_page()
        label = WidgetModel("label", name="title", x=10, y=10, width=80, height=20)
        page.root_widget.add_child(label)
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)

        def fake_set_selection(widgets=None, primary=None, sync_tree=True, sync_preview=True):
            window._selection_state.set_widgets(widgets or [], primary=primary)
            window._selected_widget = window._selection_state.primary

        monkeypatch.setattr(window, "_set_selection", fake_set_selection)
        window._selected_widget = label

        window._copy_selection()
        window._paste_selection()

        label_names = [child.name for child in page.root_widget.children if child.widget_type == "label"]
        assert label_names == ["title", "title_2"]
        assert window._selection_state.primary.name == "title_2"
        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_diagnostics_panel_lists_page_issues_and_selection_notes(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "DiagnosticsDemo"
        project = _create_project(project_dir, "DiagnosticsDemo", sdk_root)
        page = project.get_startup_page()

        invalid = WidgetModel("label", name="bad-name", x=8, y=8, width=60, height=20)
        duplicate_a = WidgetModel("label", name="dup_name", x=20, y=40, width=60, height=20)
        duplicate_b = WidgetModel("label", name="dup_name", x=230, y=40, width=30, height=20)
        layout_parent = WidgetModel("linearlayout", name="layout_parent", x=0, y=120, width=240, height=80)
        managed = WidgetModel("label", name="managed_widget", x=12, y=8, width=80, height=20)
        managed.designer_locked = True
        managed.designer_hidden = True
        layout_parent.add_child(managed)
        missing = WidgetModel("image", name="missing_image", x=16, y=220, width=48, height=48)
        missing.properties["image_file"] = "missing.png"

        page.root_widget.add_child(invalid)
        page.root_widget.add_child(duplicate_a)
        page.root_widget.add_child(duplicate_b)
        page.root_widget.add_child(layout_parent)
        page.root_widget.add_child(missing)
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
        window._selection_state.set_widgets([managed], primary=managed)
        window._selected_widget = managed
        window._update_diagnostics_panel()

        summary = window.diagnostics_panel._summary_label.text()
        items = [window.diagnostics_panel._list.item(i).text() for i in range(window.diagnostics_panel._list.count())]

        assert window.diagnostics_dock.objectName() == "diagnostics_dock"
        assert summary == "Diagnostics: 3 error(s), 2 warning(s), 3 info item(s)"
        assert any("bad-name" in item and "valid C identifier" in item for item in items)
        assert sum("dup_name" in item and "duplicated" in item for item in items) == 2
        assert any("dup_name" in item and "geometry issues" in item for item in items)
        assert any("missing_image" in item and "missing from the resource catalog" in item for item in items)
        assert any("canvas drag and resize are disabled" in item for item in items)
        assert any("canvas hit testing" in item for item in items)
        assert any("layout-managed by linearlayout" in item for item in items)

        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_diagnostic_request_switches_page_and_selects_widget(self, qapp, isolated_config, tmp_path, monkeypatch):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow

        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "DiagnosticFocusDemo"
        project = _create_project(project_dir, "DiagnosticFocusDemo", sdk_root)
        detail_page = project.create_new_page("detail_page")
        target = WidgetModel("label", name="target", x=16, y=16, width=80, height=20)
        detail_page.root_widget.add_child(target)
        project.save(str(project_dir))

        window = MainWindow(str(sdk_root))
        monkeypatch.setattr(window, "_recreate_compiler", lambda: setattr(window, "compiler", _DisabledCompiler()))
        monkeypatch.setattr(window, "_trigger_compile", lambda: None)

        window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)

        def fake_set_selection(widgets=None, primary=None, sync_tree=True, sync_preview=True):
            window._selection_state.set_widgets(widgets or [], primary=primary)
            window._selected_widget = window._selection_state.primary

        monkeypatch.setattr(window, "_set_selection", fake_set_selection)

        assert window._current_page.name == "main_page"

        window._on_diagnostic_requested("detail_page", "target")

        assert window._current_page.name == "detail_page"
        assert window._selection_state.primary is target
        assert window._selection_state.widgets == [target]

        window._undo_manager.mark_all_saved()
        _close_window(window)

    def test_missing_resource_diagnostic_activation_opens_resource_panel_usage(self, tmp_path):
        repo_root = Path(__file__).resolve().parents[4]
        script = textwrap.dedent(
            f"""
            import os
            import shutil
            import sys
            import tempfile
            from pathlib import Path

            os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

            repo_root = Path({repr(str(repo_root))})
            sys.path.insert(0, str(repo_root / "scripts"))

            from PyQt5.QtWidgets import QApplication

            from ui_designer.model.project import Project
            from ui_designer.model.widget_model import WidgetModel
            from ui_designer.ui.main_window import MainWindow


            def create_sdk_root(root: Path):
                (root / "src").mkdir(parents=True)
                (root / "porting" / "designer").mkdir(parents=True)
                (root / "Makefile").write_text("all:\\n", encoding="utf-8")


            def create_project(project_dir: Path, app_name: str, sdk_root: Path):
                project = Project(screen_width=240, screen_height=320, app_name=app_name)
                project.sdk_root = str(sdk_root)
                project.project_dir = str(project_dir)
                project.create_new_page("main_page")
                project.save(str(project_dir))
                return project


            class DisabledCompiler:
                def can_build(self):
                    return False

                def is_preview_running(self):
                    return False

                def stop_exe(self):
                    return None

                def cleanup(self):
                    return None

                def get_build_error(self):
                    return "preview disabled for test"

                def set_screen_size(self, width, height):
                    return None

                def is_exe_ready(self):
                    return False


            temp_root = Path(tempfile.mkdtemp(prefix="ui_designer_diag_resource_", dir=str(repo_root)))
            app = QApplication.instance() or QApplication([])
            try:
                sdk_root = temp_root / "sdk"
                create_sdk_root(sdk_root)
                project_dir = temp_root / "DiagnosticMissingResourceDemo"
                project = create_project(project_dir, "DiagnosticMissingResourceDemo", sdk_root)
                missing = WidgetModel("image", name="missing_image", x=16, y=16, width=48, height=48)
                missing.properties["image_file"] = "ghost.png"
                project.get_page_by_name("main_page").root_widget.add_child(missing)
                project.save(str(project_dir))

                window = MainWindow(str(sdk_root))
                window._recreate_compiler = lambda _window=window: setattr(_window, "compiler", DisabledCompiler())
                window._trigger_compile = lambda: None

                window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)

                item = window.diagnostics_panel._list.item(0)
                window.diagnostics_panel._on_item_activated(item)

                assert window._selection_state.primary is missing
                assert window.res_panel._current_resource_type == "image"
                assert window.res_panel._current_resource_name == "ghost.png"
                assert window.res_panel._tabs.currentIndex() == 0
                assert window.res_panel._usage_summary.text() == "'ghost.png' is used by 1 widget across 1 page."
                assert window.statusBar().currentMessage() == "Opened diagnostic resource check: image/ghost.png."

                window._undo_manager.mark_all_saved()
                window.close()
                window.deleteLater()
                app.sendPostedEvents()
                app.processEvents()
            finally:
                shutil.rmtree(temp_root, ignore_errors=True)
            """
        )

        env = os.environ.copy()
        env.setdefault("QT_QPA_PLATFORM", "offscreen")
        result = subprocess.run(
            [sys.executable, "-c", script],
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            env=env,
            timeout=60,
        )

        assert result.returncode == 0, f"stdout:\\n{result.stdout}\\n\\nstderr:\\n{result.stderr}"

    def test_missing_string_diagnostic_activation_opens_string_usage(self, tmp_path):
        repo_root = Path(__file__).resolve().parents[4]
        script = textwrap.dedent(
            f"""
            import os
            import shutil
            import sys
            import tempfile
            from pathlib import Path

            os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

            repo_root = Path({repr(str(repo_root))})
            sys.path.insert(0, str(repo_root / "scripts"))

            from PyQt5.QtWidgets import QApplication

            from ui_designer.model.project import Project
            from ui_designer.model.widget_model import WidgetModel
            from ui_designer.ui.main_window import MainWindow


            def create_sdk_root(root: Path):
                (root / "src").mkdir(parents=True)
                (root / "porting" / "designer").mkdir(parents=True)
                (root / "Makefile").write_text("all:\\n", encoding="utf-8")


            def create_project(project_dir: Path, app_name: str, sdk_root: Path):
                project = Project(screen_width=240, screen_height=320, app_name=app_name)
                project.sdk_root = str(sdk_root)
                project.project_dir = str(project_dir)
                project.create_new_page("main_page")
                project.save(str(project_dir))
                return project


            class DisabledCompiler:
                def can_build(self):
                    return False

                def is_preview_running(self):
                    return False

                def stop_exe(self):
                    return None

                def cleanup(self):
                    return None

                def get_build_error(self):
                    return "preview disabled for test"

                def set_screen_size(self, width, height):
                    return None

                def is_exe_ready(self):
                    return False


            temp_root = Path(tempfile.mkdtemp(prefix="ui_designer_diag_string_", dir=str(repo_root)))
            app = QApplication.instance() or QApplication([])
            try:
                sdk_root = temp_root / "sdk"
                create_sdk_root(sdk_root)
                project_dir = temp_root / "DiagnosticMissingStringDemo"
                project = create_project(project_dir, "DiagnosticMissingStringDemo", sdk_root)
                title = WidgetModel("label", name="title", x=16, y=16, width=80, height=20)
                title.properties["text"] = "@string/missing_key"
                project.get_page_by_name("main_page").root_widget.add_child(title)
                project.save(str(project_dir))

                window = MainWindow(str(sdk_root))
                window._recreate_compiler = lambda _window=window: setattr(_window, "compiler", DisabledCompiler())
                window._trigger_compile = lambda: None

                window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)

                item = window.diagnostics_panel._list.item(0)
                window.diagnostics_panel._on_item_activated(item)

                assert window._selection_state.primary is title
                assert window.res_panel._current_resource_type == "string"
                assert window.res_panel._current_resource_name == "missing_key"
                assert window.res_panel._tabs.currentIndex() == 3
                assert window.res_panel._usage_summary.text() == "'missing_key' is used by 1 widget across 1 page."
                assert window.statusBar().currentMessage() == "Opened diagnostic resource check: string/missing_key."

                window._undo_manager.mark_all_saved()
                window.close()
                window.deleteLater()
                app.sendPostedEvents()
                app.processEvents()
            finally:
                shutil.rmtree(temp_root, ignore_errors=True)
            """
        )

        env = os.environ.copy()
        env.setdefault("QT_QPA_PLATFORM", "offscreen")
        result = subprocess.run(
            [sys.executable, "-c", script],
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            env=env,
            timeout=60,
        )

        assert result.returncode == 0, f"stdout:\\n{result.stdout}\\n\\nstderr:\\n{result.stderr}"

    def test_window_state_helpers_roundtrip_with_config_storage(self, qapp, isolated_config, monkeypatch):
        from ui_designer.ui.main_window import MainWindow

        window = MainWindow("")

        saved_geometry = QByteArray(b"geometry-bytes")
        saved_state = QByteArray(b"state-bytes")
        monkeypatch.setattr(window, "saveGeometry", lambda: saved_geometry)
        monkeypatch.setattr(window, "saveState", lambda: saved_state)

        window._save_window_state_to_config()

        assert isolated_config.window_geometry == bytes(saved_geometry.toBase64()).decode("ascii")
        assert isolated_config.window_state == bytes(saved_state.toBase64()).decode("ascii")

        restore_calls = {}
        monkeypatch.setattr(window, "restoreGeometry", lambda data: restore_calls.setdefault("geometry", bytes(data)) or True)
        monkeypatch.setattr(window, "restoreState", lambda data: restore_calls.setdefault("state", bytes(data)) or True)

        window._apply_saved_window_state()

        assert restore_calls["geometry"] == b"geometry-bytes"
        assert restore_calls["state"] == b"state-bytes"
        assert window.project_dock.objectName() == "project_explorer_dock"
        assert window.tree_dock.objectName() == "widget_tree_dock"
        assert window.props_dock.objectName() == "properties_dock"
        assert window.animations_dock.objectName() == "animations_dock"
        assert window.res_dock.objectName() == "resources_dock"
        assert window.history_dock.objectName() == "history_dock"
        assert window.diagnostics_dock.objectName() == "diagnostics_dock"
        assert window.debug_dock.objectName() == "debug_output_dock"
        assert window._toolbar.objectName() == "main_toolbar"
        _close_window(window)

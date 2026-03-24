"""Qt UI tests for WidgetTreePanel name handling."""

import os

import pytest

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

try:
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


def _build_project_with_root():
    from ui_designer.model.project import Project

    project = Project(screen_width=240, screen_height=320, app_name="WidgetTreeDemo")
    project.create_new_page("main_page")
    return project, project.get_startup_page().root_widget


@_skip_no_qt
class TestWidgetTreePanel:
    def test_rename_widget_resolves_duplicate_name(self, qapp, monkeypatch):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.widget_tree import WidgetTreePanel

        project, root = _build_project_with_root()
        first = WidgetModel("label", name="title")
        second = WidgetModel("label", name="subtitle")
        root.add_child(first)
        root.add_child(second)

        panel = WidgetTreePanel()
        panel.set_project(project)

        monkeypatch.setattr(
            "ui_designer.ui.widget_tree.QInputDialog.getText",
            lambda *args, **kwargs: ("title", True),
        )

        panel._rename_widget(second)

        assert first.name == "title"
        assert second.name == "title_2"
        panel.deleteLater()

    def test_add_child_resolves_duplicate_auto_name(self, qapp):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.widget_tree import WidgetTreePanel

        project, root = _build_project_with_root()
        root.add_child(WidgetModel("button", name="button_1"))
        WidgetModel.reset_counter()

        panel = WidgetTreePanel()
        panel.set_project(project)
        panel._add_child_to(root, "button")

        names = [child.name for child in root.children if child.widget_type == "button"]
        assert names == ["button_1", "button_2"]
        panel.deleteLater()

    def test_rename_widget_rejects_invalid_identifier(self, qapp, monkeypatch):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.widget_tree import WidgetTreePanel

        project, root = _build_project_with_root()
        widget = WidgetModel("label", name="title")
        root.add_child(widget)

        panel = WidgetTreePanel()
        panel.set_project(project)
        warnings = []

        monkeypatch.setattr(
            "ui_designer.ui.widget_tree.QInputDialog.getText",
            lambda *args, **kwargs: ("123 bad-name", True),
        )
        monkeypatch.setattr("ui_designer.ui.widget_tree.QMessageBox.warning", lambda *args: warnings.append(args[1:]))

        panel._rename_widget(widget)

        assert widget.name == "title"
        assert warnings
        assert warnings[0][0] == "Invalid Widget Name"
        panel.deleteLater()

    def test_delete_selected_parent_and_child_removes_only_top_level_once(self, qapp):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.widget_tree import WidgetTreePanel

        project, root = _build_project_with_root()
        container = WidgetModel("group", name="container")
        child = WidgetModel("label", name="title")
        container.add_child(child)
        root.add_child(container)

        panel = WidgetTreePanel()
        panel.set_project(project)
        panel.set_selected_widgets([container, child], primary=container)

        panel._on_delete_clicked()

        assert root.children == []
        panel.deleteLater()

    def test_delete_selected_skips_locked_widgets_and_emits_feedback(self, qapp):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.widget_tree import WidgetTreePanel

        project, root = _build_project_with_root()
        removable = WidgetModel("label", name="removable")
        locked = WidgetModel("label", name="locked_widget")
        locked.designer_locked = True
        root.add_child(removable)
        root.add_child(locked)

        panel = WidgetTreePanel()
        panel.set_project(project)
        feedback = []
        panel.feedback_message.connect(lambda message: feedback.append(message))
        panel.set_selected_widgets([removable, locked], primary=removable)

        panel._on_delete_clicked()

        assert removable not in root.children
        assert locked in root.children
        assert feedback == ["Deleted 1 widget(s); skipped 1 locked widget"]
        panel.deleteLater()

    def test_delete_widget_blocks_locked_widget_and_emits_feedback(self, qapp):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.widget_tree import WidgetTreePanel

        project, root = _build_project_with_root()
        locked = WidgetModel("label", name="locked_widget")
        locked.designer_locked = True
        root.add_child(locked)

        panel = WidgetTreePanel()
        panel.set_project(project)
        feedback = []
        panel.feedback_message.connect(lambda message: feedback.append(message))

        panel._delete_widget(locked)

        assert locked in root.children
        assert feedback == ["Cannot delete widget: locked_widget is locked."]
        panel.deleteLater()

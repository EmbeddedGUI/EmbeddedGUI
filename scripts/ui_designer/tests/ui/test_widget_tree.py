"""Qt UI tests for WidgetTreePanel name handling."""

import os

import pytest

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

try:
    from PyQt5.QtCore import QEvent, Qt
    from PyQt5.QtGui import QKeyEvent
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

    def test_rename_widget_emits_feedback_when_duplicate_name_is_resolved(self, qapp, monkeypatch):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.widget_tree import WidgetTreePanel

        project, root = _build_project_with_root()
        first = WidgetModel("label", name="title")
        second = WidgetModel("label", name="subtitle")
        root.add_child(first)
        root.add_child(second)

        panel = WidgetTreePanel()
        panel.set_project(project)
        feedback = []
        panel.feedback_message.connect(lambda message: feedback.append(message))

        monkeypatch.setattr(
            "ui_designer.ui.widget_tree.QInputDialog.getText",
            lambda *args, **kwargs: ("title", True),
        )

        panel._rename_widget(second)

        assert second.name == "title_2"
        assert feedback == ["Widget name 'title' already exists. Renamed to 'title_2'."]
        panel.deleteLater()

    def test_rename_selected_widgets_applies_prefix_and_emits_feedback(self, qapp, monkeypatch):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.widget_tree import WidgetTreePanel

        project, root = _build_project_with_root()
        existing = WidgetModel("label", name="field_1")
        first = WidgetModel("label", name="title")
        second = WidgetModel("button", name="cta")
        root.add_child(existing)
        root.add_child(first)
        root.add_child(second)

        panel = WidgetTreePanel()
        panel.set_project(project)
        feedback = []
        panel.feedback_message.connect(lambda message: feedback.append(message))
        panel.set_selected_widgets([first, second], primary=first)

        monkeypatch.setattr(
            "ui_designer.ui.widget_tree.QInputDialog.getText",
            lambda *args, **kwargs: ("field", True),
        )

        panel._rename_selected_widgets()

        assert first.name == "field_2"
        assert second.name == "field_3"
        assert panel.selected_widgets() == [first, second]
        assert panel._get_selected_widget() is first
        assert feedback == ["Renamed 2 widget(s) with prefix 'field'."]
        panel.deleteLater()

    def test_rename_selected_widgets_rejects_invalid_prefix(self, qapp, monkeypatch):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.widget_tree import WidgetTreePanel

        project, root = _build_project_with_root()
        first = WidgetModel("label", name="title")
        second = WidgetModel("button", name="cta")
        root.add_child(first)
        root.add_child(second)

        panel = WidgetTreePanel()
        panel.set_project(project)
        warnings = []
        panel.set_selected_widgets([first, second], primary=first)

        monkeypatch.setattr(
            "ui_designer.ui.widget_tree.QInputDialog.getText",
            lambda *args, **kwargs: ("123 bad", True),
        )
        monkeypatch.setattr("ui_designer.ui.widget_tree.QMessageBox.warning", lambda *args: warnings.append(args[1:]))

        panel._rename_selected_widgets()

        assert first.name == "title"
        assert second.name == "cta"
        assert warnings
        assert warnings[0][0] == "Invalid Widget Prefix"
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

    def test_set_selected_widgets_reveals_primary_item_path(self, qapp):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.widget_tree import WidgetTreePanel

        project, root = _build_project_with_root()
        container = WidgetModel("group", name="container")
        nested = WidgetModel("group", name="nested")
        target = WidgetModel("label", name="target")
        nested.add_child(target)
        container.add_child(nested)
        root.add_child(container)

        panel = WidgetTreePanel()
        panel.set_project(project)
        container_item = panel._item_map[id(container)]
        nested_item = panel._item_map[id(nested)]
        container_item.setExpanded(False)
        nested_item.setExpanded(False)

        panel.set_selected_widgets([target], primary=target)

        assert container_item.isExpanded() is True
        assert nested_item.isExpanded() is True
        assert panel.selected_widgets() == [target]
        assert panel._get_selected_widget() is target
        panel.deleteLater()

    def test_filter_widgets_keeps_matching_paths_visible_across_rebuilds(self, qapp):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.widget_tree import WidgetTreePanel

        project, root = _build_project_with_root()
        container = WidgetModel("group", name="container")
        target = WidgetModel("label", name="target_label")
        other = WidgetModel("button", name="other_button")
        container.add_child(target)
        root.add_child(container)
        root.add_child(other)

        panel = WidgetTreePanel()
        panel.set_project(project)

        panel.filter_edit.setText("target")

        assert panel._item_map[id(root)].isHidden() is False
        assert panel._item_map[id(container)].isHidden() is False
        assert panel._item_map[id(target)].isHidden() is False
        assert panel._item_map[id(container)].isExpanded() is True
        assert panel._item_map[id(container)].font(0).bold() is False
        assert panel._item_map[id(target)].font(0).bold() is True
        assert panel._item_map[id(other)].isHidden() is True
        assert panel.filter_position_label.text() == "0/1"
        assert panel.filter_status_label.text() == "1 match"

        panel.rebuild_tree()

        assert panel._item_map[id(root)].isHidden() is False
        assert panel._item_map[id(container)].isHidden() is False
        assert panel._item_map[id(target)].isHidden() is False
        assert panel._item_map[id(other)].isHidden() is True
        assert panel._item_map[id(target)].font(0).bold() is True
        assert panel.filter_position_label.text() == "0/1"
        assert panel.filter_status_label.text() == "1 match"

        panel.filter_edit.setText("")

        assert panel._item_map[id(other)].isHidden() is False
        assert panel._item_map[id(target)].font(0).bold() is False
        assert panel.filter_position_label.text() == ""
        assert panel.filter_status_label.text() == "All widgets"
        panel.deleteLater()

    def test_filter_widgets_matches_widget_type(self, qapp):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.widget_tree import WidgetTreePanel

        project, root = _build_project_with_root()
        label = WidgetModel("label", name="headline")
        button = WidgetModel("button", name="cta")
        root.add_child(label)
        root.add_child(button)

        panel = WidgetTreePanel()
        panel.set_project(project)

        panel.filter_edit.setText("button")

        assert panel._item_map[id(label)].isHidden() is True
        assert panel._item_map[id(button)].isHidden() is False
        assert panel._item_map[id(button)].font(0).bold() is True
        assert panel.filter_position_label.text() == "0/1"
        assert panel.filter_status_label.text() == "1 match"
        panel.deleteLater()

    def test_filter_widgets_reports_no_matches(self, qapp):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.widget_tree import WidgetTreePanel

        project, root = _build_project_with_root()
        label = WidgetModel("label", name="headline")
        button = WidgetModel("button", name="cta")
        root.add_child(label)
        root.add_child(button)

        panel = WidgetTreePanel()
        panel.set_project(project)

        panel.filter_edit.setText("missing")

        assert panel._item_map[id(root)].isHidden() is True
        assert panel._item_map[id(label)].isHidden() is True
        assert panel._item_map[id(button)].isHidden() is True
        assert panel.filter_position_label.text() == ""
        assert panel.filter_status_label.text() == "No matches"
        assert panel.filter_prev_btn.isEnabled() is False
        assert panel.filter_next_btn.isEnabled() is False
        panel.deleteLater()

    def test_filter_match_navigation_cycles_through_matches(self, qapp):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.widget_tree import WidgetTreePanel

        project, root = _build_project_with_root()
        first = WidgetModel("label", name="field_label")
        second = WidgetModel("button", name="field_button")
        third = WidgetModel("switch", name="status")
        root.add_child(first)
        root.add_child(second)
        root.add_child(third)

        panel = WidgetTreePanel()
        panel.set_project(project)
        panel.filter_edit.setText("field")

        assert panel.filter_position_label.text() == "0/2"
        assert panel.filter_status_label.text() == "2 matches"
        assert panel.filter_prev_btn.isEnabled() is True
        assert panel.filter_next_btn.isEnabled() is True

        panel._select_next_filter_match()
        assert panel._get_selected_widget() is first
        assert panel.filter_position_label.text() == "1/2"

        panel._select_next_filter_match()
        assert panel._get_selected_widget() is second
        assert panel.filter_position_label.text() == "2/2"

        panel._select_next_filter_match()
        assert panel._get_selected_widget() is first
        assert panel.filter_position_label.text() == "1/2"

        panel._select_previous_filter_match()
        assert panel._get_selected_widget() is second
        assert panel.filter_position_label.text() == "2/2"
        panel.deleteLater()

    def test_filter_edit_keyboard_shortcuts_navigate_matches(self, qapp):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.widget_tree import WidgetTreePanel

        project, root = _build_project_with_root()
        first = WidgetModel("label", name="field_label")
        second = WidgetModel("button", name="field_button")
        root.add_child(first)
        root.add_child(second)

        panel = WidgetTreePanel()
        panel.set_project(project)
        panel.filter_edit.setText("field")

        qapp.sendEvent(panel.filter_edit, QKeyEvent(QEvent.KeyPress, Qt.Key_Return, Qt.NoModifier))
        assert panel._get_selected_widget() is first
        assert panel.filter_position_label.text() == "1/2"

        qapp.sendEvent(panel.filter_edit, QKeyEvent(QEvent.KeyPress, Qt.Key_Return, Qt.NoModifier))
        assert panel._get_selected_widget() is second
        assert panel.filter_position_label.text() == "2/2"

        qapp.sendEvent(panel.filter_edit, QKeyEvent(QEvent.KeyPress, Qt.Key_Return, Qt.ShiftModifier))
        assert panel._get_selected_widget() is first
        assert panel.filter_position_label.text() == "1/2"
        panel.deleteLater()

    def test_filter_edit_escape_clears_active_filter(self, qapp):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.widget_tree import WidgetTreePanel

        project, root = _build_project_with_root()
        label = WidgetModel("label", name="headline")
        button = WidgetModel("button", name="cta")
        root.add_child(label)
        root.add_child(button)

        panel = WidgetTreePanel()
        panel.set_project(project)
        panel.filter_edit.setText("head")

        qapp.sendEvent(panel.filter_edit, QKeyEvent(QEvent.KeyPress, Qt.Key_Escape, Qt.NoModifier))

        assert panel.filter_edit.text() == ""
        assert panel.filter_position_label.text() == ""
        assert panel.filter_status_label.text() == "All widgets"
        assert panel._item_map[id(root)].isHidden() is False
        assert panel._item_map[id(label)].isHidden() is False
        assert panel._item_map[id(button)].isHidden() is False
        panel.deleteLater()

    def test_filter_position_label_updates_for_manual_match_selection(self, qapp):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.widget_tree import WidgetTreePanel

        project, root = _build_project_with_root()
        first = WidgetModel("label", name="field_label")
        second = WidgetModel("button", name="field_button")
        root.add_child(first)
        root.add_child(second)

        panel = WidgetTreePanel()
        panel.set_project(project)
        panel.filter_edit.setText("field")

        panel.set_selected_widgets([second], primary=second)

        assert panel.filter_position_label.text() == "2/2"
        panel.deleteLater()

    def test_rebuild_tree_preserves_manual_collapse_state(self, qapp):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.widget_tree import WidgetTreePanel

        project, root = _build_project_with_root()
        container = WidgetModel("group", name="container")
        nested = WidgetModel("group", name="nested")
        nested.add_child(WidgetModel("label", name="target"))
        container.add_child(nested)
        root.add_child(container)

        panel = WidgetTreePanel()
        panel.set_project(project)
        container_item = panel._item_map[id(container)]
        nested_item = panel._item_map[id(nested)]
        container_item.setExpanded(False)
        nested_item.setExpanded(False)

        panel.rebuild_tree()

        assert panel._item_map[id(container)].isExpanded() is False
        assert panel._item_map[id(nested)].isExpanded() is False
        panel.deleteLater()

    def test_clearing_filter_restores_previous_collapse_state(self, qapp):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.widget_tree import WidgetTreePanel

        project, root = _build_project_with_root()
        container = WidgetModel("group", name="container")
        nested = WidgetModel("group", name="nested")
        target = WidgetModel("label", name="target")
        nested.add_child(target)
        container.add_child(nested)
        root.add_child(container)

        panel = WidgetTreePanel()
        panel.set_project(project)
        container_item = panel._item_map[id(container)]
        nested_item = panel._item_map[id(nested)]
        container_item.setExpanded(False)
        nested_item.setExpanded(False)

        panel.filter_edit.setText("target")

        assert panel._item_map[id(container)].isExpanded() is True
        assert panel._item_map[id(nested)].isExpanded() is True

        panel.filter_edit.setText("")

        assert panel._item_map[id(container)].isExpanded() is False
        assert panel._item_map[id(nested)].isExpanded() is False
        panel.deleteLater()

    def test_expand_all_updates_tree_and_saved_expansion_state(self, qapp):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.widget_tree import WidgetTreePanel

        project, root = _build_project_with_root()
        container = WidgetModel("group", name="container")
        nested = WidgetModel("group", name="nested")
        nested.add_child(WidgetModel("label", name="target"))
        container.add_child(nested)
        root.add_child(container)

        panel = WidgetTreePanel()
        panel.set_project(project)
        panel._collapse_all_items()

        panel._expand_all_items()
        panel.rebuild_tree()

        assert panel._item_map[id(root)].isExpanded() is True
        assert panel._item_map[id(container)].isExpanded() is True
        assert panel._item_map[id(nested)].isExpanded() is True
        panel.deleteLater()

    def test_collapse_all_updates_tree_and_saved_expansion_state(self, qapp):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.widget_tree import WidgetTreePanel

        project, root = _build_project_with_root()
        container = WidgetModel("group", name="container")
        nested = WidgetModel("group", name="nested")
        nested.add_child(WidgetModel("label", name="target"))
        container.add_child(nested)
        root.add_child(container)

        panel = WidgetTreePanel()
        panel.set_project(project)

        panel._collapse_all_items()
        panel.rebuild_tree()

        assert panel._item_map[id(root)].isExpanded() is False
        assert panel._item_map[id(container)].isExpanded() is False
        assert panel._item_map[id(nested)].isExpanded() is False
        panel.deleteLater()

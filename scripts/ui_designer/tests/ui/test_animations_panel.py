"""Qt UI tests for AnimationsPanel."""

import os

import pytest

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

try:
    from PyQt5.QtWidgets import QApplication, QComboBox, QFormLayout, QLineEdit

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


def _make_widget(name="title"):
    from ui_designer.model.widget_model import WidgetModel

    return WidgetModel("label", name=name, x=12, y=16, width=100, height=24)


@_skip_no_qt
class TestAnimationsPanel:
    def test_panel_shows_selected_widget_animations(self, qapp):
        from ui_designer.model.widget_animations import create_default_animation
        from ui_designer.ui.animations_panel import AnimationsPanel

        widget = _make_widget()
        widget.animations = [create_default_animation("alpha")]

        panel = AnimationsPanel()
        panel.set_selection([widget], primary=widget)

        assert panel._summary_label.text() == "Animations: 1 animation on label title"
        assert panel._table.rowCount() == 1
        assert panel._table.item(0, 0).text() == "alpha"
        assert panel._table.item(0, 2).text() == "linear"

    def test_panel_add_duplicate_remove_animation_emits_changes(self, qapp):
        from ui_designer.ui.animations_panel import AnimationsPanel

        widget = _make_widget()
        panel = AnimationsPanel()
        panel.set_selection([widget], primary=widget)
        captured = []
        panel.animations_changed.connect(lambda animations: captured.append(animations))

        panel._on_add_animation()
        qapp.processEvents()
        assert panel._table.rowCount() == 1
        assert captured[-1][0].anim_type == "alpha"

        panel._on_duplicate_animation()
        qapp.processEvents()
        assert panel._table.rowCount() == 2
        assert captured[-1][1].anim_type == "alpha"

        panel._table.selectRow(1)
        panel._on_remove_animation()
        qapp.processEvents()
        assert panel._table.rowCount() == 1
        assert len(captured[-1]) == 1

    def test_panel_type_change_rebuilds_detail_params(self, qapp):
        from ui_designer.ui.animations_panel import AnimationsPanel

        widget = _make_widget()
        panel = AnimationsPanel()
        panel.set_selection([widget], primary=widget)
        panel._on_add_animation()
        qapp.processEvents()

        type_combo = panel._detail_form.itemAt(0, QFormLayout.FieldRole).widget()
        assert isinstance(type_combo, QComboBox)

        type_combo.setCurrentText("translate")
        qapp.processEvents()

        assert panel._table.item(0, 0).text() == "translate"
        labels = []
        for row in range(panel._detail_form.rowCount()):
            item = panel._detail_form.itemAt(row, QFormLayout.LabelRole)
            if item and item.widget():
                labels.append(item.widget().text())
        assert "From X:" in labels
        assert "To Y:" in labels

    def test_panel_param_edit_updates_animation_model(self, qapp):
        from ui_designer.model.widget_animations import create_default_animation
        from ui_designer.ui.animations_panel import AnimationsPanel

        widget = _make_widget()
        widget.animations = [create_default_animation("translate")]
        panel = AnimationsPanel()
        panel.set_selection([widget], primary=widget)
        captured = []
        panel.animations_changed.connect(lambda animations: captured.append(animations))

        target_editor = None
        for row in range(panel._detail_form.rowCount()):
            label_item = panel._detail_form.itemAt(row, QFormLayout.LabelRole)
            field_item = panel._detail_form.itemAt(row, QFormLayout.FieldRole)
            if label_item and label_item.widget() and label_item.widget().text() == "To Y:":
                target_editor = field_item.widget()
                break

        assert isinstance(target_editor, QLineEdit)
        target_editor.setText("64")
        target_editor.editingFinished.emit()
        qapp.processEvents()

        assert captured[-1][0].params["to_y"] == "64"

    def test_panel_disables_multi_selection_editing(self, qapp):
        from ui_designer.ui.animations_panel import AnimationsPanel

        primary = _make_widget("primary")
        secondary = _make_widget("secondary")
        panel = AnimationsPanel()
        panel.set_selection([primary, secondary], primary=primary)

        assert "select a single widget" in panel._summary_label.text().lower()
        assert panel._add_button.isEnabled() is False

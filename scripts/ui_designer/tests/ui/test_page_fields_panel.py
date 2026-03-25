"""Qt UI tests for PageFieldsPanel."""

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


def _make_page():
    from ui_designer.model.page import Page
    from ui_designer.model.widget_model import WidgetModel

    page = Page.create_default("main_page", screen_width=240, screen_height=320)
    title = WidgetModel("label", name="title", x=12, y=16, width=100, height=24)
    page.root_widget.add_child(title)
    return page


@_skip_no_qt
class TestPageFieldsPanel:
    def test_panel_displays_current_page_fields(self, qapp):
        from ui_designer.ui.page_fields_panel import PageFieldsPanel

        page = _make_page()
        page.user_fields = [
            {"name": "counter", "type": "int", "default": "0"},
            {"name": "buffer", "type": "uint8_t[16]"},
        ]

        panel = PageFieldsPanel()
        panel.set_page(page)

        assert panel._summary_label.text() == "Page Fields: 2 fields on main_page"
        assert panel._table.rowCount() == 2
        assert panel._table.item(0, 0).text() == "counter"
        assert panel._table.item(0, 1).text() == "int"
        assert panel._table.item(0, 2).text() == "0"
        assert panel._table.item(1, 0).text() == "buffer"

    def test_panel_add_and_remove_field_emits_changes(self, qapp):
        from ui_designer.ui.page_fields_panel import PageFieldsPanel

        page = _make_page()
        panel = PageFieldsPanel()
        panel.set_page(page)
        captured = []
        panel.fields_changed.connect(lambda fields: captured.append(fields))

        panel._on_add_field()
        qapp.processEvents()

        assert panel._table.rowCount() == 1
        assert captured[-1] == [{"name": "field", "type": "int", "default": "0"}]

        panel._table.selectRow(0)
        panel._on_remove_field()
        qapp.processEvents()

        assert panel._table.rowCount() == 0
        assert captured[-1] == []

    def test_panel_rejects_conflicting_field_name(self, qapp):
        from ui_designer.ui.page_fields_panel import PageFieldsPanel

        page = _make_page()
        panel = PageFieldsPanel()
        panel.set_page(page)
        messages = []
        panel.validation_message.connect(messages.append)

        panel._on_add_field()
        qapp.processEvents()
        panel._table.item(0, 0).setText("title")
        qapp.processEvents()

        assert messages[-1] == "Page field 'title' conflicts with an auto-generated page member."
        assert panel._table.item(0, 0).text() == "field"

    def test_panel_open_lifecycle_section_emits_request(self, qapp):
        from ui_designer.ui.page_fields_panel import PageFieldsPanel

        page = _make_page()
        panel = PageFieldsPanel()
        captured = []
        panel.user_code_section_requested.connect(captured.append)
        panel.set_page(page)

        panel._request_section("init")

        assert captured == ["init"]

    def test_panel_rejects_duplicate_field_name(self, qapp):
        from ui_designer.ui.page_fields_panel import PageFieldsPanel

        page = _make_page()
        page.user_fields = [
            {"name": "counter", "type": "int", "default": "0"},
            {"name": "buffer", "type": "uint8_t[16]"},
        ]
        panel = PageFieldsPanel()
        panel.set_page(page)
        messages = []
        panel.validation_message.connect(messages.append)

        panel._table.item(1, 0).setText("counter")
        qapp.processEvents()

        assert messages[-1] == "Page field 'counter' already exists in this page."
        assert panel._table.item(1, 0).text() == "buffer"

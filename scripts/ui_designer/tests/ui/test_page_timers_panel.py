"""Qt UI tests for PageTimersPanel."""

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
class TestPageTimersPanel:
    def test_panel_displays_current_page_timers(self, qapp):
        from ui_designer.ui.page_timers_panel import PageTimersPanel

        page = _make_page()
        page.timers = [
            {"name": "refresh_timer", "callback": "tick_refresh", "delay_ms": "500", "period_ms": "1000", "auto_start": True},
        ]

        panel = PageTimersPanel()
        panel.set_page(page)

        assert panel._summary_label.text() == "Page Timers: 1 timer on main_page"
        assert panel._table.rowCount() == 1
        assert panel._table.item(0, 0).text() == "refresh_timer"
        assert panel._table.item(0, 1).text() == "tick_refresh"
        assert panel._table.item(0, 4).text() == "true"

    def test_panel_add_and_remove_timer_emit_changes(self, qapp):
        from ui_designer.ui.page_timers_panel import PageTimersPanel

        page = _make_page()
        panel = PageTimersPanel()
        panel.set_page(page)
        captured = []
        panel.timers_changed.connect(lambda timers: captured.append(timers))

        panel._on_add_timer()
        qapp.processEvents()

        assert panel._table.rowCount() == 1
        assert captured[-1][0]["name"] == "timer"
        assert captured[-1][0]["callback"] == "egui_main_page_timer_callback"

        panel._table.selectRow(0)
        panel._on_remove_timer()
        qapp.processEvents()

        assert panel._table.rowCount() == 0
        assert captured[-1] == []

    def test_panel_rejects_conflicting_timer_name(self, qapp):
        from ui_designer.ui.page_timers_panel import PageTimersPanel

        page = _make_page()
        panel = PageTimersPanel()
        panel.set_page(page)
        messages = []
        panel.validation_message.connect(messages.append)

        panel._on_add_timer()
        qapp.processEvents()
        panel._table.item(0, 0).setText("title")
        qapp.processEvents()

        assert messages[-1] == "Page timer 'title' conflicts with an auto-generated page member."
        assert panel._table.item(0, 0).text() == "timer"

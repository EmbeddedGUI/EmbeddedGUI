"""Qt UI smoke tests for preview fallback and workspace-aware build gating."""

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


@_skip_no_qt
class TestPreviewPanelFallback:
    def test_show_python_preview_sets_pixmap_and_status(self, qapp):
        from ui_designer.model.page import Page
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.preview_panel import PreviewPanel

        root = WidgetModel("group", name="root", x=0, y=0, width=240, height=320)
        label = WidgetModel("label", name="label", x=10, y=10, width=100, height=20)
        label.properties["text"] = "Hello"
        root.add_child(label)
        page = Page(file_path="layout/main_page.xml", root_widget=root)

        panel = PreviewPanel(screen_width=240, screen_height=320)
        panel.show_python_preview(page, "fallback")

        assert panel.is_python_preview_active() is True
        assert panel._preview_label.pixmap() is not None
        assert "Python fallback" in panel.status_label.text()
        panel.deleteLater()

    def test_runtime_failed_emits_after_repeated_frame_failures(self, qapp):
        from ui_designer.ui.preview_panel import PreviewPanel

        class FakeCompiler:
            def __init__(self):
                self.calls = 0

            def get_frame(self):
                self.calls += 1
                return None

            def get_last_runtime_error(self):
                return "bridge lost"

        panel = PreviewPanel(screen_width=240, screen_height=320)
        compiler = FakeCompiler()
        reasons = []
        panel.runtime_failed.connect(reasons.append)
        panel.start_rendering(compiler)

        panel._refresh_frame()
        panel._refresh_frame()
        panel._refresh_frame()

        assert reasons == ["bridge lost"]
        assert panel.is_embedded is False
        panel.deleteLater()

    def test_grid_size_uses_configured_value(self, qapp):
        from ui_designer.ui.preview_panel import PreviewPanel

        panel = PreviewPanel(screen_width=240, screen_height=320)
        panel.set_grid_size(12)

        assert panel.grid_size() == 12
        assert panel.overlay._effective_grid_size() == 12

        panel.overlay.set_zoom(2.0)
        assert panel.overlay._effective_grid_size() == 12
        panel.deleteLater()


@_skip_no_qt
class TestMainWindowBuildAvailability:
    def test_compile_actions_disabled_when_compiler_cannot_build(self, qapp):
        from ui_designer.model.project import Project
        from ui_designer.ui.main_window import MainWindow

        class FakeCompiler:
            def can_build(self):
                return False

            def is_preview_running(self):
                return False

        window = MainWindow("", app_name="HelloDesigner")
        window.project = Project()
        window.compiler = FakeCompiler()
        window._update_compile_availability()

        assert window._compile_action.isEnabled() is False
        assert window.auto_compile_action.isEnabled() is False
        assert window._stop_action.isEnabled() is False
        window.deleteLater()

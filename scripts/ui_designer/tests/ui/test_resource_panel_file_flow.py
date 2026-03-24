"""Qt UI tests for ResourcePanel import dialog defaults."""

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
class TestResourcePanelFileFlow:
    def test_import_image_dialog_uses_project_images_dir_by_default(self, qapp, tmp_path, monkeypatch):
        from ui_designer.ui.resource_panel import ResourcePanel

        resource_dir = tmp_path / "project" / ".eguiproject" / "resources"
        images_dir = resource_dir / "images"
        images_dir.mkdir(parents=True)
        captured = {}

        panel = ResourcePanel()
        panel.set_resource_dir(str(resource_dir))

        def fake_get_open_file_names(parent, title, directory, filters):
            captured["title"] = title
            captured["directory"] = directory
            captured["filters"] = filters
            return [], ""

        monkeypatch.setattr("ui_designer.ui.resource_panel.QFileDialog.getOpenFileNames", fake_get_open_file_names)

        panel._on_import_image()

        assert captured["title"] == "Import Images"
        assert captured["directory"] == os.path.normpath(os.path.abspath(images_dir))
        assert "Images" in captured["filters"]
        panel.deleteLater()

    def test_import_font_dialog_prefers_last_external_import_directory(self, qapp, tmp_path, monkeypatch):
        from ui_designer.ui.resource_panel import ResourcePanel

        resource_dir = tmp_path / "project" / ".eguiproject" / "resources"
        resource_dir.mkdir(parents=True)
        external_dir = tmp_path / "external_fonts"
        external_dir.mkdir()
        font_path = external_dir / "demo.ttf"
        font_path.write_bytes(b"FONT")
        captured = {}

        panel = ResourcePanel()
        panel.set_resource_dir(str(resource_dir))
        monkeypatch.setattr("ui_designer.ui.resource_panel.QInputDialog.getText", lambda *args, **kwargs: ("demo.ttf", True))

        def fake_get_open_file_names_first(parent, title, directory, filters):
            captured["first_directory"] = directory
            return [str(font_path)], ""

        def fake_get_open_file_names_second(parent, title, directory, filters):
            captured["second_directory"] = directory
            return [], ""

        monkeypatch.setattr("ui_designer.ui.resource_panel.QFileDialog.getOpenFileNames", fake_get_open_file_names_first)
        panel._on_import_font()

        monkeypatch.setattr("ui_designer.ui.resource_panel.QFileDialog.getOpenFileNames", fake_get_open_file_names_second)
        panel._on_import_font()

        assert captured["first_directory"] == os.path.normpath(os.path.abspath(resource_dir))
        assert captured["second_directory"] == os.path.normpath(os.path.abspath(external_dir))
        panel.deleteLater()

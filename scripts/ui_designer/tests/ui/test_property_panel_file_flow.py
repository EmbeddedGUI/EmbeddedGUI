"""Qt UI tests for PropertyPanel file browsing and auto-import."""

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
class TestPropertyPanelFileFlow:
    def test_browse_file_warns_when_project_resource_dir_is_missing(self, qapp, monkeypatch):
        from ui_designer.ui.property_panel import PropertyPanel

        panel = PropertyPanel()
        selector = panel._create_file_selector("image_file", "", [], "Images (*.png *.bmp *.jpg *.jpeg)")
        combo = panel._editors["prop_image_file"]
        warnings = []
        dialog_calls = []

        monkeypatch.setattr("ui_designer.ui.property_panel.QMessageBox.warning", lambda *args: warnings.append(args[1:]))
        monkeypatch.setattr(
            "ui_designer.ui.property_panel.QFileDialog.getOpenFileName",
            lambda *args, **kwargs: dialog_calls.append(args) or ("", ""),
        )

        panel._browse_file(combo, "Images (*.png *.bmp *.jpg *.jpeg)")

        assert warnings
        assert warnings[0][0] == "Resource Directory Missing"
        assert dialog_calls == []
        assert combo.currentText() == ""
        assert selector is not None
        panel.deleteLater()

    def test_browse_file_uses_images_subdir_as_default_directory(self, qapp, tmp_path, monkeypatch):
        from ui_designer.ui.property_panel import PropertyPanel

        resource_dir = tmp_path / "project" / ".eguiproject" / "resources"
        images_dir = resource_dir / "images"
        images_dir.mkdir(parents=True)
        captured = {}

        panel = PropertyPanel()
        panel.set_source_resource_dir(str(resource_dir))
        selector = panel._create_file_selector("image_file", "", [], "Images (*.png *.bmp *.jpg *.jpeg)")
        combo = panel._editors["prop_image_file"]

        def fake_get_open_file_name(parent, title, directory, filters):
            captured["title"] = title
            captured["directory"] = directory
            captured["filters"] = filters
            return "", ""

        monkeypatch.setattr("ui_designer.ui.property_panel.QFileDialog.getOpenFileName", fake_get_open_file_name)

        panel._browse_file(combo, "Images (*.png *.bmp *.jpg *.jpeg)")

        assert captured["title"] == "Select File"
        assert captured["directory"] == os.path.normpath(os.path.abspath(images_dir))
        assert "Images" in captured["filters"]
        assert selector is not None
        panel.deleteLater()

    def test_browse_file_auto_imports_image_and_emits_resource_imported(self, qapp, tmp_path, monkeypatch):
        from ui_designer.model.resource_catalog import ResourceCatalog
        from ui_designer.ui.property_panel import PropertyPanel

        resource_dir = tmp_path / "project" / ".eguiproject" / "resources"
        images_dir = resource_dir / "images"
        images_dir.mkdir(parents=True)
        external_dir = tmp_path / "external"
        external_dir.mkdir()
        image_path = external_dir / "star.png"
        image_path.write_bytes(b"PNG")

        panel = PropertyPanel()
        panel.set_source_resource_dir(str(resource_dir))
        catalog = ResourceCatalog()
        panel.set_resource_catalog(catalog)
        selector = panel._create_file_selector("image_file", "", [], "Images (*.png *.bmp *.jpg *.jpeg)")
        combo = panel._editors["prop_image_file"]
        imported_events = []
        panel.resource_imported.connect(lambda: imported_events.append("imported"))

        monkeypatch.setattr(
            "ui_designer.ui.property_panel.QFileDialog.getOpenFileName",
            lambda *args, **kwargs: (str(image_path), "Images (*.png *.bmp *.jpg *.jpeg)"),
        )

        panel._browse_file(combo, "Images (*.png *.bmp *.jpg *.jpeg)")

        assert (images_dir / "star.png").is_file()
        assert catalog.has_image("star.png")
        assert combo.currentText() == "star.png"
        assert imported_events == ["imported"]
        assert selector is not None
        panel.deleteLater()

    def test_browse_file_prefers_last_external_directory_after_import(self, qapp, tmp_path, monkeypatch):
        from ui_designer.model.resource_catalog import ResourceCatalog
        from ui_designer.ui.property_panel import PropertyPanel

        resource_dir = tmp_path / "project" / ".eguiproject" / "resources"
        images_dir = resource_dir / "images"
        images_dir.mkdir(parents=True)
        external_dir = tmp_path / "external"
        external_dir.mkdir()
        image_path = external_dir / "star.png"
        image_path.write_bytes(b"PNG")
        captured = {}

        panel = PropertyPanel()
        panel.set_source_resource_dir(str(resource_dir))
        panel.set_resource_catalog(ResourceCatalog())
        selector = panel._create_file_selector("image_file", "", [], "Images (*.png *.bmp *.jpg *.jpeg)")
        combo = panel._editors["prop_image_file"]

        monkeypatch.setattr(
            "ui_designer.ui.property_panel.QFileDialog.getOpenFileName",
            lambda *args, **kwargs: (str(image_path), "Images (*.png *.bmp *.jpg *.jpeg)"),
        )
        panel._browse_file(combo, "Images (*.png *.bmp *.jpg *.jpeg)")

        def fake_get_open_file_name_second(parent, title, directory, filters):
            captured["directory"] = directory
            return "", ""

        monkeypatch.setattr("ui_designer.ui.property_panel.QFileDialog.getOpenFileName", fake_get_open_file_name_second)
        panel._browse_file(combo, "Images (*.png *.bmp *.jpg *.jpeg)")

        assert captured["directory"] == os.path.normpath(os.path.abspath(external_dir))
        assert selector is not None
        panel.deleteLater()

    def test_browse_file_selects_existing_catalog_image_without_reimport(self, qapp, tmp_path, monkeypatch):
        from ui_designer.model.resource_catalog import ResourceCatalog
        from ui_designer.ui.property_panel import PropertyPanel

        resource_dir = tmp_path / "project" / ".eguiproject" / "resources"
        images_dir = resource_dir / "images"
        images_dir.mkdir(parents=True)
        image_path = images_dir / "star.png"
        image_path.write_bytes(b"PNG")

        panel = PropertyPanel()
        panel.set_source_resource_dir(str(resource_dir))
        catalog = ResourceCatalog()
        catalog.add_image("star.png")
        panel.set_resource_catalog(catalog)
        selector = panel._create_file_selector("image_file", "", ["star.png"], "Images (*.png *.bmp *.jpg *.jpeg)")
        combo = panel._editors["prop_image_file"]
        imported_events = []
        panel.resource_imported.connect(lambda: imported_events.append("imported"))

        monkeypatch.setattr(
            "ui_designer.ui.property_panel.QFileDialog.getOpenFileName",
            lambda *args, **kwargs: (str(image_path), "Images (*.png *.bmp *.jpg *.jpeg)"),
        )

        panel._browse_file(combo, "Images (*.png *.bmp *.jpg *.jpeg)")

        assert catalog.images == ["star.png"]
        assert combo.currentText() == "star.png"
        assert imported_events == []
        assert selector is not None
        panel.deleteLater()

    def test_browse_text_file_uses_resource_root_as_default_directory(self, qapp, tmp_path, monkeypatch):
        from ui_designer.ui.property_panel import PropertyPanel

        resource_dir = tmp_path / "project" / ".eguiproject" / "resources"
        resource_dir.mkdir(parents=True)
        captured = {}

        panel = PropertyPanel()
        panel.set_source_resource_dir(str(resource_dir))
        selector = panel._create_file_selector("font_text_file", "", [], "Text files (*.txt)")
        combo = panel._editors["prop_font_text_file"]

        def fake_get_open_file_name(parent, title, directory, filters):
            captured["title"] = title
            captured["directory"] = directory
            captured["filters"] = filters
            return "", ""

        monkeypatch.setattr("ui_designer.ui.property_panel.QFileDialog.getOpenFileName", fake_get_open_file_name)

        panel._browse_file(combo, "Text files (*.txt)")

        assert captured["title"] == "Select File"
        assert captured["directory"] == os.path.normpath(os.path.abspath(resource_dir))
        assert "Text files" in captured["filters"]
        assert selector is not None
        panel.deleteLater()

    def test_browse_text_file_auto_imports_and_emits_resource_imported(self, qapp, tmp_path, monkeypatch):
        from ui_designer.model.resource_catalog import ResourceCatalog
        from ui_designer.ui.property_panel import PropertyPanel

        resource_dir = tmp_path / "project" / ".eguiproject" / "resources"
        resource_dir.mkdir(parents=True)
        external_dir = tmp_path / "external"
        external_dir.mkdir()
        text_path = external_dir / "chars.txt"
        text_path.write_text("abc\n123\n", encoding="utf-8")

        panel = PropertyPanel()
        panel.set_source_resource_dir(str(resource_dir))
        catalog = ResourceCatalog()
        panel.set_resource_catalog(catalog)
        selector = panel._create_file_selector("font_text_file", "", [], "Text files (*.txt)")
        combo = panel._editors["prop_font_text_file"]
        imported_events = []
        panel.resource_imported.connect(lambda: imported_events.append("imported"))

        monkeypatch.setattr(
            "ui_designer.ui.property_panel.QFileDialog.getOpenFileName",
            lambda *args, **kwargs: (str(text_path), "Text files (*.txt)"),
        )

        panel._browse_file(combo, "Text files (*.txt)")

        assert (resource_dir / "chars.txt").is_file()
        assert catalog.has_text_file("chars.txt")
        assert combo.currentText() == "chars.txt"
        assert imported_events == ["imported"]
        assert selector is not None
        panel.deleteLater()

    def test_multi_selection_form_toggles_designer_flags_for_all_widgets(self, qapp):
        from qfluentwidgets import CheckBox
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.property_panel import PropertyPanel

        first = WidgetModel("label", name="first")
        second = WidgetModel("button", name="second")

        panel = PropertyPanel()
        panel.set_selection([first, second], primary=second)

        summary_group = panel._layout.itemAt(0).widget()
        assert summary_group.title() == "Selection - 2 Widgets"

        checkboxes = {checkbox.text(): checkbox for checkbox in panel.findChildren(CheckBox)}
        checkboxes["Locked"].setChecked(True)
        checkboxes["Hidden"].setChecked(True)

        assert first.designer_locked is True
        assert second.designer_locked is True
        assert first.designer_hidden is True
        assert second.designer_hidden is True
        panel.deleteLater()

    def test_multi_selection_common_geometry_and_text_update_all_widgets(self, qapp):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.property_panel import PropertyPanel

        first = WidgetModel("label", name="first", x=10, y=20, width=80, height=24)
        second = WidgetModel("button", name="second", x=30, y=40, width=90, height=28)

        panel = PropertyPanel()
        panel.set_selection([first, second], primary=second)

        panel._editors["multi_width"].setValue(120)
        panel._editors["prop_text"].setText("Shared")

        assert first.width == 120
        assert second.width == 120
        assert first.properties["text"] == "Shared"
        assert second.properties["text"] == "Shared"
        panel.deleteLater()

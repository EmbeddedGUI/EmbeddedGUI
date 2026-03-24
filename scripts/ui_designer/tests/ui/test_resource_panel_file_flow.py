"""Qt UI tests for ResourcePanel import dialog defaults."""

import os

import pytest

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

try:
    from PyQt5.QtWidgets import QApplication, QMessageBox

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
    def test_import_image_warns_before_opening_dialog_when_resource_dir_missing(self, qapp, monkeypatch):
        from ui_designer.ui.resource_panel import ResourcePanel

        panel = ResourcePanel()
        warnings = []
        dialog_calls = []

        monkeypatch.setattr("ui_designer.ui.resource_panel.QMessageBox.warning", lambda *args: warnings.append(args[1:]))
        monkeypatch.setattr(
            "ui_designer.ui.resource_panel.QFileDialog.getOpenFileNames",
            lambda *args, **kwargs: dialog_calls.append(args) or ([], ""),
        )

        panel._on_import_image()

        assert warnings
        assert warnings[0][0] == "Error"
        assert "No resource directory configured" in warnings[0][1]
        assert dialog_calls == []
        panel.deleteLater()

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

    def test_set_resource_dir_populates_text_tab_and_emits_selection(self, qapp, tmp_path):
        from ui_designer.model.resource_catalog import ResourceCatalog
        from ui_designer.ui.resource_panel import ResourcePanel

        resource_dir = tmp_path / "project" / ".eguiproject" / "resources"
        resource_dir.mkdir(parents=True)
        text_path = resource_dir / "supported_text.txt"
        text_path.write_text("Hello\nWorld\n", encoding="utf-8")

        catalog = ResourceCatalog()
        catalog.add_text_file("supported_text.txt")

        panel = ResourcePanel()
        panel.set_resource_dir(str(resource_dir))
        panel.set_resource_catalog(catalog)

        captured = []
        panel.resource_selected.connect(lambda res_type, filename: captured.append((res_type, filename)))

        assert panel._text_list.count() == 1
        assert panel._tabs.tabText(2) == "Text (1)"

        item = panel._text_list.item(0)
        panel._on_text_clicked(item)

        assert captured == [("text", "supported_text.txt")]
        panel.deleteLater()

    def test_tab_title_shows_missing_resource_count(self, qapp, tmp_path):
        from ui_designer.model.resource_catalog import ResourceCatalog
        from ui_designer.ui.resource_panel import ResourcePanel

        resource_dir = tmp_path / "project" / ".eguiproject" / "resources"
        images_dir = resource_dir / "images"
        images_dir.mkdir(parents=True)
        (images_dir / "present.png").write_bytes(b"PNG")

        catalog = ResourceCatalog()
        catalog.add_image("missing.png")
        catalog.add_image("present.png")

        panel = ResourcePanel()
        panel.set_resource_dir(str(resource_dir))
        panel.set_resource_catalog(catalog)

        assert panel._tabs.tabText(0) == "Images (2, 1 missing)"
        panel.deleteLater()

    def test_import_text_refreshes_text_tab_and_catalog(self, qapp, tmp_path, monkeypatch):
        from ui_designer.ui.resource_panel import ResourcePanel

        resource_dir = tmp_path / "project" / ".eguiproject" / "resources"
        resource_dir.mkdir(parents=True)
        external_dir = tmp_path / "external_text"
        external_dir.mkdir()
        text_path = external_dir / "demo.txt"
        text_path.write_text("abc\n123\n", encoding="utf-8")

        panel = ResourcePanel()
        panel.set_resource_dir(str(resource_dir))

        imported = []
        panel.resource_imported.connect(lambda: imported.append(True))

        monkeypatch.setattr(
            "ui_designer.ui.resource_panel.QFileDialog.getOpenFileNames",
            lambda *args, **kwargs: ([str(text_path)], ""),
        )
        monkeypatch.setattr(
            "ui_designer.ui.resource_panel.QInputDialog.getText",
            lambda *args, **kwargs: ("demo.txt", True),
        )

        panel._on_import_text()

        assert (resource_dir / "demo.txt").is_file()
        assert panel.get_resource_catalog().text_files == ["demo.txt"]
        assert panel._text_list.count() == 1
        assert imported == [True]
        panel.deleteLater()

    def test_restore_missing_image_copies_file_and_clears_missing_state(self, qapp, tmp_path, monkeypatch):
        from ui_designer.model.resource_catalog import ResourceCatalog
        from ui_designer.ui.resource_panel import ResourcePanel

        resource_dir = tmp_path / "project" / ".eguiproject" / "resources"
        images_dir = resource_dir / "images"
        images_dir.mkdir(parents=True)
        external_dir = tmp_path / "external_images"
        external_dir.mkdir()
        source_path = external_dir / "external.png"
        source_path.write_bytes(b"PNG")

        catalog = ResourceCatalog()
        catalog.add_image("missing.png")

        panel = ResourcePanel()
        panel.set_resource_dir(str(resource_dir))
        panel.set_resource_catalog(catalog)

        imported = []
        panel.resource_imported.connect(lambda: imported.append(True))
        monkeypatch.setattr(
            "ui_designer.ui.resource_panel.QFileDialog.getOpenFileName",
            lambda *args, **kwargs: (str(source_path), "Images (*.png *.bmp *.jpg *.jpeg)"),
        )

        panel._restore_missing_resource("missing.png", "image")

        restored_path = images_dir / "missing.png"
        assert restored_path.is_file()
        assert restored_path.read_bytes() == b"PNG"
        assert imported == [True]
        item = panel._image_list.item(0)
        assert "File not found!" not in item.toolTip()
        panel.deleteLater()

    def test_restore_missing_font_rejects_extension_mismatch(self, qapp, tmp_path, monkeypatch):
        from ui_designer.model.resource_catalog import ResourceCatalog
        from ui_designer.ui.resource_panel import ResourcePanel

        resource_dir = tmp_path / "project" / ".eguiproject" / "resources"
        resource_dir.mkdir(parents=True)
        external_dir = tmp_path / "external_fonts"
        external_dir.mkdir()
        source_path = external_dir / "replacement.otf"
        source_path.write_bytes(b"FONT")

        catalog = ResourceCatalog()
        catalog.add_font("missing.ttf")

        panel = ResourcePanel()
        panel.set_resource_dir(str(resource_dir))
        panel.set_resource_catalog(catalog)

        warnings = []
        monkeypatch.setattr(
            "ui_designer.ui.resource_panel.QFileDialog.getOpenFileName",
            lambda *args, **kwargs: (str(source_path), "Fonts (*.ttf *.otf)"),
        )
        monkeypatch.setattr("ui_designer.ui.resource_panel.QMessageBox.warning", lambda *args: warnings.append(args[1:]))

        panel._restore_missing_resource("missing.ttf", "font")

        assert not (resource_dir / "missing.ttf").exists()
        assert warnings
        assert warnings[0][0] == "Extension Mismatch"
        assert "Expected a '.ttf' file to restore 'missing.ttf'." in warnings[0][1]
        panel.deleteLater()

    def test_restore_missing_resources_batch_restores_only_matching_files(self, qapp, tmp_path, monkeypatch):
        from ui_designer.model.resource_catalog import ResourceCatalog
        from ui_designer.ui.resource_panel import ResourcePanel

        resource_dir = tmp_path / "project" / ".eguiproject" / "resources"
        images_dir = resource_dir / "images"
        images_dir.mkdir(parents=True)
        external_dir = tmp_path / "external_images"
        external_dir.mkdir()
        first_match = external_dir / "missing_a.png"
        first_match.write_bytes(b"A")
        no_match = external_dir / "extra.png"
        no_match.write_bytes(b"X")

        catalog = ResourceCatalog()
        catalog.add_image("missing_a.png")
        catalog.add_image("missing_b.png")

        panel = ResourcePanel()
        panel.set_resource_dir(str(resource_dir))
        panel.set_resource_catalog(catalog)

        imported = []
        panel.resource_imported.connect(lambda: imported.append(True))
        monkeypatch.setattr(
            "ui_designer.ui.resource_panel.QFileDialog.getOpenFileNames",
            lambda *args, **kwargs: ([str(first_match), str(no_match)], "Images (*.png *.bmp *.jpg *.jpeg)"),
        )

        panel._restore_missing_resources("image")

        assert (images_dir / "missing_a.png").is_file()
        assert not (images_dir / "missing_b.png").exists()
        assert imported == [True]
        first_item = panel._image_list.item(0)
        second_item = panel._image_list.item(1)
        assert "File not found!" not in first_item.toolTip()
        assert "File not found!" in second_item.toolTip()
        panel.deleteLater()

    def test_restore_missing_resources_warns_when_no_matching_files_selected(self, qapp, tmp_path, monkeypatch):
        from ui_designer.model.resource_catalog import ResourceCatalog
        from ui_designer.ui.resource_panel import ResourcePanel

        resource_dir = tmp_path / "project" / ".eguiproject" / "resources"
        resource_dir.mkdir(parents=True)
        external_dir = tmp_path / "external_fonts"
        external_dir.mkdir()
        no_match = external_dir / "other.ttf"
        no_match.write_bytes(b"FONT")

        catalog = ResourceCatalog()
        catalog.add_font("missing.ttf")

        panel = ResourcePanel()
        panel.set_resource_dir(str(resource_dir))
        panel.set_resource_catalog(catalog)

        warnings = []
        monkeypatch.setattr(
            "ui_designer.ui.resource_panel.QFileDialog.getOpenFileNames",
            lambda *args, **kwargs: ([str(no_match)], "Fonts (*.ttf *.otf)"),
        )
        monkeypatch.setattr("ui_designer.ui.resource_panel.QMessageBox.warning", lambda *args: warnings.append(args[1:]))

        panel._restore_missing_resources("font")

        assert not (resource_dir / "missing.ttf").exists()
        assert warnings
        assert warnings[0][0] == "Restore Missing Resources"
        assert "No matching missing font resources were found in the selected files." in warnings[0][1]
        panel.deleteLater()

    def test_rename_text_resource_updates_catalog_and_emits_signal(self, qapp, tmp_path, monkeypatch):
        from ui_designer.model.resource_catalog import ResourceCatalog
        from ui_designer.ui.resource_panel import ResourcePanel

        resource_dir = tmp_path / "project" / ".eguiproject" / "resources"
        resource_dir.mkdir(parents=True)
        old_path = resource_dir / "chars.txt"
        old_path.write_text("abc\n", encoding="utf-8")

        catalog = ResourceCatalog()
        catalog.add_text_file("chars.txt")

        panel = ResourcePanel()
        panel.set_resource_dir(str(resource_dir))
        panel.set_resource_catalog(catalog)

        renamed = []
        imported = []
        panel.resource_renamed.connect(lambda res_type, old, new: renamed.append((res_type, old, new)))
        panel.resource_imported.connect(lambda: imported.append(True))
        monkeypatch.setattr(
            "ui_designer.ui.resource_panel.QInputDialog.getText",
            lambda *args, **kwargs: ("chars_new.txt", True),
        )

        panel._rename_resource("chars.txt", "text")

        assert not old_path.exists()
        assert (resource_dir / "chars_new.txt").is_file()
        assert panel.get_resource_catalog().text_files == ["chars_new.txt"]
        assert renamed == [("text", "chars.txt", "chars_new.txt")]
        assert imported == [True]
        panel.deleteLater()

    def test_delete_text_resource_updates_catalog_and_emits_signal(self, qapp, tmp_path, monkeypatch):
        from ui_designer.model.resource_catalog import ResourceCatalog
        from ui_designer.ui.resource_panel import ResourcePanel

        resource_dir = tmp_path / "project" / ".eguiproject" / "resources"
        resource_dir.mkdir(parents=True)
        text_path = resource_dir / "chars.txt"
        text_path.write_text("abc\n", encoding="utf-8")

        catalog = ResourceCatalog()
        catalog.add_text_file("chars.txt")

        panel = ResourcePanel()
        panel.set_resource_dir(str(resource_dir))
        panel.set_resource_catalog(catalog)

        deleted = []
        imported = []
        panel.resource_deleted.connect(lambda res_type, filename: deleted.append((res_type, filename)))
        panel.resource_imported.connect(lambda: imported.append(True))
        monkeypatch.setattr("ui_designer.ui.resource_panel.QMessageBox.question", lambda *args, **kwargs: QMessageBox.Yes)

        panel._delete_resource("chars.txt", "text")

        assert not text_path.exists()
        assert panel.get_resource_catalog().text_files == []
        assert panel._text_list.count() == 0
        assert deleted == [("text", "chars.txt")]
        assert imported == [True]
        panel.deleteLater()

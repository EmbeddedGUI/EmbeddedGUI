"""Qt UI tests for PropertyPanel file browsing and auto-import."""

import os

import pytest

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

try:
    from PyQt5.QtWidgets import QApplication, QFormLayout, QGroupBox, QLabel

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


def _find_group(panel, title):
    for group in panel.findChildren(QGroupBox):
        if group.title() == title:
            return group
    raise AssertionError(f"Group not found: {title}")


def _form_labels(group):
    form = group.layout()
    assert isinstance(form, QFormLayout)
    labels = []
    for row in range(form.rowCount()):
        item = form.itemAt(row, QFormLayout.LabelRole)
        if item and item.widget():
            labels.append(item.widget().text())
    return labels


def _form_value_text(group, label_text):
    form = group.layout()
    assert isinstance(form, QFormLayout)
    for row in range(form.rowCount()):
        label_item = form.itemAt(row, QFormLayout.LabelRole)
        field_item = form.itemAt(row, QFormLayout.FieldRole)
        if label_item and label_item.widget() and label_item.widget().text() == label_text:
            if field_item and field_item.widget():
                return field_item.widget().text()
            break
    raise AssertionError(f"Form row not found: {label_text}")


def _group_label_texts(group):
    return [label.text() for label in group.findChildren(QLabel)]


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

    def test_single_selection_marks_missing_file_property(self, qapp):
        from ui_designer.model.resource_catalog import ResourceCatalog
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.property_panel import PropertyPanel

        widget = WidgetModel("label", name="title")
        widget.properties["font_file"] = "missing.ttf"

        panel = PropertyPanel()
        panel.set_resource_catalog(ResourceCatalog())
        panel.set_widget(widget)

        font_group = _find_group(panel, "Font Config")
        editor = panel._editors["prop_font_file"]

        assert "File (Missing):" in _form_labels(font_group)
        assert "not present in the project catalog" in editor.toolTip()
        panel.deleteLater()

    def test_single_selection_marks_disk_missing_file_property(self, qapp, tmp_path):
        from ui_designer.model.resource_catalog import ResourceCatalog
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.property_panel import PropertyPanel

        resource_dir = tmp_path / "project" / ".eguiproject" / "resources"
        resource_dir.mkdir(parents=True)

        widget = WidgetModel("label", name="title")
        widget.properties["font_file"] = "missing.ttf"

        catalog = ResourceCatalog()
        catalog.add_font("missing.ttf")

        panel = PropertyPanel()
        panel.set_source_resource_dir(str(resource_dir))
        panel.set_resource_catalog(catalog)
        panel.set_widget(widget)

        font_group = _find_group(panel, "Font Config")
        editor = panel._editors["prop_font_file"]

        assert "File (Missing):" in _form_labels(font_group)
        assert "source file is missing on disk" in editor.toolTip()
        panel.deleteLater()

    def test_multi_selection_marks_missing_when_any_disk_file_is_missing(self, qapp, tmp_path):
        from ui_designer.model.resource_catalog import ResourceCatalog
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.property_panel import PropertyPanel

        resource_dir = tmp_path / "project" / ".eguiproject" / "resources"
        resource_dir.mkdir(parents=True)
        (resource_dir / "present.ttf").write_bytes(b"FONT")

        first = WidgetModel("label", name="first")
        second = WidgetModel("label", name="second")
        first.properties["font_file"] = "missing.ttf"
        second.properties["font_file"] = "present.ttf"

        catalog = ResourceCatalog()
        catalog.add_font("missing.ttf")
        catalog.add_font("present.ttf")

        panel = PropertyPanel()
        panel.set_source_resource_dir(str(resource_dir))
        panel.set_resource_catalog(catalog)
        panel.set_selection([first, second], primary=second)

        common_group = _find_group(panel, "Common Properties")
        editor = panel._editors["prop_font_file"]

        assert any(label.startswith("Font File") and "(Missing)" in label for label in _form_labels(common_group))
        assert "missing from the project catalog or source directory" in editor.toolTip()
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

    def test_multi_selection_marks_mixed_text_state(self, qapp):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.property_panel import PropertyPanel

        first = WidgetModel("label", name="first", x=10, y=20, width=80, height=24)
        second = WidgetModel("label", name="second", x=10, y=20, width=80, height=24)
        first.properties["text"] = "Alpha"
        second.properties["text"] = "Beta"

        panel = PropertyPanel()
        panel.set_selection([first, second], primary=second)

        summary_group = _find_group(panel, "Selection - 2 Widgets")
        common_group = _find_group(panel, "Common Properties")
        text_editor = panel._editors["prop_text"]

        assert _form_value_text(summary_group, "Mixed:") == "1"
        assert "Text (Mixed):" in _form_labels(common_group)
        assert text_editor.text() == ""
        assert text_editor.placeholderText() == "Mixed values"
        assert "different values" in text_editor.toolTip()
        panel.deleteLater()

    def test_multi_selection_marks_mixed_bool_state(self, qapp):
        from PyQt5.QtCore import Qt
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.property_panel import PropertyPanel

        first = WidgetModel("switch", name="first")
        second = WidgetModel("switch", name="second")
        first.properties["is_checked"] = False
        second.properties["is_checked"] = True

        panel = PropertyPanel()
        panel.set_selection([first, second], primary=second)

        common_group = _find_group(panel, "Common Properties")
        editor = panel._editors["prop_is_checked"]

        assert "Is Checked (Mixed):" in _form_labels(common_group)
        assert editor.isTristate() is True
        assert editor.checkState() == Qt.PartiallyChecked
        assert "different values" in editor.toolTip()
        panel.deleteLater()

    def test_multi_selection_marks_mixed_file_state(self, qapp):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.property_panel import PropertyPanel

        first = WidgetModel("label", name="first")
        second = WidgetModel("label", name="second")
        first.properties["font_file"] = "alpha.ttf"
        second.properties["font_file"] = "beta.ttf"

        panel = PropertyPanel()
        panel.set_selection([first, second], primary=second)

        common_group = _find_group(panel, "Common Properties")
        editor = panel._editors["prop_font_file"]

        assert "Font File (Mixed):" in _form_labels(common_group)
        assert editor.currentIndex() == -1
        assert editor.placeholderText() == "Mixed values"
        assert "different values" in editor.toolTip()
        panel.deleteLater()

    def test_single_selection_shows_interaction_notes_for_locked_hidden_layout_widget(self, qapp):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.property_panel import PropertyPanel

        root = WidgetModel("linearlayout", name="root")
        child = WidgetModel("switch", name="child")
        child.designer_locked = True
        child.designer_hidden = True
        root.add_child(child)

        panel = PropertyPanel()
        panel.set_widget(child)

        notes = _group_label_texts(_find_group(panel, "Interaction Notes"))
        assert any(text.startswith("Locked:") for text in notes)
        assert any(text.startswith("Hidden:") for text in notes)
        assert any(text.startswith("Layout-managed:") for text in notes)
        panel.deleteLater()

    def test_multi_selection_shows_interaction_note_counts(self, qapp):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.property_panel import PropertyPanel

        root = WidgetModel("linearlayout", name="root")
        first = WidgetModel("label", name="first")
        second = WidgetModel("label", name="second")
        first.designer_locked = True
        second.designer_hidden = True
        root.add_child(first)
        root.add_child(second)

        panel = PropertyPanel()
        panel.set_selection([first, second], primary=second)

        notes = _group_label_texts(_find_group(panel, "Interaction Notes"))
        assert "Locked: 1 selected widget cannot be moved or resized from the canvas." in notes
        assert "Hidden: 1 selected widget is skipped by canvas hit testing." in notes
        assert "Layout-managed: 2 selected widgets use parent-controlled positioning." in notes
        panel.deleteLater()

    def test_single_selection_name_edit_rejects_invalid_identifier(self, qapp):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.property_panel import PropertyPanel

        widget = WidgetModel("switch", name="title")
        panel = PropertyPanel()
        messages = []
        panel.validation_message.connect(messages.append)
        panel.set_widget(widget)

        editor = panel._editors["name"]
        editor.setText("123 bad-name")
        editor.editingFinished.emit()

        assert widget.name == "title"
        assert panel._editors["name"].text() == "title"
        assert messages[-1].startswith("Widget name must be a valid C identifier")
        panel.deleteLater()

    def test_single_selection_name_edit_resolves_duplicate_identifier(self, qapp):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.property_panel import PropertyPanel

        root = WidgetModel("group", name="root")
        first = WidgetModel("switch", name="title")
        second = WidgetModel("switch", name="subtitle")
        root.add_child(first)
        root.add_child(second)

        panel = PropertyPanel()
        messages = []
        panel.validation_message.connect(messages.append)
        panel.set_widget(second)

        editor = panel._editors["name"]
        editor.setText("title")
        editor.editingFinished.emit()

        assert second.name == "title_2"
        assert panel._editors["name"].text() == "title_2"
        assert messages[-1] == "Widget name 'title' already exists. Renamed to 'title_2'."
        panel.deleteLater()

"""Property editor panel for EmbeddedGUI Designer."""

import os
import re
import json

from PyQt5.QtWidgets import (
    QWidget, QVBoxLayout, QFormLayout, QLabel,
    QGroupBox, QScrollArea, QHBoxLayout,
    QDialog, QListWidget, QListWidgetItem,
    QDialogButtonBox, QMessageBox, QFileDialog,
)
from PyQt5.QtCore import pyqtSignal, Qt, QSignalBlocker
from PyQt5.QtGui import QFont

from qfluentwidgets import (
    ComboBox, EditableComboBox, SpinBox, LineEdit, CheckBox, ToolButton,
    ListWidget, SearchLineEdit,
)

from ..model.widget_model import (
    WidgetModel, BackgroundModel,
    COLORS, ALPHAS, FONTS, ALIGNS, BG_TYPES,
    IMAGE_FORMATS, IMAGE_ALPHAS, IMAGE_EXTERNALS,
    FONT_PIXELSIZES, FONT_BITSIZES, FONT_EXTERNALS,
)
from ..model.resource_binding import assign_resource_to_widget
from ..model.widget_name import resolve_widget_name, sanitize_widget_name, is_valid_widget_name
from ..model.widget_registry import WidgetRegistry
from .widgets.color_picker import EguiColorPicker
from .widgets.font_selector import EguiFontSelector

# UI group display names
_UI_GROUP_LABELS = {
    "main": "Properties",
    "font_config": "Font Config",
    "image_config": "Image Config",
    "properties": "Properties",
}

_CALLBACK_INVALID_MESSAGE = (
    "Callback name must be a valid C identifier using letters, numbers, and underscores, "
    "and it cannot start with a digit."
)

_MULTI_SUPPORTED_PROPERTY_TYPES = {
    "string",
    "int",
    "bool",
    "color",
    "alpha",
    "align",
    "orientation",
    "image_format",
    "image_alpha",
    "image_external",
    "font_pixelsize",
    "font_fontbitsize",
    "font_external",
    "image_file",
    "font_file",
    "text_file",
}


class CollapsibleGroupBox(QGroupBox):
    """A QGroupBox that can be collapsed/expanded by clicking its title."""

    def __init__(self, title, parent=None):
        super().__init__(title, parent)
        self.setCheckable(True)
        self.setChecked(True)
        self.toggled.connect(self._on_toggled)
        self._content_height = 0

    def _on_toggled(self, checked):
        for i in range(self.layout().count()) if self.layout() else []:
            item = self.layout().itemAt(i)
            if item.widget():
                item.widget().setVisible(checked)
        if checked:
            self.setMaximumHeight(16777215)
        else:
            self.setMaximumHeight(30)


class PropertyPanel(QWidget):
    """Dynamic property editor for the selected widget."""

    property_changed = pyqtSignal()  # emits when any property changes
    resource_imported = pyqtSignal()  # emits when browse auto-import adds a new resource
    validation_message = pyqtSignal(str)  # emits lightweight validation/normalization feedback

    def __init__(self, parent=None):
        super().__init__(parent)
        self._widget = None
        self._selection = []
        self._primary_widget = None
        self._updating = False  # prevent signal loops
        self._editors = {}
        self._resource_dir = ""      # resource/ dir (for generated font scanning)
        self._source_resource_dir = ""  # .eguiproject/resources/ (source files)
        self._last_external_file_dir = ""  # last browsed external file directory
        self._custom_fonts = []       # C expressions from project resource/
        self._resource_catalog = None  # ResourceCatalog instance
        self._string_keys = []        # list of i18n string keys for @string/ completions
        self.setAcceptDrops(True)
        self._init_ui()

    # ── Resource integration API ───────────────────────────────────

    def set_resource_dir(self, path):
        """Store the resource directory (resource/) for generated font scanning."""
        self._resource_dir = path or ""

    def set_source_resource_dir(self, path):
        """Store the source resource directory (.eguiproject/resources/)
        for file browsing, drag-drop resolution, and auto-import."""
        self._source_resource_dir = path or ""

    def set_resource_catalog(self, catalog):
        """Set the resource catalog for populating file selectors."""
        self._resource_catalog = catalog
        # Rebuild form if currently displaying a widget (to update combos)
        if self._primary_widget is not None:
            self._updating = True
            self._rebuild_form()
            self._updating = False

    def set_string_keys(self, keys):
        """Set i18n string keys for @string/ completions in text properties."""
        self._string_keys = list(keys) if keys else []
        # Rebuild form if currently displaying a label/button
        if (self._primary_widget is not None and
                self._primary_widget.widget_type in ("label", "button")):
            self._updating = True
            self._rebuild_form()
            self._updating = False

    def set_custom_fonts(self, font_exprs):
        """Set custom font C expressions from the project resource dir.

        The font QComboBox will show FONTS + these custom entries.
        """
        self._custom_fonts = list(font_exprs) if font_exprs else []
        # Rebuild form if currently displaying a widget (to update font combos)
        if self._primary_widget is not None:
            self._updating = True
            self._rebuild_form()
            self._updating = False

    def _merged_fonts(self):
        """Return built-in FONTS merged with generated font resources (no dups)."""
        seen = set(FONTS)
        merged = list(FONTS)

        # Scan generated font resources from resource/font/
        if self._resource_dir:
            font_dir = os.path.join(self._resource_dir, "font")
            if os.path.isdir(font_dir):
                pattern = re.compile(r'^(egui_res_font_\w+)\.c$')
                for fname in sorted(os.listdir(font_dir)):
                    m = pattern.match(fname)
                    if m:
                        res_name = m.group(1)
                        # Skip _bin variants (external storage)
                        if not res_name.endswith("_bin"):
                            expr = f"&{res_name}"
                            if expr not in seen:
                                merged.append(expr)
                                seen.add(expr)

        # Also include custom fonts from config (for backward compatibility)
        for f in self._custom_fonts:
            if f not in seen:
                merged.append(f)
                seen.add(f)

        return merged

    def _catalog_images(self):
        """Return list of image filenames from catalog."""
        if self._resource_catalog:
            return list(self._resource_catalog.images)
        return []

    def _catalog_fonts(self):
        """Return list of font filenames from catalog."""
        if self._resource_catalog:
            return list(self._resource_catalog.fonts)
        return []

    def _catalog_text_files(self):
        """Return list of text filenames from catalog."""
        if self._resource_catalog:
            return list(self._resource_catalog.text_files)
        return []

    # ── Drop target for resource MIME ──────────────────────────────

    def dragEnterEvent(self, event):
        from .resource_panel import EGUI_RESOURCE_MIME
        if event.mimeData().hasFormat(EGUI_RESOURCE_MIME):
            event.acceptProposedAction()
        else:
            event.ignore()

    def dropEvent(self, event):
        from .resource_panel import EGUI_RESOURCE_MIME
        if not event.mimeData().hasFormat(EGUI_RESOURCE_MIME):
            event.ignore()
            return
        try:
            raw = bytes(event.mimeData().data(EGUI_RESOURCE_MIME)).decode("utf-8")
            info = json.loads(raw)
        except Exception:
            event.ignore()
            return
        res_type = info.get("type", "")
        # Support both new (filename) and legacy (expr) payload formats
        filename = info.get("filename", "")
        expr = info.get("expr", "")
        if self._primary_widget is None:
            event.ignore()
            return

        if filename and assign_resource_to_widget(self._primary_widget, res_type, filename):
            self._rebuild_form()
            self.property_changed.emit()
            event.acceptProposedAction()
        elif res_type == "image" and "image_file" in self._primary_widget.properties:
            if expr:
                # Legacy: parse expr to extract filename
                from ..model.widget_model import parse_legacy_image_expr, _guess_filename_from_c_name
                parsed = parse_legacy_image_expr(expr)
                if parsed:
                    src_dir = os.path.join(self._source_resource_dir, "images") if self._source_resource_dir else ""
                    fn = _guess_filename_from_c_name(parsed["name"], [".png", ".bmp", ".jpg"], src_dir)
                    self._primary_widget.properties["image_file"] = fn
                    self._primary_widget.properties["image_format"] = parsed["format"]
                    self._primary_widget.properties["image_alpha"] = parsed["alpha"]
            self._rebuild_form()
            self.property_changed.emit()
            event.acceptProposedAction()
        elif res_type == "font" and "font_file" in self._primary_widget.properties:
            if expr:
                from ..model.widget_model import parse_legacy_font_expr, _guess_filename_from_c_name
                parsed = parse_legacy_font_expr(expr)
                if parsed and "montserrat" not in parsed["name"]:
                    src_dir = self._source_resource_dir or ""
                    fn = _guess_filename_from_c_name(parsed["name"], [".ttf", ".otf"], src_dir)
                    self._primary_widget.properties["font_file"] = fn
                    self._primary_widget.properties["font_pixelsize"] = parsed["pixelsize"]
                    self._primary_widget.properties["font_fontbitsize"] = parsed["fontbitsize"]
                elif parsed:
                    self._primary_widget.properties["font_builtin"] = expr
            self._rebuild_form()
            self.property_changed.emit()
            event.acceptProposedAction()
        else:
            event.ignore()

    def _init_ui(self):
        outer = QVBoxLayout(self)
        outer.setContentsMargins(4, 4, 4, 4)

        # Search filter
        self._search_edit = SearchLineEdit()
        self._search_edit.setPlaceholderText("Filter properties...")
        self._search_edit.textChanged.connect(self._on_search_changed)
        outer.addWidget(self._search_edit)

        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        outer.addWidget(scroll)

        self._container = QWidget()
        self._layout = QVBoxLayout(self._container)
        self._layout.setContentsMargins(4, 4, 4, 4)
        scroll.setWidget(self._container)

        self._no_selection_label = QLabel("No widget selected")
        self._layout.addWidget(self._no_selection_label)

    def _on_search_changed(self, text):
        """Filter visible property rows by search text."""
        text = text.strip().lower()
        for i in range(self._layout.count()):
            item = self._layout.itemAt(i)
            w = item.widget()
            if not isinstance(w, QGroupBox):
                continue
            layout = w.layout()
            if not isinstance(layout, QFormLayout):
                w.setVisible(not text)
                continue
            any_visible = False
            for row in range(layout.rowCount()):
                label_item = layout.itemAt(row, QFormLayout.LabelRole)
                field_item = layout.itemAt(row, QFormLayout.FieldRole)
                label_text = ""
                if label_item and label_item.widget():
                    label_text = label_item.widget().text().lower()
                visible = not text or text in label_text
                if label_item and label_item.widget():
                    label_item.widget().setVisible(visible)
                if field_item and field_item.widget():
                    field_item.widget().setVisible(visible)
                if visible:
                    any_visible = True
            w.setVisible(any_visible or not text)

    def set_widget(self, widget):
        """Set the widget to edit. None to clear."""
        self.set_selection([widget] if widget is not None else [], primary=widget)

    def set_selection(self, widgets, primary=None):
        widgets = [widget for widget in (widgets or []) if widget is not None]
        self._selection = widgets
        if primary is None or all(widget is not primary for widget in widgets):
            primary = widgets[-1] if widgets else None
        self._primary_widget = primary
        self._widget = primary
        self._rebuild_form()

    def _clear_layout(self, layout):
        while layout.count():
            item = layout.takeAt(0)
            if item.widget():
                item.widget().deleteLater()
            elif item.layout():
                self._clear_layout(item.layout())

    def _rebuild_form(self):
        self._clear_layout(self._layout)
        self._editors = {}

        if self._primary_widget is None:
            self._no_selection_label = QLabel("No widget selected")
            self._layout.addWidget(self._no_selection_label)
            return

        if len(self._selection) > 1:
            self._build_multi_selection_form()
            return

        w = self._primary_widget

        # Common properties group
        common_group = QGroupBox(f"{w.widget_type} - {w.name}")
        common_form = QFormLayout()
        common_group.setLayout(common_form)

        # Name
        name_edit = LineEdit()
        name_edit.setText(w.name)
        name_edit.editingFinished.connect(lambda editor=name_edit: self._on_name_editing_finished(editor))
        common_form.addRow("Name:", name_edit)
        self._editors["name"] = name_edit

        # Position/Size
        for field, label in [("x", "X:"), ("y", "Y:"), ("width", "Width:"), ("height", "Height:")]:
            spin = SpinBox()
            spin.setRange(-9999, 9999)
            spin.setValue(getattr(w, field))
            spin.valueChanged.connect(lambda val, f=field: self._on_common_changed(f, val))
            common_form.addRow(label, spin)
            self._editors[field] = spin

        self._layout.addWidget(common_group)
        self._layout.addWidget(self._build_designer_state_group())
        feedback_group = self._build_selection_feedback_group()
        if feedback_group is not None:
            self._layout.addWidget(feedback_group)

        # Type-specific properties - data-driven grouping
        type_info = WidgetRegistry.instance().get(w.widget_type)
        props = type_info.get("properties", {})

        if props:
            self._build_grouped_properties(w, props)

        callbacks_group = self._build_callbacks_group(w)
        if callbacks_group is not None:
            self._layout.addWidget(callbacks_group)

        # Background properties
        bg_group = QGroupBox("Background")
        bg_form = QFormLayout()
        bg_group.setLayout(bg_form)

        bg = w.background or BackgroundModel()

        bg_type_combo = ComboBox()
        bg_type_combo.addItems(BG_TYPES)
        bg_type_combo.setCurrentText(bg.bg_type)
        bg_type_combo.currentTextChanged.connect(lambda val: self._on_bg_changed("bg_type", val))
        bg_form.addRow("Type:", bg_type_combo)
        self._editors["bg_type"] = bg_type_combo

        if bg.bg_type != "none":
            # Color
            bg_color = EguiColorPicker()
            bg_color.set_value(bg.color)
            bg_color.color_changed.connect(lambda val: self._on_bg_changed("color", val))
            bg_form.addRow("Color:", bg_color)

            # Alpha
            bg_alpha = ComboBox()
            bg_alpha.addItems(ALPHAS)
            bg_alpha.setCurrentText(bg.alpha)
            bg_alpha.currentTextChanged.connect(lambda val: self._on_bg_changed("alpha", val))
            bg_form.addRow("Alpha:", bg_alpha)

            # Radius (for round_rectangle and circle)
            if bg.bg_type in ("round_rectangle", "circle"):
                radius_spin = SpinBox()
                radius_spin.setRange(0, 999)
                radius_spin.setValue(bg.radius)
                radius_spin.valueChanged.connect(lambda val: self._on_bg_changed("radius", val))
                bg_form.addRow("Radius:", radius_spin)

            # Corner radii (for round_rectangle_corners)
            if bg.bg_type == "round_rectangle_corners":
                for corner in ["radius_left_top", "radius_left_bottom", "radius_right_top", "radius_right_bottom"]:
                    spin = SpinBox()
                    spin.setRange(0, 999)
                    spin.setValue(getattr(bg, corner))
                    spin.valueChanged.connect(lambda val, c=corner: self._on_bg_changed(c, val))
                    label = corner.replace("radius_", "").replace("_", " ").title() + ":"
                    bg_form.addRow(label, spin)

            # Stroke
            stroke_spin = SpinBox()
            stroke_spin.setRange(0, 50)
            stroke_spin.setValue(bg.stroke_width)
            stroke_spin.valueChanged.connect(lambda val: self._on_bg_changed("stroke_width", val))
            bg_form.addRow("Stroke Width:", stroke_spin)

            if bg.stroke_width > 0:
                stroke_color = EguiColorPicker()
                stroke_color.set_value(bg.stroke_color)
                stroke_color.color_changed.connect(lambda val: self._on_bg_changed("stroke_color", val))
                bg_form.addRow("Stroke Color:", stroke_color)

                stroke_alpha = ComboBox()
                stroke_alpha.addItems(ALPHAS)
                stroke_alpha.setCurrentText(bg.stroke_alpha)
                stroke_alpha.currentTextChanged.connect(lambda val: self._on_bg_changed("stroke_alpha", val))
                bg_form.addRow("Stroke Alpha:", stroke_alpha)

            # Pressed state
            pressed_check = CheckBox("Enable pressed state")
            pressed_check.setChecked(bg.has_pressed)
            pressed_check.toggled.connect(lambda val: self._on_bg_changed("has_pressed", val))
            bg_form.addRow(pressed_check)

            if bg.has_pressed:
                pressed_color = EguiColorPicker()
                pressed_color.set_value(bg.pressed_color)
                pressed_color.color_changed.connect(lambda val: self._on_bg_changed("pressed_color", val))
                bg_form.addRow("Pressed Color:", pressed_color)

        self._layout.addWidget(bg_group)
        self._layout.addStretch()

    def _build_multi_selection_form(self):
        summary = QGroupBox(f"Selection - {len(self._selection)} Widgets")
        summary_form = QFormLayout()
        summary.setLayout(summary_form)

        widget_types = sorted({widget.widget_type for widget in self._selection})
        mixed_geometry = sum(1 for field in ("x", "y", "width", "height") if self._is_mixed_values(getattr(widget, field) for widget in self._selection))
        mixed_props = sum(1 for prop_name, _ in self._collect_multi_common_properties() if self._is_mixed_values(widget.properties.get(prop_name) for widget in self._selection))
        summary_form.addRow("Primary:", QLabel(self._primary_widget.name if self._primary_widget else ""))
        summary_form.addRow("Types:", QLabel(", ".join(widget_types)))
        summary_form.addRow("Mixed:", QLabel(str(mixed_geometry + mixed_props)))
        summary_form.addRow("Hint:", QLabel("Batch edits apply the same value to all selected widgets."))
        self._layout.addWidget(summary)

        geometry_group = QGroupBox("Batch Geometry")
        geometry_form = QFormLayout()
        geometry_group.setLayout(geometry_form)
        for field, label in (("x", "X:"), ("y", "Y:"), ("width", "Width:"), ("height", "Height:")):
            spin = SpinBox()
            spin.setRange(-9999, 9999)
            spin.setValue(getattr(self._primary_widget, field))
            is_mixed = self._is_mixed_values(getattr(widget, field) for widget in self._selection)
            if is_mixed:
                spin.setToolTip("Selected widgets currently have different values. Editing here will normalize them.")
            spin.valueChanged.connect(lambda value, f=field: self._on_multi_common_changed(f, value))
            geometry_form.addRow(f"{label[:-1]} (Mixed):" if is_mixed else label, spin)
            self._editors[f"multi_{field}"] = spin
        self._layout.addWidget(geometry_group)

        self._build_multi_common_properties_group()
        self._layout.addWidget(self._build_designer_state_group())
        feedback_group = self._build_selection_feedback_group()
        if feedback_group is not None:
            self._layout.addWidget(feedback_group)
        self._layout.addStretch()

    def _build_designer_state_group(self):
        group = QGroupBox("Designer")
        form = QFormLayout()
        group.setLayout(form)

        locked = CheckBox("Locked")
        locked.setChecked(all(getattr(widget, "designer_locked", False) for widget in self._selection) if self._selection else False)
        locked.toggled.connect(lambda value: self._on_designer_flag_changed("designer_locked", value))
        form.addRow(locked)

        hidden = CheckBox("Hidden")
        hidden.setChecked(all(getattr(widget, "designer_hidden", False) for widget in self._selection) if self._selection else False)
        hidden.toggled.connect(lambda value: self._on_designer_flag_changed("designer_hidden", value))
        form.addRow(hidden)

        return group

    def _layout_parent_name(self, widget):
        parent = getattr(widget, "parent", None)
        if parent is None:
            return ""
        type_info = WidgetRegistry.instance().get(parent.widget_type)
        if type_info.get("layout_func") is None:
            return ""
        return parent.widget_type

    def _selection_feedback_messages(self):
        if not self._selection:
            return []

        if len(self._selection) == 1:
            widget = self._selection[0]
            messages = []
            if getattr(widget, "designer_locked", False):
                messages.append("Locked: canvas drag and resize are disabled for this widget.")
            if getattr(widget, "designer_hidden", False):
                messages.append("Hidden: this widget is skipped by canvas hit testing.")
            layout_parent = self._layout_parent_name(widget)
            if layout_parent:
                messages.append(
                    f"Layout-managed: x/y come from parent {layout_parent}, so canvas handles are disabled."
                )
            return messages

        messages = []
        locked_count = sum(1 for widget in self._selection if getattr(widget, "designer_locked", False))
        hidden_count = sum(1 for widget in self._selection if getattr(widget, "designer_hidden", False))
        layout_count = sum(1 for widget in self._selection if self._layout_parent_name(widget))

        if locked_count:
            noun = "widget" if locked_count == 1 else "widgets"
            messages.append(
                f"Locked: {locked_count} selected {noun} cannot be moved or resized from the canvas."
            )
        if hidden_count:
            noun = "widget" if hidden_count == 1 else "widgets"
            verb = "is" if hidden_count == 1 else "are"
            messages.append(
                f"Hidden: {hidden_count} selected {noun} {verb} skipped by canvas hit testing."
            )
        if layout_count:
            noun = "widget" if layout_count == 1 else "widgets"
            messages.append(
                f"Layout-managed: {layout_count} selected {noun} use parent-controlled positioning."
            )

        return messages

    def _build_selection_feedback_group(self):
        messages = self._selection_feedback_messages()
        if not messages:
            return None

        group = QGroupBox("Interaction Notes")
        layout = QVBoxLayout()
        group.setLayout(layout)

        for message in messages:
            label = QLabel(message)
            label.setWordWrap(True)
            layout.addWidget(label)

        return group

    def _build_multi_common_properties_group(self):
        common_props = self._collect_multi_common_properties()
        if not common_props:
            return

        group = QGroupBox("Common Properties")
        form = QFormLayout()
        group.setLayout(form)

        for prop_name, prop_info in common_props:
            current_value = self._primary_widget.properties.get(prop_name)
            values = [widget.properties.get(prop_name) for widget in self._selection]
            is_mixed = self._is_mixed_values(values)
            has_missing_file = any(self._is_missing_file_property(prop_name, prop_info, value) for value in values)
            editor = self._create_property_editor(
                prop_name,
                prop_info,
                current_value,
                prop_changed_handler=self._on_multi_prop_changed,
            )
            if editor is None:
                continue

            if is_mixed:
                self._apply_mixed_editor_state(editor, prop_name, prop_info, values)
            if has_missing_file:
                self._apply_missing_file_editor_state(editor, prop_name, prop_info, current_value, values=values)

            label = prop_name.replace("_", " ").title()
            if is_mixed:
                label += " (Mixed)"
            if has_missing_file:
                label += " (Missing)"
            label += ":"
            form.addRow(label, editor)

        if form.rowCount() > 0:
            self._layout.addWidget(group)

    def _normalize_mixed_value(self, value):
        if isinstance(value, dict):
            return tuple(
                (key, self._normalize_mixed_value(item))
                for key, item in sorted(value.items())
            )
        if isinstance(value, (list, tuple)):
            return tuple(self._normalize_mixed_value(item) for item in value)
        if isinstance(value, set):
            return tuple(sorted(self._normalize_mixed_value(item) for item in value))
        return value

    def _is_mixed_values(self, values):
        iterator = iter(values)
        try:
            first_value = self._normalize_mixed_value(next(iterator))
        except StopIteration:
            return False

        for value in iterator:
            if self._normalize_mixed_value(value) != first_value:
                return True
        return False

    def _apply_mixed_editor_state(self, editor, prop_name, prop_info, values):
        del values

        tooltip = "Selected widgets currently have different values. Editing here will normalize them."
        editor.setToolTip(tooltip)

        ptype = prop_info.get("type", "string")
        target = editor
        if ptype in {"image_file", "font_file", "text_file"}:
            target = self._editors.get(f"prop_{prop_name}", editor)

        if isinstance(target, LineEdit):
            with QSignalBlocker(target):
                target.clear()
                target.setPlaceholderText("Mixed values")
            target.setToolTip(tooltip)
            return

        if isinstance(target, EditableComboBox):
            with QSignalBlocker(target):
                if hasattr(target, "setPlaceholderText"):
                    target.setPlaceholderText("Mixed values")
                if hasattr(target, "setCurrentIndex"):
                    target.setCurrentIndex(-1)
            target.setToolTip(tooltip)
            return

        if isinstance(target, ComboBox):
            with QSignalBlocker(target):
                if hasattr(target, "setPlaceholderText"):
                    target.setPlaceholderText("Mixed values")
                if hasattr(target, "setCurrentIndex"):
                    target.setCurrentIndex(-1)
            target.setToolTip(tooltip)
            return

        if isinstance(target, CheckBox):
            with QSignalBlocker(target):
                target.setTristate(True)
                target.setCheckState(Qt.PartiallyChecked)
            target.setToolTip(tooltip)
            return

    def _is_missing_file_property(self, prop_name, prop_info, value):
        return bool(self._missing_file_property_reason(prop_name, prop_info, value))

    def _source_file_path_for_property(self, prop_info, value):
        if not value or not self._source_resource_dir:
            return ""

        ptype = prop_info.get("type", "")
        if ptype == "image_file":
            return os.path.join(self._source_resource_dir, "images", value)
        if ptype in {"font_file", "text_file"}:
            return os.path.join(self._source_resource_dir, value)
        return ""

    def _missing_file_property_reason(self, prop_name, prop_info, value):
        del prop_name

        if not value:
            return ""

        ptype = prop_info.get("type", "")
        if ptype == "image_file":
            if self._resource_catalog is not None and not self._resource_catalog.has_image(value):
                return "catalog"
        elif ptype == "font_file":
            if self._resource_catalog is not None and not self._resource_catalog.has_font(value):
                return "catalog"
        elif ptype == "text_file":
            if self._resource_catalog is not None and not self._resource_catalog.has_text_file(value):
                return "catalog"
        else:
            return ""

        source_path = self._source_file_path_for_property(prop_info, value)
        if source_path and not os.path.isfile(source_path):
            return "disk"
        return ""

    def _missing_file_message(self, reason, plural=False):
        if plural:
            return "One or more selected widgets reference resource files that are missing from the project catalog or source directory."
        if reason == "disk":
            return "Selected resource file is listed in the project catalog, but the source file is missing on disk. Restore it or choose another file."
        return "Selected resource file is not present in the project catalog. Re-import it or choose another file."

    def _apply_missing_file_editor_state(self, editor, prop_name, prop_info, current_value, values=None):
        reason = self._missing_file_property_reason(prop_name, prop_info, current_value)
        plural = False
        if values is not None:
            reasons = {
                self._missing_file_property_reason(prop_name, prop_info, value)
                for value in values
            }
            reasons.discard("")
            if not reasons:
                return
            if len(reasons) > 1 or len(values) > 1:
                plural = True
                reason = next(iter(reasons))
            elif not reason:
                reason = next(iter(reasons))

        if not reason:
            return

        target = editor
        if prop_info.get("type", "") in {"image_file", "font_file", "text_file"}:
            target = self._editors.get(f"prop_{prop_name}", editor)

        message = self._missing_file_message(reason, plural=plural)
        existing_tooltip = target.toolTip().strip()
        if existing_tooltip and message not in existing_tooltip:
            message = f"{existing_tooltip}\n{message}"
        target.setToolTip(message)

    def _collect_multi_common_properties(self):
        if not self._selection:
            return []

        descriptors = [WidgetRegistry.instance().get(widget.widget_type) for widget in self._selection]
        if not descriptors:
            return []

        base_props = descriptors[0].get("properties", {})
        result = []
        for prop_name, prop_info in base_props.items():
            ptype = prop_info.get("type", "string")
            if ptype not in _MULTI_SUPPORTED_PROPERTY_TYPES:
                continue

            shared = True
            for widget, descriptor in zip(self._selection[1:], descriptors[1:]):
                other_info = descriptor.get("properties", {}).get(prop_name)
                if not other_info:
                    shared = False
                    break
                if other_info.get("type", "string") != ptype:
                    shared = False
                    break
                visible_when = other_info.get("ui_visible_when")
                if visible_when and not self._check_visibility(widget, visible_when):
                    shared = False
                    break

            visible_when = prop_info.get("ui_visible_when")
            if visible_when and not self._check_visibility(self._selection[0], visible_when):
                shared = False

            if shared:
                result.append((prop_name, prop_info))

        return result

    # ── Property group builders ───────────────────────────────────

    def _check_visibility(self, widget, condition):
        """Check ui_visible_when condition against widget properties.

        Supported conditions:
            {"prop_name": "empty"}   — visible when prop is empty/falsy
            {"prop_name": "!empty"}  — visible when prop is non-empty/truthy
        """
        for prop_name, rule in condition.items():
            val = widget.properties.get(prop_name, "")
            if rule == "empty":
                if val:
                    return False
            elif rule == "!empty":
                if not val:
                    return False
        return True

    def _build_grouped_properties(self, w, props):
        """Build property groups driven by ui_group and ui_visible_when descriptors.

        Properties are grouped by their ``ui_group`` value (default "properties").
        Properties with ``ui_visible_when`` are conditionally shown/hidden.
        Groups are rendered in encounter order.
        """
        from collections import OrderedDict
        groups = OrderedDict()

        for prop_name, prop_info in props.items():
            # Check visibility condition
            vis = prop_info.get("ui_visible_when")
            if vis and not self._check_visibility(w, vis):
                continue

            group_key = prop_info.get("ui_group", "properties")
            if group_key not in groups:
                groups[group_key] = []
            groups[group_key].append((prop_name, prop_info))

        for group_key, group_props in groups.items():
            group_label = _UI_GROUP_LABELS.get(group_key, group_key.replace("_", " ").title())
            group_box = CollapsibleGroupBox(group_label)
            form = QFormLayout()
            group_box.setLayout(form)

            for prop_name, prop_info in group_props:
                editor = self._create_property_editor(prop_name, prop_info, w.properties.get(prop_name))
                if editor:
                    # Derive a human-readable label from prop_name
                    label = prop_name
                    # Strip common prefixes for cleaner display
                    for prefix in ("image_", "font_"):
                        if label.startswith(prefix) and group_key != "main":
                            label = label[len(prefix):]
                            break
                    label = label.replace("_", " ").title()
                    if self._is_missing_file_property(prop_name, prop_info, w.properties.get(prop_name)):
                        label += " (Missing)"
                        self._apply_missing_file_editor_state(editor, prop_name, prop_info, w.properties.get(prop_name))
                    label += ":"
                    form.addRow(label, editor)

            self._layout.addWidget(group_box)

    def _humanize_callback_name(self, event_name):
        name = str(event_name or "").strip()
        if name.startswith("on") and len(name) > 2 and name[2].isupper():
            name = name[2:]
        name = re.sub(r"([a-z0-9])([A-Z])", r"\1 \2", name)
        return name.strip().title() or "Callback"

    def _suggest_callback_name(self, widget, event_name):
        widget_name = sanitize_widget_name(getattr(widget, "name", "")) or getattr(widget, "widget_type", "widget")
        suffix = self._humanize_callback_name(event_name).lower().replace(" ", "_")
        return f"on_{widget_name}_{suffix}"

    def _callback_signature_preview(self, signature):
        if not signature:
            return ""
        try:
            return signature.format(func_name="callback_name")
        except Exception:
            return signature

    def _callback_tooltip(self, widget, event_name, signature):
        parts = [
            "Leave empty to disable this callback.",
            f"Suggested: {self._suggest_callback_name(widget, event_name)}",
        ]
        preview = self._callback_signature_preview(signature)
        if preview:
            parts.append(f"Signature: {preview}")
        return "\n".join(parts)

    def _callback_entries(self, widget):
        entries = [
            {
                "event_name": "onClick",
                "value": widget.on_click,
                "signature": "void {func_name}(egui_view_t *self)",
                "use_event_dict": False,
            }
        ]

        descriptor = WidgetRegistry.instance().get(widget.widget_type)
        for event_name, event_info in descriptor.get("events", {}).items():
            entries.append(
                {
                    "event_name": event_name,
                    "value": widget.events.get(event_name, ""),
                    "signature": event_info.get("signature", ""),
                    "use_event_dict": True,
                }
            )

        known_names = {entry["event_name"] for entry in entries}
        for event_name in sorted(widget.events):
            if event_name in known_names:
                continue
            entries.append(
                {
                    "event_name": event_name,
                    "value": widget.events.get(event_name, ""),
                    "signature": "",
                    "use_event_dict": True,
                }
            )

        return entries

    def _build_callbacks_group(self, widget):
        entries = self._callback_entries(widget)
        if not entries:
            return None

        group = CollapsibleGroupBox("Callbacks")
        form = QFormLayout()
        group.setLayout(form)

        for entry in entries:
            event_name = entry["event_name"]
            editor = LineEdit()
            editor.setText(entry["value"])
            editor.setPlaceholderText(self._suggest_callback_name(widget, event_name))
            editor.setToolTip(self._callback_tooltip(widget, event_name, entry["signature"]))
            editor.editingFinished.connect(
                lambda editor=editor,
                current_widget=widget,
                event_name=event_name,
                signature=entry["signature"],
                use_event_dict=entry["use_event_dict"]: self._on_callback_editing_finished(
                    editor,
                    current_widget,
                    event_name,
                    signature,
                    use_event_dict,
                )
            )
            self._editors[f"callback_{event_name}"] = editor
            form.addRow(f"{self._humanize_callback_name(event_name)}:", editor)

        return group

    # ── Property editor factory ───────────────────────────────────

    def _create_property_editor(self, prop_name, prop_info, current_value, prop_changed_handler=None, file_prop_handler=None):
        ptype = prop_info.get("type", "string")
        prop_changed_handler = prop_changed_handler or self._on_prop_changed
        file_prop_handler = file_prop_handler or self._on_file_prop_changed

        if ptype == "string":
            # For "text" property on label/button, use EditableComboBox with @string/ completions
            if prop_name == "text" and self._string_keys:
                editor = EditableComboBox()
                # Add @string/key references from the i18n catalog
                for key in self._string_keys:
                    editor.addItem(f"@string/{key}")
                cur = str(current_value or "")
                if cur and editor.findText(cur) < 0:
                    editor.addItem(cur)
                editor.setCurrentText(cur)
                editor.currentTextChanged.connect(lambda val: prop_changed_handler(prop_name, val))
                self._editors[f"prop_{prop_name}"] = editor
                return editor
            else:
                editor = LineEdit()
                editor.setText(str(current_value or ""))
                editor.textChanged.connect(lambda val: prop_changed_handler(prop_name, val))
                self._editors[f"prop_{prop_name}"] = editor
                return editor

        elif ptype == "int":
            editor = SpinBox()
            editor.setRange(prop_info.get("min", 0), prop_info.get("max", 9999))
            editor.setValue(int(current_value or 0))
            editor.valueChanged.connect(lambda val: prop_changed_handler(prop_name, val))
            self._editors[f"prop_{prop_name}"] = editor
            return editor

        elif ptype == "bool":
            editor = CheckBox()
            editor.setChecked(bool(current_value))
            editor.toggled.connect(lambda val: prop_changed_handler(prop_name, val))
            self._editors[f"prop_{prop_name}"] = editor
            return editor

        elif ptype == "color":
            editor = EguiColorPicker()
            editor.set_value(str(current_value or COLORS[0]))
            editor.color_changed.connect(lambda val: prop_changed_handler(prop_name, val))
            self._editors[f"prop_{prop_name}"] = editor
            return editor

        elif ptype == "alpha":
            editor = ComboBox()
            editor.addItems(ALPHAS)
            editor.setCurrentText(str(current_value or "EGUI_ALPHA_100"))
            editor.currentTextChanged.connect(lambda val: prop_changed_handler(prop_name, val))
            self._editors[f"prop_{prop_name}"] = editor
            return editor

        elif ptype == "font":
            # Built-in font selector with preview
            merged = self._merged_fonts()
            editor = EguiFontSelector(fonts=merged)
            cur = str(current_value or merged[0] if merged else "")
            editor.set_value(cur)
            editor.font_changed.connect(lambda val: prop_changed_handler(prop_name, val))
            self._editors[f"prop_{prop_name}"] = editor
            return editor

        elif ptype == "align":
            editor = ComboBox()
            editor.addItems(ALIGNS)
            editor.setCurrentText(str(current_value or "EGUI_ALIGN_CENTER"))
            editor.currentTextChanged.connect(lambda val: prop_changed_handler(prop_name, val))
            self._editors[f"prop_{prop_name}"] = editor
            return editor

        elif ptype == "orientation":
            editor = ComboBox()
            editor.addItems(["vertical", "horizontal"])
            editor.setCurrentText(str(current_value or "vertical"))
            editor.currentTextChanged.connect(lambda val: prop_changed_handler(prop_name, val))
            self._editors[f"prop_{prop_name}"] = editor
            return editor

        # ── New resource-related property types ───────────────────

        elif ptype == "image_file":
            return self._create_file_selector(prop_name, current_value,
                                              self._catalog_images(), "Image files (*.png *.bmp *.jpg *.jpeg *.gif)", file_prop_handler=file_prop_handler)

        elif ptype == "image_format":
            editor = ComboBox()
            editor.addItems(IMAGE_FORMATS)
            editor.setCurrentText(str(current_value or "rgb565"))
            editor.currentTextChanged.connect(lambda val: prop_changed_handler(prop_name, val))
            self._editors[f"prop_{prop_name}"] = editor
            return editor

        elif ptype == "image_alpha":
            editor = ComboBox()
            editor.addItems(IMAGE_ALPHAS)
            editor.setCurrentText(str(current_value or "4"))
            editor.currentTextChanged.connect(lambda val: prop_changed_handler(prop_name, val))
            self._editors[f"prop_{prop_name}"] = editor
            return editor

        elif ptype == "image_external":
            editor = ComboBox()
            editor.addItems(IMAGE_EXTERNALS)
            editor.setCurrentText(str(current_value or "0"))
            editor.currentTextChanged.connect(lambda val: prop_changed_handler(prop_name, val))
            self._editors[f"prop_{prop_name}"] = editor
            return editor

        elif ptype == "font_file":
            return self._create_file_selector(prop_name, current_value,
                                              self._catalog_fonts(), "Font files (*.ttf *.otf)", file_prop_handler=file_prop_handler)

        elif ptype == "font_pixelsize":
            editor = EditableComboBox()
            editor.addItems(FONT_PIXELSIZES)
            editor.setCurrentText(str(current_value or "16"))
            editor.currentTextChanged.connect(lambda val: prop_changed_handler(prop_name, val))
            self._editors[f"prop_{prop_name}"] = editor
            return editor

        elif ptype == "font_fontbitsize":
            editor = ComboBox()
            editor.addItems(FONT_BITSIZES)
            editor.setCurrentText(str(current_value or "4"))
            editor.currentTextChanged.connect(lambda val: prop_changed_handler(prop_name, val))
            self._editors[f"prop_{prop_name}"] = editor
            return editor

        elif ptype == "font_external":
            editor = ComboBox()
            editor.addItems(FONT_EXTERNALS)
            editor.setCurrentText(str(current_value or "0"))
            editor.currentTextChanged.connect(lambda val: prop_changed_handler(prop_name, val))
            self._editors[f"prop_{prop_name}"] = editor
            return editor

        elif ptype == "text_file":
            return self._create_file_selector(prop_name, current_value,
                                              self._catalog_text_files(), "Text files (*.txt)", file_prop_handler=file_prop_handler)

        return None

    def _create_file_selector(self, prop_name, current_value, catalog_items, file_filter, file_prop_handler=None):
        """Create a ComboBox + '...' browse button for file selection."""
        file_prop_handler = file_prop_handler or self._on_file_prop_changed
        container = QWidget()
        h_layout = QHBoxLayout(container)
        h_layout.setContentsMargins(0, 0, 0, 0)
        h_layout.setSpacing(2)

        combo = EditableComboBox()
        # Add empty option (none selected)
        items = [""] + catalog_items
        combo.addItems(items)
        cur = str(current_value or "")
        if cur and combo.findText(cur) < 0:
            combo.addItem(cur)
        combo.setCurrentText(cur)
        combo.currentTextChanged.connect(lambda val: file_prop_handler(prop_name, val))
        h_layout.addWidget(combo, 1)

        browse_btn = ToolButton()
        browse_btn.setText("...")
        browse_btn.setToolTip("Browse files")
        browse_btn.clicked.connect(lambda: self._browse_file(combo, file_filter))
        h_layout.addWidget(browse_btn)

        self._editors[f"prop_{prop_name}"] = combo
        return container

    def _browse_file(self, combo, file_filter):
        """Open a file dialog to select a file."""
        src_dir = self._default_file_browse_dir(file_filter)
        if not src_dir:
            QMessageBox.warning(
                self,
                "Resource Directory Missing",
                "Please save the project first so Designer has a resource directory for importing files.",
            )
            return

        path, _ = QFileDialog.getOpenFileName(self, "Select File", src_dir, file_filter)
        if path:
            filename = os.path.basename(path)
            path_dir = os.path.dirname(path)
            if path_dir and os.path.isdir(path_dir):
                self._last_external_file_dir = path_dir
            imported = False
            # Auto-import: copy to .eguiproject/resources/ if not there
            if self._source_resource_dir:
                # Images go in images/ subfolder, fonts/text go in root
                ext = os.path.splitext(filename)[1].lower()
                if ext in ('.png', '.bmp', '.jpg', '.jpeg'):
                    dest_dir = os.path.join(self._source_resource_dir, "images")
                else:
                    dest_dir = self._source_resource_dir
                dest = os.path.join(dest_dir, filename)
                if not os.path.isfile(dest):
                    import shutil
                    os.makedirs(dest_dir, exist_ok=True)
                    shutil.copy2(path, dest)
                    imported = True
                # Add to catalog
                if self._resource_catalog:
                    had_file = (
                        self._resource_catalog.has_image(filename)
                        or self._resource_catalog.has_font(filename)
                        or self._resource_catalog.has_text_file(filename)
                    )
                    self._resource_catalog.add_file(filename)
                    if not had_file:
                        imported = True

            # Ensure filename is in combo
            if combo.findText(filename) < 0:
                combo.addItem(filename)
            combo.setCurrentText(filename)
            if imported:
                self.resource_imported.emit()

    def _default_file_browse_dir(self, file_filter):
        if self._last_external_file_dir and os.path.isdir(self._last_external_file_dir):
            return self._last_external_file_dir

        src_dir = self._source_resource_dir or ""
        if not src_dir or not os.path.isdir(src_dir):
            return ""

        lower_filter = (file_filter or "").lower()
        if any(ext in lower_filter for ext in (".png", ".bmp", ".jpg", ".jpeg")):
            images_dir = os.path.join(src_dir, "images")
            if os.path.isdir(images_dir):
                return images_dir

        return src_dir

    def _on_file_prop_changed(self, prop_name, value):
        """Handle file property change - rebuild form if needed for conditional groups."""
        if self._updating or self._primary_widget is None:
            return
        self._primary_widget.properties[prop_name] = value

        # Changing font_file or image_file may show/hide config groups
        if prop_name in ("image_file", "font_file"):
            self._updating = True
            self._rebuild_form()
            self._updating = False

        self.property_changed.emit()

    def _on_common_changed(self, field, value):
        if self._updating or self._primary_widget is None:
            return
        if field == "name":
            self._primary_widget.name = value
        else:
            setattr(self._primary_widget, field, value)
        self.property_changed.emit()

    def _on_name_editing_finished(self, editor):
        if self._updating or self._primary_widget is None:
            return

        raw_name = editor.text()
        ok, resolved_name, message = resolve_widget_name(self._primary_widget, raw_name)
        current_name = self._primary_widget.name

        if not ok:
            with QSignalBlocker(editor):
                editor.setText(current_name)
            editor.setToolTip(message)
            self.validation_message.emit(message)
            return

        name_changed = resolved_name != current_name
        text_changed = raw_name != resolved_name
        if name_changed:
            self._primary_widget.name = resolved_name

        if name_changed or text_changed:
            self._updating = True
            self._rebuild_form()
            self._updating = False
            refreshed = self._editors.get("name")
            if refreshed is not None and message:
                refreshed.setToolTip(message)
        elif message:
            editor.setToolTip(message)

        if message:
            self.validation_message.emit(message)

        if name_changed:
            self.property_changed.emit()

    def _current_callback_value(self, widget, event_name, use_event_dict):
        if use_event_dict:
            return widget.events.get(event_name, "")
        return widget.on_click

    def _set_callback_value(self, widget, event_name, use_event_dict, value):
        if use_event_dict:
            if value:
                widget.events[event_name] = value
            else:
                widget.events.pop(event_name, None)
            return
        widget.on_click = value

    def _on_callback_editing_finished(self, editor, widget, event_name, signature, use_event_dict):
        if self._updating or widget is None:
            return

        raw_name = editor.text()
        normalized = sanitize_widget_name(raw_name)
        current_value = self._current_callback_value(widget, event_name, use_event_dict)

        if normalized and not is_valid_widget_name(normalized):
            with QSignalBlocker(editor):
                editor.setText(current_value)
            editor.setToolTip(self._callback_tooltip(widget, event_name, signature))
            self.validation_message.emit(_CALLBACK_INVALID_MESSAGE)
            return

        changed = normalized != current_value
        text_changed = raw_name != normalized

        if changed:
            self._set_callback_value(widget, event_name, use_event_dict, normalized)

        if changed or text_changed:
            with QSignalBlocker(editor):
                editor.setText(normalized)

        editor.setToolTip(self._callback_tooltip(widget, event_name, signature))

        if text_changed and normalized:
            self.validation_message.emit(f"Callback name normalized to '{normalized}'.")

        if changed:
            self.property_changed.emit()

    def _on_prop_changed(self, prop_name, value):
        if self._updating or self._primary_widget is None:
            return
        self._primary_widget.properties[prop_name] = value
        self.property_changed.emit()

    def _on_multi_common_changed(self, field, value):
        if self._updating or not self._selection:
            return
        for widget in self._selection:
            setattr(widget, field, value)
        self.property_changed.emit()

    def _on_multi_prop_changed(self, prop_name, value):
        if self._updating or not self._selection:
            return

        for widget in self._selection:
            if prop_name in widget.properties:
                widget.properties[prop_name] = value

        if self._multi_prop_requires_rebuild(prop_name):
            self._updating = True
            self._rebuild_form()
            self._updating = False

        self.property_changed.emit()

    def _multi_prop_requires_rebuild(self, prop_name):
        for widget in self._selection:
            descriptor = WidgetRegistry.instance().get(widget.widget_type)
            for info in descriptor.get("properties", {}).values():
                visible_when = info.get("ui_visible_when", {})
                if prop_name in visible_when:
                    return True
        return False

    def _on_bg_changed(self, field, value):
        if self._updating or self._primary_widget is None:
            return

        if self._primary_widget.background is None:
            self._primary_widget.background = BackgroundModel()

        bg = self._primary_widget.background
        setattr(bg, field, value)

        # If bg_type changed to "none", remove background
        if field == "bg_type" and value == "none":
            self._primary_widget.background = None

        # Rebuild form to show/hide dynamic fields
        if field in ("bg_type", "stroke_width", "has_pressed"):
            self._updating = True
            self._rebuild_form()
            self._updating = False

        self.property_changed.emit()

    def _on_designer_flag_changed(self, field, value):
        if self._updating or not self._selection:
            return
        for widget in self._selection:
            setattr(widget, field, bool(value))
        self.property_changed.emit()

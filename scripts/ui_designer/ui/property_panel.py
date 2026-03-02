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
from PyQt5.QtCore import pyqtSignal, Qt
from PyQt5.QtGui import QFont

from qfluentwidgets import (
    ComboBox, EditableComboBox, SpinBox, LineEdit, CheckBox, ToolButton,
    BodyLabel, ListWidget, SearchLineEdit,
)

from ..model.widget_model import (
    WidgetModel, BackgroundModel,
    COLORS, ALPHAS, FONTS, ALIGNS, BG_TYPES,
    IMAGE_FORMATS, IMAGE_ALPHAS, IMAGE_EXTERNALS,
    FONT_PIXELSIZES, FONT_BITSIZES, FONT_EXTERNALS,
)
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

    def __init__(self, parent=None):
        super().__init__(parent)
        self._widget = None
        self._updating = False  # prevent signal loops
        self._editors = {}
        self._resource_dir = ""      # resource/ dir (for generated font scanning)
        self._source_resource_dir = ""  # .eguiproject/resources/ (source files)
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
        if self._widget is not None:
            self._updating = True
            self._rebuild_form()
            self._updating = False

    def set_string_keys(self, keys):
        """Set i18n string keys for @string/ completions in text properties."""
        self._string_keys = list(keys) if keys else []
        # Rebuild form if currently displaying a label/button
        if (self._widget is not None and
                self._widget.widget_type in ("label", "button")):
            self._updating = True
            self._rebuild_form()
            self._updating = False

    def set_custom_fonts(self, font_exprs):
        """Set custom font C expressions from the project resource dir.

        The font QComboBox will show FONTS + these custom entries.
        """
        self._custom_fonts = list(font_exprs) if font_exprs else []
        # Rebuild form if currently displaying a widget (to update font combos)
        if self._widget is not None:
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
        if self._widget is None:
            event.ignore()
            return

        wtype = self._widget.widget_type
        if res_type == "image" and "image_file" in self._widget.properties:
            if filename:
                self._widget.properties["image_file"] = filename
            elif expr:
                # Legacy: parse expr to extract filename
                from ..model.widget_model import parse_legacy_image_expr, _guess_filename_from_c_name
                parsed = parse_legacy_image_expr(expr)
                if parsed:
                    src_dir = os.path.join(self._source_resource_dir, "images") if self._source_resource_dir else ""
                    fn = _guess_filename_from_c_name(parsed["name"], [".png", ".bmp", ".jpg"], src_dir)
                    self._widget.properties["image_file"] = fn
                    self._widget.properties["image_format"] = parsed["format"]
                    self._widget.properties["image_alpha"] = parsed["alpha"]
            self._rebuild_form()
            self.property_changed.emit()
            event.acceptProposedAction()
        elif res_type == "font" and "font_file" in self._widget.properties:
            if filename:
                self._widget.properties["font_file"] = filename
            elif expr:
                from ..model.widget_model import parse_legacy_font_expr, _guess_filename_from_c_name
                parsed = parse_legacy_font_expr(expr)
                if parsed and "montserrat" not in parsed["name"]:
                    src_dir = self._source_resource_dir or ""
                    fn = _guess_filename_from_c_name(parsed["name"], [".ttf", ".otf"], src_dir)
                    self._widget.properties["font_file"] = fn
                    self._widget.properties["font_pixelsize"] = parsed["pixelsize"]
                    self._widget.properties["font_fontbitsize"] = parsed["fontbitsize"]
                elif parsed:
                    self._widget.properties["font_builtin"] = expr
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
        self._widget = widget
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

        if self._widget is None:
            self._no_selection_label = QLabel("No widget selected")
            self._layout.addWidget(self._no_selection_label)
            return

        w = self._widget

        # Common properties group
        common_group = QGroupBox(f"{w.widget_type} - {w.name}")
        common_form = QFormLayout()
        common_group.setLayout(common_form)

        # Name
        name_edit = LineEdit()
        name_edit.setText(w.name)
        name_edit.textChanged.connect(lambda val: self._on_common_changed("name", val))
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

        # Type-specific properties - data-driven grouping
        type_info = WidgetRegistry.instance().get(w.widget_type)
        props = type_info.get("properties", {})

        if props:
            self._build_grouped_properties(w, props)

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
                    label = label.replace("_", " ").title() + ":"
                    form.addRow(label, editor)

            self._layout.addWidget(group_box)

    # ── Property editor factory ───────────────────────────────────

    def _create_property_editor(self, prop_name, prop_info, current_value):
        ptype = prop_info.get("type", "string")

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
                editor.currentTextChanged.connect(lambda val: self._on_prop_changed(prop_name, val))
                self._editors[f"prop_{prop_name}"] = editor
                return editor
            else:
                editor = LineEdit()
                editor.setText(str(current_value or ""))
                editor.textChanged.connect(lambda val: self._on_prop_changed(prop_name, val))
                self._editors[f"prop_{prop_name}"] = editor
                return editor

        elif ptype == "int":
            editor = SpinBox()
            editor.setRange(prop_info.get("min", 0), prop_info.get("max", 9999))
            editor.setValue(int(current_value or 0))
            editor.valueChanged.connect(lambda val: self._on_prop_changed(prop_name, val))
            self._editors[f"prop_{prop_name}"] = editor
            return editor

        elif ptype == "bool":
            editor = CheckBox()
            editor.setChecked(bool(current_value))
            editor.toggled.connect(lambda val: self._on_prop_changed(prop_name, val))
            self._editors[f"prop_{prop_name}"] = editor
            return editor

        elif ptype == "color":
            editor = EguiColorPicker()
            editor.set_value(str(current_value or COLORS[0]))
            editor.color_changed.connect(lambda val: self._on_prop_changed(prop_name, val))
            self._editors[f"prop_{prop_name}"] = editor
            return editor

        elif ptype == "alpha":
            editor = ComboBox()
            editor.addItems(ALPHAS)
            editor.setCurrentText(str(current_value or "EGUI_ALPHA_100"))
            editor.currentTextChanged.connect(lambda val: self._on_prop_changed(prop_name, val))
            self._editors[f"prop_{prop_name}"] = editor
            return editor

        elif ptype == "font":
            # Built-in font selector with preview
            merged = self._merged_fonts()
            editor = EguiFontSelector(fonts=merged)
            cur = str(current_value or merged[0] if merged else "")
            editor.set_value(cur)
            editor.font_changed.connect(lambda val: self._on_prop_changed(prop_name, val))
            self._editors[f"prop_{prop_name}"] = editor
            return editor

        elif ptype == "align":
            editor = ComboBox()
            editor.addItems(ALIGNS)
            editor.setCurrentText(str(current_value or "EGUI_ALIGN_CENTER"))
            editor.currentTextChanged.connect(lambda val: self._on_prop_changed(prop_name, val))
            self._editors[f"prop_{prop_name}"] = editor
            return editor

        elif ptype == "orientation":
            editor = ComboBox()
            editor.addItems(["vertical", "horizontal"])
            editor.setCurrentText(str(current_value or "vertical"))
            editor.currentTextChanged.connect(lambda val: self._on_prop_changed(prop_name, val))
            self._editors[f"prop_{prop_name}"] = editor
            return editor

        # ── New resource-related property types ───────────────────

        elif ptype == "image_file":
            return self._create_file_selector(prop_name, current_value,
                                              self._catalog_images(), "Image files (*.png *.bmp *.jpg *.jpeg *.gif)")

        elif ptype == "image_format":
            editor = ComboBox()
            editor.addItems(IMAGE_FORMATS)
            editor.setCurrentText(str(current_value or "rgb565"))
            editor.currentTextChanged.connect(lambda val: self._on_prop_changed(prop_name, val))
            self._editors[f"prop_{prop_name}"] = editor
            return editor

        elif ptype == "image_alpha":
            editor = ComboBox()
            editor.addItems(IMAGE_ALPHAS)
            editor.setCurrentText(str(current_value or "4"))
            editor.currentTextChanged.connect(lambda val: self._on_prop_changed(prop_name, val))
            self._editors[f"prop_{prop_name}"] = editor
            return editor

        elif ptype == "image_external":
            editor = ComboBox()
            editor.addItems(IMAGE_EXTERNALS)
            editor.setCurrentText(str(current_value or "0"))
            editor.currentTextChanged.connect(lambda val: self._on_prop_changed(prop_name, val))
            self._editors[f"prop_{prop_name}"] = editor
            return editor

        elif ptype == "font_file":
            return self._create_file_selector(prop_name, current_value,
                                              self._catalog_fonts(), "Font files (*.ttf *.otf)")

        elif ptype == "font_pixelsize":
            editor = EditableComboBox()
            editor.addItems(FONT_PIXELSIZES)
            editor.setCurrentText(str(current_value or "16"))
            editor.currentTextChanged.connect(lambda val: self._on_prop_changed(prop_name, val))
            self._editors[f"prop_{prop_name}"] = editor
            return editor

        elif ptype == "font_fontbitsize":
            editor = ComboBox()
            editor.addItems(FONT_BITSIZES)
            editor.setCurrentText(str(current_value or "4"))
            editor.currentTextChanged.connect(lambda val: self._on_prop_changed(prop_name, val))
            self._editors[f"prop_{prop_name}"] = editor
            return editor

        elif ptype == "font_external":
            editor = ComboBox()
            editor.addItems(FONT_EXTERNALS)
            editor.setCurrentText(str(current_value or "0"))
            editor.currentTextChanged.connect(lambda val: self._on_prop_changed(prop_name, val))
            self._editors[f"prop_{prop_name}"] = editor
            return editor

        elif ptype == "text_file":
            return self._create_file_selector(prop_name, current_value,
                                              self._catalog_text_files(), "Text files (*.txt)")

        return None

    def _create_file_selector(self, prop_name, current_value, catalog_items, file_filter):
        """Create a ComboBox + '...' browse button for file selection."""
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
        combo.currentTextChanged.connect(lambda val: self._on_file_prop_changed(prop_name, val))
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
        src_dir = self._source_resource_dir or ""
        if not src_dir or not os.path.isdir(src_dir):
            src_dir = ""

        path, _ = QFileDialog.getOpenFileName(self, "Select File", src_dir, file_filter)
        if path:
            filename = os.path.basename(path)
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
                # Add to catalog
                if self._resource_catalog:
                    self._resource_catalog.add_file(filename)

            # Ensure filename is in combo
            if combo.findText(filename) < 0:
                combo.addItem(filename)
            combo.setCurrentText(filename)

    def _on_file_prop_changed(self, prop_name, value):
        """Handle file property change - rebuild form if needed for conditional groups."""
        if self._updating or self._widget is None:
            return
        self._widget.properties[prop_name] = value

        # Changing font_file or image_file may show/hide config groups
        if prop_name in ("image_file", "font_file"):
            self._updating = True
            self._rebuild_form()
            self._updating = False

        self.property_changed.emit()

    def _on_common_changed(self, field, value):
        if self._updating or self._widget is None:
            return
        if field == "name":
            self._widget.name = value
        else:
            setattr(self._widget, field, value)
        self.property_changed.emit()

    def _on_prop_changed(self, prop_name, value):
        if self._updating or self._widget is None:
            return
        self._widget.properties[prop_name] = value
        self.property_changed.emit()

    def _on_bg_changed(self, field, value):
        if self._updating or self._widget is None:
            return

        if self._widget.background is None:
            self._widget.background = BackgroundModel()

        bg = self._widget.background
        setattr(bg, field, value)

        # If bg_type changed to "none", remove background
        if field == "bg_type" and value == "none":
            self._widget.background = None

        # Rebuild form to show/hide dynamic fields
        if field in ("bg_type", "stroke_width", "has_pressed"):
            self._updating = True
            self._rebuild_form()
            self._updating = False

        self.property_changed.emit()

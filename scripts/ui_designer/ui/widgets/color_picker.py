"""Color picker widget supporting EGUI named colors and EGUI_COLOR_HEX format."""

import re

from PyQt5.QtWidgets import QWidget, QHBoxLayout, QColorDialog
from PyQt5.QtCore import pyqtSignal, Qt, QSize
from PyQt5.QtGui import QColor, QPainter, QBrush

from qfluentwidgets import EditableComboBox, ToolButton

from ...model.widget_model import COLORS, COLOR_RGB


# Regex to parse EGUI_COLOR_HEX(0xRRGGBB)
_HEX_RE = re.compile(r'^EGUI_COLOR_HEX\(\s*0x([0-9A-Fa-f]{6})\s*\)$')


def egui_color_to_qcolor(value):
    """Convert an EGUI color string to a QColor.

    Supports named colors (EGUI_COLOR_RED) and hex (EGUI_COLOR_HEX(0xFF0000)).
    Returns None if the value cannot be parsed.
    """
    if not value:
        return None
    # Named color
    rgb = COLOR_RGB.get(value)
    if rgb:
        return QColor(*rgb)
    # Hex color
    m = _HEX_RE.match(value)
    if m:
        hex_str = m.group(1)
        return QColor(int(hex_str[:2], 16), int(hex_str[2:4], 16), int(hex_str[4:6], 16))
    return None


def qcolor_to_egui_hex(qcolor):
    """Convert a QColor to EGUI_COLOR_HEX(0xRRGGBB) string."""
    return f"EGUI_COLOR_HEX(0x{qcolor.red():02X}{qcolor.green():02X}{qcolor.blue():02X})"


class ColorSwatch(QWidget):
    """Small square widget that displays a solid color."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self._color = QColor(Qt.white)
        self.setFixedSize(QSize(22, 22))

    def set_color(self, qcolor):
        if qcolor and qcolor.isValid():
            self._color = qcolor
        else:
            self._color = QColor(Qt.white)
        self.update()

    def paintEvent(self, event):
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing)
        p.setBrush(QBrush(self._color))
        p.setPen(Qt.gray)
        p.drawRoundedRect(1, 1, 20, 20, 3, 3)
        p.end()


class EguiColorPicker(QWidget):
    """Combo box with color swatch and a '...' button to open a color dialog.

    Emits ``color_changed(str)`` with the EGUI color expression.
    """

    color_changed = pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(2)

        self._swatch = ColorSwatch()
        layout.addWidget(self._swatch)

        self._combo = EditableComboBox()
        self._combo.addItems(COLORS)
        self._combo.currentTextChanged.connect(self._on_text_changed)
        layout.addWidget(self._combo, 1)

        self._btn = ToolButton()
        self._btn.setText("...")
        self._btn.setToolTip("Pick custom color")
        self._btn.clicked.connect(self._open_dialog)
        layout.addWidget(self._btn)

    def set_value(self, value):
        """Set the current color value (EGUI expression)."""
        value = str(value or COLORS[0])
        if self._combo.findText(value) < 0:
            self._combo.addItem(value)
        self._combo.setCurrentText(value)
        self._update_swatch(value)

    def value(self):
        return self._combo.currentText()

    def _on_text_changed(self, text):
        self._update_swatch(text)
        self.color_changed.emit(text)

    def _update_swatch(self, text):
        qc = egui_color_to_qcolor(text)
        self._swatch.set_color(qc)

    def _open_dialog(self):
        initial = egui_color_to_qcolor(self._combo.currentText()) or QColor(Qt.white)
        color = QColorDialog.getColor(initial, self, "Select Color")
        if color.isValid():
            # Check if it matches a named color
            for name, rgb in COLOR_RGB.items():
                if (color.red(), color.green(), color.blue()) == rgb:
                    self.set_value(name)
                    return
            self.set_value(qcolor_to_egui_hex(color))

"""Font selector widget with preview label for EmbeddedGUI Designer."""

import re

from PyQt5.QtWidgets import QWidget, QHBoxLayout
from PyQt5.QtCore import pyqtSignal

from qfluentwidgets import EditableComboBox, BodyLabel

from ...model.widget_model import FONTS


# Parse font resource name to extract display info
# e.g. "&egui_res_font_montserrat_14_4" -> "montserrat 14px 4bpp"
_FONT_RE = re.compile(r'&egui_res_font_(\w+?)_(\d+)_(\d+)$')


def _font_display_info(font_expr):
    """Extract human-readable info from a font expression.

    Returns (family, size, bpp) or None.
    """
    m = _FONT_RE.match(font_expr)
    if m:
        return m.group(1), m.group(2), m.group(3)
    return None


class EguiFontSelector(QWidget):
    """Editable combo box for font selection with a preview label.

    Emits ``font_changed(str)`` with the EGUI font expression.
    """

    font_changed = pyqtSignal(str)

    def __init__(self, fonts=None, parent=None):
        super().__init__(parent)
        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(4)

        self._combo = EditableComboBox()
        items = list(fonts) if fonts else list(FONTS)
        self._combo.addItems(items)
        self._combo.currentTextChanged.connect(self._on_changed)
        layout.addWidget(self._combo, 1)

        self._preview = BodyLabel("Abc")
        self._preview.setFixedWidth(80)
        layout.addWidget(self._preview)

    def set_value(self, value):
        """Set the current font value."""
        value = str(value or "")
        if value and self._combo.findText(value) < 0:
            self._combo.addItem(value)
        self._combo.setCurrentText(value)
        self._update_preview(value)

    def value(self):
        return self._combo.currentText()

    def _on_changed(self, text):
        self._update_preview(text)
        self.font_changed.emit(text)

    def _update_preview(self, text):
        info = _font_display_info(text)
        if info:
            family, size, bpp = info
            self._preview.setText(f"{family} {size}px")
            try:
                pt = max(8, min(int(size), 20))
                self._preview.setStyleSheet(f"font-size: {pt}px;")
            except ValueError:
                self._preview.setStyleSheet("")
        elif text == "EGUI_CONFIG_FONT_DEFAULT":
            self._preview.setText("Default")
            self._preview.setStyleSheet("")
        else:
            self._preview.setText("Custom")
            self._preview.setStyleSheet("")

"""Diagnostics dock widget for lightweight editor feedback."""

from PyQt5.QtCore import Qt, pyqtSignal
from PyQt5.QtWidgets import QHBoxLayout, QLabel, QListWidget, QListWidgetItem, QPushButton, QVBoxLayout, QWidget


_SEVERITY_PREFIX = {
    "error": "Error",
    "warning": "Warning",
    "info": "Info",
}


def _format_entry_text(entry):
    scope = entry.page_name or "selection"
    widget = f"/{entry.widget_name}" if entry.widget_name else ""
    prefix = _SEVERITY_PREFIX.get(entry.severity, entry.severity.title())
    return f"[{prefix}] {scope}{widget}: {entry.message}"


class DiagnosticsPanel(QWidget):
    """Read-only list of diagnostics with optional widget focusing."""

    diagnostic_activated = pyqtSignal(str, str)  # page_name, widget_name
    copy_requested = pyqtSignal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self._entries = []
        self._activated_entry = None
        self._init_ui()
        self.clear()

    def _init_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(8, 8, 8, 8)
        layout.setSpacing(6)

        self._summary_label = QLabel("")
        self._copy_button = QPushButton("Copy Summary")
        self._copy_button.clicked.connect(self.copy_requested.emit)
        self._hint_label = QLabel("Double-click a diagnostic to switch page or focus the widget.")
        self._hint_label.setWordWrap(True)

        self._list = QListWidget()
        self._list.setFocusPolicy(Qt.NoFocus)
        self._list.itemDoubleClicked.connect(self._on_item_activated)

        header_layout = QHBoxLayout()
        header_layout.setContentsMargins(0, 0, 0, 0)
        header_layout.addWidget(self._summary_label, 1)
        header_layout.addWidget(self._copy_button)

        layout.addLayout(header_layout)
        layout.addWidget(self._hint_label)
        layout.addWidget(self._list, 1)

    def clear(self):
        self._entries = []
        self._activated_entry = None
        self._summary_label.setText("Diagnostics: no active issues")
        self._copy_button.setEnabled(False)
        self._list.clear()

    def set_entries(self, entries):
        self._entries = list(entries or [])
        self._list.clear()
        self._copy_button.setEnabled(bool(self._entries))

        if not self._entries:
            self._summary_label.setText("Diagnostics: no active issues")
            return

        errors = sum(1 for entry in self._entries if entry.severity == "error")
        warnings = sum(1 for entry in self._entries if entry.severity == "warning")
        infos = sum(1 for entry in self._entries if entry.severity == "info")
        self._summary_label.setText(f"Diagnostics: {errors} error(s), {warnings} warning(s), {infos} info item(s)")

        for entry in self._entries:
            item = QListWidgetItem(_format_entry_text(entry))
            item.setData(Qt.UserRole, (entry.page_name, entry.widget_name))
            item.setData(Qt.UserRole + 1, entry)
            self._list.addItem(item)

    def has_entries(self):
        return bool(self._entries)

    def summary_text(self):
        if not self._entries:
            return self._summary_label.text()
        return "\n".join([self._summary_label.text()] + [_format_entry_text(entry) for entry in self._entries])

    def current_activated_entry(self):
        return self._activated_entry

    def _on_item_activated(self, item):
        self._activated_entry = item.data(Qt.UserRole + 1)
        page_name, widget_name = item.data(Qt.UserRole) or ("", "")
        self.diagnostic_activated.emit(page_name or "", widget_name or "")

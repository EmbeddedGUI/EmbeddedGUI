"""Diagnostics dock widget for lightweight editor feedback."""

from PyQt5.QtCore import Qt, pyqtSignal
from PyQt5.QtWidgets import QLabel, QListWidget, QListWidgetItem, QVBoxLayout, QWidget


_SEVERITY_PREFIX = {
    "error": "Error",
    "warning": "Warning",
    "info": "Info",
}


class DiagnosticsPanel(QWidget):
    """Read-only list of diagnostics with optional widget focusing."""

    diagnostic_activated = pyqtSignal(str, str)  # page_name, widget_name

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
        self._hint_label = QLabel("Double-click a diagnostic to switch page or focus the widget.")
        self._hint_label.setWordWrap(True)

        self._list = QListWidget()
        self._list.setFocusPolicy(Qt.NoFocus)
        self._list.itemDoubleClicked.connect(self._on_item_activated)

        layout.addWidget(self._summary_label)
        layout.addWidget(self._hint_label)
        layout.addWidget(self._list, 1)

    def clear(self):
        self._entries = []
        self._activated_entry = None
        self._summary_label.setText("Diagnostics: no active issues")
        self._list.clear()

    def set_entries(self, entries):
        self._entries = list(entries or [])
        self._list.clear()

        if not self._entries:
            self._summary_label.setText("Diagnostics: no active issues")
            return

        errors = sum(1 for entry in self._entries if entry.severity == "error")
        warnings = sum(1 for entry in self._entries if entry.severity == "warning")
        infos = sum(1 for entry in self._entries if entry.severity == "info")
        self._summary_label.setText(f"Diagnostics: {errors} error(s), {warnings} warning(s), {infos} info item(s)")

        for entry in self._entries:
            scope = entry.page_name or "selection"
            widget = f"/{entry.widget_name}" if entry.widget_name else ""
            prefix = _SEVERITY_PREFIX.get(entry.severity, entry.severity.title())
            item = QListWidgetItem(f"[{prefix}] {scope}{widget}: {entry.message}")
            item.setData(Qt.UserRole, (entry.page_name, entry.widget_name))
            item.setData(Qt.UserRole + 1, entry)
            self._list.addItem(item)

    def current_activated_entry(self):
        return self._activated_entry

    def _on_item_activated(self, item):
        self._activated_entry = item.data(Qt.UserRole + 1)
        page_name, widget_name = item.data(Qt.UserRole) or ("", "")
        self.diagnostic_activated.emit(page_name or "", widget_name or "")

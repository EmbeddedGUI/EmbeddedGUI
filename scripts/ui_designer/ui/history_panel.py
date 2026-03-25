"""History dock widget for undo/redo visualization."""

from PyQt5.QtCore import Qt
from PyQt5.QtWidgets import QLabel, QListWidget, QListWidgetItem, QVBoxLayout, QWidget


class HistoryPanel(QWidget):
    """Read-only panel that visualizes the current page undo stack."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self._init_ui()
        self.clear()

    def _init_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(8, 8, 8, 8)
        layout.setSpacing(6)

        self._page_value = QLabel("")
        self._stack_value = QLabel("")
        self._dirty_value = QLabel("")
        self._source_value = QLabel("")
        self._history_list = QListWidget()
        self._history_list.setSelectionMode(QListWidget.NoSelection)
        self._history_list.setFocusPolicy(Qt.NoFocus)

        layout.addWidget(self._page_value)
        layout.addWidget(self._stack_value)
        layout.addWidget(self._dirty_value)
        layout.addWidget(self._source_value)
        layout.addWidget(self._history_list, 1)

    def clear(self):
        self._page_value.setText("Page: -")
        self._stack_value.setText("History: 0 entries")
        self._dirty_value.setText("Dirty: No")
        self._source_value.setText("Source: Saved state")
        self._history_list.clear()

    def set_history(self, page_name, entries, dirty=False, dirty_source="", can_undo=False, can_redo=False):
        page_name = page_name or "-"
        dirty = bool(dirty)
        entries = list(entries or [])

        self._page_value.setText(f"Page: {page_name}")
        self._stack_value.setText(
            f"History: {len(entries)} entries | Undo: {'Yes' if can_undo else 'No'} | Redo: {'Yes' if can_redo else 'No'}"
        )
        self._dirty_value.setText(f"Dirty: {'Yes' if dirty else 'No'}")
        self._source_value.setText(f"Source: {dirty_source or 'Saved state'}")

        self._history_list.clear()
        for entry in reversed(entries):
            markers = []
            if entry.get("is_current"):
                markers.append("Current")
            if entry.get("is_saved"):
                markers.append("Saved")

            marker_prefix = f"[{'/'.join(markers)}] " if markers else ""
            label = entry.get("label") or "State capture"
            item = QListWidgetItem(f"{marker_prefix}{entry.get('index', 0) + 1}. {label}")
            self._history_list.addItem(item)

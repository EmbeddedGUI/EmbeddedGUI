"""SDK example selection dialog for EmbeddedGUI Designer."""

from __future__ import annotations

from PyQt5.QtCore import Qt
from PyQt5.QtWidgets import (
    QCheckBox,
    QDialog,
    QFileDialog,
    QGroupBox,
    QHBoxLayout,
    QListWidget,
    QListWidgetItem,
    QMessageBox,
    QVBoxLayout,
)

from qfluentwidgets import LineEdit, PrimaryPushButton, PushButton

from ..model.config import get_config
from ..model.workspace import is_valid_sdk_root, normalize_path


class AppSelectorDialog(QDialog):
    """Dialog for opening Designer-aware or legacy SDK examples."""

    def __init__(self, parent=None, egui_root=None):
        super().__init__(parent)
        self.setWindowTitle("Open SDK Example")
        self.setMinimumSize(560, 420)
        self.resize(620, 480)

        self._config = get_config()
        self._egui_root = normalize_path(egui_root or self._config.sdk_root)
        self._selected_entry = None

        self._init_ui()
        self._refresh_app_list()

    def _init_ui(self):
        layout = QVBoxLayout(self)
        layout.setSpacing(12)

        root_group = QGroupBox("EmbeddedGUI SDK Root")
        root_layout = QHBoxLayout(root_group)

        self._root_edit = LineEdit()
        self._root_edit.setText(self._egui_root)
        self._root_edit.setReadOnly(True)
        root_layout.addWidget(self._root_edit, 1)

        browse_btn = PushButton("Browse...")
        browse_btn.clicked.connect(self._browse_root)
        root_layout.addWidget(browse_btn)
        layout.addWidget(root_group)

        app_group = QGroupBox("SDK Examples")
        app_layout = QVBoxLayout(app_group)

        self._show_legacy = QCheckBox("Show legacy examples without .egui")
        self._show_legacy.setChecked(self._config.show_all_examples)
        self._show_legacy.toggled.connect(self._on_toggle_legacy)
        app_layout.addWidget(self._show_legacy)

        self._app_list = QListWidget()
        self._app_list.itemDoubleClicked.connect(self._on_item_double_clicked)
        self._app_list.currentItemChanged.connect(self._on_selection_changed)
        app_layout.addWidget(self._app_list)
        layout.addWidget(app_group, 1)

        buttons = QHBoxLayout()
        buttons.addStretch()
        cancel_btn = PushButton("Cancel")
        cancel_btn.clicked.connect(self.reject)
        buttons.addWidget(cancel_btn)

        self._open_btn = PrimaryPushButton("Open")
        self._open_btn.clicked.connect(self._on_open)
        self._open_btn.setEnabled(False)
        buttons.addWidget(self._open_btn)
        layout.addLayout(buttons)

    def _browse_root(self):
        path = QFileDialog.getExistingDirectory(self, "Select EmbeddedGUI SDK Root", self._egui_root or "")
        if not path:
            return
        path = normalize_path(path)
        if not is_valid_sdk_root(path):
            QMessageBox.warning(self, "Invalid SDK Root", "This directory does not appear to be a valid EmbeddedGUI SDK root.")
            return
        self._egui_root = path
        self._root_edit.setText(path)
        self._refresh_app_list()

    def _on_toggle_legacy(self, checked):
        self._config.show_all_examples = checked
        self._config.save()
        self._refresh_app_list()

    def _refresh_app_list(self):
        self._app_list.clear()
        self._open_btn.setEnabled(False)
        self._selected_entry = None

        if not self._egui_root:
            return

        entries = self._config.list_available_app_entries(
            sdk_root=self._egui_root,
            include_legacy=self._show_legacy.isChecked(),
        )
        for entry in entries:
            label = entry["app_name"]
            if entry["is_legacy"]:
                label += " [Legacy]"
            item = QListWidgetItem(label)
            item.setData(Qt.UserRole, entry)
            if entry["has_project"]:
                item.setToolTip(entry["project_path"])
            else:
                item.setToolTip(f"{entry['app_dir']}\nLegacy example without .egui. Opening it will initialize a Designer project.")
            self._app_list.addItem(item)

        last_app = self._config.last_app
        for index in range(self._app_list.count()):
            item = self._app_list.item(index)
            entry = item.data(Qt.UserRole) or {}
            if entry.get("app_name") == last_app:
                self._app_list.setCurrentRow(index)
                break

    def _on_selection_changed(self, current, previous):
        self._selected_entry = current.data(Qt.UserRole) if current else None
        self._open_btn.setEnabled(self._selected_entry is not None)

    def _on_item_double_clicked(self, item):
        self._selected_entry = item.data(Qt.UserRole)
        self.accept()

    def _on_open(self):
        if self._selected_entry:
            self.accept()

    @property
    def selected_entry(self):
        return self._selected_entry

    @property
    def selected_app(self):
        if self._selected_entry:
            return self._selected_entry.get("app_name")
        return None

    @property
    def egui_root(self):
        return self._egui_root

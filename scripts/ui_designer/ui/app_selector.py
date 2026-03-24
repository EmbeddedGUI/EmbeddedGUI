"""SDK example selection dialog for EmbeddedGUI Designer."""

from __future__ import annotations

from PyQt5.QtCore import Qt
from PyQt5.QtWidgets import (
    QCheckBox,
    QDialog,
    QFileDialog,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QListWidget,
    QListWidgetItem,
    QMessageBox,
    QVBoxLayout,
)

from qfluentwidgets import LineEdit, PrimaryPushButton, PushButton

from ..model.config import get_config
from ..model.sdk_bootstrap import default_sdk_install_dir, describe_auto_download_plan, is_bundled_sdk_root
from ..model.workspace import (
    describe_sdk_root,
    is_valid_sdk_root,
    normalize_path,
    resolve_available_sdk_root,
    resolve_sdk_root_candidate,
)


class AppSelectorDialog(QDialog):
    """Dialog for opening Designer-aware or legacy SDK examples."""

    def __init__(self, parent=None, egui_root=None, on_download_sdk=None):
        super().__init__(parent)
        self.setWindowTitle("Open SDK Example")
        self.setMinimumSize(560, 420)
        self.resize(620, 480)

        self._config = get_config()
        self._egui_root = resolve_available_sdk_root(
            egui_root,
            self._config.sdk_root,
            self._config.egui_root,
            cached_sdk_root=default_sdk_install_dir(),
        )
        self._selected_entry = None
        self._on_download_sdk = on_download_sdk

        self._init_ui()
        self._refresh_app_list()

    def _init_ui(self):
        layout = QVBoxLayout(self)
        layout.setSpacing(12)

        root_group = QGroupBox("EmbeddedGUI SDK Root")
        root_group_layout = QVBoxLayout(root_group)
        root_group_layout.setSpacing(8)
        root_row = QHBoxLayout()

        self._root_edit = LineEdit()
        self._root_edit.setText(self._egui_root)
        self._root_edit.setReadOnly(True)
        root_row.addWidget(self._root_edit, 1)

        browse_btn = PushButton("Browse...")
        browse_btn.clicked.connect(self._browse_root)
        root_row.addWidget(browse_btn)

        self._download_btn = PushButton("Download SDK...")
        self._download_btn.clicked.connect(self._download_sdk)
        self._download_btn.setToolTip(describe_auto_download_plan())
        root_row.addWidget(self._download_btn)

        self._root_status_label = QLabel("")
        self._root_status_label.setWordWrap(True)

        root_group_layout.addLayout(root_row)
        root_group_layout.addWidget(self._root_status_label)
        layout.addWidget(root_group)

        app_group = QGroupBox("SDK Examples")
        app_layout = QVBoxLayout(app_group)

        self._show_legacy = QCheckBox("Show legacy examples without .egui")
        self._show_legacy.setChecked(self._config.show_all_examples)
        self._show_legacy.toggled.connect(self._on_toggle_legacy)
        app_layout.addWidget(self._show_legacy)

        self._search_edit = LineEdit()
        self._search_edit.setPlaceholderText("Filter examples by name...")
        self._search_edit.textChanged.connect(self._refresh_app_list)
        app_layout.addWidget(self._search_edit)

        self._app_list = QListWidget()
        self._app_list.itemDoubleClicked.connect(self._on_item_double_clicked)
        self._app_list.currentItemChanged.connect(self._on_selection_changed)
        app_layout.addWidget(self._app_list)

        self._selection_hint_label = QLabel("")
        self._selection_hint_label.setWordWrap(True)
        app_layout.addWidget(self._selection_hint_label)
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
        path = resolve_sdk_root_candidate(path)
        if not path:
            QMessageBox.warning(
                self,
                "Invalid SDK Root",
                "This directory does not appear to contain a valid EmbeddedGUI SDK root.",
            )
            return
        self._egui_root = path
        self._root_edit.setText(path)
        self._refresh_app_list()

    def _download_sdk(self):
        if self._on_download_sdk is None:
            QMessageBox.warning(
                self,
                "Download Unavailable",
                "This dialog was opened without an SDK download handler.",
            )
            return

        path = self._on_download_sdk()
        path = resolve_sdk_root_candidate(path) or normalize_path(path)
        if not path:
            return

        self._egui_root = path
        self._root_edit.setText(path)
        self._refresh_app_list()

    def _on_toggle_legacy(self, checked):
        self._config.show_all_examples = checked
        self._config.save()
        self._refresh_app_list()

    def _refresh_app_list(self):
        previous_app = ""
        if self._selected_entry:
            previous_app = self._selected_entry.get("app_name", "")

        self._app_list.clear()
        self._set_selection_feedback(None)
        self._refresh_root_status()

        if not self._egui_root:
            item = QListWidgetItem("(Set or download an SDK root first)")
            item.setFlags(Qt.NoItemFlags)
            self._app_list.addItem(item)
            return

        if not is_valid_sdk_root(self._egui_root):
            item = QListWidgetItem("(Current SDK root is invalid)")
            item.setFlags(Qt.NoItemFlags)
            self._app_list.addItem(item)
            return

        search_text = self._search_edit.text().strip().lower()
        entries = self._config.list_available_app_entries(
            sdk_root=self._egui_root,
            include_legacy=self._show_legacy.isChecked(),
        )
        for entry in entries:
            if search_text and search_text not in entry["app_name"].lower():
                continue
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

        if self._app_list.count() == 0:
            item = QListWidgetItem("(No matching examples)" if search_text else "(No SDK examples found)")
            item.setFlags(Qt.NoItemFlags)
            self._app_list.addItem(item)
            return

        preferred_app = previous_app or self._config.last_app
        for index in range(self._app_list.count()):
            item = self._app_list.item(index)
            entry = item.data(Qt.UserRole) or {}
            if entry.get("app_name") == preferred_app:
                self._app_list.setCurrentRow(index)
                break
        else:
            self._app_list.setCurrentRow(0)

    def _refresh_root_status(self):
        status = describe_sdk_root(self._egui_root)
        if status == "ready":
            if is_bundled_sdk_root(self._egui_root):
                self._root_status_label.setText("Ready: using bundled SDK examples below.")
            else:
                self._root_status_label.setText("Ready: SDK examples are available below.")
            self._root_status_label.setStyleSheet("color: #4caf50;")
            return

        if status == "invalid":
            self._root_status_label.setText(
                "Invalid: current SDK root needs attention. Browse to a valid SDK root or download a fresh copy.\n"
                f"{describe_auto_download_plan()}"
            )
            self._root_status_label.setStyleSheet("color: #ff9800;")
            return

        self._root_status_label.setText(
            "Missing: no SDK root selected. Browse to an existing SDK or download one now.\n"
            f"{describe_auto_download_plan()}"
        )
        self._root_status_label.setStyleSheet("color: #f44336;")

    def _on_selection_changed(self, current, previous):
        self._set_selection_feedback(current.data(Qt.UserRole) if current else None)

    def _on_item_double_clicked(self, item):
        entry = item.data(Qt.UserRole)
        if entry is None:
            return
        self._set_selection_feedback(entry)
        self.accept()

    def _on_open(self):
        if self._selected_entry:
            self.accept()

    def _set_selection_feedback(self, entry):
        self._selected_entry = entry
        self._open_btn.setEnabled(entry is not None)
        if not entry:
            self._open_btn.setText("Open")
            self._selection_hint_label.setText(
                "Select a Designer project or a legacy example from the list."
            )
            self._selection_hint_label.setStyleSheet("color: #888;")
            return

        if entry.get("is_legacy"):
            self._open_btn.setText("Import Legacy Example")
            self._selection_hint_label.setText(
                f"Legacy example path:\n{entry.get('app_dir', '')}\n\n"
                "Opening it will initialize a Designer project in this app directory."
            )
            self._selection_hint_label.setStyleSheet("color: #ff9800;")
            return

        self._open_btn.setText("Open")
        self._selection_hint_label.setText(
            f"Designer project path:\n{entry.get('project_path', '')}"
        )
        self._selection_hint_label.setStyleSheet("color: #4caf50;")

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

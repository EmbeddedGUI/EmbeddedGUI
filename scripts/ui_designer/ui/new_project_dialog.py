"""Dialog for creating a new standard EmbeddedGUI app project."""

from __future__ import annotations

import os

from PyQt5.QtWidgets import (
    QDialog,
    QFileDialog,
    QFormLayout,
    QHBoxLayout,
    QLabel,
    QMessageBox,
    QSpinBox,
    QVBoxLayout,
)

from qfluentwidgets import LineEdit, PrimaryPushButton, PushButton

from ..model.workspace import is_valid_sdk_root, normalize_path, resolve_sdk_root_candidate


class NewProjectDialog(QDialog):
    """Collect parameters for a new project."""

    def __init__(self, parent=None, sdk_root="", default_parent_dir=""):
        super().__init__(parent)
        self.setWindowTitle("New Project")
        self.resize(560, 260)

        self._sdk_root = resolve_sdk_root_candidate(sdk_root) or normalize_path(sdk_root)
        self._parent_dir = normalize_path(default_parent_dir) or self._sdk_root

        self._init_ui()

    def _init_ui(self):
        layout = QVBoxLayout(self)
        form = QFormLayout()
        form.setSpacing(12)

        self._sdk_edit = LineEdit()
        self._sdk_edit.setReadOnly(True)
        self._sdk_edit.setText(self._sdk_root)
        sdk_row = QHBoxLayout()
        sdk_row.addWidget(self._sdk_edit, 1)
        sdk_browse = PushButton("Browse...")
        sdk_browse.clicked.connect(self._browse_sdk_root)
        sdk_row.addWidget(sdk_browse)
        sdk_clear = PushButton("Clear")
        sdk_clear.clicked.connect(self._clear_sdk_root)
        sdk_row.addWidget(sdk_clear)
        form.addRow("SDK Root", sdk_row)

        self._sdk_hint_label = QLabel("Optional. Leave empty to create an editing-only project and set the SDK later.")
        self._sdk_hint_label.setWordWrap(True)
        self._sdk_hint_label.setStyleSheet("color: #888;")
        form.addRow("", self._sdk_hint_label)

        self._parent_edit = LineEdit()
        self._parent_edit.setReadOnly(True)
        self._parent_edit.setText(self._parent_dir)
        parent_row = QHBoxLayout()
        parent_row.addWidget(self._parent_edit, 1)
        parent_browse = PushButton("Browse...")
        parent_browse.clicked.connect(self._browse_parent_dir)
        parent_row.addWidget(parent_browse)
        form.addRow("Parent Dir", parent_row)

        self._app_name_edit = LineEdit()
        self._app_name_edit.setPlaceholderText("e.g. MyDashboard")
        form.addRow("App Name", self._app_name_edit)

        self._width_spin = QSpinBox()
        self._width_spin.setRange(16, 4096)
        self._width_spin.setValue(240)
        form.addRow("Width", self._width_spin)

        self._height_spin = QSpinBox()
        self._height_spin.setRange(16, 4096)
        self._height_spin.setValue(320)
        form.addRow("Height", self._height_spin)

        layout.addLayout(form)

        buttons = QHBoxLayout()
        buttons.addStretch()
        cancel_btn = PushButton("Cancel")
        cancel_btn.clicked.connect(self.reject)
        buttons.addWidget(cancel_btn)
        create_btn = PrimaryPushButton("Create")
        create_btn.clicked.connect(self._accept_if_valid)
        buttons.addWidget(create_btn)
        layout.addLayout(buttons)

    def _browse_sdk_root(self):
        path = QFileDialog.getExistingDirectory(self, "Select SDK Root", self._sdk_root or "")
        if not path:
            return
        path = resolve_sdk_root_candidate(path)
        if not path:
            QMessageBox.warning(
                self,
                "Invalid SDK Root",
                "The selected directory does not contain a valid EmbeddedGUI SDK root.",
            )
            return
        self._sdk_root = path
        self._sdk_edit.setText(path)
        if not self._parent_dir or self._parent_dir == normalize_path(os.getcwd()):
            self._parent_dir = os.path.join(path, "example")
            self._parent_edit.setText(self._parent_dir)

    def _clear_sdk_root(self):
        self._sdk_root = ""
        self._sdk_edit.setText("")

    def _browse_parent_dir(self):
        path = QFileDialog.getExistingDirectory(self, "Select Parent Directory", self._parent_dir or "")
        if not path:
            return
        self._parent_dir = normalize_path(path)
        self._parent_edit.setText(self._parent_dir)

    def _accept_if_valid(self):
        app_name = self.app_name
        if self._sdk_root and not is_valid_sdk_root(self._sdk_root):
            QMessageBox.warning(self, "Invalid SDK Root", "Please select a valid EmbeddedGUI SDK root or clear it.")
            return
        if not self._parent_dir:
            QMessageBox.warning(self, "Parent Directory", "Please select a parent directory.")
            return
        if not app_name:
            QMessageBox.warning(self, "App Name", "Please enter an app name.")
            return
        if not app_name.replace("_", "").isalnum():
            QMessageBox.warning(self, "App Name", "App name can only contain letters, numbers, and underscores.")
            return
        self.accept()

    @property
    def sdk_root(self):
        return self._sdk_root

    @property
    def parent_dir(self):
        return self._parent_dir

    @property
    def app_name(self):
        return self._app_name_edit.text().strip()

    @property
    def screen_width(self):
        return self._width_spin.value()

    @property
    def screen_height(self):
        return self._height_spin.value()

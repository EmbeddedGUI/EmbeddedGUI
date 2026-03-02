"""App selection dialog for EmbeddedGUI Designer.

Allows users to:
- Select the EmbeddedGUI root directory
- Choose an existing app or create a new one
"""

import os

from PyQt5.QtWidgets import (
    QDialog, QVBoxLayout, QHBoxLayout, QLabel,
    QListWidget, QListWidgetItem, QFileDialog, QMessageBox,
    QGroupBox, QFormLayout,
)
from PyQt5.QtCore import Qt

from qfluentwidgets import (
    PushButton, LineEdit, PrimaryPushButton,
)

from ..model.config import get_config


class AppSelectorDialog(QDialog):
    """Dialog for selecting an app to open/create."""

    def __init__(self, parent=None, egui_root=None):
        super().__init__(parent)
        self.setWindowTitle("Open App")
        self.setMinimumSize(500, 400)
        self.resize(550, 450)

        self._config = get_config()
        self._egui_root = egui_root or self._config.egui_root
        self._selected_app = None

        self._init_ui()
        self._refresh_app_list()

    def _init_ui(self):
        layout = QVBoxLayout(self)
        layout.setSpacing(12)

        # EmbeddedGUI Root Section
        root_group = QGroupBox("EmbeddedGUI Root Directory")
        root_layout = QHBoxLayout(root_group)

        self._root_edit = LineEdit()
        self._root_edit.setText(self._egui_root)
        self._root_edit.setReadOnly(True)
        root_layout.addWidget(self._root_edit, 1)

        browse_btn = PushButton("Browse...")
        browse_btn.clicked.connect(self._browse_root)
        root_layout.addWidget(browse_btn)

        layout.addWidget(root_group)

        # App Selection Section
        app_group = QGroupBox("Select App")
        app_layout = QVBoxLayout(app_group)

        self._app_list = QListWidget()
        self._app_list.setMinimumHeight(200)
        self._app_list.itemDoubleClicked.connect(self._on_item_double_clicked)
        self._app_list.currentItemChanged.connect(self._on_selection_changed)
        app_layout.addWidget(self._app_list)

        # New app creation
        new_layout = QHBoxLayout()
        new_layout.addWidget(QLabel("New App Name:"))
        self._new_app_edit = LineEdit()
        self._new_app_edit.setPlaceholderText("e.g., MyNewApp")
        new_layout.addWidget(self._new_app_edit, 1)
        create_btn = PushButton("Create")
        create_btn.clicked.connect(self._create_app)
        new_layout.addWidget(create_btn)
        app_layout.addLayout(new_layout)

        layout.addWidget(app_group, 1)

        # Buttons
        btn_layout = QHBoxLayout()
        btn_layout.addStretch()

        cancel_btn = PushButton("Cancel")
        cancel_btn.clicked.connect(self.reject)
        btn_layout.addWidget(cancel_btn)

        self._open_btn = PrimaryPushButton("Open")
        self._open_btn.clicked.connect(self._on_open)
        self._open_btn.setEnabled(False)
        btn_layout.addWidget(self._open_btn)

        layout.addLayout(btn_layout)

    def _browse_root(self):
        """Browse for EmbeddedGUI root directory."""
        path = QFileDialog.getExistingDirectory(
            self, "Select EmbeddedGUI Root Directory",
            self._egui_root or ""
        )
        if not path:
            return

        # Validate it's a valid EmbeddedGUI root
        if not os.path.isfile(os.path.join(path, "Makefile")):
            QMessageBox.warning(
                self, "Invalid Directory",
                "This directory does not appear to be an EmbeddedGUI root.\n"
                "No Makefile found."
            )
            return

        self._egui_root = path
        self._root_edit.setText(path)
        self._refresh_app_list()

    def _refresh_app_list(self):
        """Refresh the list of available apps."""
        self._app_list.clear()
        self._open_btn.setEnabled(False)

        if not self._egui_root:
            return

        apps = self._config.list_available_apps(self._egui_root)
        for app_name in apps:
            item = QListWidgetItem(app_name)
            app_dir = os.path.join(self._egui_root, "example", app_name)

            # Check if project file exists (.egui)
            eui_file = os.path.join(app_dir, f"{app_name}.egui")
            if os.path.isfile(eui_file):
                item.setToolTip(f"{app_dir}\n(has {app_name}.egui)")
            else:
                item.setToolTip(f"{app_dir}\n(no project yet)")

            self._app_list.addItem(item)

        # Select last used app if available
        last_app = self._config.last_app
        for i in range(self._app_list.count()):
            if self._app_list.item(i).text() == last_app:
                self._app_list.setCurrentRow(i)
                break

    def _on_selection_changed(self, current, previous):
        """Handle app selection change."""
        self._open_btn.setEnabled(current is not None)

    def _on_item_double_clicked(self, item):
        """Handle double-click on app item."""
        self._selected_app = item.text()
        self.accept()

    def _on_open(self):
        """Handle Open button click."""
        item = self._app_list.currentItem()
        if item:
            self._selected_app = item.text()
            self.accept()

    def _create_app(self):
        """Create a new app."""
        app_name = self._new_app_edit.text().strip()
        if not app_name:
            QMessageBox.warning(self, "Error", "Please enter an app name.")
            return

        # Validate app name (alphanumeric and underscore only)
        if not app_name.replace("_", "").isalnum():
            QMessageBox.warning(
                self, "Invalid Name",
                "App name can only contain letters, numbers, and underscores."
            )
            return

        if not self._egui_root:
            QMessageBox.warning(self, "Error", "Please select EmbeddedGUI root first.")
            return

        app_dir = os.path.join(self._egui_root, "example", app_name)
        if os.path.exists(app_dir):
            QMessageBox.warning(
                self, "Error",
                f"App '{app_name}' already exists."
            )
            return

        # Create basic app structure
        try:
            os.makedirs(app_dir, exist_ok=True)
            os.makedirs(os.path.join(app_dir, ".eguiproject", "resources", "images"), exist_ok=True)
            os.makedirs(os.path.join(app_dir, "resource", "src"), exist_ok=True)

            # Create minimal build.mk
            build_mk = os.path.join(app_dir, "build.mk")
            with open(build_mk, "w", encoding="utf-8") as f:
                f.write(f"# Build configuration for {app_name}\n\n")
                f.write("EGUI_CODE_SRC += \\\n")
                f.write(f"    $(EGUI_APP_PATH)/uicode.c \\\n")
                f.write("\n")
                f.write("EGUI_CODE_INCLUDE += \\\n")
                f.write("    -I$(EGUI_APP_PATH) \\\n")

            # Create minimal app_egui_config.h
            config_h = os.path.join(app_dir, "app_egui_config.h")
            with open(config_h, "w", encoding="utf-8") as f:
                f.write(f"#ifndef _APP_EGUI_CONFIG_H_\n")
                f.write(f"#define _APP_EGUI_CONFIG_H_\n\n")
                f.write(f"/* Configuration for {app_name} */\n\n")
                f.write(f"#define EGUI_CONFIG_SCEEN_WIDTH  240\n")
                f.write(f"#define EGUI_CONFIG_SCEEN_HEIGHT 320\n\n")
                f.write(f"#endif /* _APP_EGUI_CONFIG_H_ */\n")

            # Create empty resource config
            res_config = os.path.join(app_dir, "resource", "src", "app_resource_config.json")
            with open(res_config, "w", encoding="utf-8") as f:
                f.write('{\n    "img": [],\n    "font": []\n}\n')

            self._refresh_app_list()

            # Select the new app
            for i in range(self._app_list.count()):
                if self._app_list.item(i).text() == app_name:
                    self._app_list.setCurrentRow(i)
                    break

            self._new_app_edit.clear()
            QMessageBox.information(
                self, "Success",
                f"App '{app_name}' created successfully.\n"
                f"Click Open to start designing."
            )

        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to create app:\n{e}")

    @property
    def selected_app(self):
        """Get the selected app name."""
        return self._selected_app

    @property
    def egui_root(self):
        """Get the selected EmbeddedGUI root path."""
        return self._egui_root

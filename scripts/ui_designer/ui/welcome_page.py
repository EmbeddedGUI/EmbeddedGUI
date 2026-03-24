"""Welcome page for EmbeddedGUI Designer."""

from __future__ import annotations

import os

from PyQt5.QtCore import Qt, pyqtSignal
from PyQt5.QtGui import QFont
from PyQt5.QtWidgets import QLabel, QHBoxLayout, QVBoxLayout, QWidget

from qfluentwidgets import PrimaryPushButton, PushButton

from ..model.config import get_config
from ..model.sdk_bootstrap import default_sdk_install_dir
from ..model.workspace import describe_sdk_root


class RecentProjectItem(QWidget):
    """Card widget for a recent project entry."""

    item_clicked = pyqtSignal(str, str)

    def __init__(self, project_path, sdk_root, display_name, parent=None):
        super().__init__(parent)
        self.project_path = project_path
        self.sdk_root = sdk_root
        self.display_name = display_name
        self.setCursor(Qt.PointingHandCursor)
        self.setFixedHeight(82)
        self.setStyleSheet(
            "background-color: #262626; border: 1px solid #333; border-radius: 10px;"
        )

        layout = QHBoxLayout(self)
        layout.setContentsMargins(16, 12, 16, 12)
        layout.setSpacing(12)

        icon_label = QLabel()
        icon_label.setFixedSize(40, 40)
        icon_label.setStyleSheet(
            "background-color: #0078d4; border-radius: 6px; color: white; font-size: 18px; font-weight: bold;"
        )
        icon_label.setAlignment(Qt.AlignCenter)
        icon_label.setText(display_name[0].upper() if display_name else "?")
        layout.addWidget(icon_label)

        text_layout = QVBoxLayout()
        text_layout.setSpacing(4)

        name_label = QLabel(display_name)
        name_label.setFont(QFont("Segoe UI", 11, QFont.DemiBold))
        name_label.setStyleSheet("color: #fff;")
        text_layout.addWidget(name_label)

        path_label = QLabel(project_path)
        path_label.setStyleSheet("color: #888;")
        text_layout.addWidget(path_label)

        sdk_status = describe_sdk_root(sdk_root)
        status_label = QLabel(f"SDK: {sdk_status}")
        if sdk_status == "ready":
            status_label.setStyleSheet("color: #4caf50;")
        elif sdk_status == "invalid":
            status_label.setStyleSheet("color: #ff9800;")
        else:
            status_label.setStyleSheet("color: #f44336;")
        text_layout.addWidget(status_label)

        layout.addLayout(text_layout, 1)

    def mouseReleaseEvent(self, event):
        if event.button() == Qt.LeftButton:
            self.item_clicked.emit(self.project_path, self.sdk_root)
        super().mouseReleaseEvent(event)


class WelcomePage(QWidget):
    """Welcome page shown when no project is loaded."""

    open_recent = pyqtSignal(str, str)
    new_project = pyqtSignal()
    open_project = pyqtSignal()
    open_app = pyqtSignal()
    set_sdk_root = pyqtSignal()
    download_sdk = pyqtSignal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self._config = get_config()
        self._init_ui()

    def _init_ui(self):
        self.setStyleSheet("QWidget { background-color: #1e1e1e; }")

        main_layout = QHBoxLayout(self)
        main_layout.setContentsMargins(0, 0, 0, 0)

        center_widget = QWidget()
        center_widget.setMaximumWidth(900)
        center_layout = QVBoxLayout(center_widget)
        center_layout.setContentsMargins(40, 40, 40, 40)
        center_layout.setSpacing(24)

        title = QLabel("EmbeddedGUI Designer")
        title.setFont(QFont("Segoe UI", 28, QFont.Light))
        title.setStyleSheet("color: #fff;")
        center_layout.addWidget(title)

        subtitle = QLabel("Visual designer with SDK-backed preview and external app workspace support")
        subtitle.setFont(QFont("Segoe UI", 12))
        subtitle.setStyleSheet("color: #888;")
        center_layout.addWidget(subtitle)

        center_layout.addSpacing(20)

        content_layout = QHBoxLayout()
        content_layout.setSpacing(40)

        left_col = QVBoxLayout()
        left_col.setSpacing(12)

        start_label = QLabel("Start")
        start_label.setFont(QFont("Segoe UI", 14, QFont.DemiBold))
        start_label.setStyleSheet("color: #ccc;")
        left_col.addWidget(start_label)

        self._new_project_btn = PrimaryPushButton("New Project...")
        self._new_project_btn.setFixedWidth(220)
        self._new_project_btn.clicked.connect(self.new_project.emit)
        left_col.addWidget(self._new_project_btn)

        self._open_project_btn = PushButton("Open Project File...")
        self._open_project_btn.setFixedWidth(220)
        self._open_project_btn.clicked.connect(self.open_project.emit)
        left_col.addWidget(self._open_project_btn)

        self._open_app_btn = PushButton("Open SDK Example...")
        self._open_app_btn.setFixedWidth(220)
        self._open_app_btn.clicked.connect(self.open_app.emit)
        left_col.addWidget(self._open_app_btn)

        self._set_sdk_root_btn = PushButton("Set SDK Root...")
        self._set_sdk_root_btn.setFixedWidth(220)
        self._set_sdk_root_btn.clicked.connect(self.set_sdk_root.emit)
        left_col.addWidget(self._set_sdk_root_btn)

        self._download_sdk_btn = PushButton("Download SDK...")
        self._download_sdk_btn.setFixedWidth(220)
        self._download_sdk_btn.clicked.connect(self.download_sdk.emit)
        left_col.addWidget(self._download_sdk_btn)

        self._sdk_card = QWidget()
        self._sdk_card.setStyleSheet("background-color: #262626; border: 1px solid #333; border-radius: 10px;")
        sdk_layout = QVBoxLayout(self._sdk_card)
        sdk_layout.setContentsMargins(16, 14, 16, 14)
        sdk_layout.setSpacing(6)

        sdk_title = QLabel("SDK Status")
        sdk_title.setFont(QFont("Segoe UI", 11, QFont.DemiBold))
        sdk_title.setStyleSheet("color: #fff;")
        sdk_layout.addWidget(sdk_title)

        self._sdk_status_label = QLabel("")
        self._sdk_status_label.setWordWrap(True)
        sdk_layout.addWidget(self._sdk_status_label)

        self._sdk_path_label = QLabel("")
        self._sdk_path_label.setWordWrap(True)
        self._sdk_path_label.setStyleSheet("color: #888;")
        sdk_layout.addWidget(self._sdk_path_label)

        self._sdk_hint_label = QLabel("")
        self._sdk_hint_label.setWordWrap(True)
        self._sdk_hint_label.setStyleSheet("color: #666;")
        sdk_layout.addWidget(self._sdk_hint_label)

        left_col.addSpacing(12)
        left_col.addWidget(self._sdk_card)

        left_col.addStretch()
        content_layout.addLayout(left_col)

        right_col = QVBoxLayout()
        right_col.setSpacing(12)

        recent_label = QLabel("Recent Projects")
        recent_label.setFont(QFont("Segoe UI", 14, QFont.DemiBold))
        recent_label.setStyleSheet("color: #ccc;")
        right_col.addWidget(recent_label)

        self._recent_list = QVBoxLayout()
        self._recent_list.setSpacing(8)
        right_col.addLayout(self._recent_list)
        right_col.addStretch()
        content_layout.addLayout(right_col, 1)

        center_layout.addLayout(content_layout, 1)

        footer = QLabel("Press Ctrl+Shift+O to open an SDK example, Ctrl+O to open a .egui project, or Ctrl+N to create a new project")
        footer.setStyleSheet("color: #666; font-size: 11px;")
        footer.setAlignment(Qt.AlignCenter)
        center_layout.addWidget(footer)

        main_layout.addStretch()
        main_layout.addWidget(center_widget)
        main_layout.addStretch()

        self._refresh_sdk_status()
        self._refresh_recent_list()

    def _refresh_sdk_status(self):
        sdk_root = self._config.sdk_root or self._config.egui_root
        sdk_status = describe_sdk_root(sdk_root)
        default_cache_dir = default_sdk_install_dir()
        if sdk_status == "ready":
            self._sdk_status_label.setText("Ready: compile preview available")
            self._sdk_status_label.setStyleSheet("color: #4caf50;")
            self._sdk_hint_label.setText("Projects can use real SDK-backed preview when buildable.")
        elif sdk_status == "invalid":
            self._sdk_status_label.setText("Invalid: SDK path needs attention")
            self._sdk_status_label.setStyleSheet("color: #ff9800;")
            self._sdk_hint_label.setText(
                "Select a valid SDK root, or download one automatically to restore compile preview.\n"
                f"Default auto-download cache: {default_cache_dir}"
            )
        else:
            self._sdk_status_label.setText("Missing: editing only, Python preview fallback")
            self._sdk_status_label.setStyleSheet("color: #f44336;")
            self._sdk_hint_label.setText(
                "You can still edit projects, but compile preview stays disabled until you set or download an SDK.\n"
                f"Default auto-download cache: {default_cache_dir}"
            )

        self._sdk_path_label.setText(sdk_root or "No SDK root configured")

    def _refresh_recent_list(self):
        while self._recent_list.count():
            item = self._recent_list.takeAt(0)
            if item.widget():
                item.widget().deleteLater()

        recent = self._config.recent_projects
        if not recent:
            no_recent = QLabel("No recent projects")
            no_recent.setStyleSheet("color: #666; font-style: italic;")
            self._recent_list.addWidget(no_recent)
            return

        for item_data in recent[:8]:
            project_path = item_data.get("project_path", "")
            sdk_root = item_data.get("sdk_root", "")
            display_name = item_data.get("display_name") or os.path.splitext(os.path.basename(project_path))[0]
            item = RecentProjectItem(project_path, sdk_root, display_name)
            item.item_clicked.connect(self._on_recent_clicked)
            self._recent_list.addWidget(item)

    def _on_recent_clicked(self, project_path, sdk_root):
        self.open_recent.emit(project_path, sdk_root)

    def refresh(self):
        self._refresh_sdk_status()
        self._refresh_recent_list()

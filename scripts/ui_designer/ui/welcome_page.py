"""Welcome page for EmbeddedGUI Designer - shown when no project is loaded.

Similar to VSCode's welcome page, shows recent projects and quick actions.
"""

import os

from PyQt5.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QLabel,
    QListWidget, QListWidgetItem, QFrame, QSizePolicy,
)
from PyQt5.QtCore import Qt, pyqtSignal, QSize
from PyQt5.QtGui import QFont, QIcon, QPixmap, QPainter, QColor

from qfluentwidgets import (
    PushButton, PrimaryPushButton, SubtitleLabel, BodyLabel,
    CardWidget, IconWidget, FluentIcon,
)

from ..model.config import get_config


class RecentProjectItem(CardWidget):
    """Card widget for a recent project entry."""

    item_clicked = pyqtSignal(str, str)  # app_name, egui_root

    def __init__(self, app_name, egui_root, parent=None):
        super().__init__(parent)
        self.app_name = app_name
        self.egui_root = egui_root
        self.setCursor(Qt.PointingHandCursor)
        self.setFixedHeight(70)

        layout = QHBoxLayout(self)
        layout.setContentsMargins(16, 12, 16, 12)
        layout.setSpacing(12)

        # Icon
        icon_label = QLabel()
        icon_label.setFixedSize(40, 40)
        icon_label.setStyleSheet("""
            background-color: #0078d4;
            border-radius: 6px;
            color: white;
            font-size: 18px;
            font-weight: bold;
        """)
        icon_label.setAlignment(Qt.AlignCenter)
        icon_label.setText(app_name[0].upper() if app_name else "?")
        layout.addWidget(icon_label)

        # Text info
        text_layout = QVBoxLayout()
        text_layout.setSpacing(4)

        name_label = SubtitleLabel(app_name)
        name_label.setStyleSheet("color: #fff;")
        text_layout.addWidget(name_label)

        # Show relative path
        app_dir = os.path.join(egui_root, "example", app_name)
        path_label = BodyLabel(app_dir)
        path_label.setStyleSheet("color: #888;")
        text_layout.addWidget(path_label)

        layout.addLayout(text_layout, 1)

        # Check if project exists
        eui_file = os.path.join(app_dir, f"{app_name}.egui")
        if os.path.isfile(eui_file):
            status = QLabel("Has project")
            status.setStyleSheet("color: #4caf50; font-size: 11px;")
        else:
            status = QLabel("No project")
            status.setStyleSheet("color: #ff9800; font-size: 11px;")
        layout.addWidget(status)

    def mouseReleaseEvent(self, event):
        if event.button() == Qt.LeftButton:
            self.item_clicked.emit(self.app_name, self.egui_root)
        super().mouseReleaseEvent(event)


class WelcomePage(QWidget):
    """Welcome page shown when no project is loaded.

    Signals:
        open_recent(app_name, egui_root): User clicked a recent project
        new_project(): User clicked New Project
        open_app(): User clicked Open App
    """

    open_recent = pyqtSignal(str, str)
    new_project = pyqtSignal()
    open_app = pyqtSignal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self._config = get_config()
        self._init_ui()

    def _init_ui(self):
        self.setStyleSheet("""
            QWidget {
                background-color: #1e1e1e;
            }
        """)

        main_layout = QHBoxLayout(self)
        main_layout.setContentsMargins(0, 0, 0, 0)

        # Center content with max width
        center_widget = QWidget()
        center_widget.setMaximumWidth(800)
        center_layout = QVBoxLayout(center_widget)
        center_layout.setContentsMargins(40, 40, 40, 40)
        center_layout.setSpacing(24)

        # Title
        title = QLabel("EmbeddedGUI Designer")
        title.setFont(QFont("Segoe UI", 28, QFont.Light))
        title.setStyleSheet("color: #fff;")
        center_layout.addWidget(title)

        # Subtitle
        subtitle = QLabel("Lightweight GUI framework for embedded systems")
        subtitle.setFont(QFont("Segoe UI", 12))
        subtitle.setStyleSheet("color: #888;")
        center_layout.addWidget(subtitle)

        center_layout.addSpacing(20)

        # Content area: two columns
        content_layout = QHBoxLayout()
        content_layout.setSpacing(40)

        # Left column: Start
        left_col = QVBoxLayout()
        left_col.setSpacing(12)

        start_label = QLabel("Start")
        start_label.setFont(QFont("Segoe UI", 14, QFont.DemiBold))
        start_label.setStyleSheet("color: #ccc;")
        left_col.addWidget(start_label)

        # New Project button
        new_btn = PrimaryPushButton("New Project...")
        new_btn.setFixedWidth(200)
        new_btn.clicked.connect(self.new_project.emit)
        left_col.addWidget(new_btn)

        # Open App button
        open_btn = PushButton("Open App...")
        open_btn.setFixedWidth(200)
        open_btn.clicked.connect(self.open_app.emit)
        left_col.addWidget(open_btn)

        left_col.addStretch()
        content_layout.addLayout(left_col)

        # Right column: Recent
        right_col = QVBoxLayout()
        right_col.setSpacing(12)

        recent_label = QLabel("Recent")
        recent_label.setFont(QFont("Segoe UI", 14, QFont.DemiBold))
        recent_label.setStyleSheet("color: #ccc;")
        right_col.addWidget(recent_label)

        # Recent projects list
        self._recent_list = QVBoxLayout()
        self._recent_list.setSpacing(8)
        right_col.addLayout(self._recent_list)

        right_col.addStretch()
        content_layout.addLayout(right_col, 1)

        center_layout.addLayout(content_layout, 1)

        # Footer
        footer = QLabel("Press Ctrl+Shift+O to open an app, or Ctrl+N to create a new project")
        footer.setStyleSheet("color: #666; font-size: 11px;")
        footer.setAlignment(Qt.AlignCenter)
        center_layout.addWidget(footer)

        # Center the content
        main_layout.addStretch()
        main_layout.addWidget(center_widget)
        main_layout.addStretch()

        self._refresh_recent_list()

    def _refresh_recent_list(self):
        """Refresh the recent projects list."""
        # Clear existing items
        while self._recent_list.count():
            item = self._recent_list.takeAt(0)
            if item.widget():
                item.widget().deleteLater()

        recent = self._config.recent_apps
        if not recent:
            no_recent = QLabel("No recent projects")
            no_recent.setStyleSheet("color: #666; font-style: italic;")
            self._recent_list.addWidget(no_recent)
            return

        for app_name, egui_root in recent[:8]:  # Show max 8 recent
            item = RecentProjectItem(app_name, egui_root)
            item.item_clicked.connect(self._on_recent_clicked)
            self._recent_list.addWidget(item)

    def _on_recent_clicked(self, app_name, egui_root):
        """Handle click on recent project item."""
        self.open_recent.emit(app_name, egui_root)

    def refresh(self):
        """Refresh the welcome page (e.g., after config change)."""
        self._refresh_recent_list()

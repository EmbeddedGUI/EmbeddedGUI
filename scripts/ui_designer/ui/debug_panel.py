"""Debug output panel for EmbeddedGUI Designer."""

import datetime

from PyQt5.QtWidgets import (
    QWidget,
    QVBoxLayout,
    QHBoxLayout,
    QPlainTextEdit,
    QPushButton,
)
from PyQt5.QtCore import Qt
from PyQt5.QtGui import QFont, QTextCharFormat, QColor


class DebugPanel(QWidget):
    """Panel for displaying compile output and debug information."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self._init_ui()

    def _init_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)

        # Toolbar
        toolbar = QHBoxLayout()
        toolbar.setContentsMargins(4, 2, 4, 2)
        toolbar.setSpacing(4)

        self._clear_btn = QPushButton("Clear")
        self._clear_btn.setFixedWidth(80)
        self._clear_btn.clicked.connect(self.clear)
        toolbar.addWidget(self._clear_btn)

        toolbar.addStretch()

        toolbar_widget = QWidget()
        toolbar_widget.setLayout(toolbar)
        layout.addWidget(toolbar_widget)

        # Output text area
        self._output = QPlainTextEdit()
        self._output.setReadOnly(True)
        self._output.setFont(QFont("Consolas", 9))
        self._output.setLineWrapMode(QPlainTextEdit.NoWrap)
        self._output.setMaximumBlockCount(5000)  # Limit lines
        layout.addWidget(self._output)

        # Formats for different message types
        self._error_format = QTextCharFormat()
        self._error_format.setForeground(QColor("#ff6b6b"))

        self._success_format = QTextCharFormat()
        self._success_format.setForeground(QColor("#69db7c"))

        self._info_format = QTextCharFormat()
        self._info_format.setForeground(QColor("#a0a0a0"))

        self._action_format = QTextCharFormat()
        self._action_format.setForeground(QColor("#74c0fc"))

        self._cmd_format = QTextCharFormat()
        self._cmd_format.setForeground(QColor("#ffd43b"))

    def _timestamp(self):
        """Get current timestamp string with milliseconds."""
        return datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]

    def clear(self):
        """Clear all output."""
        self._output.clear()

    def get_output_font(self):
        """Get the current font used in the debug output."""
        return self._output.font()

    def set_output_font_size(self, pixel_size):
        """Set the debug output font size (pixels)."""
        font = self._output.font()
        font.setPixelSize(int(pixel_size))
        self._output.setFont(font)

    def set_output_font_size_pt(self, point_size):
        """Set the debug output font size (points)."""
        font = self._output.font()
        font.setPointSize(int(point_size))
        self._output.setFont(font)

    def append_text(self, text, msg_type="info"):
        """Append text to the output.

        Args:
            text: Text to append
            msg_type: "info", "error", "success", "action", or "cmd"
        """
        cursor = self._output.textCursor()
        cursor.movePosition(cursor.End)

        fmt = self._info_format
        if msg_type == "error":
            fmt = self._error_format
        elif msg_type == "success":
            fmt = self._success_format
        elif msg_type == "action":
            fmt = self._action_format
        elif msg_type == "cmd":
            fmt = self._cmd_format

        cursor.insertText(text + "\n", fmt)

        # Auto-scroll to bottom
        self._output.setTextCursor(cursor)
        self._output.ensureCursorVisible()

    def log(self, message, msg_type="info"):
        """Log a message with timestamp.

        Args:
            message: Message to log
            msg_type: "info", "error", "success", "action", or "cmd"
        """
        self.append_text(f"[{self._timestamp()}] {message}", msg_type)

    def log_action(self, action):
        """Log an action (e.g., 'Starting compile...')."""
        self.log(action, "action")

    def log_cmd(self, cmd):
        """Log a command being executed."""
        self.log(f"$ {cmd}", "cmd")

    def log_success(self, message):
        """Log a success message."""
        self.log(message, "success")

    def log_error(self, message):
        """Log an error message."""
        self.log(message, "error")

    def log_info(self, message):
        """Log an info message."""
        self.log(message, "info")

    def log_compile_output(self, success, output):
        """Log compile output with appropriate formatting.

        Args:
            success: Whether compilation succeeded
            output: Full compile output text
        """
        self.append_text("")

        if success:
            self.append_text("=== Compilation Successful ===", "success")
        else:
            self.append_text("=== Compilation Failed ===", "error")

        self.append_text("")

        # Add output lines
        for line in output.split("\n"):
            if not line.strip():
                continue

            # Detect error/warning lines
            line_lower = line.lower()
            if "error:" in line_lower or "error " in line_lower:
                self.append_text(line, "error")
            elif "warning:" in line_lower:
                self.append_text(line, "info")
            else:
                self.append_text(line, "info")

    # Keep old method for compatibility
    def set_compile_output(self, success, output):
        """Set compile output (clears first, for backward compatibility)."""
        self.log_compile_output(success, output)

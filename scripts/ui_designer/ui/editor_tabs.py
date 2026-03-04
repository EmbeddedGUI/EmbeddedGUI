"""Editor tabs panel — Design / Split / Code tri-view.

Mimics Android Studio's layout editor mode switching:
  - Design: visual drag-and-drop canvas (PreviewPanel)
  - Code:   XML source editor with syntax highlighting
  - Split:  side-by-side XML editor + live canvas preview

Bidirectional sync:
  Design → Code: model changes regenerate XML text
  Code → Design: XML edits parse back into model (debounced)
"""

from PyQt5.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QStackedWidget,
    QPlainTextEdit, QSplitter, QPushButton, QButtonGroup,
    QLabel, QFrame,
)
from PyQt5.QtCore import Qt, QTimer, pyqtSignal
from PyQt5.QtGui import QFont, QColor, QPalette

from .xml_highlighter import XmlSyntaxHighlighter
from .preview_panel import PreviewPanel


# Editor modes
MODE_DESIGN = "design"
MODE_SPLIT = "split"
MODE_CODE = "code"


class XmlEditor(QPlainTextEdit):
    """Plain text editor styled for XML editing."""

    def __init__(self, parent=None):
        super().__init__(parent)
        font = QFont("Consolas", 10)
        font.setStyleHint(QFont.Monospace)
        self.setFont(font)
        self.setLineWrapMode(QPlainTextEdit.NoWrap)
        self.setTabStopDistance(
            self.fontMetrics().horizontalAdvance(" ") * 4
        )
        # Dark theme
        pal = self.palette()
        pal.setColor(QPalette.Base, QColor("#1E1E1E"))
        pal.setColor(QPalette.Text, QColor("#D4D4D4"))
        self.setPalette(pal)
        # Syntax highlighter
        self._highlighter = XmlSyntaxHighlighter(self.document())


class EditorTabs(QWidget):
    """Tri-view editor: Design / Split / Code.

    Signals:
        xml_changed(str): emitted when user edits XML (debounced)
        mode_changed(str): emitted when view mode switches
    """

    xml_changed = pyqtSignal(str)   # debounced XML text from code editor
    mode_changed = pyqtSignal(str)  # MODE_DESIGN / MODE_SPLIT / MODE_CODE
    save_requested = pyqtSignal()    # Ctrl+S pressed in the XML editor

    def __init__(self, preview_panel, parent=None):
        super().__init__(parent)
        self._preview = preview_panel
        self._mode = MODE_DESIGN
        self._syncing = False  # prevent feedback loops

        # Debounce timer for Code → Design sync
        self._parse_timer = QTimer()
        self._parse_timer.setSingleShot(True)
        self._parse_timer.setInterval(300)
        self._parse_timer.timeout.connect(self._emit_xml_changed)

        self._init_ui()

    def _init_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)

        # Stacked content area
        self._stack = QStackedWidget()
        layout.addWidget(self._stack, 1)

        # ── Design view (index 0) ──
        self._design_container = QWidget()
        design_layout = QVBoxLayout(self._design_container)
        design_layout.setContentsMargins(0, 0, 0, 0)
        # PreviewPanel is passed in and reparented here
        design_layout.addWidget(self._preview)
        self._stack.addWidget(self._design_container)

        # ── Code view (index 1) ──
        self._code_editor = XmlEditor()
        self._code_editor.textChanged.connect(self._on_code_text_changed)
        self._stack.addWidget(self._code_editor)

        # ── Split view (index 2) ──
        self._split_container = QWidget()
        split_layout = QVBoxLayout(self._split_container)
        split_layout.setContentsMargins(0, 0, 0, 0)
        self._split = QSplitter(Qt.Horizontal)
        self._split_editor = XmlEditor()
        self._split_editor.textChanged.connect(self._on_code_text_changed)
        self._split_preview_container = QWidget()
        # Split preview will hold a duplicate reference managed externally
        self._split.addWidget(self._split_editor)
        self._split.addWidget(self._split_preview_container)
        self._split.setSizes([400, 400])
        split_layout.addWidget(self._split)
        self._stack.addWidget(self._split_container)

        # ── Mode switch toolbar (bottom) ──
        toolbar = QFrame()
        toolbar.setFrameStyle(QFrame.StyledPanel)
        toolbar.setMaximumHeight(36)
        tb_layout = QHBoxLayout(toolbar)
        tb_layout.setContentsMargins(4, 2, 4, 2)

        self._btn_group = QButtonGroup(self)
        self._btn_group.setExclusive(True)

        for label, mode in [("Code", MODE_CODE), ("Split", MODE_SPLIT), ("Design", MODE_DESIGN)]:
            btn = QPushButton(label)
            btn.setCheckable(True)
            btn.setMinimumWidth(70)
            btn.setStyleSheet("""
                QPushButton { padding: 4px 12px; border: 1px solid #555; border-radius: 3px; }
                QPushButton:checked { background-color: #0078D4; color: white; border-color: #0078D4; }
            """)
            if mode == MODE_DESIGN:
                btn.setChecked(True)
            self._btn_group.addButton(btn)
            tb_layout.addWidget(btn)
            btn.clicked.connect(lambda checked, m=mode: self.set_mode(m))

        tb_layout.addStretch()
        layout.addWidget(toolbar)

        # Ctrl+S is handled by the main window QAction (no local QShortcut
        # here — creating one would cause an ambiguous shortcut conflict).

    # ── Public API ─────────────────────────────────────────────────

    @property
    def mode(self):
        return self._mode

    def set_mode(self, mode):
        """Switch between Design / Split / Code."""
        if mode == self._mode:
            return
        old_mode = self._mode
        self._mode = mode
        if mode == MODE_DESIGN:
            # Flush pending XML changes before switching to Design
            if self._parse_timer.isActive():
                self._parse_timer.stop()
                self._emit_xml_changed()
            # Reparent preview back to design container
            self._design_container.layout().addWidget(self._preview)
            self._stack.setCurrentIndex(0)
        elif mode == MODE_CODE:
            self._stack.setCurrentIndex(1)
        elif mode == MODE_SPLIT:
            # Reparent preview to split right pane
            split_layout = self._split_preview_container.layout()
            if split_layout is None:
                split_layout = QVBoxLayout(self._split_preview_container)
                split_layout.setContentsMargins(0, 0, 0, 0)
            split_layout.addWidget(self._preview)
            self._stack.setCurrentIndex(2)
        self.mode_changed.emit(mode)

    def set_xml_text(self, xml_text):
        """Update both XML editors with new text (Design → Code direction).

        Call this when the model changes from Design interactions.
        """
        self._syncing = True
        self._code_editor.setPlainText(xml_text)
        self._split_editor.setPlainText(xml_text)
        self._syncing = False

    def get_xml_text(self):
        """Get current XML text from the active editor."""
        if self._mode == MODE_SPLIT:
            return self._split_editor.toPlainText()
        return self._code_editor.toPlainText()

    @property
    def code_editor(self):
        return self._code_editor

    @property
    def split_editor(self):
        return self._split_editor

    @property
    def preview(self):
        return self._preview

    # ── Internal ───────────────────────────────────────────────────

    def _on_code_text_changed(self):
        """User is typing in XML editor — debounce parse."""
        if self._syncing:
            return
        self._parse_timer.start()

    def _emit_xml_changed(self):
        """Debounce expired — emit parsed XML."""
        text = self.get_xml_text()
        # Sync the other editor
        self._syncing = True
        if self._mode == MODE_CODE:
            self._split_editor.setPlainText(text)
        elif self._mode == MODE_SPLIT:
            self._code_editor.setPlainText(text)
        self._syncing = False
        self.xml_changed.emit(text)

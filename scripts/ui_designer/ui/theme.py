
from PyQt5.QtWidgets import QApplication
try:
    from qfluentwidgets import setTheme, Theme
    HAS_FLUENT = True
except ImportError:
    HAS_FLUENT = False


STYLESHEET_DARK = """
/* Main Window & Containers */
QMainWindow, QDialog, QDockWidget, QScrollArea, QFrame {
    background-color: #202020;
    color: #e0e0e0;
}
QWidget {
    background-color: #202020;
    color: #e0e0e0;
}

/* Text & Labels */
QLabel, QCheckBox, QRadioButton, QToolButton {
    color: #e0e0e0;
    background-color: transparent;
}

/* Input Fields */
QLineEdit, QTextEdit, QPlainTextEdit, QAbstractSpinBox, QSpinBox, QDoubleSpinBox {
    background-color: #2d2d2d;
    color: #ffffff;
    border: 1px solid #454545;
    border-radius: 4px;
    padding: 4px;
    selection-background-color: #0078d4;
    selection-color: #ffffff;
}
QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus, QAbstractSpinBox:focus, QSpinBox:focus, QDoubleSpinBox:focus {
    border: 1px solid #0078d4;
    background-color: #333333;
}
QLineEdit:disabled, QTextEdit:disabled, QPlainTextEdit:disabled, QAbstractSpinBox:disabled, QSpinBox:disabled, QDoubleSpinBox:disabled {
    background-color: #252525;
    color: #808080;
    border: 1px solid #353535;
}

/* Buttons (Standard only) */
QPushButton {
    background-color: #333333;
    color: #e0e0e0;
    border: 1px solid #555555;
    border-radius: 4px;
    padding: 5px 12px;
}
QPushButton:hover {
    background-color: #404040;
    border-color: #0078d4;
}
QPushButton:pressed {
    background-color: #282828;
    border-color: #005a9e;
}
QPushButton:disabled {
    background-color: #252525;
    color: #707070;
    border-color: #353535;
}

/* Lists, Trees, Tables */
QListView, QTreeView, QTableView, QListWidget, QTreeWidget, QTableWidget {
    background-color: #1e1e1e;
    color: #e0e0e0;
    border: 1px solid #353535;
    outline: none;
}
QListView::item, QTreeView::item, QTableView::item {
    padding: 4px;
}
QListView::item:hover, QTreeView::item:hover, QTableView::item:hover {
    background-color: #2d2d2d;
}
QListView::item:selected, QTreeView::item:selected, QTableView::item:selected {
    background-color: #0064c0;
    color: #ffffff;
}
QHeaderView::section {
    background-color: #333333;
    color: #e0e0e0;
    padding: 4px;
    border: none;
    border-right: 1px solid #454545;
    border-bottom: 1px solid #454545;
}

/* Scrollbars (Basic) */
QScrollBar:vertical {
    background: #202020;
    width: 12px;
    margin: 0px;
}
QScrollBar::handle:vertical {
    background: #4d4d4d;
    min-height: 20px;
    border-radius: 6px;
    margin: 2px;
}
QScrollBar::handle:vertical:hover {
    background: #666666;
}
QScrollBar:horizontal {
    background: #202020;
    height: 12px;
    margin: 0px;
}
QScrollBar::handle:horizontal {
    background: #4d4d4d;
    min-width: 20px;
    border-radius: 6px;
    margin: 2px;
}
QScrollBar::handle:horizontal:hover {
    background: #666666;
}
QScrollArea {
    background-color: transparent;
    border: none;
}

/* Menus */
QMenuBar {
    background-color: #202020;
    color: #e0e0e0;
    border-bottom: 1px solid #353535;
}
QMenuBar::item {
    padding: 6px 10px;
    background: transparent;
}
QMenuBar::item:selected {
    background-color: #333333;
}
QMenu {
    background-color: #252525;
    color: #e0e0e0;
    border: 1px solid #454545;
}
QMenu::item {
    padding: 6px 24px 6px 12px;
}
QMenu::item:selected {
    background-color: #0064c0;
    color: #ffffff;
}
QMenu::separator {
    height: 1px;
    background: #454545;
    margin: 4px 0;
}

/* ComboBox */
QComboBox {
    background-color: #2d2d2d;
    color: #e0e0e0;
    border: 1px solid #454545;
    border-radius: 4px;
    padding: 4px;
}
QComboBox:hover {
    border-color: #0078d4;
}
QComboBox::drop-down {
    border: none;
    width: 20px;
    background: transparent;
}
QComboBox QAbstractItemView {
    background-color: #252525;
    color: #e0e0e0;
    selection-background-color: #0064c0;
    selection-color: #ffffff;
    border: 1px solid #454545;
}

/* Tooltips */
QToolTip {
    background-color: #333333;
    color: #e0e0e0;
    border: 1px solid #555555;
    padding: 4px;
}

/* StatusBar */
QStatusBar {
    background-color: #202020;
    color: #999999;
    border-top: 1px solid #353535;
}

/* Dialog Button Box */
QDialogButtonBox QPushButton {
    background-color: #333333;
    color: #e0e0e0;
    border: 1px solid #555555;
    padding: 6px 16px;
    border-radius: 4px;
}
QDialogButtonBox QPushButton:hover {
    background-color: #404040;
    border-color: #0078d4;
}

/* GroupBox - consistent with property panel needs */
QGroupBox {
    color: #dddddd;
    background-color: transparent;
    border: 1px solid #555555;
    border-radius: 4px;
    margin-top: 16px;
    padding-top: 8px;
}
QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    left: 8px;
    top: 0px;
    padding: 0 4px;
}
"""

STYLESHEET_LIGHT = """
/* Main Window & Containers */
QMainWindow, QDialog, QDockWidget, QScrollArea, QFrame {
    background-color: #f9f9f9;
    color: #000000;
}
QWidget {
    background-color: #f9f9f9;
    color: #000000;
}

/* Text & Labels */
QLabel, QCheckBox, QRadioButton, QToolButton {
    color: #000000;
    background-color: transparent;
}

/* Input Fields */
QLineEdit, QTextEdit, QPlainTextEdit, QAbstractSpinBox {
    background-color: #ffffff;
    color: #000000;
    border: 1px solid #c0c0c0;
    border-radius: 4px;
    padding: 4px;
    selection-background-color: #0078d4;
    selection-color: #ffffff;
}
QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus, QAbstractSpinBox:focus {
    border: 1px solid #0078d4;
    background-color: #ffffff;
}
QLineEdit:disabled, QTextEdit:disabled, QPlainTextEdit:disabled, QAbstractSpinBox:disabled {
    background-color: #f0f0f0;
    color: #a0a0a0;
    border: 1px solid #d0d0d0;
}

/* Buttons */
QPushButton {
    background-color: #ffffff;
    color: #333333;
    border: 1px solid #cccccc;
    border-radius: 4px;
    padding: 5px 12px;
}
QPushButton:hover {
    background-color: #f0f0f0;
    border-color: #0078d4;
}
QPushButton:pressed {
    background-color: #e0e0e0;
    border-color: #005a9e;
}
QPushButton:disabled {
    background-color: #f0f0f0;
    color: #a0a0a0;
    border-color: #d0d0d0;
}

/* Lists, Trees */
QListView, QTreeView, QTableView, QListWidget, QTreeWidget, QTableWidget {
    background-color: #ffffff;
    color: #000000;
    border: 1px solid #e0e0e0;
}
QListView::item, QTreeView::item, QTableView::item {
    padding: 4px;
}
QListView::item:hover, QTreeView::item:hover, QTableView::item:hover {
    background-color: #e8e8e8;
}
QListView::item:selected, QTreeView::item:selected, QTableView::item:selected {
    background-color: #0078d4;
    color: #ffffff;
}
QHeaderView::section {
    background-color: #f0f0f0;
    color: #333333;
    padding: 4px;
    border: none;
    border-right: 1px solid #d0d0d0;
    border-bottom: 1px solid #d0d0d0;
}

/* Scrollbars */
QScrollBar:vertical {
    background: #f0f0f0;
    width: 12px;
    margin: 0px;
}
QScrollBar::handle:vertical {
    background: #c0c0c0;
    min-height: 20px;
    border-radius: 6px;
    margin: 2px;
}
QScrollBar::handle:vertical:hover {
    background: #a0a0a0;
}
QScrollBar:horizontal {
    background: #f0f0f0;
    height: 12px;
    margin: 0px;
}
QScrollBar::handle:horizontal {
    background: #c0c0c0;
    min-width: 20px;
    border-radius: 6px;
    margin: 2px;
}
QScrollBar::handle:horizontal:hover {
    background: #a0a0a0;
}
QScrollArea {
    background-color: transparent;
    border: none;
}

/* Menus */
QMenuBar {
    background-color: #f9f9f9;
    color: #000000;
    border-bottom: 1px solid #e0e0e0;
}
QMenuBar::item {
    padding: 6px 10px;
    background: transparent;
}
QMenuBar::item:selected {
    background-color: #e0e0e0;
}
QMenu {
    background-color: #ffffff;
    color: #000000;
    border: 1px solid #cccccc;
     border-radius: 0px;
}
QMenu::item {
    padding: 6px 24px 6px 12px;
}
QMenu::item:selected {
    background-color: #0078d4;
    color: #ffffff;
}
QMenu::separator {
    height: 1px;
    background: #e0e0e0;
    margin: 4px 0;
}

/* ComboBox */
QComboBox {
    background-color: #ffffff;
    color: #000000;
    border: 1px solid #c0c0c0;
    border-radius: 4px;
    padding: 4px;
}
QComboBox:hover {
    border-color: #0078d4;
}
QComboBox::drop-down {
    border: none;
    width: 20px;
    background: transparent;
}
QComboBox QAbstractItemView {
    background-color: #ffffff;
    color: #000000;
    selection-background-color: #0078d4;
    selection-color: #ffffff;
    border: 1px solid #c0c0c0;
}

/* StatusBar */
QStatusBar {
    background-color: #007acc;
    color: #ffffff;
    border-top: 1px solid #005a9e;
}

/* GroupBox */
QGroupBox {
    color: #333333;
    background-color: transparent;
    border: 1px solid #c0c0c0;
    border-radius: 4px;
    margin-top: 16px;
    padding-top: 8px;
}
QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    left: 8px;
    top: 0px;
    padding: 0 4px;
}
"""

def apply_theme(app: QApplication, mode='dark'):
    """Apply Dark or Light theme."""
    if mode == 'dark':
        if HAS_FLUENT:
            setTheme(Theme.DARK)
        app.setStyleSheet(STYLESHEET_DARK)
    else:
        if HAS_FLUENT:
            setTheme(Theme.LIGHT)
        app.setStyleSheet(STYLESHEET_LIGHT)

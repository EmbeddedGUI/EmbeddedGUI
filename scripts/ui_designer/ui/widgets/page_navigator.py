"""Page navigator widget with thumbnails for multi-page management."""

from PyQt5.QtWidgets import (
    QWidget, QVBoxLayout, QScrollArea, QLabel, QMenu, QAction,
    QInputDialog, QMessageBox,
)
from PyQt5.QtCore import pyqtSignal, Qt, QSize
from PyQt5.QtGui import QPixmap, QImage, QPainter, QPen, QColor

from ...engine.python_renderer import render_page


# Thumbnail size
THUMB_WIDTH = 120
THUMB_HEIGHT = 160


def _pil_to_qpixmap(pil_image):
    """Convert a PIL Image to QPixmap."""
    data = pil_image.tobytes("raw", "RGBA")
    qimg = QImage(data, pil_image.width, pil_image.height, QImage.Format_RGBA8888)
    return QPixmap.fromImage(qimg)


class PageThumbnail(QWidget):
    """Single page thumbnail with label and click selection."""

    clicked = pyqtSignal(str)  # page_name
    context_menu_requested = pyqtSignal(str, object)  # page_name, QPoint

    def __init__(self, page_name, parent=None):
        super().__init__(parent)
        self.page_name = page_name
        self._dirty = False
        self._selected = False
        self.setFixedSize(THUMB_WIDTH + 8, THUMB_HEIGHT + 24)
        self.setCursor(Qt.PointingHandCursor)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(4, 4, 4, 4)
        layout.setSpacing(2)

        self._thumb_label = QLabel()
        self._thumb_label.setFixedSize(THUMB_WIDTH, THUMB_HEIGHT)
        self._thumb_label.setAlignment(Qt.AlignCenter)
        self._thumb_label.setStyleSheet("border: 1px solid #555; background: #2a2a2a;")
        layout.addWidget(self._thumb_label)

        self._name_label = QLabel(page_name)
        self._name_label.setAlignment(Qt.AlignCenter)
        self._name_label.setStyleSheet("color: #ccc; font-size: 11px;")
        layout.addWidget(self._name_label)

    def set_selected(self, selected):
        self._selected = selected
        border = "2px solid #4a9eff" if selected else "1px solid #555"
        self._thumb_label.setStyleSheet(f"border: {border}; background: #2a2a2a;")

    def set_thumbnail(self, pixmap):
        scaled = pixmap.scaled(THUMB_WIDTH, THUMB_HEIGHT, Qt.KeepAspectRatio, Qt.SmoothTransformation)
        self._thumb_label.setPixmap(scaled)

    def set_dirty(self, dirty):
        self._dirty = bool(dirty)
        self._name_label.setText(f"{self.page_name}*" if self._dirty else self.page_name)

    def mousePressEvent(self, event):
        if event.button() == Qt.LeftButton:
            self.clicked.emit(self.page_name)
        elif event.button() == Qt.RightButton:
            self.context_menu_requested.emit(self.page_name, event.globalPos())

    def mouseDoubleClickEvent(self, event):
        if event.button() == Qt.LeftButton:
            self.clicked.emit(self.page_name)


# Page templates
PAGE_TEMPLATES = {
    "blank": {"name": "Blank", "widgets": []},
    "list": {"name": "List Page", "widgets": [
        {"type": "label", "name": "title", "x": 10, "y": 10, "w": 220, "h": 30, "text": "Title"},
        {"type": "scroll", "name": "list_scroll", "x": 0, "y": 50, "w": 240, "h": 270},
    ]},
    "detail": {"name": "Detail Page", "widgets": [
        {"type": "label", "name": "title", "x": 10, "y": 10, "w": 220, "h": 30, "text": "Detail"},
        {"type": "image", "name": "hero_image", "x": 10, "y": 50, "w": 220, "h": 120},
        {"type": "label", "name": "description", "x": 10, "y": 180, "w": 220, "h": 100, "text": "Description"},
    ]},
    "settings": {"name": "Settings Page", "widgets": [
        {"type": "label", "name": "title", "x": 10, "y": 10, "w": 220, "h": 30, "text": "Settings"},
        {"type": "switch", "name": "option_1", "x": 180, "y": 60, "w": 50, "h": 24},
        {"type": "switch", "name": "option_2", "x": 180, "y": 100, "w": 50, "h": 24},
    ]},
}


class PageNavigator(QWidget):
    """Sidebar widget showing page thumbnails with navigation.

    Signals:
        page_selected(str): Emitted when a page is clicked
        page_copy_requested(str): Emitted when copy is requested
        page_delete_requested(str): Emitted when delete is requested
        page_add_requested(str, str): Emitted with (template_key, page_name)
    """

    page_selected = pyqtSignal(str)
    page_copy_requested = pyqtSignal(str)
    page_delete_requested = pyqtSignal(str)
    page_add_requested = pyqtSignal(str, str)  # template_key, page_name

    def __init__(self, parent=None):
        super().__init__(parent)
        self._pages = {}  # page_name -> Page
        self._thumbnails = {}  # page_name -> PageThumbnail
        self._current_page = None
        self._dirty_pages = set()
        self._screen_width = 240
        self._screen_height = 320
        self._init_ui()

    def _init_ui(self):
        outer = QVBoxLayout(self)
        outer.setContentsMargins(4, 4, 4, 4)

        title = QLabel("Pages")
        title.setStyleSheet("font-weight: bold; color: #ccc; font-size: 13px;")
        outer.addWidget(title)

        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        outer.addWidget(scroll)

        self._container = QWidget()
        self._layout = QVBoxLayout(self._container)
        self._layout.setContentsMargins(0, 0, 0, 0)
        self._layout.setSpacing(4)
        self._layout.addStretch()
        scroll.setWidget(self._container)

    def set_screen_size(self, w, h):
        self._screen_width = w
        self._screen_height = h

    def set_pages(self, pages_dict):
        """Set all pages. pages_dict: {page_name: Page}."""
        self._pages = dict(pages_dict)
        self._rebuild()

    def set_current_page(self, page_name):
        self._current_page = page_name
        for name, thumb in self._thumbnails.items():
            thumb.set_selected(name == page_name)

    def set_dirty_pages(self, page_names):
        self._dirty_pages = set(page_names or [])
        for name, thumb in self._thumbnails.items():
            thumb.set_dirty(name in self._dirty_pages)

    def refresh_thumbnail(self, page_name):
        """Re-render thumbnail for a specific page."""
        if page_name not in self._pages or page_name not in self._thumbnails:
            return
        page = self._pages[page_name]
        try:
            img = render_page(page, self._screen_width, self._screen_height)
            pixmap = _pil_to_qpixmap(img)
            self._thumbnails[page_name].set_thumbnail(pixmap)
        except Exception:
            pass

    def refresh_all(self):
        for name in self._pages:
            self.refresh_thumbnail(name)

    def _rebuild(self):
        # Clear existing thumbnails
        for thumb in self._thumbnails.values():
            thumb.setParent(None)
            thumb.deleteLater()
        self._thumbnails.clear()

        # Remove stretch
        while self._layout.count():
            item = self._layout.takeAt(0)

        # Add thumbnails
        for name, page in self._pages.items():
            thumb = PageThumbnail(name)
            thumb.clicked.connect(self._on_thumb_clicked)
            thumb.context_menu_requested.connect(self._on_context_menu)
            thumb.set_selected(name == self._current_page)
            thumb.set_dirty(name in self._dirty_pages)
            self._thumbnails[name] = thumb
            self._layout.addWidget(thumb)

            # Render thumbnail
            try:
                img = render_page(page, self._screen_width, self._screen_height)
                pixmap = _pil_to_qpixmap(img)
                thumb.set_thumbnail(pixmap)
            except Exception:
                pass

        self._layout.addStretch()

    def _on_thumb_clicked(self, page_name):
        self.set_current_page(page_name)
        self.page_selected.emit(page_name)

    def _on_context_menu(self, page_name, pos):
        menu = QMenu(self)

        copy_action = menu.addAction("Copy Page")
        delete_action = menu.addAction("Delete Page")
        menu.addSeparator()

        # Add from template submenu
        template_menu = menu.addMenu("Add Page from Template")
        template_actions = {}
        for key, tmpl in PAGE_TEMPLATES.items():
            action = template_menu.addAction(tmpl["name"])
            template_actions[action] = key

        chosen = menu.exec_(pos)
        if chosen == copy_action:
            self.page_copy_requested.emit(page_name)
        elif chosen == delete_action:
            self.page_delete_requested.emit(page_name)
        elif chosen in template_actions:
            self.page_add_requested.emit(template_actions[chosen], page_name)

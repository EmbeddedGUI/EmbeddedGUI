"""Preview panel that embeds the exe window with draggable widget overlays."""

import sys
import json
from PyQt5.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QLabel, QFrame,
    QScrollArea, QSizePolicy, QPushButton, QSplitter,
)
from PyQt5.QtCore import Qt, QRect, QPoint, QPointF, QTimer, pyqtSignal, QRectF, QEvent
from PyQt5.QtGui import QPainter, QPen, QColor, QFont, QBrush, QTransform, QPixmap, QImage

from ..model.widget_registry import WidgetRegistry
from ..engine.python_renderer import render_page


# Overlay display modes
MODE_VERTICAL = "vertical"
MODE_HORIZONTAL = "horizontal"
MODE_HIDDEN = "hidden"

# Resize handle positions
HANDLE_NONE = 0
HANDLE_TOP_LEFT = 1
HANDLE_TOP = 2
HANDLE_TOP_RIGHT = 3
HANDLE_RIGHT = 4
HANDLE_BOTTOM_RIGHT = 5
HANDLE_BOTTOM = 6
HANDLE_BOTTOM_LEFT = 7
HANDLE_LEFT = 8

# Handle size in pixels
HANDLE_SIZE = 8


def _parent_has_layout(widget):
    """Check if widget's parent uses a layout function (LinearLayout etc)."""
    if widget.parent is None:
        return False
    type_info = WidgetRegistry.instance().get(widget.parent.widget_type)
    return type_info.get("layout_func") is not None


def _parent_is_horizontal(widget):
    """Check if widget's parent LinearLayout is horizontal."""
    if widget.parent is None:
        return False
    return widget.parent.properties.get("orientation", "vertical") == "horizontal"


def _snap_to_grid(value, grid_size):
    """Snap a value to the nearest grid point."""
    if grid_size <= 1:
        return value
    return round(value / grid_size) * grid_size


class WidgetOverlay(QWidget):
    """Overlay showing widget bounds with drag, resize, and grid snap.

    Features:
    - Dual-mode drag: free drag for Group, reorder drag for LinearLayout
    - Resize handles on selected widget (8 handles: corners + edges)
    - Grid snap with configurable grid size
    - Coordinate display while dragging/resizing
    - Snap guide lines
    """

    widget_moved = pyqtSignal(object, int, int)  # widget, new_x, new_y
    widget_resized = pyqtSignal(object, int, int)  # widget, new_width, new_height
    widget_selected = pyqtSignal(object)  # widget
    widget_reordered = pyqtSignal(object, int)  # widget, new_index
    zoom_changed = pyqtSignal(float)  # zoom factor
    resource_dropped = pyqtSignal(object, str, str)  # widget, res_type, filename
    mouse_position_changed = pyqtSignal(int, int, object)  # x, y, widget_under_cursor (or None)
    drag_started = pyqtSignal()   # emitted when drag or resize begins
    drag_finished = pyqtSignal()  # emitted when drag or resize ends

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setAttribute(Qt.WA_TransparentForMouseEvents, False)
        self.setAttribute(Qt.WA_TranslucentBackground, True)
        self.setMouseTracking(True)
        self.setAcceptDrops(True)
        self._solid_background = False

        self._widgets = []  # list of WidgetModel
        self._selected = None
        self._hovered = None
        self._multi_selected = set()  # set of WidgetModel for multi-select

        # Zoom state
        self._zoom = 1.0
        self._zoom_min = 0.25
        self._zoom_max = 4.0
        self._base_width = 240   # set by PreviewPanel later
        self._base_height = 320

        # Drag state
        self._dragging = False
        self._drag_offset = QPoint()
        self._reorder_mode = False
        self._insert_index = -1
        self._insert_line_rect = None

        # Resize state
        self._resizing = False
        self._resize_handle = HANDLE_NONE
        self._resize_start_rect = None  # Original widget rect before resize
        self._resize_start_pos = None  # Mouse position at resize start

        # Grid snap settings
        self._grid_size = 8  # Snap to 8px grid
        self._show_grid = True
        self._snap_guides = []  # List of (orientation, position) for snap lines

        # Coordinate display
        self._show_coords = False  # True while dragging/resizing

        # Background mockup image
        self._bg_image = None       # QPixmap or None
        self._bg_image_visible = True
        self._bg_image_opacity = 0.3  # 0.0 ~ 1.0

        # Rubber-band selection
        self._rubber_band = False
        self._rubber_start = QPoint()
        self._rubber_rect = QRect()

        self.setFocusPolicy(Qt.StrongFocus)  # Enable keyboard events

    # ── Zoom helpers ───────────────────────────────────────────────

    def set_base_size(self, w, h):
        """Set the logical (unscaled) canvas size."""
        self._base_width = w
        self._base_height = h
        self._apply_zoom()

    def _apply_zoom(self):
        """Resize the widget according to the current zoom factor."""
        new_w = int(self._base_width * self._zoom)
        new_h = int(self._base_height * self._zoom)
        self.setFixedSize(new_w, new_h)
        self.update()

    def set_zoom(self, factor):
        factor = max(self._zoom_min, min(self._zoom_max, factor))
        if factor != self._zoom:
            self._zoom = factor
            self._apply_zoom()
            self.zoom_changed.emit(self._zoom)

    def zoom_in(self):
        self.set_zoom(self._zoom * 1.15)

    def zoom_out(self):
        self.set_zoom(self._zoom / 1.15)

    def zoom_reset(self):
        self.set_zoom(1.0)

    def _to_logical(self, pos):
        """Convert screen-space mouse position to logical (unscaled) position."""
        return QPoint(int(pos.x() / self._zoom), int(pos.y() / self._zoom))

    def wheelEvent(self, event):
        """Ctrl+Scroll to zoom in/out."""
        if event.modifiers() & Qt.ControlModifier:
            delta = event.angleDelta().y()
            if delta > 0:
                self.zoom_in()
            elif delta < 0:
                self.zoom_out()
            event.accept()
        else:
            super().wheelEvent(event)

    def set_grid_size(self, size):
        """Set base grid snap size (0 to disable)."""
        self._grid_size = max(0, size)
        self.update()

    def _effective_grid_size(self):
        """Calculate effective grid size based on zoom level.

        At higher zoom levels, the grid becomes finer (down to 1px).
        At lower zoom levels, the grid becomes coarser.
        """
        if self._grid_size <= 0:
            return 0

        # Scale grid inversely with zoom: higher zoom = finer grid
        # zoom >= 4.0 -> grid = 1
        # zoom >= 2.0 -> grid = 2
        # zoom >= 1.0 -> grid = 4
        # zoom >= 0.5 -> grid = 8
        # zoom < 0.5  -> grid = 16
        if self._zoom >= 4.0:
            return 1
        elif self._zoom >= 2.0:
            return 2
        elif self._zoom >= 1.0:
            return 4
        elif self._zoom >= 0.5:
            return 8
        else:
            return 16

    def set_show_grid(self, show):
        """Toggle grid visibility."""
        self._show_grid = show
        self.update()

    def set_solid_background(self, solid):
        """Switch between transparent overlay and solid background mode."""
        self._solid_background = solid
        self.setAttribute(Qt.WA_TranslucentBackground, not solid)
        if solid:
            self.setAutoFillBackground(True)
            self.setStyleSheet("background-color: #2a2a2a;")
        else:
            self.setAutoFillBackground(False)
            self.setStyleSheet("")
        self.update()

    # ── Background mockup image ──────────────────────────────────

    def set_background_image(self, pixmap):
        """Set the background mockup image (QPixmap). None to clear."""
        self._bg_image = pixmap
        self.update()

    def set_background_image_visible(self, visible):
        """Toggle background image visibility."""
        self._bg_image_visible = visible
        self.update()

    def set_background_image_opacity(self, opacity):
        """Set background image opacity (0.0 to 1.0)."""
        self._bg_image_opacity = max(0.0, min(1.0, opacity))
        self.update()

    def clear_background_image(self):
        """Remove the background image."""
        self._bg_image = None
        self.update()

    def set_widgets(self, widgets):
        """Set the flat list of widgets to display."""
        self._widgets = widgets or []
        self.update()

    def set_selected(self, widget):
        """Set the currently selected widget."""
        self._selected = widget
        self.update()

    def _widget_at(self, pos):
        """Find widget at given position using display coordinates."""
        for w in reversed(self._widgets):
            rect = QRect(w.display_x, w.display_y, w.width, w.height)
            if rect.contains(pos):
                return w
        return None

    def _get_handle_rects(self, widget):
        """Get rectangles for all 8 resize handles of a widget.

        Returns rects in *logical* space but handle size is divided by the
        current zoom so that handles appear as constant screen-pixels.
        """
        if widget is None:
            return {}
        x, y = widget.display_x, widget.display_y
        w, h = widget.width, widget.height
        hs = HANDLE_SIZE / self._zoom   # constant screen size
        hh = hs / 2.0

        def _r(cx, cy):
            return QRectF(cx - hh, cy - hh, hs, hs)

        return {
            HANDLE_TOP_LEFT:     _r(x, y),
            HANDLE_TOP:          _r(x + w / 2, y),
            HANDLE_TOP_RIGHT:    _r(x + w, y),
            HANDLE_RIGHT:        _r(x + w, y + h / 2),
            HANDLE_BOTTOM_RIGHT: _r(x + w, y + h),
            HANDLE_BOTTOM:       _r(x + w / 2, y + h),
            HANDLE_BOTTOM_LEFT:  _r(x, y + h),
            HANDLE_LEFT:         _r(x, y + h / 2),
        }

    def _handle_at(self, pos):
        """Check if position is over a resize handle of the selected widget.

        Returns HANDLE_NONE if widget is in a layout container (no resize allowed).
        """
        if self._selected is None:
            return HANDLE_NONE
        # No resize handles for widgets inside LinearLayout etc.
        if _parent_has_layout(self._selected):
            return HANDLE_NONE
        handles = self._get_handle_rects(self._selected)
        posf = QPointF(pos)
        for handle_id, rect in handles.items():
            if rect.contains(posf):
                return handle_id
        return HANDLE_NONE

    def _cursor_for_handle(self, handle):
        """Get appropriate cursor for a resize handle."""
        cursors = {
            HANDLE_TOP_LEFT: Qt.SizeFDiagCursor,
            HANDLE_TOP: Qt.SizeVerCursor,
            HANDLE_TOP_RIGHT: Qt.SizeBDiagCursor,
            HANDLE_RIGHT: Qt.SizeHorCursor,
            HANDLE_BOTTOM_RIGHT: Qt.SizeFDiagCursor,
            HANDLE_BOTTOM: Qt.SizeVerCursor,
            HANDLE_BOTTOM_LEFT: Qt.SizeBDiagCursor,
            HANDLE_LEFT: Qt.SizeHorCursor,
        }
        return cursors.get(handle, Qt.ArrowCursor)

    def _compute_insert_index(self, widget, mouse_pos):
        """Compute the target insertion index based on mouse position."""
        parent = widget.parent
        if parent is None or not parent.children:
            return -1, None

        siblings = parent.children
        is_horizontal = _parent_is_horizontal(widget)
        best_index = len(siblings)
        best_rect = None

        for i, sibling in enumerate(siblings):
            if is_horizontal:
                center_x = sibling.display_x + sibling.width // 2
                if mouse_pos.x() < center_x:
                    best_index = i
                    best_rect = QRect(sibling.display_x - 2, sibling.display_y, 4, sibling.height)
                    break
            else:
                center_y = sibling.display_y + sibling.height // 2
                if mouse_pos.y() < center_y:
                    best_index = i
                    best_rect = QRect(sibling.display_x, sibling.display_y - 2, sibling.width, 4)
                    break

        if best_rect is None and siblings:
            last = siblings[-1]
            if is_horizontal:
                best_rect = QRect(last.display_x + last.width - 2, last.display_y, 4, last.height)
            else:
                best_rect = QRect(last.display_x, last.display_y + last.height - 2, last.width, 4)

        return best_index, best_rect

    def _find_snap_guides(self, widget, new_x, new_y, new_w, new_h):
        """Find snap guide lines based on other widgets' edges."""
        guides = []
        snap_threshold = 5

        edges_x = [new_x, new_x + new_w // 2, new_x + new_w]  # left, center, right
        edges_y = [new_y, new_y + new_h // 2, new_y + new_h]  # top, center, bottom

        for w in self._widgets:
            if w == widget:
                continue
            other_x = [w.display_x, w.display_x + w.width // 2, w.display_x + w.width]
            other_y = [w.display_y, w.display_y + w.height // 2, w.display_y + w.height]

            for ex in edges_x:
                for ox in other_x:
                    if abs(ex - ox) < snap_threshold:
                        guides.append(('v', ox))  # vertical line
            for ey in edges_y:
                for oy in other_y:
                    if abs(ey - oy) < snap_threshold:
                        guides.append(('h', oy))  # horizontal line

        return guides

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)
        painter.setRenderHint(QPainter.TextAntialiasing)

        z = self._zoom
        bw, bh = self._base_width, self._base_height

        # ── Phase 1: Scaled space (geometry) ──────────────────────
        painter.scale(z, z)

        # Draw background mockup image (at 1:1 logical pixel scale)
        if self._bg_image is not None and self._bg_image_visible:
            painter.save()
            painter.setOpacity(self._bg_image_opacity)
            painter.drawPixmap(0, 0, self._bg_image)
            painter.restore()

        # Draw grid if enabled (grid size adapts to zoom level)
        eff_grid = self._effective_grid_size()
        if self._show_grid and eff_grid >= 1 and self._solid_background:
            painter.setPen(QPen(QColor(60, 60, 60, 100), 1.0 / z, Qt.DotLine))
            for x in range(0, bw, eff_grid):
                painter.drawLine(x, 0, x, bh)
            for y in range(0, bh, eff_grid):
                painter.drawLine(0, y, bw, y)

        # Draw all widget bounds (rects + fills only)
        for w in self._widgets:
            rect = QRect(w.display_x, w.display_y, w.width, w.height)

            if w == self._selected:
                pen = QPen(QColor(255, 100, 100, 200), 2.0 / z, Qt.SolidLine)
                fill = QColor(255, 120, 100, 40)
            elif w in self._multi_selected:
                pen = QPen(QColor(255, 180, 80, 200), 2.0 / z, Qt.SolidLine)
                fill = QColor(255, 180, 80, 40)
            elif w == self._hovered:
                pen = QPen(QColor(100, 200, 255, 180), 1.5 / z, Qt.DashLine)
                fill = QColor(100, 200, 255, 30)
            else:
                # Normal widgets also get a visible light fill
                if self._solid_background:
                    pen = QPen(QColor(140, 140, 140, 180), 1.0 / z, Qt.DotLine)
                    fill = QColor(160, 180, 210, 28)
                else:
                    pen = QPen(QColor(100, 100, 100, 100), 1.0 / z, Qt.DotLine)
                    fill = QColor(150, 170, 200, 20)

            painter.setPen(pen)
            painter.setBrush(fill)
            painter.drawRect(rect)

        # Draw snap guide lines (in logical space)
        if self._snap_guides:
            painter.setPen(QPen(QColor(255, 100, 100, 180), 1.0 / z, Qt.DashLine))
            for orientation, pos in self._snap_guides:
                if orientation == 'v':
                    painter.drawLine(pos, 0, pos, bh)
                else:
                    painter.drawLine(0, pos, bw, pos)

        # Draw insertion indicator line for reorder mode
        if self._reorder_mode and self._insert_line_rect is not None:
            painter.setPen(Qt.NoPen)
            painter.setBrush(QBrush(QColor(80, 160, 255, 200)))
            painter.drawRect(self._insert_line_rect)

        # Draw rubber-band selection rectangle
        if self._rubber_band and not self._rubber_rect.isNull():
            painter.setPen(QPen(QColor(100, 180, 255, 200), 1.0 / z, Qt.DashLine))
            painter.setBrush(QBrush(QColor(100, 180, 255, 40)))
            painter.drawRect(self._rubber_rect)

        # ── Phase 2: Screen space (text, handles, tooltips) ──────
        painter.resetTransform()

        # Helper: logical → screen coordinate conversion
        def _s(v):
            return int(v * z)

        # Draw widget labels in screen space (constant font size)
        label_font = QFont("Segoe UI", 7)
        painter.setFont(label_font)
        fm = painter.fontMetrics()
        lh = fm.height() + 2

        for w in self._widgets:
            sx, sy = _s(w.display_x), _s(w.display_y)
            sw, sh = _s(w.width), _s(w.height)
            label_text = f"{w.widget_type} : {w.name}"

            if self._solid_background:
                # Skip label if widget is too small on screen to fit it
                if sh < lh:
                    continue
                label_rect = QRect(sx, sy, sw, lh)
                painter.fillRect(label_rect, QColor(30, 30, 30, 180))
                painter.setPen(QColor(220, 220, 220))
                # Clip text within the widget width
                text_rect = QRect(sx + 3, sy, max(sw - 6, 0), lh)
                painter.drawText(text_rect, Qt.AlignLeft | Qt.AlignVCenter, label_text)
            elif w == self._selected or w == self._hovered:
                lw = max(sw, fm.horizontalAdvance(label_text) + 8)
                label_rect = QRect(sx, sy - lh, lw, lh)
                painter.fillRect(label_rect, QColor(50, 50, 50, 200))
                painter.setPen(QColor(255, 255, 255, 240))
                painter.drawText(label_rect, Qt.AlignCenter, label_text)

        # Draw resize handles in screen space (constant pixel size)
        if self._selected and not _parent_has_layout(self._selected):
            sel = self._selected
            sx, sy = _s(sel.display_x), _s(sel.display_y)
            sw, sh = _s(sel.width), _s(sel.height)
            hs = HANDLE_SIZE
            hh = hs // 2
            screen_handles = [
                QRect(sx - hh, sy - hh, hs, hs),                     # top-left
                QRect(sx + sw // 2 - hh, sy - hh, hs, hs),           # top
                QRect(sx + sw - hh, sy - hh, hs, hs),                # top-right
                QRect(sx + sw - hh, sy + sh // 2 - hh, hs, hs),      # right
                QRect(sx + sw - hh, sy + sh - hh, hs, hs),           # bottom-right
                QRect(sx + sw // 2 - hh, sy + sh - hh, hs, hs),      # bottom
                QRect(sx - hh, sy + sh - hh, hs, hs),                # bottom-left
                QRect(sx - hh, sy + sh // 2 - hh, hs, hs),           # left
            ]
            painter.setPen(QPen(QColor(255, 255, 255), 1))
            painter.setBrush(QBrush(QColor(100, 150, 255)))
            for r in screen_handles:
                painter.drawRect(r)

        # Draw coordinate tooltip in screen space while dragging/resizing
        if self._show_coords and self._selected:
            w = self._selected
            coord_text = f"({w.x}, {w.y})  {w.width}\u00d7{w.height}"
            painter.setFont(QFont("Consolas", 7))
            fm2 = painter.fontMetrics()
            tw = fm2.horizontalAdvance(coord_text) + 8
            th = fm2.height() + 4

            tip_sx = _s(w.display_x)
            tip_sy = _s(w.display_y) - th - 4
            if tip_sy < 0:
                tip_sy = _s(w.display_y + w.height) + 4

            tip_rect = QRect(tip_sx, tip_sy, tw, th)
            # Dark background with blue accent border for better visibility
            painter.fillRect(tip_rect, QColor(30, 30, 35, 240))
            painter.setPen(QPen(QColor(0, 120, 212), 1))
            painter.drawRect(tip_rect)
            painter.setPen(QColor(255, 255, 255))
            painter.drawText(tip_rect, Qt.AlignCenter, coord_text)


    def mousePressEvent(self, event):
        if event.button() != Qt.LeftButton:
            return

        pos = self._to_logical(event.pos())
        ctrl = bool(event.modifiers() & Qt.ControlModifier)

        # Check for resize handle first
        handle = self._handle_at(pos)
        if handle != HANDLE_NONE and self._selected and not _parent_has_layout(self._selected):
            self._resizing = True
            self._resize_handle = handle
            self._resize_start_rect = QRect(
                self._selected.x, self._selected.y,
                self._selected.width, self._selected.height
            )
            self._resize_start_pos = pos
            self._show_coords = True
            self.drag_started.emit()
            self.update()
            return

        # Check for widget selection/drag
        w = self._widget_at(pos)
        if w:
            if ctrl:
                # Ctrl+Click: toggle multi-select
                if w in self._multi_selected:
                    self._multi_selected.discard(w)
                    if self._selected == w:
                        self._selected = next(iter(self._multi_selected), None)
                else:
                    self._multi_selected.add(w)
                    if self._selected and self._selected != w:
                        self._multi_selected.add(self._selected)
                    self._selected = w
                self.widget_selected.emit(w)
                self.update()
                return

            # Normal click: single select + start drag
            self._selected = w
            self._multi_selected.clear()
            self._dragging = True
            self._drag_offset = pos - QPoint(w.display_x, w.display_y)
            self._reorder_mode = _parent_has_layout(w)
            self._insert_index = -1
            self._insert_line_rect = None
            self._show_coords = not self._reorder_mode
            if self._reorder_mode:
                self.setCursor(Qt.ClosedHandCursor)
            else:
                self.setCursor(Qt.SizeAllCursor)
            self.drag_started.emit()
            self.widget_selected.emit(w)
            self.update()
        else:
            if not ctrl:
                # Click on empty space: start rubber-band selection
                self._selected = None
                self._multi_selected.clear()
                self._rubber_band = True
                self._rubber_start = pos
                self._rubber_rect = QRect()
                self.widget_selected.emit(None)
                self.update()

    def mouseMoveEvent(self, event):
        pos = self._to_logical(event.pos())

        # Emit mouse position for status bar (always, even when not dragging)
        widget_under = self._widget_at(pos)
        self.mouse_position_changed.emit(pos.x(), pos.y(), widget_under)

        if self._rubber_band:
            self._rubber_rect = QRect(self._rubber_start, pos).normalized()
            self.update()
            return

        if self._resizing and self._selected:
            self._do_resize(pos)
            return

        if self._dragging and self._selected:
            if self._reorder_mode:
                idx, rect = self._compute_insert_index(self._selected, pos)
                self._insert_index = idx
                self._insert_line_rect = rect
                self.update()
            else:
                self._do_free_drag(pos)
            return

        # Hover detection and cursor update
        handle = self._handle_at(pos)
        if handle != HANDLE_NONE:
            self.setCursor(self._cursor_for_handle(handle))
        else:
            w = self._widget_at(pos)
            if w is not None:
                # Show move cursor when hovering over a draggable widget
                if _parent_has_layout(w):
                    # In layout container: show grab cursor for reorder
                    self.setCursor(Qt.OpenHandCursor)
                else:
                    # Free positioning: show move cursor
                    self.setCursor(Qt.SizeAllCursor)
            else:
                self.setCursor(Qt.ArrowCursor)
            if w != self._hovered:
                self._hovered = w
                self.update()

    def _do_free_drag(self, pos):
        """Handle free drag movement with grid snap."""
        new_pos = pos - self._drag_offset
        new_x = new_pos.x()
        new_y = new_pos.y()

        # Apply grid snap (grid size adapts to zoom level)
        eff_grid = self._effective_grid_size()
        if eff_grid >= 1:
            new_x = _snap_to_grid(new_x, eff_grid)
            new_y = _snap_to_grid(new_y, eff_grid)

        # Clamp to logical bounds
        new_x = max(0, min(new_x, self._base_width - self._selected.width))
        new_y = max(0, min(new_y, self._base_height - self._selected.height))

        # Find snap guides
        self._snap_guides = self._find_snap_guides(
            self._selected, new_x, new_y,
            self._selected.width, self._selected.height
        )

        if new_x != self._selected.x or new_y != self._selected.y:
            self._selected.x = new_x
            self._selected.y = new_y
            self._selected.display_x = new_x
            self._selected.display_y = new_y
            self.widget_moved.emit(self._selected, new_x, new_y)
            self.update()

    def _do_resize(self, pos):
        """Handle resize with grid snap."""
        dx = pos.x() - self._resize_start_pos.x()
        dy = pos.y() - self._resize_start_pos.y()
        r = self._resize_start_rect
        h = self._resize_handle

        new_x, new_y = r.x(), r.y()
        new_w, new_h = r.width(), r.height()
        min_size = 10  # Minimum widget size

        # Calculate new dimensions based on handle
        if h in (HANDLE_LEFT, HANDLE_TOP_LEFT, HANDLE_BOTTOM_LEFT):
            new_x = r.x() + dx
            new_w = r.width() - dx
        if h in (HANDLE_RIGHT, HANDLE_TOP_RIGHT, HANDLE_BOTTOM_RIGHT):
            new_w = r.width() + dx
        if h in (HANDLE_TOP, HANDLE_TOP_LEFT, HANDLE_TOP_RIGHT):
            new_y = r.y() + dy
            new_h = r.height() - dy
        if h in (HANDLE_BOTTOM, HANDLE_BOTTOM_LEFT, HANDLE_BOTTOM_RIGHT):
            new_h = r.height() + dy

        # Apply grid snap (grid size adapts to zoom level)
        eff_grid = self._effective_grid_size()
        if eff_grid >= 1:
            new_x = _snap_to_grid(new_x, eff_grid)
            new_y = _snap_to_grid(new_y, eff_grid)
            new_w = _snap_to_grid(new_w, eff_grid)
            new_h = _snap_to_grid(new_h, eff_grid)

        # Enforce minimum size
        new_w = max(min_size, new_w)
        new_h = max(min_size, new_h)

        # Clamp to logical canvas bounds
        new_x = max(0, new_x)
        new_y = max(0, new_y)
        if new_x + new_w > self._base_width:
            new_w = self._base_width - new_x
        if new_y + new_h > self._base_height:
            new_h = self._base_height - new_y

        # Find snap guides
        self._snap_guides = self._find_snap_guides(self._selected, new_x, new_y, new_w, new_h)

        # Update widget
        changed = False
        if new_x != self._selected.x or new_y != self._selected.y:
            self._selected.x = new_x
            self._selected.y = new_y
            self._selected.display_x = new_x
            self._selected.display_y = new_y
            changed = True
        if new_w != self._selected.width or new_h != self._selected.height:
            self._selected.width = new_w
            self._selected.height = new_h
            changed = True

        if changed:
            self.widget_moved.emit(self._selected, new_x, new_y)
            self.widget_resized.emit(self._selected, new_w, new_h)
            self.update()

    def mouseReleaseEvent(self, event):
        if event.button() != Qt.LeftButton:
            return

        if self._rubber_band:
            # Complete rubber-band selection
            self._rubber_band = False
            rect = self._rubber_rect
            self._multi_selected.clear()
            for w in self._widgets:
                wr = QRect(w.display_x, w.display_y, w.width, w.height)
                if rect.intersects(wr):
                    self._multi_selected.add(w)
            if len(self._multi_selected) == 1:
                self._selected = next(iter(self._multi_selected))
                self._multi_selected.clear()
                self.widget_selected.emit(self._selected)
            elif self._multi_selected:
                self._selected = next(iter(self._multi_selected))
                self.widget_selected.emit(self._selected)
            self._rubber_rect = QRect()
            self.update()
            return

        if self._resizing:
            self._resizing = False
            self._resize_handle = HANDLE_NONE
            self._resize_start_rect = None
            self._resize_start_pos = None
            self._show_coords = False
            self._snap_guides = []
            self.setCursor(Qt.ArrowCursor)
            self.drag_finished.emit()
            self.update()
            return

        if self._dragging:
            if self._reorder_mode and self._selected:
                if self._insert_index >= 0 and self._selected.parent:
                    parent = self._selected.parent
                    old_index = parent.children.index(self._selected)
                    if old_index != self._insert_index and old_index != self._insert_index - 1:
                        parent.children.remove(self._selected)
                        new_idx = self._insert_index
                        if old_index < new_idx:
                            new_idx -= 1
                        parent.children.insert(new_idx, self._selected)
                        self.widget_reordered.emit(self._selected, new_idx)

            self._dragging = False
            self._reorder_mode = False
            self._insert_index = -1
            self._insert_line_rect = None
            self._show_coords = False
            self._snap_guides = []
            self.setCursor(Qt.ArrowCursor)
            self.drag_finished.emit()
            self.update()

    def leaveEvent(self, event):
        self._hovered = None
        self.setCursor(Qt.ArrowCursor)
        self.mouse_position_changed.emit(-1, -1, None)  # Clear position display
        self.update()

    def keyPressEvent(self, event):
        """Handle keyboard nudge for selected widget(s)."""
        targets = self._get_move_targets()
        if not targets:
            super().keyPressEvent(event)
            return

        key = event.key()
        if key not in (Qt.Key_Left, Qt.Key_Right, Qt.Key_Up, Qt.Key_Down):
            super().keyPressEvent(event)
            return

        step = 10 if event.modifiers() & Qt.ShiftModifier else 1
        dx, dy = 0, 0
        if key == Qt.Key_Left:
            dx = -step
        elif key == Qt.Key_Right:
            dx = step
        elif key == Qt.Key_Up:
            dy = -step
        elif key == Qt.Key_Down:
            dy = step

        for w in targets:
            if _parent_has_layout(w):
                continue
            new_x = max(0, min(w.x + dx, self._base_width - w.width))
            new_y = max(0, min(w.y + dy, self._base_height - w.height))
            if new_x != w.x or new_y != w.y:
                w.x = new_x
                w.y = new_y
                w.display_x = new_x
                w.display_y = new_y
                self.widget_moved.emit(w, new_x, new_y)
        self.update()
        event.accept()

    def _get_move_targets(self):
        """Get list of widgets to move (multi-select or single selected)."""
        if self._multi_selected:
            return list(self._multi_selected)
        if self._selected:
            return [self._selected]
        return []

    # ── Resource drag-drop onto canvas ─────────────────────────────

    def dragEnterEvent(self, event):
        from .resource_panel import EGUI_RESOURCE_MIME
        if event.mimeData().hasFormat(EGUI_RESOURCE_MIME):
            event.acceptProposedAction()
        else:
            event.ignore()

    def dragMoveEvent(self, event):
        from .resource_panel import EGUI_RESOURCE_MIME
        if event.mimeData().hasFormat(EGUI_RESOURCE_MIME):
            # Highlight widget under cursor
            pos = self._to_logical(event.pos())
            w = self._widget_at(pos)
            if w != self._hovered:
                self._hovered = w
                self.update()
            event.acceptProposedAction()
        else:
            event.ignore()

    def dropEvent(self, event):
        from .resource_panel import EGUI_RESOURCE_MIME
        if not event.mimeData().hasFormat(EGUI_RESOURCE_MIME):
            event.ignore()
            return
        try:
            raw = bytes(event.mimeData().data(EGUI_RESOURCE_MIME)).decode("utf-8")
            info = json.loads(raw)
        except Exception:
            event.ignore()
            return
        res_type = info.get("type", "")
        filename = info.get("filename", "")
        if not filename:
            event.ignore()
            return

        # Find target widget under drop position
        pos = self._to_logical(event.pos())
        target = self._widget_at(pos)
        if target is None:
            event.ignore()
            return

        # Auto-assign based on resource type and widget type
        assigned = False
        if res_type == "image" and "image_file" in target.properties:
            target.properties["image_file"] = filename
            assigned = True
        elif res_type == "font" and "font_file" in target.properties:
            target.properties["font_file"] = filename
            assigned = True

        if assigned:
            self._selected = target
            self.widget_selected.emit(target)
            self.resource_dropped.emit(target, res_type, filename)
            self._hovered = None
            self.update()
            event.acceptProposedAction()
        else:
            event.ignore()


class PreviewPanel(QWidget):
    """Panel that embeds the exe window with overlay for widget manipulation.

    Supports multiple display modes:
    - vertical:   exe on top, overlay below (splitter adjustable)
    - horizontal: exe on left, overlay on right (splitter adjustable)
    - hidden:     overlay only, exe preview hidden
    """

    widget_moved = pyqtSignal(object, int, int)
    widget_resized = pyqtSignal(object, int, int)
    widget_selected = pyqtSignal(object)
    widget_reordered = pyqtSignal(object, int)
    resource_dropped = pyqtSignal(object, str, str)  # widget, res_type, filename
    drag_started = pyqtSignal()
    drag_finished = pyqtSignal()
    runtime_failed = pyqtSignal(str)

    def __init__(self, screen_width=240, screen_height=320, parent=None):
        super().__init__(parent)
        self.screen_width = screen_width
        self.screen_height = screen_height
        self._exe_hwnd = None
        self._embedded = False
        self._mode = MODE_HORIZONTAL
        self._splitter = None
        self._flipped = True  # wireframe first (left in horizontal, top in vertical)
        self._compiler = None  # set by start_rendering()
        self._render_timer = QTimer(self)
        self._render_timer.timeout.connect(self._refresh_frame)
        self._python_preview_active = False
        self._frame_failure_count = 0
        self._runtime_error_emitted = False

        self._init_ui()

    def _init_ui(self):
        self._main_layout = QVBoxLayout(self)
        self._main_layout.setContentsMargins(4, 4, 4, 4)
        self._main_layout.setSpacing(4)

        # Status label
        self.status_label = QLabel("Preview - waiting for exe...")
        self.status_label.setAlignment(Qt.AlignCenter)
        self._main_layout.addWidget(self.status_label)

        # Container for the preview + overlay arrangement
        self._content = QWidget()
        self._main_layout.addWidget(self._content, 1)

        # Create the preview frame (always exists)
        self.preview_frame = QFrame()
        self.preview_frame.setFrameStyle(QFrame.Box | QFrame.Sunken)
        self.preview_frame.setFixedSize(self.screen_width + 4, self.screen_height + 4)
        self.preview_frame.setStyleSheet("background-color: #333;")

        # Label inside preview_frame for headless frame rendering
        self._preview_label = QLabel(self.preview_frame)
        self._preview_label.setGeometry(2, 2, self.screen_width, self.screen_height)
        self._preview_label.setMouseTracking(True)
        self._preview_label.installEventFilter(self)
        self._mouse_pressed = False

        # Create the overlay (always exists) — zoomable
        self.overlay = WidgetOverlay()
        self.overlay.set_base_size(self.screen_width, self.screen_height)
        self.overlay.widget_moved.connect(self.widget_moved.emit)
        self.overlay.widget_resized.connect(self.widget_resized.emit)
        self.overlay.widget_selected.connect(self.widget_selected.emit)
        self.overlay.widget_reordered.connect(self.widget_reordered.emit)
        self.overlay.resource_dropped.connect(self.resource_dropped.emit)
        self.overlay.drag_started.connect(self.drag_started.emit)
        self.overlay.drag_finished.connect(self.drag_finished.emit)

        # Wrap overlay in a scroll area so it can be zoomed bigger than the panel
        self._overlay_scroll = QScrollArea()
        self._overlay_scroll.setWidgetResizable(False)
        self._overlay_scroll.setAlignment(Qt.AlignCenter)
        self._overlay_scroll.setWidget(self.overlay)

        # Status bar at bottom (coordinates + zoom controls)
        self._status_bar = QWidget()
        sbl = QHBoxLayout(self._status_bar)
        sbl.setContentsMargins(8, 2, 8, 2)
        sbl.setSpacing(8)

        # Status label on the left (shows coordinates)
        self._status_label = QLabel("")
        self._status_label.setStyleSheet("color:#ccc; font-size:13px; font-family:Consolas,monospace;")
        self._status_label.setMinimumWidth(280)
        sbl.addWidget(self._status_label)

        sbl.addStretch()

        # Zoom controls on the right
        _zbtn_style = (
            "QPushButton { background:#3c3c3c; color:#ccc; border:1px solid #555;"
            "  border-radius:3px; font-size:16px; font-weight:bold; }"
            "QPushButton:hover { background:#505050; }"
            "QPushButton:pressed { background:#606060; }"
        )

        self._btn_zoom_out = QPushButton("\u2212")  # minus sign
        self._btn_zoom_out.setFixedSize(28, 28)
        self._btn_zoom_out.setStyleSheet(_zbtn_style)
        self._btn_zoom_out.setToolTip("Zoom Out (Ctrl+-)")
        self._btn_zoom_out.clicked.connect(self._on_zoom_out)

        self._zoom_label = QLabel("100% (4px)")
        self._zoom_label.setFixedWidth(90)
        self._zoom_label.setAlignment(Qt.AlignCenter)
        self._zoom_label.setStyleSheet("color:#ccc; font-size:13px;")

        self._btn_zoom_in = QPushButton("+")
        self._btn_zoom_in.setFixedSize(28, 28)
        self._btn_zoom_in.setStyleSheet(_zbtn_style)
        self._btn_zoom_in.setToolTip("Zoom In (Ctrl+=)")
        self._btn_zoom_in.clicked.connect(self._on_zoom_in)

        sbl.addWidget(self._btn_zoom_out)
        sbl.addWidget(self._zoom_label)
        sbl.addWidget(self._btn_zoom_in)

        self._main_layout.addWidget(self._status_bar)

        # Connect signals
        self.overlay.zoom_changed.connect(self._update_zoom_label)
        self.overlay.mouse_position_changed.connect(self._update_status_label)

        # Apply initial layout mode
        self._apply_mode()

    def _clear_content_layout(self):
        """Remove all widgets from the content layout without deleting them."""
        # Detach splitter children first so they aren't deleted
        if hasattr(self, '_splitter') and self._splitter is not None:
            # Remove widgets from splitter by reparenting
            for i in range(self._splitter.count() - 1, -1, -1):
                w = self._splitter.widget(i)
                if w:
                    w.setParent(None)
            self._splitter.setParent(None)
            self._splitter = None

        old_layout = self._content.layout()
        if old_layout is not None:
            # Reparent children so they aren't deleted with the layout
            while old_layout.count():
                item = old_layout.takeAt(0)
                w = item.widget()
                if w:
                    w.setParent(None)
            # Delete the old layout by assigning it to a temporary widget
            QWidget().setLayout(old_layout)

    def _apply_mode(self):
        """Rebuild the content layout based on current mode."""
        self._clear_content_layout()

        if self._mode == MODE_VERTICAL:
            layout = QVBoxLayout(self._content)
            layout.setContentsMargins(0, 0, 0, 0)
            layout.setSpacing(0)
            # Splitter for adjustable top/bottom split
            self._splitter = QSplitter(Qt.Vertical)
            self.overlay.set_solid_background(True)
            self._overlay_scroll.setParent(None)
            if self._flipped:
                self._splitter.addWidget(self._overlay_scroll)
                self._splitter.addWidget(self.preview_frame)
                self._splitter.setStretchFactor(0, 1)
                self._splitter.setStretchFactor(1, 0)
            else:
                self._splitter.addWidget(self.preview_frame)
                self._splitter.addWidget(self._overlay_scroll)
                self._splitter.setStretchFactor(0, 0)
                self._splitter.setStretchFactor(1, 1)
            layout.addWidget(self._splitter, 1)
            self.preview_frame.show()
            self._overlay_scroll.show()
            self._status_bar.show()

        elif self._mode == MODE_HORIZONTAL:
            layout = QVBoxLayout(self._content)
            layout.setContentsMargins(0, 0, 0, 0)
            layout.setSpacing(0)
            # Splitter for adjustable left/right split
            self._splitter = QSplitter(Qt.Horizontal)
            self.overlay.set_solid_background(True)
            self._overlay_scroll.setParent(None)
            if self._flipped:
                self._splitter.addWidget(self._overlay_scroll)
                self._splitter.addWidget(self.preview_frame)
                self._splitter.setStretchFactor(0, 1)
                self._splitter.setStretchFactor(1, 0)
            else:
                self._splitter.addWidget(self.preview_frame)
                self._splitter.addWidget(self._overlay_scroll)
                self._splitter.setStretchFactor(0, 0)
                self._splitter.setStretchFactor(1, 1)
            layout.addWidget(self._splitter, 1)
            self.preview_frame.show()
            self._overlay_scroll.show()
            self._status_bar.show()

        elif self._mode == MODE_HIDDEN:
            layout = QVBoxLayout(self._content)
            layout.setContentsMargins(0, 0, 0, 0)
            layout.setSpacing(0)
            # Overlay mode: hide exe preview, only show overlay
            self.overlay.set_solid_background(True)
            self._overlay_scroll.setParent(self._content)
            layout.addWidget(self._overlay_scroll, 1)
            self.preview_frame.setParent(self._content)
            self.preview_frame.hide()
            self._overlay_scroll.show()
            self._status_bar.show()

    def update_screen_size(self, width, height):
        """Update the logical screen size and resize all preview components."""
        self.screen_width = width
        self.screen_height = height
        self.preview_frame.setFixedSize(width + 4, height + 4)
        self._preview_label.setGeometry(2, 2, width, height)
        self.overlay.set_base_size(width, height)

    def set_overlay_mode(self, mode):
        """Switch overlay display mode."""
        if mode == self._mode:
            return
        self._mode = mode
        self._apply_mode()

    def flip_layout(self):
        """Swap the position of exe preview and overlay in splitter modes."""
        if self._mode == MODE_HIDDEN:
            return
        self._flipped = not self._flipped
        self._apply_mode()

    @property
    def overlay_mode(self):
        return self._mode

    # ── Zoom helpers (delegate to overlay) ─────────────────────────

    def _on_zoom_in(self):
        self.overlay.zoom_in()

    def _on_zoom_out(self):
        self.overlay.zoom_out()

    def _update_zoom_label(self, factor=None):
        pct = int(self.overlay._zoom * 100)
        grid = self.overlay._effective_grid_size()
        self._zoom_label.setText(f"{pct}% ({grid}px)")

    def _update_status_label(self, x, y, widget):
        """Update status bar with mouse position and widget info."""
        if x < 0 or y < 0:
            self._status_label.setText("")
            return

        if widget is not None:
            # Show widget info: position, size, and name
            text = f"({x}, {y})  |  {widget.widget_type}: {widget.name}  [{widget.x}, {widget.y}, {widget.width}\u00d7{widget.height}]"
        else:
            # Just show mouse position
            text = f"({x}, {y})"

        self._status_label.setText(text)

    def set_widgets(self, widgets):
        """Update the widget list for overlay display."""
        self.overlay.set_widgets(widgets)

    def set_selected(self, widget):
        """Set the selected widget for highlighting."""
        self.overlay.set_selected(widget)

    # ── Background mockup image (delegate to overlay) ─────────

    def set_background_image(self, pixmap):
        """Set background mockup image on overlay."""
        self.overlay.set_background_image(pixmap)

    def set_background_image_visible(self, visible):
        """Toggle background image visibility."""
        self.overlay.set_background_image_visible(visible)

    def set_background_image_opacity(self, opacity):
        """Set background image opacity (0.0 to 1.0)."""
        self.overlay.set_background_image_opacity(opacity)

    def clear_background_image(self):
        """Remove the background image."""
        self.overlay.clear_background_image()

    def embed_window(self, hwnd):
        """Legacy method - no longer needed with headless rendering."""
        return True

    def release_window(self):
        """Legacy method - no longer needed with headless rendering."""
        self._exe_hwnd = None
        self._embedded = False

    @property
    def is_embedded(self):
        return self._embedded

    # ── Headless frame rendering ────────────────────────────

    def start_rendering(self, compiler):
        """Start periodic frame refresh from headless bridge."""
        self.clear_python_preview_mode()
        self._compiler = compiler
        self._frame_failure_count = 0
        self._runtime_error_emitted = False
        self._render_timer.start(33)  # ~30fps
        self._embedded = True
        self.status_label.setText("Preview - headless rendering")

    def stop_rendering(self):
        """Stop frame refresh."""
        self._render_timer.stop()
        self._compiler = None
        self._embedded = False
        self._frame_failure_count = 0
        self._runtime_error_emitted = False

    def _set_preview_pixmap(self, pixmap):
        self._preview_label.setPixmap(pixmap)

    def show_python_preview(self, page, reason=""):
        """Render the current page with the Python fallback renderer."""
        self.stop_rendering()
        self._python_preview_active = True

        if page is None:
            self._preview_label.clear()
            self.status_label.setText("Preview - Python fallback")
            return

        image = render_page(page, self.screen_width, self.screen_height).convert("RGBA")
        raw = image.tobytes("raw", "RGBA")
        qimage = QImage(raw, image.width, image.height, image.width * 4, QImage.Format_RGBA8888).copy()
        self._set_preview_pixmap(QPixmap.fromImage(qimage))
        if reason:
            self.status_label.setText(f"Preview - Python fallback ({reason})")
        else:
            self.status_label.setText("Preview - Python fallback")

    def clear_python_preview_mode(self):
        """Leave Python fallback mode without clearing the current frame."""
        self._python_preview_active = False

    def is_python_preview_active(self):
        """Return True when the panel shows Python-rendered preview."""
        return self._python_preview_active

    def _refresh_frame(self):
        """Fetch frame from bridge and display as QPixmap."""
        if self._compiler is None:
            return
        frame_data = self._compiler.get_frame()
        if not frame_data:
            self._frame_failure_count += 1
            if self._frame_failure_count >= 3 and not self._runtime_error_emitted:
                self._render_timer.stop()
                self._embedded = False
                self._runtime_error_emitted = True
                reason = ""
                if self._compiler is not None:
                    reason = self._compiler.get_last_runtime_error()
                self.runtime_failed.emit(reason or "Headless preview stopped responding")
            return

        expected = self.screen_width * self.screen_height * 3
        if len(frame_data) != expected:
            self._frame_failure_count += 1
            if self._frame_failure_count >= 3 and not self._runtime_error_emitted:
                self._render_timer.stop()
                self._embedded = False
                self._runtime_error_emitted = True
                self.runtime_failed.emit(
                    f"Headless preview returned invalid frame size: {len(frame_data)} != {expected}"
                )
            return

        self._frame_failure_count = 0
        self._runtime_error_emitted = False
        img = QImage(
            frame_data,
            self.screen_width,
            self.screen_height,
            self.screen_width * 3,
            QImage.Format_RGB888,
        )
        self._set_preview_pixmap(QPixmap.fromImage(img))

    def eventFilter(self, obj, event):
        """Capture mouse events on _preview_label and forward to bridge."""
        if obj is self._preview_label and self._embedded:
            etype = event.type()
            if etype == QEvent.MouseButtonPress:
                pos = event.pos()
                self._mouse_pressed = True
                self.preview_mouse_press(pos.x(), pos.y())
                return True
            elif etype == QEvent.MouseButtonRelease:
                pos = event.pos()
                self._mouse_pressed = False
                self.preview_mouse_release(pos.x(), pos.y())
                return True
            elif etype == QEvent.MouseMove and self._mouse_pressed:
                pos = event.pos()
                self.preview_mouse_move(pos.x(), pos.y())
                return True
        return super().eventFilter(obj, event)

    def preview_mouse_press(self, x, y):
        """Forward mouse press to bridge as touch down."""
        from ..engine.designer_bridge import TOUCH_DOWN
        if self._compiler:
            self._compiler.inject_touch(TOUCH_DOWN, x, y)

    def preview_mouse_release(self, x, y):
        """Forward mouse release to bridge as touch up."""
        from ..engine.designer_bridge import TOUCH_UP
        if self._compiler:
            self._compiler.inject_touch(TOUCH_UP, x, y)

    def preview_mouse_move(self, x, y):
        """Forward mouse move to bridge as touch move."""
        from ..engine.designer_bridge import TOUCH_MOVE
        if self._compiler:
            self._compiler.inject_touch(TOUCH_MOVE, x, y)

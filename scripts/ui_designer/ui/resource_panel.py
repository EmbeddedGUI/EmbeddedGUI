"""Resource panel -- image/font/text catalog browser for EmbeddedGUI Designer.

Independent QWidget designed to be placed inside a QDockWidget.
Manages the designer-side ``.eguiproject/resources/`` directory and
project-level ``resources.xml`` catalog.

Designer resource directory layout:
    .eguiproject/
        resources/
            images/
                star.png
            test.ttf
            supported_text.txt

This panel is a **catalog browser** only -- no per-resource config editors.
Resource configuration (format, alpha, pixelsize, etc.) is managed at the
widget level in the property panel.
"""

import os
import re
import json
import shutil

from PyQt5.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QLabel, QListWidget,
    QListWidgetItem, QFileDialog, QMessageBox,
    QDialog, QDialogButtonBox, QMenu, QApplication,
    QSplitter, QSizePolicy, QAbstractItemView,
    QInputDialog, QTableWidget, QTableWidgetItem, QHeaderView,
    QComboBox, QCheckBox,
)
from PyQt5.QtCore import (
    Qt, QSize, pyqtSignal, QMimeData, QUrl, QTimer, QRect,
)
from PyQt5.QtGui import (
    QPixmap, QIcon, QPixmapCache, QFontDatabase, QFont,
    QPainter, QColor, QDrag, QPen,
)

from qfluentwidgets import (
    PushButton, TabWidget, CaptionLabel,
)

from ..model.resource_catalog import ResourceCatalog, IMAGE_EXTENSIONS, FONT_EXTENSIONS, TEXT_EXTENSIONS
from ..model.string_resource import StringResourceCatalog, DEFAULT_LOCALE


# -- Constants ----------------------------------------------------------

_IMAGE_EXTS = tuple(IMAGE_EXTENSIONS)
_FONT_EXTS = tuple(FONT_EXTENSIONS)
_TEXT_EXTS = tuple(TEXT_EXTENSIONS)

EGUI_RESOURCE_MIME = "application/x-egui-resource"

# Regex for validating English-only filenames
_VALID_FILENAME_RE = re.compile(r'^[A-Za-z0-9_\-]+\.[A-Za-z0-9]+$')


# -- Helpers ------------------------------------------------------------

def _file_size_str(path):
    """Human-readable file size."""
    try:
        size = os.path.getsize(path)
    except OSError:
        return "?"
    if size < 1024:
        return f"{size} B"
    elif size < 1024 * 1024:
        return f"{size / 1024:.1f} KB"
    else:
        return f"{size / (1024 * 1024):.2f} MB"


def _validate_english_filename(name):
    """Check filename is ASCII letters, digits, underscore, dash, one dot."""
    return bool(_VALID_FILENAME_RE.match(name))


# -- Lazy-loading image list --------------------------------------------

class _LazyImageList(QListWidget):
    """QListWidget that loads thumbnails lazily via QPixmapCache."""

    THUMB_SIZE = 48

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setViewMode(QListWidget.IconMode)
        self.setIconSize(QSize(self.THUMB_SIZE, self.THUMB_SIZE))
        self.setGridSize(QSize(self.THUMB_SIZE + 16, self.THUMB_SIZE + 28))
        self.setResizeMode(QListWidget.Adjust)
        self.setMovement(QListWidget.Static)
        self.setSelectionMode(QAbstractItemView.SingleSelection)
        self.setDragEnabled(True)

        self._load_timer = QTimer(self)
        self._load_timer.setSingleShot(True)
        self._load_timer.setInterval(30)
        self._load_timer.timeout.connect(self._load_visible)

    def showEvent(self, event):
        super().showEvent(event)
        self._schedule_load()

    def resizeEvent(self, event):
        super().resizeEvent(event)
        self._schedule_load()

    def scrollContentsBy(self, dx, dy):
        super().scrollContentsBy(dx, dy)
        self._schedule_load()

    def _schedule_load(self):
        if not self._load_timer.isActive():
            self._load_timer.start()

    def _load_visible(self):
        vr = self.viewport().rect()
        for i in range(self.count()):
            item = self.item(i)
            item_rect = self.visualItemRect(item)
            if not vr.intersects(item_rect):
                continue
            if item.data(Qt.UserRole + 10):
                continue
            path = item.data(Qt.UserRole)
            if not path:
                continue
            self._load_thumb(item, path)

    def _load_thumb(self, item, path):
        try:
            mtime = int(os.path.getmtime(path))
        except OSError:
            mtime = 0
        cache_key = f"egui_thumb:{path}:{mtime}"
        pm = QPixmapCache.find(cache_key)
        if pm is None or pm.isNull():
            pm = QPixmap(path)
            if pm.isNull():
                return
            pm = pm.scaled(self.THUMB_SIZE, self.THUMB_SIZE,
                           Qt.KeepAspectRatio, Qt.SmoothTransformation)
            QPixmapCache.insert(cache_key, pm)
        item.setIcon(QIcon(pm))
        item.setData(Qt.UserRole + 10, True)

    def startDrag(self, supportedActions):
        item = self.currentItem()
        if item is None:
            return
        filename = item.data(Qt.UserRole + 1)  # filename string
        if not filename:
            return
        path = item.data(Qt.UserRole) or ""

        mime = QMimeData()
        data = json.dumps({"type": "image", "filename": filename, "path": path})
        mime.setData(EGUI_RESOURCE_MIME, data.encode("utf-8"))
        mime.setText(filename)

        drag = QDrag(self)
        drag.setMimeData(mime)
        if not item.icon().isNull():
            drag.setPixmap(item.icon().pixmap(32, 32))
        drag.exec_(Qt.CopyAction)


# -- Drag-enabled font list ---------------------------------------------

class _DragResourceList(QListWidget):
    """QListWidget for simple resource types with drag-out support."""

    def __init__(self, resource_type, parent=None):
        super().__init__(parent)
        self._resource_type = resource_type
        self.setSelectionMode(QAbstractItemView.SingleSelection)
        self.setDragEnabled(True)

    def startDrag(self, supportedActions):
        item = self.currentItem()
        if item is None:
            return
        filename = item.data(Qt.UserRole + 1)  # filename string
        if not filename:
            return
        path = item.data(Qt.UserRole) or ""

        mime = QMimeData()
        data = json.dumps({"type": self._resource_type, "filename": filename, "path": path})
        mime.setData(EGUI_RESOURCE_MIME, data.encode("utf-8"))
        mime.setText(filename)

        drag = QDrag(self)
        drag.setMimeData(mime)
        drag.exec_(Qt.CopyAction)


# -- Preview widget ------------------------------------------------------

class _PreviewWidget(QWidget):
    """Bottom preview area -- image large preview or font samples."""

    _SAMPLE_TEXT = "AaBbCc 0123"
    _FONT_SIZES = [12, 16, 24, 32]

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setMinimumHeight(140)
        self._mode = None
        self._pixmap = None
        self._meta_lines = []
        self._font_family = None
        self._font_file = None
        self._text_lines = []

    def show_image(self, path):
        self._mode = "image"
        self._font_family = None
        pm = QPixmap(path)
        if pm.isNull():
            self._pixmap = None
            self.update()
            return

        orig_w, orig_h = pm.width(), pm.height()
        fname = os.path.basename(path)
        self._meta_lines = [
            fname,
            f"Size: {orig_w} \u00d7 {orig_h}",
            _file_size_str(path),
        ]

        self._pixmap = pm
        self.update()

    def show_font(self, path, font_family):
        self._mode = "font"
        self._pixmap = None
        self._font_family = font_family
        self._font_file = path
        self._text_lines = []
        self._meta_lines = [
            os.path.basename(path),
            font_family or "Unknown family",
            _file_size_str(path),
        ]
        self.update()

    def show_text(self, path):
        self._mode = "text"
        self._pixmap = None
        self._font_family = None
        self._font_file = None

        content = ""
        try:
            with open(path, "r", encoding="utf-8") as f:
                content = f.read()
        except UnicodeDecodeError:
            with open(path, "r", encoding="utf-8", errors="replace") as f:
                content = f.read()
        except OSError:
            self.clear_preview()
            return

        lines = content.splitlines()
        self._text_lines = lines[:6] or ["(Empty file)"]
        self._meta_lines = [
            os.path.basename(path),
            _file_size_str(path),
            f"Lines: {len(lines)}",
        ]
        self.update()

    def clear_preview(self):
        self._mode = None
        self._pixmap = None
        self._font_family = None
        self._meta_lines = []
        self._text_lines = []
        self.update()

    def paintEvent(self, event):
        if self._mode is None:
            return
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)
        painter.setRenderHint(QPainter.TextAntialiasing)
        w, h = self.width(), self.height()
        if self._mode == "image" and self._pixmap:
            self._paint_image(painter, w, h)
        elif self._mode == "font" and self._font_family:
            self._paint_font(painter, w, h)
        elif self._mode == "text":
            self._paint_text(painter, w, h)
        painter.end()

    def _paint_image(self, painter, w, h):
        pm = self._pixmap
        preview_w = max(w - 140, 60)
        preview_h = h - 8
        scaled = pm.scaled(preview_w, preview_h,
                           Qt.KeepAspectRatio, Qt.SmoothTransformation)
        x = 4
        y = (h - scaled.height()) // 2
        painter.drawPixmap(x, y, scaled)
        text_x = x + scaled.width() + 8
        painter.setPen(QColor(200, 200, 200))
        painter.setFont(QFont("Segoe UI", 9))
        fm = painter.fontMetrics()
        ty = 8
        for line in self._meta_lines:
            painter.drawText(text_x, ty + fm.ascent(), line)
            ty += fm.height() + 2

    def _paint_font(self, painter, w, h):
        painter.setPen(QColor(180, 180, 180))
        painter.setFont(QFont("Segoe UI", 8))
        fm = painter.fontMetrics()
        ty = 4
        for line in self._meta_lines:
            painter.drawText(8, ty + fm.ascent(), line)
            ty += fm.height() + 1
        ty += 4
        painter.setPen(QColor(220, 220, 220))
        for sz in self._FONT_SIZES:
            if ty > h - 4:
                break
            font = QFont(self._font_family, sz)
            painter.setFont(font)
            fm2 = painter.fontMetrics()
            painter.drawText(8, ty + fm2.ascent(), f"{sz}px: {self._SAMPLE_TEXT}")
            ty += fm2.height() + 2

    def _paint_text(self, painter, w, h):
        painter.setPen(QColor(180, 180, 180))
        painter.setFont(QFont("Segoe UI", 8))
        fm = painter.fontMetrics()
        ty = 4
        for line in self._meta_lines:
            painter.drawText(8, ty + fm.ascent(), line)
            ty += fm.height() + 1

        preview_rect = QRect(8, ty + 6, max(w - 16, 0), max(h - ty - 12, 0))
        painter.setPen(QColor(220, 220, 220))
        painter.setFont(QFont("Consolas", 9))
        painter.drawText(preview_rect, Qt.TextWordWrap | Qt.AlignTop | Qt.AlignLeft, "\n".join(self._text_lines))


class _MissingResourceReplaceDialog(QDialog):
    """Map missing project resources to external replacement files."""

    def __init__(self, missing_names, source_paths, parent=None):
        super().__init__(parent)
        self._missing_names = list(missing_names)
        self._source_paths = list(source_paths)
        self._combos = []
        self.setWindowTitle("Replace Missing Resources")
        self.resize(640, 360)

        layout = QVBoxLayout(self)

        caption = CaptionLabel(
            "Choose replacement files for missing resources. "
            "The selected file names become the new project resource names."
        )
        caption.setWordWrap(True)
        layout.addWidget(caption)

        self._table = QTableWidget(len(self._missing_names), 2, self)
        self._table.setHorizontalHeaderLabels(["Missing Resource", "Replacement File"])
        self._table.horizontalHeader().setStretchLastSection(True)
        self._table.horizontalHeader().setSectionResizeMode(0, QHeaderView.ResizeToContents)
        self._table.verticalHeader().setVisible(False)
        self._table.setSelectionMode(QAbstractItemView.NoSelection)
        self._table.setEditTriggers(QAbstractItemView.NoEditTriggers)
        layout.addWidget(self._table, 1)

        for row, missing_name in enumerate(self._missing_names):
            name_item = QTableWidgetItem(missing_name)
            name_item.setFlags(Qt.ItemIsEnabled)
            self._table.setItem(row, 0, name_item)

            combo = QComboBox(self._table)
            combo.addItem("(Skip)", "")
            for source_path in self._source_paths:
                combo.addItem(os.path.basename(source_path), source_path)

            exact_index = 0
            for index in range(1, combo.count()):
                source_path = combo.itemData(index)
                if os.path.basename(source_path).lower() == missing_name.lower():
                    exact_index = index
                    break
            if exact_index == 0 and len(self._missing_names) == 1 and len(self._source_paths) == 1:
                exact_index = 1
            combo.setCurrentIndex(exact_index)

            self._table.setCellWidget(row, 1, combo)
            self._combos.append((missing_name, combo))

        buttons = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel, parent=self)
        buttons.accepted.connect(self.accept)
        buttons.rejected.connect(self.reject)
        layout.addWidget(buttons)

    def selected_mapping(self):
        mapping = {}
        for missing_name, combo in self._combos:
            source_path = combo.currentData()
            if source_path:
                mapping[missing_name] = source_path
        return mapping

    def accept(self):
        selected_paths = []
        for _, combo in self._combos:
            source_path = combo.currentData()
            if not source_path:
                continue
            if source_path in selected_paths:
                QMessageBox.warning(
                    self,
                    "Duplicate Replacement",
                    "Each replacement file can only be used once in a batch replace.",
                )
                return
            selected_paths.append(source_path)

        if not selected_paths:
            QMessageBox.warning(
                self,
                "No Replacements Selected",
                "Choose at least one replacement file or cancel the dialog.",
            )
            return

        super().accept()


class _ReferenceImpactDialog(QDialog):
    """Confirm destructive actions and show impacted references."""

    NAVIGATE_RESULT = 2

    def __init__(self, parent, title, summary, usages, confirm_text):
        super().__init__(parent)
        self._usages = list(usages)
        self._selected_usage = ("", "")
        self.setWindowTitle(title)
        self.resize(560, 360)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(12, 12, 12, 12)
        layout.setSpacing(8)

        summary_label = QLabel(summary)
        summary_label.setWordWrap(True)
        layout.addWidget(summary_label)

        self._table = QTableWidget(len(usages), 3, self)
        self._table.setHorizontalHeaderLabels(["Page", "Widget", "Property"])
        self._table.horizontalHeader().setSectionResizeMode(0, QHeaderView.ResizeToContents)
        self._table.horizontalHeader().setSectionResizeMode(1, QHeaderView.Stretch)
        self._table.horizontalHeader().setSectionResizeMode(2, QHeaderView.ResizeToContents)
        self._table.verticalHeader().setVisible(False)
        self._table.setSelectionBehavior(QAbstractItemView.SelectRows)
        self._table.setSelectionMode(QAbstractItemView.SingleSelection)
        self._table.setEditTriggers(QAbstractItemView.NoEditTriggers)

        for row, entry in enumerate(usages):
            page_item = QTableWidgetItem(entry.page_name)
            widget_text = entry.widget_name
            if entry.widget_type:
                widget_text = f"{entry.widget_name} ({entry.widget_type})"
            widget_item = QTableWidgetItem(widget_text)
            prop_item = QTableWidgetItem(entry.property_name)
            self._table.setItem(row, 0, page_item)
            self._table.setItem(row, 1, widget_item)
            self._table.setItem(row, 2, prop_item)

        if usages:
            self._table.selectRow(0)
        self._table.itemDoubleClicked.connect(lambda *_args: self._open_selected_usage())
        layout.addWidget(self._table, 1)

        buttons = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel, parent=self)
        self._open_usage_button = buttons.addButton("Open Selected Usage", QDialogButtonBox.ActionRole)
        ok_button = buttons.button(QDialogButtonBox.Ok)
        if ok_button is not None:
            ok_button.setText(confirm_text or "Continue")
        cancel_button = buttons.button(QDialogButtonBox.Cancel)
        if cancel_button is not None:
            cancel_button.setText("Cancel")
        self._open_usage_button.clicked.connect(self._open_selected_usage)
        buttons.accepted.connect(self.accept)
        buttons.rejected.connect(self.reject)
        layout.addWidget(buttons)

    def selected_usage(self):
        return self._selected_usage

    def _open_selected_usage(self):
        row = self._table.currentRow()
        if row < 0 or row >= len(self._usages):
            return
        entry = self._usages[row]
        self._selected_usage = (entry.page_name, entry.widget_name)
        self.done(self.NAVIGATE_RESULT)


class _BatchReplaceImpactDialog(QDialog):
    """Preview grouped rename impacts before batch replacement."""

    NAVIGATE_RESULT = 2

    def __init__(self, parent, title, resource_type, impacts, total_rename_count, confirm_text, current_page_name=""):
        super().__init__(parent)
        self._all_impacts = list(impacts)
        self._visible_impacts = []
        self._selected_usage = ("", "")
        self._resource_type = resource_type or ""
        self._total_rename_count = total_rename_count
        self._current_page_name = current_page_name or ""
        self.setWindowTitle(title)
        self.resize(720, 460)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(12, 12, 12, 12)
        layout.setSpacing(8)

        self._summary_label = QLabel("")
        self._summary_label.setWordWrap(True)
        layout.addWidget(self._summary_label)

        if self._current_page_name:
            filter_row = QHBoxLayout()
            self._current_page_only = QCheckBox("Current Page Only")
            self._current_page_only.toggled.connect(self._refresh_impact_view)
            filter_row.addWidget(self._current_page_only)
            filter_row.addStretch()
            layout.addLayout(filter_row)
        else:
            self._current_page_only = None

        group_caption = QLabel("Rename Impact Summary")
        layout.addWidget(group_caption)

        self._impact_table = QTableWidget(0, 4, self)
        self._impact_table.setHorizontalHeaderLabels(["Missing Resource", "Replacement File", "Widgets", "Pages"])
        self._impact_table.horizontalHeader().setSectionResizeMode(0, QHeaderView.Stretch)
        self._impact_table.horizontalHeader().setSectionResizeMode(1, QHeaderView.Stretch)
        self._impact_table.horizontalHeader().setSectionResizeMode(2, QHeaderView.ResizeToContents)
        self._impact_table.horizontalHeader().setSectionResizeMode(3, QHeaderView.ResizeToContents)
        self._impact_table.verticalHeader().setVisible(False)
        self._impact_table.setSelectionBehavior(QAbstractItemView.SelectRows)
        self._impact_table.setSelectionMode(QAbstractItemView.SingleSelection)
        self._impact_table.setEditTriggers(QAbstractItemView.NoEditTriggers)
        self._impact_table.setMinimumHeight(160)
        self._impact_table.itemSelectionChanged.connect(self._refresh_usage_view)
        layout.addWidget(self._impact_table)

        usage_caption = QLabel("Affected Usages")
        layout.addWidget(usage_caption)

        self._usage_table = QTableWidget(0, 3, self)
        self._usage_table.setHorizontalHeaderLabels(["Page", "Widget", "Property"])
        self._usage_table.horizontalHeader().setSectionResizeMode(0, QHeaderView.ResizeToContents)
        self._usage_table.horizontalHeader().setSectionResizeMode(1, QHeaderView.Stretch)
        self._usage_table.horizontalHeader().setSectionResizeMode(2, QHeaderView.ResizeToContents)
        self._usage_table.verticalHeader().setVisible(False)
        self._usage_table.setSelectionBehavior(QAbstractItemView.SelectRows)
        self._usage_table.setSelectionMode(QAbstractItemView.SingleSelection)
        self._usage_table.setEditTriggers(QAbstractItemView.NoEditTriggers)
        self._usage_table.itemDoubleClicked.connect(lambda *_args: self._open_selected_usage())
        layout.addWidget(self._usage_table, 1)

        buttons = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel, parent=self)
        self._open_usage_button = buttons.addButton("Open Selected Usage", QDialogButtonBox.ActionRole)
        ok_button = buttons.button(QDialogButtonBox.Ok)
        if ok_button is not None:
            ok_button.setText(confirm_text or "Continue")
        cancel_button = buttons.button(QDialogButtonBox.Cancel)
        if cancel_button is not None:
            cancel_button.setText("Cancel")
        self._open_usage_button.clicked.connect(self._open_selected_usage)
        buttons.accepted.connect(self.accept)
        buttons.rejected.connect(self.reject)
        layout.addWidget(buttons)

        self._refresh_impact_view()

    def selected_usage(self):
        return self._selected_usage

    def _filter_to_current_page(self):
        return self._current_page_only is not None and self._current_page_only.isChecked() and bool(self._current_page_name)

    def _build_visible_impacts(self):
        impacts = []
        for entry in self._all_impacts:
            usages = list(entry["usages"])
            if self._filter_to_current_page():
                usages = [usage for usage in usages if usage.page_name == self._current_page_name]
            if not usages:
                continue

            impacts.append(
                {
                    "old_name": entry["old_name"],
                    "new_name": entry["new_name"],
                    "usages": usages,
                    "widget_count": len(usages),
                    "page_count": len({usage.page_name for usage in usages}),
                }
            )
        return impacts

    def _update_summary(self):
        total_impacted_rename_count = len(self._all_impacts)
        total_widget_count = sum(entry["widget_count"] for entry in self._all_impacts)
        total_page_count = len(
            {
                usage.page_name
                for entry in self._all_impacts
                for usage in entry["usages"]
            }
        )
        rename_noun = "resource" if self._total_rename_count == 1 else "resources"
        impacted_noun = "rename" if total_impacted_rename_count == 1 else "renames"
        widget_noun = "widget reference" if total_widget_count == 1 else "widget references"
        page_noun = "page" if total_page_count == 1 else "pages"

        summary_lines = [f"The selected replacements will rename {self._total_rename_count} missing {self._resource_type} {rename_noun}."]
        if total_impacted_rename_count != self._total_rename_count:
            summary_lines.append(f"{total_impacted_rename_count} {impacted_noun} affect widget references.")

        if not self._filter_to_current_page():
            summary_lines.append(
                f"Those renames affect {total_widget_count} {widget_noun} across {total_page_count} {page_noun}. "
                "Select a rename to inspect the impacted widgets before continuing."
            )
            self._summary_label.setText("\n".join(summary_lines))
            return

        visible_rename_count = len(self._visible_impacts)
        visible_widget_count = sum(entry["widget_count"] for entry in self._visible_impacts)
        current_page_widget_noun = "widget reference" if visible_widget_count == 1 else "widget references"
        current_page_rename_noun = "rename" if visible_rename_count == 1 else "renames"
        summary_lines.append(f"Showing impacts on the current page: {self._current_page_name}.")
        if not self._visible_impacts:
            summary_lines.append(
                f"No affected usages were found on the current page ({total_widget_count} total {widget_noun} across {total_page_count} {page_noun})."
            )
            summary_lines.append("Uncheck Current Page Only to inspect all project usages.")
            self._summary_label.setText("\n".join(summary_lines))
            return

        summary_lines.append(
            f"{visible_rename_count} {current_page_rename_noun} affect {visible_widget_count} {current_page_widget_noun} on the current page "
            f"({total_widget_count} total across {total_page_count} {page_noun})."
        )
        summary_lines.append("Select a rename to inspect the impacted widgets before continuing.")
        self._summary_label.setText("\n".join(summary_lines))

    def _refresh_impact_view(self):
        selected_key = None
        current_impact = self._current_impact()
        if current_impact is not None:
            selected_key = (current_impact["old_name"], current_impact["new_name"])

        self._visible_impacts = self._build_visible_impacts()
        self._impact_table.setRowCount(len(self._visible_impacts))
        target_row = 0
        matched_row = False
        for row, entry in enumerate(self._visible_impacts):
            self._impact_table.setItem(row, 0, QTableWidgetItem(entry["old_name"]))
            self._impact_table.setItem(row, 1, QTableWidgetItem(entry["new_name"]))
            self._impact_table.setItem(row, 2, QTableWidgetItem(str(entry["widget_count"])))
            self._impact_table.setItem(row, 3, QTableWidgetItem(str(entry["page_count"])))
            if selected_key == (entry["old_name"], entry["new_name"]):
                target_row = row
                matched_row = True

        self._update_summary()
        if self._visible_impacts:
            self._impact_table.selectRow(target_row if matched_row else 0)
        self._refresh_usage_view()

    def _current_impact(self):
        row = self._impact_table.currentRow()
        if row < 0 or row >= len(self._visible_impacts):
            return None
        return self._visible_impacts[row]

    def _refresh_usage_view(self):
        impact = self._current_impact()
        usages = [] if impact is None else impact["usages"]
        self._usage_table.setRowCount(len(usages))
        for row, entry in enumerate(usages):
            page_item = QTableWidgetItem(entry.page_name)
            widget_text = entry.widget_name
            if entry.widget_type:
                widget_text = f"{entry.widget_name} ({entry.widget_type})"
            widget_item = QTableWidgetItem(widget_text)
            prop_item = QTableWidgetItem(entry.property_name)
            self._usage_table.setItem(row, 0, page_item)
            self._usage_table.setItem(row, 1, widget_item)
            self._usage_table.setItem(row, 2, prop_item)
        has_usages = bool(usages)
        self._open_usage_button.setEnabled(has_usages)
        if has_usages:
            self._usage_table.selectRow(0)

    def _open_selected_usage(self):
        impact = self._current_impact()
        if impact is None:
            return
        usages = impact["usages"]
        row = self._usage_table.currentRow()
        if row < 0 or row >= len(usages):
            return
        entry = usages[row]
        self._selected_usage = (entry.page_name, entry.widget_name)
        self.done(self.NAVIGATE_RESULT)


# -- Main ResourcePanel --------------------------------------------------

class ResourcePanel(QWidget):
    """Catalog browser for project resources.

    Displays images, fonts, and text files from the resource catalog (resources.xml).
    No per-resource configuration -- config is at widget level in property panel.

    Signals:
        resource_selected(str, str): (resource_type, filename)
        resource_renamed(str, str, str): (resource_type, old_name, new_name)
        resource_deleted(str, str): (resource_type, filename)
        resource_imported():         files were imported, refresh needed
        feedback_message(str):       user-facing operation summary for status bars
        usage_activated(str, str):   (page_name, widget_name)
        string_key_renamed(str, str): (old_key, new_key)
        string_key_deleted(str, str): (key, replacement_text)
    """

    resource_selected = pyqtSignal(str, str)
    resource_renamed = pyqtSignal(str, str, str)
    resource_deleted = pyqtSignal(str, str)
    resource_imported = pyqtSignal()
    feedback_message = pyqtSignal(str)
    usage_activated = pyqtSignal(str, str)
    string_key_renamed = pyqtSignal(str, str)
    string_key_deleted = pyqtSignal(str, str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self._resource_dir = ""      # .eguiproject/resources/ base directory
        self._src_dir = ""           # same as _resource_dir (fonts/text root)
        self._images_dir = ""        # .eguiproject/resources/images/ subfolder
        self._last_external_import_dir = ""
        self._catalog = ResourceCatalog()
        self._string_catalog = StringResourceCatalog()
        self._font_id_cache = {}
        self._font_family_cache = {}
        self._string_table_updating = False  # guard against cellChanged feedback
        self._resource_usage_index = {}
        self._current_resource_type = ""
        self._current_resource_name = ""
        self._usage_page_name = ""
        self.setAcceptDrops(True)
        self._init_ui()

    # -- UI construction --

    def _init_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(4, 4, 4, 4)
        layout.setSpacing(4)

        splitter = QSplitter(Qt.Vertical)
        splitter.setChildrenCollapsible(False)
        layout.addWidget(splitter, 1)

        # -- Top: Tabs --
        top_widget = QWidget()
        top_layout = QVBoxLayout(top_widget)
        top_layout.setContentsMargins(0, 0, 0, 0)
        top_layout.setSpacing(2)

        self._tabs = TabWidget()
        self._tabs.currentChanged.connect(lambda _: self._refresh_usage_view())

        # Images tab
        img_tab = QWidget()
        img_tab_layout = QVBoxLayout(img_tab)
        img_tab_layout.setContentsMargins(2, 2, 2, 2)
        img_tab_layout.setSpacing(2)

        self._image_list = _LazyImageList()
        self._image_list.itemClicked.connect(self._on_image_clicked)
        self._image_list.itemDoubleClicked.connect(self._on_image_double_clicked)
        self._image_list.setContextMenuPolicy(Qt.CustomContextMenu)
        self._image_list.customContextMenuRequested.connect(
            lambda pos: self._show_context_menu(pos, "image")
        )
        img_tab_layout.addWidget(self._image_list, 1)

        img_btn_layout = QHBoxLayout()
        import_img_btn = PushButton("Import Image...")
        import_img_btn.clicked.connect(self._on_import_image)
        img_btn_layout.addWidget(import_img_btn)
        restore_img_btn = PushButton("Restore Missing...")
        restore_img_btn.setToolTip("Restore missing image files by matching selected filenames against missing catalog entries.")
        restore_img_btn.clicked.connect(lambda: self._restore_missing_resources("image"))
        img_btn_layout.addWidget(restore_img_btn)
        replace_img_btn = PushButton("Replace Missing...")
        replace_img_btn.setToolTip("Replace missing image resources with new files and rewrite widget references to the new filenames.")
        replace_img_btn.clicked.connect(lambda: self._replace_missing_resources("image"))
        img_btn_layout.addWidget(replace_img_btn)
        next_missing_img_btn = PushButton("Next Missing")
        next_missing_img_btn.setToolTip("Select the next missing image resource in this tab.")
        next_missing_img_btn.clicked.connect(lambda: self._focus_missing_resource("image"))
        img_btn_layout.addWidget(next_missing_img_btn)
        img_btn_layout.addStretch()
        img_tab_layout.addLayout(img_btn_layout)
        self._tabs.addTab(img_tab, "Images")

        # Fonts tab
        font_tab = QWidget()
        font_tab_layout = QVBoxLayout(font_tab)
        font_tab_layout.setContentsMargins(2, 2, 2, 2)
        font_tab_layout.setSpacing(2)

        self._font_list = _DragResourceList("font")
        self._font_list.itemClicked.connect(self._on_font_clicked)
        self._font_list.itemDoubleClicked.connect(self._on_font_double_clicked)
        self._font_list.setContextMenuPolicy(Qt.CustomContextMenu)
        self._font_list.customContextMenuRequested.connect(
            lambda pos: self._show_context_menu(pos, "font")
        )
        font_tab_layout.addWidget(self._font_list, 1)

        font_btn_layout = QHBoxLayout()
        import_font_btn = PushButton("Import Font...")
        import_font_btn.clicked.connect(self._on_import_font)
        font_btn_layout.addWidget(import_font_btn)
        restore_font_btn = PushButton("Restore Missing...")
        restore_font_btn.setToolTip("Restore missing font files by matching selected filenames against missing catalog entries.")
        restore_font_btn.clicked.connect(lambda: self._restore_missing_resources("font"))
        font_btn_layout.addWidget(restore_font_btn)
        replace_font_btn = PushButton("Replace Missing...")
        replace_font_btn.setToolTip("Replace missing font resources with new files and rewrite widget references to the new filenames.")
        replace_font_btn.clicked.connect(lambda: self._replace_missing_resources("font"))
        font_btn_layout.addWidget(replace_font_btn)
        next_missing_font_btn = PushButton("Next Missing")
        next_missing_font_btn.setToolTip("Select the next missing font resource in this tab.")
        next_missing_font_btn.clicked.connect(lambda: self._focus_missing_resource("font"))
        font_btn_layout.addWidget(next_missing_font_btn)
        font_btn_layout.addStretch()
        font_tab_layout.addLayout(font_btn_layout)
        self._tabs.addTab(font_tab, "Fonts")

        # Text tab
        text_tab = QWidget()
        text_tab_layout = QVBoxLayout(text_tab)
        text_tab_layout.setContentsMargins(2, 2, 2, 2)
        text_tab_layout.setSpacing(2)

        self._text_list = _DragResourceList("text")
        self._text_list.itemClicked.connect(self._on_text_clicked)
        self._text_list.itemDoubleClicked.connect(self._on_text_double_clicked)
        self._text_list.setContextMenuPolicy(Qt.CustomContextMenu)
        self._text_list.customContextMenuRequested.connect(
            lambda pos: self._show_context_menu(pos, "text")
        )
        text_tab_layout.addWidget(self._text_list, 1)

        text_btn_layout = QHBoxLayout()
        import_text_btn = PushButton("Import Text...")
        import_text_btn.setToolTip("Import supported-text .txt file into .eguiproject/resources/")
        import_text_btn.clicked.connect(self._on_import_text)
        text_btn_layout.addWidget(import_text_btn)
        restore_text_btn = PushButton("Restore Missing...")
        restore_text_btn.setToolTip("Restore missing text files by matching selected filenames against missing catalog entries.")
        restore_text_btn.clicked.connect(lambda: self._restore_missing_resources("text"))
        text_btn_layout.addWidget(restore_text_btn)
        replace_text_btn = PushButton("Replace Missing...")
        replace_text_btn.setToolTip("Replace missing text resources with new files and rewrite widget references to the new filenames.")
        replace_text_btn.clicked.connect(lambda: self._replace_missing_resources("text"))
        text_btn_layout.addWidget(replace_text_btn)
        next_missing_text_btn = PushButton("Next Missing")
        next_missing_text_btn.setToolTip("Select the next missing text resource in this tab.")
        next_missing_text_btn.clicked.connect(lambda: self._focus_missing_resource("text"))
        text_btn_layout.addWidget(next_missing_text_btn)
        text_btn_layout.addStretch()
        text_tab_layout.addLayout(text_btn_layout)
        self._tabs.addTab(text_tab, "Text")

        # Strings (i18n) tab
        strings_tab = QWidget()
        strings_tab_layout = QVBoxLayout(strings_tab)
        strings_tab_layout.setContentsMargins(2, 2, 2, 2)
        strings_tab_layout.setSpacing(2)

        # Locale selector
        locale_row = QHBoxLayout()
        locale_row.addWidget(QLabel("Locale:"))
        self._locale_combo = QComboBox()
        self._locale_combo.setMinimumWidth(80)
        self._locale_combo.currentIndexChanged.connect(self._on_locale_changed)
        locale_row.addWidget(self._locale_combo)
        add_locale_btn = PushButton("Add Locale...")
        add_locale_btn.clicked.connect(self._on_add_locale)
        locale_row.addWidget(add_locale_btn)
        remove_locale_btn = PushButton("Remove Locale")
        remove_locale_btn.clicked.connect(self._on_remove_locale)
        locale_row.addWidget(remove_locale_btn)
        locale_row.addStretch()
        strings_tab_layout.addLayout(locale_row)

        # String table
        self._string_table = QTableWidget()
        self._string_table.setColumnCount(2)
        self._string_table.setHorizontalHeaderLabels(["Key", "Value"])
        self._string_table.horizontalHeader().setStretchLastSection(True)
        self._string_table.horizontalHeader().setSectionResizeMode(
            0, QHeaderView.ResizeToContents
        )
        self._string_table.setSelectionBehavior(QTableWidget.SelectRows)
        self._string_table.setSelectionMode(QTableWidget.SingleSelection)
        self._string_table.cellChanged.connect(self._on_string_cell_changed)
        self._string_table.currentCellChanged.connect(self._on_string_current_cell_changed)
        self._string_table.itemSelectionChanged.connect(self._refresh_usage_view)
        strings_tab_layout.addWidget(self._string_table, 1)

        # Buttons
        str_btn_layout = QHBoxLayout()
        add_key_btn = PushButton("Add Key...")
        add_key_btn.clicked.connect(self._on_add_string_key)
        str_btn_layout.addWidget(add_key_btn)
        rename_key_btn = PushButton("Rename Key...")
        rename_key_btn.clicked.connect(self._on_rename_string_key)
        str_btn_layout.addWidget(rename_key_btn)
        remove_key_btn = PushButton("Remove Key")
        remove_key_btn.clicked.connect(self._on_remove_string_key)
        str_btn_layout.addWidget(remove_key_btn)
        str_btn_layout.addStretch()
        strings_tab_layout.addLayout(str_btn_layout)

        self._tabs.addTab(strings_tab, "Strings")

        top_layout.addWidget(self._tabs, 1)
        splitter.addWidget(top_widget)

        # -- Bottom: Preview + usage area --
        bottom_widget = QWidget()
        bottom_layout = QVBoxLayout(bottom_widget)
        bottom_layout.setContentsMargins(0, 0, 0, 0)
        bottom_layout.setSpacing(4)

        preview_caption = QLabel("Preview")
        bottom_layout.addWidget(preview_caption)

        self._preview = _PreviewWidget()
        self._preview.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Preferred)
        bottom_layout.addWidget(self._preview, 1)

        usage_caption = QLabel("Usage")
        bottom_layout.addWidget(usage_caption)

        usage_filter_row = QHBoxLayout()
        self._usage_current_page_only = QCheckBox("Current Page Only")
        self._usage_current_page_only.toggled.connect(self._refresh_usage_view)
        usage_filter_row.addWidget(self._usage_current_page_only)
        usage_filter_row.addStretch()
        bottom_layout.addLayout(usage_filter_row)

        self._usage_summary = QLabel("Select an image, font, text resource, or string key to inspect references.")
        self._usage_summary.setWordWrap(True)
        bottom_layout.addWidget(self._usage_summary)

        self._usage_table = QTableWidget()
        self._usage_table.setColumnCount(3)
        self._usage_table.setHorizontalHeaderLabels(["Page", "Widget", "Property"])
        self._usage_table.horizontalHeader().setSectionResizeMode(0, QHeaderView.ResizeToContents)
        self._usage_table.horizontalHeader().setSectionResizeMode(1, QHeaderView.Stretch)
        self._usage_table.horizontalHeader().setSectionResizeMode(2, QHeaderView.ResizeToContents)
        self._usage_table.verticalHeader().setVisible(False)
        self._usage_table.setSelectionBehavior(QAbstractItemView.SelectRows)
        self._usage_table.setSelectionMode(QAbstractItemView.SingleSelection)
        self._usage_table.setEditTriggers(QAbstractItemView.NoEditTriggers)
        self._usage_table.itemDoubleClicked.connect(self._on_usage_item_activated)
        bottom_layout.addWidget(self._usage_table, 1)

        splitter.addWidget(bottom_widget)

        splitter.setStretchFactor(0, 3)
        splitter.setStretchFactor(1, 2)

    # -- Public API --

    def set_resource_dir(self, resource_dir):
        """Set the source directory for resource files and reload.

        The directory should be .eguiproject/resources/ which contains:
          - images/  subfolder for image files
          - font/text files directly
        """
        selected_type = self._current_resource_type
        selected_name = self._current_resource_name
        self._resource_dir = resource_dir or ""
        self._src_dir = resource_dir or ""
        self._images_dir = os.path.join(resource_dir, "images") if resource_dir else ""
        self._image_list.clear()
        self._font_list.clear()
        self._text_list.clear()
        self._preview.clear_preview()

        if not self._src_dir or not os.path.isdir(self._src_dir):
            self._catalog = ResourceCatalog()
            self._update_tab_titles()
            self._refresh_usage_view()
            return

        # Populate image list from catalog (images live in images/ subfolder)
        for fname in self._catalog.images:
            full_path = os.path.join(self._images_dir, fname)
            item = QListWidgetItem(fname)
            item.setData(Qt.UserRole, full_path)
            item.setData(Qt.UserRole + 1, fname)  # filename for drag
            item.setData(Qt.UserRole + 10, False)  # not yet loaded
            exists = os.path.isfile(full_path)
            tooltip = fname
            if not exists:
                tooltip += "\n\u26a0 File not found!"
                item.setForeground(QColor(255, 100, 100))
            item.setToolTip(tooltip)
            self._image_list.addItem(item)

        # Populate font list from catalog
        for fname in self._catalog.fonts:
            full_path = os.path.join(self._src_dir, fname)
            family = self._load_font(full_path) if os.path.isfile(full_path) else ""

            item = QListWidgetItem(fname)
            item.setData(Qt.UserRole, full_path)
            item.setData(Qt.UserRole + 1, fname)  # filename for drag
            tooltip = fname
            if family:
                tooltip += f"\nFamily: {family}"
            if not os.path.isfile(full_path):
                tooltip += "\n\u26a0 File not found!"
                item.setForeground(QColor(255, 100, 100))
            item.setToolTip(tooltip)
            self._font_list.addItem(item)

        # Populate text file list from catalog
        for fname in self._catalog.text_files:
            full_path = os.path.join(self._src_dir, fname)

            item = QListWidgetItem(fname)
            item.setData(Qt.UserRole, full_path)
            item.setData(Qt.UserRole + 1, fname)
            tooltip = fname
            if not os.path.isfile(full_path):
                tooltip += "\n\u26a0 File not found!"
                item.setForeground(QColor(255, 100, 100))
            item.setToolTip(tooltip)
            self._text_list.addItem(item)

        self._update_tab_titles()
        if selected_type and selected_name:
            self._select_resource_item(selected_type, selected_name)
        self._refresh_usage_view()

    def set_resource_catalog(self, catalog):
        """Set the resource catalog and refresh the panel."""
        self._catalog = catalog or ResourceCatalog()
        if self._resource_dir:
            self.set_resource_dir(self._resource_dir)
            return

        self._update_tab_titles()
        self._refresh_usage_view()

    def get_resource_catalog(self):
        """Return the current resource catalog."""
        return self._catalog

    def get_resource_dir(self):
        return self._resource_dir

    def get_src_dir(self):
        return self._src_dir

    def set_string_catalog(self, catalog):
        """Set the i18n string catalog and refresh the Strings tab."""
        self._string_catalog = catalog or StringResourceCatalog()
        self._refresh_string_tab()

    def get_string_catalog(self):
        """Return the current string catalog."""
        return self._string_catalog

    def set_resource_usage_index(self, usage_index):
        """Set the current project resource usage map."""
        self._resource_usage_index = usage_index or {}
        self._refresh_usage_view()

    def set_usage_page_context(self, page_name):
        """Set the current page name used by usage filtering."""
        self._usage_page_name = page_name or ""
        self._refresh_usage_view()

    # -- Internal helpers --

    def _format_resource_tab_title(self, label, total, missing):
        if missing <= 0:
            return f"{label} ({total})"
        return f"{label} ({total}, {missing} missing)"

    def _update_tab_titles(self):
        n_img = len(self._catalog.images)
        n_font = len(self._catalog.fonts)
        n_text = len(self._catalog.text_files)
        missing_img = len(self._missing_resource_names("image"))
        missing_font = len(self._missing_resource_names("font"))
        missing_text = len(self._missing_resource_names("text"))
        n_str = len(self._string_catalog.all_keys)
        self._tabs.setTabText(0, self._format_resource_tab_title("Images", n_img, missing_img))
        self._tabs.setTabText(1, self._format_resource_tab_title("Fonts", n_font, missing_font))
        self._tabs.setTabText(2, self._format_resource_tab_title("Text", n_text, missing_text))
        self._tabs.setTabText(3, f"Strings ({n_str})")

    def _selected_resource_for_active_tab(self):
        current_index = self._tabs.currentIndex()
        if current_index == 0:
            item = self._image_list.currentItem()
            return "image", item.data(Qt.UserRole + 1) if item is not None else ""
        if current_index == 1:
            item = self._font_list.currentItem()
            return "font", item.data(Qt.UserRole + 1) if item is not None else ""
        if current_index == 2:
            item = self._text_list.currentItem()
            return "text", item.data(Qt.UserRole + 1) if item is not None else ""
        if current_index == 3:
            row = self._string_table.currentRow()
            key_item = self._string_table.item(row, 0) if row >= 0 else None
            return "string", key_item.text() if key_item is not None else ""
        return "", ""

    def _update_current_resource(self, resource_type, filename):
        self._current_resource_type = resource_type or ""
        self._current_resource_name = filename or ""
        self._refresh_usage_view()

    def _clear_usage_view(self, summary):
        self._usage_summary.setText(summary)
        self._usage_table.setRowCount(0)

    def _refresh_usage_view(self):
        if not hasattr(self, "_usage_table"):
            return

        resource_type = self._current_resource_type
        resource_name = self._current_resource_name
        active_type, active_name = self._selected_resource_for_active_tab()
        if active_name:
            resource_type = active_type
            resource_name = active_name
            self._current_resource_type = active_type
            self._current_resource_name = active_name

        if not resource_type or not resource_name:
            self._clear_usage_view("Select an image, font, text resource, or string key to inspect references.")
            return

        all_usages = list(self._resource_usage_index.get((resource_type, resource_name), []))
        if not all_usages:
            self._clear_usage_view(f"'{resource_name}' is currently unused.")
            return

        usages = all_usages
        filter_to_current_page = self._usage_current_page_only.isChecked() and bool(self._usage_page_name)
        if filter_to_current_page:
            usages = [entry for entry in all_usages if entry.page_name == self._usage_page_name]
            if not usages:
                self._clear_usage_view(f"'{resource_name}' has no references on the current page.")
                return

        page_count = len({entry.page_name for entry in usages})
        widget_count = len(usages)
        page_noun = "page" if page_count == 1 else "pages"
        widget_noun = "widget" if widget_count == 1 else "widgets"
        if filter_to_current_page:
            total_widget_count = len(all_usages)
            total_page_count = len({entry.page_name for entry in all_usages})
            total_page_noun = "page" if total_page_count == 1 else "pages"
            self._usage_summary.setText(
                f"'{resource_name}' is used by {widget_count} {widget_noun} on the current page "
                f"({total_widget_count} total across {total_page_count} {total_page_noun})."
            )
        else:
            self._usage_summary.setText(
                f"'{resource_name}' is used by {widget_count} {widget_noun} across {page_count} {page_noun}."
            )
        self._usage_table.setRowCount(len(usages))
        for row, entry in enumerate(usages):
            page_item = QTableWidgetItem(entry.page_name)
            page_item.setData(Qt.UserRole, entry.page_name)
            page_item.setData(Qt.UserRole + 1, entry.widget_name)
            widget_text = entry.widget_name
            if entry.widget_type:
                widget_text = f"{entry.widget_name} ({entry.widget_type})"
            widget_item = QTableWidgetItem(widget_text)
            prop_item = QTableWidgetItem(entry.property_name)
            self._usage_table.setItem(row, 0, page_item)
            self._usage_table.setItem(row, 1, widget_item)
            self._usage_table.setItem(row, 2, prop_item)
        if self._usage_table.rowCount() > 0:
            self._usage_table.selectRow(0)

    def _on_usage_item_activated(self, item):
        if item is None:
            return
        page_item = self._usage_table.item(item.row(), 0)
        if page_item is None:
            return
        page_name = page_item.data(Qt.UserRole) or ""
        widget_name = page_item.data(Qt.UserRole + 1) or ""
        if page_name and widget_name:
            self.usage_activated.emit(page_name, widget_name)

    def _target_dir_for_resource_type(self, resource_type):
        return self._images_dir if resource_type == "image" else self._src_dir

    def _list_widget_for_resource_type(self, resource_type):
        if resource_type == "image":
            return self._image_list
        if resource_type == "font":
            return self._font_list
        if resource_type == "text":
            return self._text_list
        return None

    def _missing_resource_names(self, resource_type):
        target_dir = self._target_dir_for_resource_type(resource_type)
        if resource_type == "image":
            names = self._catalog.images
        elif resource_type == "font":
            names = self._catalog.fonts
        elif resource_type == "text":
            names = self._catalog.text_files
        else:
            return []
        return [name for name in names if not os.path.isfile(os.path.join(target_dir, name))]

    def _select_resource_item(self, resource_type, filename):
        if resource_type == "image":
            self._tabs.setCurrentIndex(0)
        elif resource_type == "font":
            self._tabs.setCurrentIndex(1)
        elif resource_type == "text":
            self._tabs.setCurrentIndex(2)
        elif resource_type == "string":
            self._tabs.setCurrentIndex(3)
            matches = self._string_table.findItems(filename, Qt.MatchExactly)
            if matches:
                self._string_table.setCurrentItem(matches[0])
            self._update_current_resource(resource_type, filename)
            return
        lst = self._list_widget_for_resource_type(resource_type)
        if lst is None:
            return
        matches = lst.findItems(filename, Qt.MatchExactly)
        if matches:
            lst.setCurrentItem(matches[0])
        self._update_current_resource(resource_type, filename)

    def _focus_missing_resource(self, resource_type):
        lst = self._list_widget_for_resource_type(resource_type)
        if lst is None:
            return ""

        missing_names = self._missing_resource_names(resource_type)
        if not missing_names:
            self.feedback_message.emit(f"No missing {resource_type} resources were found.")
            return ""

        current_item = lst.currentItem()
        current_name = current_item.data(Qt.UserRole + 1) if current_item is not None else ""
        if current_name in missing_names:
            target_index = (missing_names.index(current_name) + 1) % len(missing_names)
        else:
            target_index = 0

        target_name = missing_names[target_index]
        matches = lst.findItems(target_name, Qt.MatchExactly)
        if matches:
            lst.setCurrentItem(matches[0])
            lst.scrollToItem(matches[0])

        self._preview.clear_preview()
        self.feedback_message.emit(
            f"Focused missing {resource_type} resource {target_index + 1}/{len(missing_names)}: {target_name}."
        )
        return target_name

    def _emit_operation_summary(self, action, resource_type, restored=None, renamed=None, unmatched=None, failures=None, remaining_missing=0):
        parts = []
        if renamed:
            parts.append(f"{len(renamed)} renamed")
        if restored:
            parts.append(f"{len(restored)} restored")
        if unmatched:
            parts.append(f"{len(unmatched)} unmatched")
        if failures:
            parts.append(f"{len(failures)} failed")
        if remaining_missing:
            parts.append(f"{remaining_missing} remaining missing")
        if not parts:
            return
        self.feedback_message.emit(f"{action} {resource_type} resources: {', '.join(parts)}.")

    def _confirm_reference_impact(self, title, resource_name, usages, unused_prompt, impact_text, confirm_text):
        if not usages:
            if not unused_prompt:
                return True
            reply = QMessageBox.question(
                self,
                title,
                unused_prompt,
                QMessageBox.Yes | QMessageBox.No,
            )
            return reply == QMessageBox.Yes

        page_count = len({entry.page_name for entry in usages})
        widget_count = len(usages)
        page_noun = "page" if page_count == 1 else "pages"
        widget_noun = "widget" if widget_count == 1 else "widgets"
        summary = (
            f"'{resource_name}' is used by {widget_count} {widget_noun} across {page_count} {page_noun}.\n"
            f"{impact_text}"
        )
        dialog = _ReferenceImpactDialog(self, title, summary, usages, confirm_text)
        result = dialog.exec_()
        if result == _ReferenceImpactDialog.NAVIGATE_RESULT:
            page_name, widget_name = dialog.selected_usage()
            if page_name and widget_name:
                self.usage_activated.emit(page_name, widget_name)
            return False
        return result == QDialog.Accepted

    def _collect_batch_replace_impacts(self, resource_type, replacements):
        impacts = []
        total_rename_count = 0
        for old_name, source_path in replacements.items():
            new_name = os.path.basename(source_path or "")
            if not new_name or new_name == old_name:
                continue

            total_rename_count += 1
            usages = list(self._resource_usage_index.get((resource_type, old_name), []))
            if not usages:
                continue

            impacts.append(
                {
                    "old_name": old_name,
                    "new_name": new_name,
                    "usages": usages,
                    "widget_count": len(usages),
                    "page_count": len({entry.page_name for entry in usages}),
                }
            )

        impacts.sort(key=lambda entry: (entry["old_name"].lower(), entry["new_name"].lower()))
        return impacts, total_rename_count

    def _confirm_batch_replace_impact(self, resource_type, impacts, total_rename_count):
        if not impacts:
            return True

        dialog = _BatchReplaceImpactDialog(
            self,
            "Replace Missing Resources",
            resource_type,
            impacts,
            total_rename_count,
            "Replace",
            current_page_name=self._usage_page_name,
        )
        result = dialog.exec_()
        if result == _BatchReplaceImpactDialog.NAVIGATE_RESULT:
            page_name, widget_name = dialog.selected_usage()
            if page_name and widget_name:
                self.usage_activated.emit(page_name, widget_name)
            return False
        return result == QDialog.Accepted

    def _dialog_filter_for_resource_type(self, resource_type):
        if resource_type == "image":
            return "Images (*.png *.bmp *.jpg *.jpeg)"
        if resource_type == "font":
            return "Fonts (*.ttf *.otf)"
        if resource_type == "text":
            return "Text Files (*.txt);;All Files (*.*)"
        return "All Files (*.*)"

    def _allowed_extensions_for_resource_type(self, resource_type):
        if resource_type == "image":
            return IMAGE_EXTENSIONS
        if resource_type == "font":
            return FONT_EXTENSIONS
        if resource_type == "text":
            return TEXT_EXTENSIONS
        return set()

    def _validate_unique_source_filenames(self, source_paths):
        seen = {}
        duplicates = []
        for source_path in source_paths:
            filename = os.path.basename(source_path).lower()
            if filename in seen:
                duplicates.append(os.path.basename(source_path))
            else:
                seen[filename] = source_path
        if duplicates:
            dup_list = ", ".join(sorted(set(duplicates)))
            QMessageBox.warning(
                self,
                "Duplicate Replacement Filenames",
                f"Selected replacement files must have unique filenames.\nDuplicates: {dup_list}",
            )
            return False
        return True

    def _replace_missing_resource_with_path(self, old_name, resource_type, source_path):
        new_name = os.path.basename(source_path)
        if not _validate_english_filename(new_name):
            return "", f"'{new_name}' is invalid. Use only ASCII letters, digits, underscore, and dash."

        extension = os.path.splitext(new_name)[1].lower()
        if extension not in self._allowed_extensions_for_resource_type(resource_type):
            return "", f"'{new_name}' is not a supported {resource_type} resource."

        target_dir = self._target_dir_for_resource_type(resource_type)
        old_path = os.path.join(target_dir, old_name)
        target_path = os.path.join(target_dir, new_name)
        if new_name != old_name and os.path.exists(target_path):
            return "", f"'{new_name}' already exists."

        try:
            shutil.copy2(source_path, old_path if new_name == old_name else target_path)
        except OSError as exc:
            return "", str(exc)

        if new_name != old_name:
            self._catalog.remove_file(old_name)
        self._catalog.add_file(new_name)
        return new_name, ""

    def _replace_missing_resources_from_mapping(self, resource_type, replacements):
        if not self._ensure_src_dir():
            return [], [], [("__all__", "No resource directory configured.")]

        restored = []
        renamed = []
        failures = []
        first_selected_name = ""

        for old_name, source_path in replacements.items():
            new_name, error = self._replace_missing_resource_with_path(old_name, resource_type, source_path)
            if error:
                failures.append((old_name, error))
                continue

            if not first_selected_name:
                first_selected_name = new_name
            if new_name == old_name:
                restored.append(old_name)
            else:
                renamed.append((old_name, new_name))

        if restored or renamed:
            self.set_resource_dir(self._resource_dir)
            if first_selected_name:
                self._select_resource_item(resource_type, first_selected_name)
            self.resource_imported.emit()
            for old_name, new_name in renamed:
                self.resource_renamed.emit(resource_type, old_name, new_name)
            remaining_missing = len(self._missing_resource_names(resource_type))
            self._emit_operation_summary(
                "Replaced",
                resource_type,
                restored=restored,
                renamed=renamed,
                failures=failures,
                remaining_missing=remaining_missing,
            )

        return restored, renamed, failures

    def _load_font(self, path):
        if path in self._font_id_cache:
            return self._font_family_cache.get(path, "")
        fid = QFontDatabase.addApplicationFont(path)
        self._font_id_cache[path] = fid
        if fid >= 0:
            families = QFontDatabase.applicationFontFamilies(fid)
            family = families[0] if families else ""
        else:
            family = ""
        self._font_family_cache[path] = family
        return family

    def _ensure_src_dir(self):
        if not self._resource_dir:
            QMessageBox.warning(self, "Error",
                                "No resource directory configured.\n"
                                "Please save the project first or open an existing project.")
            return False
        self._src_dir = self._resource_dir
        self._images_dir = os.path.join(self._resource_dir, "images")
        os.makedirs(self._src_dir, exist_ok=True)
        os.makedirs(self._images_dir, exist_ok=True)
        return True

    def _default_external_import_dir(self, resource_type=""):
        candidate = self._last_external_import_dir
        if candidate and os.path.isdir(candidate):
            return candidate

        if resource_type == "image" and self._images_dir and os.path.isdir(self._images_dir):
            return self._images_dir

        if self._src_dir and os.path.isdir(self._src_dir):
            return self._src_dir

        return os.path.normpath(os.getcwd())

    def _remember_external_import_paths(self, paths):
        if not paths:
            return
        first_path = paths[0]
        parent_dir = os.path.dirname(first_path)
        if parent_dir and os.path.isdir(parent_dir):
            self._last_external_import_dir = parent_dir

    # -- Selection / double-click --

    def _on_image_clicked(self, item):
        path = item.data(Qt.UserRole)
        filename = item.data(Qt.UserRole + 1)
        if path and os.path.isfile(path):
            self._preview.show_image(path)
        else:
            self._preview.clear_preview()
        self._update_current_resource("image", filename)
        self.resource_selected.emit("image", filename)

    def _on_font_clicked(self, item):
        path = item.data(Qt.UserRole)
        filename = item.data(Qt.UserRole + 1)
        family = self._font_family_cache.get(path, "")
        if path and os.path.isfile(path):
            self._preview.show_font(path, family)
        else:
            self._preview.clear_preview()
        self._update_current_resource("font", filename)
        self.resource_selected.emit("font", filename)

    def _on_text_clicked(self, item):
        path = item.data(Qt.UserRole)
        filename = item.data(Qt.UserRole + 1)
        if path and os.path.isfile(path):
            self._preview.show_text(path)
        else:
            self._preview.clear_preview()
        self._update_current_resource("text", filename)
        self.resource_selected.emit("text", filename)

    def _on_image_double_clicked(self, item):
        filename = item.data(Qt.UserRole + 1)
        self._update_current_resource("image", filename)
        self.resource_selected.emit("image", filename)

    def _on_font_double_clicked(self, item):
        filename = item.data(Qt.UserRole + 1)
        self._update_current_resource("font", filename)
        self.resource_selected.emit("font", filename)

    def _on_text_double_clicked(self, item):
        filename = item.data(Qt.UserRole + 1)
        self._update_current_resource("text", filename)
        self.resource_selected.emit("text", filename)

    # -- Import (buttons) --

    def _on_import_image(self):
        if not self._ensure_src_dir():
            return
        paths, _ = QFileDialog.getOpenFileNames(
            self, "Import Images", self._default_external_import_dir("image"),
            self._dialog_filter_for_resource_type("image")
        )
        if paths:
            self._remember_external_import_paths(paths)
            self._do_import(paths, "image")

    def _on_import_font(self):
        if not self._ensure_src_dir():
            return
        paths, _ = QFileDialog.getOpenFileNames(
            self, "Import Fonts", self._default_external_import_dir("font"),
            self._dialog_filter_for_resource_type("font")
        )
        if paths:
            self._remember_external_import_paths(paths)
            self._do_import(paths, "font")

    def _on_import_text(self):
        if not self._ensure_src_dir():
            return
        paths, _ = QFileDialog.getOpenFileNames(
            self, "Import Text Files", self._default_external_import_dir("text"),
            self._dialog_filter_for_resource_type("text")
        )
        if paths:
            self._remember_external_import_paths(paths)
            self._do_import(paths, "text")

    def _restore_missing_resources_from_paths(self, resource_type, source_paths):
        target_dir = self._target_dir_for_resource_type(resource_type)
        missing_map = {name.lower(): name for name in self._missing_resource_names(resource_type)}
        restored = []
        unmatched = []
        failures = []

        for source_path in source_paths:
            source_name = os.path.basename(source_path)
            target_name = missing_map.get(source_name.lower())
            if not target_name:
                unmatched.append(source_name)
                continue

            target_path = os.path.join(target_dir, target_name)
            try:
                shutil.copy2(source_path, target_path)
            except OSError as exc:
                failures.append((target_name, str(exc)))
                continue

            self._catalog.add_file(target_name)
            restored.append(target_name)
            missing_map.pop(source_name.lower(), None)

        if restored:
            self.set_resource_dir(self._resource_dir)
            self._select_resource_item(resource_type, restored[0])
            self.resource_imported.emit()
            remaining_missing = len(self._missing_resource_names(resource_type))
            self._emit_operation_summary(
                "Restored",
                resource_type,
                restored=restored,
                unmatched=unmatched,
                failures=failures,
                remaining_missing=remaining_missing,
            )

        return restored, unmatched, failures

    def _restore_missing_resources(self, resource_type):
        if not self._ensure_src_dir():
            return

        missing_names = self._missing_resource_names(resource_type)
        if not missing_names:
            QMessageBox.information(
                self,
                "Restore Missing Resources",
                f"No missing {resource_type} resources were found.",
            )
            return

        source_paths, _ = QFileDialog.getOpenFileNames(
            self,
            "Restore Missing Resources",
            self._default_external_import_dir(resource_type),
            self._dialog_filter_for_resource_type(resource_type),
        )
        if not source_paths:
            return

        self._remember_external_import_paths(source_paths)
        restored, unmatched, failures = self._restore_missing_resources_from_paths(resource_type, source_paths)
        if restored:
            return

        message = f"No matching missing {resource_type} resources were found in the selected files."
        if failures:
            details = "\n".join(f"{name}: {error}" for name, error in failures)
            message = f"{message}\n\nFailed copies:\n{details}"
        elif unmatched:
            message = f"{message}\n\nSelected files: {', '.join(unmatched)}"
        QMessageBox.warning(self, "Restore Missing Resources", message)

    def _restore_missing_resource(self, filename, resource_type):
        if not self._ensure_src_dir():
            return

        target_dir = self._target_dir_for_resource_type(resource_type)
        target_path = os.path.join(target_dir, filename)
        if os.path.isfile(target_path):
            return

        source_path, _ = QFileDialog.getOpenFileName(
            self,
            "Restore Missing Resource",
            self._default_external_import_dir(resource_type),
            self._dialog_filter_for_resource_type(resource_type),
        )
        if not source_path:
            return
        self._remember_external_import_paths([source_path])

        expected_ext = os.path.splitext(filename)[1].lower()
        source_ext = os.path.splitext(source_path)[1].lower()
        if expected_ext and source_ext != expected_ext:
            QMessageBox.warning(
                self,
                "Extension Mismatch",
                f"Expected a '{expected_ext}' file to restore '{filename}'.",
            )
            return

        try:
            shutil.copy2(source_path, target_path)
        except OSError as exc:
            QMessageBox.warning(self, "Error", f"Restore failed: {exc}")
            return

        self._catalog.add_file(filename)
        self.set_resource_dir(self._resource_dir)

        self._select_resource_item(resource_type, filename)

        self.resource_imported.emit()

    def _replace_missing_resources(self, resource_type):
        if not self._ensure_src_dir():
            return

        missing_names = self._missing_resource_names(resource_type)
        if not missing_names:
            QMessageBox.information(
                self,
                "Replace Missing Resources",
                f"No missing {resource_type} resources were found.",
            )
            return

        source_paths, _ = QFileDialog.getOpenFileNames(
            self,
            "Replace Missing Resources",
            self._default_external_import_dir(resource_type),
            self._dialog_filter_for_resource_type(resource_type),
        )
        if not source_paths:
            return
        self._remember_external_import_paths(source_paths)

        if not self._validate_unique_source_filenames(source_paths):
            return

        dialog = _MissingResourceReplaceDialog(missing_names, source_paths, self)
        if dialog.exec_() != QDialog.Accepted:
            return

        replacements = dialog.selected_mapping()
        impacts, total_rename_count = self._collect_batch_replace_impacts(resource_type, replacements)
        if impacts and not self._confirm_batch_replace_impact(resource_type, impacts, total_rename_count):
            return

        restored, renamed, failures = self._replace_missing_resources_from_mapping(resource_type, replacements)
        if restored or renamed:
            if failures:
                details = "\n".join(f"{name}: {error}" for name, error in failures)
                QMessageBox.warning(
                    self,
                    "Replace Missing Resources",
                    f"Some replacements could not be applied:\n{details}",
                )
            return

        if failures:
            details = "\n".join(f"{name}: {error}" for name, error in failures)
            QMessageBox.warning(self, "Replace Missing Resources", details)

    def _replace_missing_resource(self, filename, resource_type):
        if not self._ensure_src_dir():
            return

        source_path, _ = QFileDialog.getOpenFileName(
            self,
            "Replace Missing Resource",
            self._default_external_import_dir(resource_type),
            self._dialog_filter_for_resource_type(resource_type),
        )
        if not source_path:
            return
        self._remember_external_import_paths([source_path])

        replacements = {filename: source_path}
        impacts, total_rename_count = self._collect_batch_replace_impacts(resource_type, replacements)
        if impacts and not self._confirm_batch_replace_impact(resource_type, impacts, total_rename_count):
            return

        restored, renamed, failures = self._replace_missing_resources_from_mapping(
            resource_type,
            replacements,
        )
        if restored or renamed:
            return

        if failures:
            QMessageBox.warning(self, "Replace Missing Resource", failures[0][1])

    def _do_import(self, source_paths, resource_type):
        if not self._ensure_src_dir():
            return
        self._remember_external_import_paths(source_paths)
        imported = 0
        target_dir = self._target_dir_for_resource_type(resource_type)
        for src in source_paths:
            fname = os.path.basename(src)
            # Show rename dialog for each file
            new_name, ok = QInputDialog.getText(
                self, "Rename Resource",
                f"Import '{fname}' as:",
                text=fname,
            )
            if not ok or not new_name:
                continue
            fname = new_name
            if not _validate_english_filename(fname):
                QMessageBox.warning(
                    self, "Invalid Filename",
                    f"'{fname}' is invalid.\n"
                    "Use only ASCII letters, digits, underscore, and dash."
                )
                continue
            dst = os.path.join(target_dir, fname)
            if os.path.exists(dst):
                QMessageBox.warning(self, "Error", f"'{fname}' already exists.")
                continue
            shutil.copy2(src, dst)
            # Add to catalog
            self._catalog.add_file(fname)
            imported += 1
        if imported > 0:
            self.set_resource_dir(self._resource_dir)
            self.resource_imported.emit()

    # -- Drag-drop import from OS --

    def dragEnterEvent(self, event):
        if event.mimeData().hasUrls():
            for url in event.mimeData().urls():
                path = url.toLocalFile()
                lower = path.lower()
                if lower.endswith(_IMAGE_EXTS + _FONT_EXTS + _TEXT_EXTS):
                    event.acceptProposedAction()
                    return
        event.ignore()

    def dragMoveEvent(self, event):
        if event.mimeData().hasUrls():
            event.acceptProposedAction()
        else:
            event.ignore()

    def dropEvent(self, event):
        if not event.mimeData().hasUrls():
            event.ignore()
            return
        images = []
        fonts = []
        texts = []
        for url in event.mimeData().urls():
            path = url.toLocalFile()
            if not path or not os.path.isfile(path):
                continue
            lower = path.lower()
            if lower.endswith(_IMAGE_EXTS):
                images.append(path)
            elif lower.endswith(_FONT_EXTS):
                fonts.append(path)
            elif lower.endswith(_TEXT_EXTS):
                texts.append(path)

        did_import = False
        if images:
            self._do_import(images, "image")
            did_import = True
        if fonts:
            self._do_import(fonts, "font")
            did_import = True
        if texts:
            self._do_import(texts, "text")
            did_import = True

        if did_import:
            event.acceptProposedAction()
        else:
            event.ignore()

    # -- Right-click context menu --

    def _show_context_menu(self, pos, resource_type):
        lst = self._list_widget_for_resource_type(resource_type)
        if lst is None:
            return

        item = lst.itemAt(pos)
        if item is None:
            return

        filename = item.data(Qt.UserRole + 1)
        path = item.data(Qt.UserRole) or ""

        menu = QMenu(self)

        assign_act = menu.addAction("Assign to Selected Widget")
        assign_act.triggered.connect(lambda: self.resource_selected.emit(resource_type, filename))

        copy_act = menu.addAction(f"Copy: {filename}")
        copy_act.triggered.connect(lambda: QApplication.clipboard().setText(filename))

        menu.addSeparator()

        if not os.path.isfile(path):
            restore_act = menu.addAction("Restore Missing File...")
            restore_act.triggered.connect(lambda: self._restore_missing_resource(filename, resource_type))
            replace_act = menu.addAction("Replace With File...")
            replace_act.triggered.connect(lambda: self._replace_missing_resource(filename, resource_type))
            menu.addSeparator()

        reveal_act = menu.addAction("Reveal in File Manager")
        reveal_act.triggered.connect(lambda: self._reveal_in_explorer(path))

        menu.addSeparator()

        rename_act = menu.addAction("Rename...")
        rename_act.triggered.connect(lambda: self._rename_resource(filename, resource_type))

        delete_act = menu.addAction("Delete")
        delete_act.triggered.connect(lambda: self._delete_resource(filename, resource_type))

        menu.exec_(lst.viewport().mapToGlobal(pos))

    def _reveal_in_explorer(self, path):
        import subprocess
        import sys as _sys
        if _sys.platform == "win32":
            subprocess.Popen(["explorer", "/select,", os.path.normpath(path)])
        elif _sys.platform == "darwin":
            subprocess.Popen(["open", "-R", path])
        else:
            folder = os.path.dirname(path)
            subprocess.Popen(["xdg-open", folder])

    def _rename_resource(self, old_name, resource_type):
        new_name, ok = QInputDialog.getText(
            self, "Rename Resource", "New filename:", text=old_name
        )
        if not ok or not new_name or new_name == old_name:
            return
        if not _validate_english_filename(new_name):
            QMessageBox.warning(
                self, "Invalid Filename",
                f"'{new_name}' is invalid.\n"
                "Use only ASCII letters, digits, underscore, and dash."
            )
            return
        file_dir = self._target_dir_for_resource_type(resource_type)
        old_path = os.path.join(file_dir, old_name)
        new_path = os.path.join(file_dir, new_name)
        if os.path.exists(new_path):
            QMessageBox.warning(self, "Error", f"'{new_name}' already exists.")
            return
        usages = list(self._resource_usage_index.get((resource_type, old_name), []))
        confirmed = self._confirm_reference_impact(
            "Rename Resource",
            old_name,
            usages,
            "",
            f"Renaming it to '{new_name}' will update those widget references.",
            "Rename",
        )
        if not confirmed:
            return
        try:
            if os.path.exists(old_path):
                os.rename(old_path, new_path)
            # Update catalog
            self._catalog.remove_file(old_name)
            self._catalog.add_file(new_name)
            self.set_resource_dir(self._resource_dir)
            self.resource_renamed.emit(resource_type, old_name, new_name)
            self.resource_imported.emit()
        except OSError as e:
            QMessageBox.warning(self, "Error", f"Rename failed: {e}")

    def _delete_resource(self, filename, resource_type):
        usages = list(self._resource_usage_index.get((resource_type, filename), []))
        confirmed = self._confirm_reference_impact(
            "Delete Resource",
            filename,
            usages,
            f"Remove '{filename}' from catalog and delete the file?\nThis cannot be undone.",
            "Deleting it will clear those widget references.",
            "Delete",
        )
        if not confirmed:
            return
        self._catalog.remove_file(filename)
        file_dir = self._target_dir_for_resource_type(resource_type)
        file_path = os.path.join(file_dir, filename)
        if os.path.isfile(file_path):
            try:
                os.remove(file_path)
            except OSError:
                pass
        self.set_resource_dir(self._resource_dir)
        self.resource_deleted.emit(resource_type, filename)
        self.resource_imported.emit()

    # -- Strings tab methods --

    def _refresh_string_tab(self):
        """Refresh locale combo and string table from the catalog."""
        self._string_table_updating = True
        try:
            # Refresh locale combo
            prev_locale = self._get_selected_locale()
            self._locale_combo.blockSignals(True)
            self._locale_combo.clear()
            names = self._string_catalog.locale_display_names
            for locale_code in self._string_catalog.locales:
                display = names.get(locale_code, locale_code or "Default")
                self._locale_combo.addItem(display, locale_code)
            # Restore selection
            idx = self._locale_combo.findData(prev_locale)
            if idx >= 0:
                self._locale_combo.setCurrentIndex(idx)
            self._locale_combo.blockSignals(False)

            self._refresh_string_table()
            self._update_tab_titles()
        finally:
            self._string_table_updating = False
        self._refresh_usage_view()

    def _get_selected_locale(self):
        """Get the locale code of the currently selected combo item."""
        idx = self._locale_combo.currentIndex()
        if idx < 0:
            return DEFAULT_LOCALE
        return self._locale_combo.itemData(idx) or DEFAULT_LOCALE

    def _refresh_string_table(self):
        """Repopulate the table for the selected locale."""
        self._string_table_updating = True
        try:
            locale = self._get_selected_locale()
            keys = self._string_catalog.all_keys
            prev_key = ""
            current_row = self._string_table.currentRow()
            current_key_item = self._string_table.item(current_row, 0) if current_row >= 0 else None
            if current_key_item is not None:
                prev_key = current_key_item.text()

            self._string_table.setRowCount(len(keys))
            for row, key in enumerate(keys):
                # Key column (read-only)
                key_item = QTableWidgetItem(key)
                key_item.setFlags(key_item.flags() & ~Qt.ItemIsEditable)
                self._string_table.setItem(row, 0, key_item)

                # Value column (editable)
                value = self._string_catalog.get(key, locale)
                val_item = QTableWidgetItem(value)
                self._string_table.setItem(row, 1, val_item)

            if keys:
                target_key = prev_key if prev_key in keys else keys[0]
                target_row = keys.index(target_key)
                self._string_table.setCurrentCell(target_row, 0)
            else:
                self._update_current_resource("", "")
        finally:
            self._string_table_updating = False

    def _on_locale_changed(self, index):
        """Locale combo selection changed."""
        if not self._string_table_updating:
            self._refresh_string_table()

    def _on_string_current_cell_changed(self, current_row, current_column, previous_row, previous_column):
        del current_column, previous_row, previous_column
        if self._string_table_updating:
            return
        key_item = self._string_table.item(current_row, 0) if current_row >= 0 else None
        key = key_item.text() if key_item is not None else ""
        self._preview.clear_preview()
        self._update_current_resource("string" if key else "", key)

    def _on_string_cell_changed(self, row, col):
        """Handle user editing a string value in the table."""
        if self._string_table_updating:
            return
        if col != 1:
            return
        locale = self._get_selected_locale()
        key_item = self._string_table.item(row, 0)
        val_item = self._string_table.item(row, 1)
        if key_item and val_item:
            key = key_item.text()
            value = val_item.text()
            self._string_catalog.set(key, value, locale)
            self.resource_imported.emit()

    def _on_add_string_key(self):
        """Add a new string key to all locales."""
        key, ok = QInputDialog.getText(
            self, "Add String Key",
            "Enter string key name (e.g. app_name):"
        )
        if not ok or not key:
            return
        key = key.strip()

        # Ensure default locale exists
        if DEFAULT_LOCALE not in self._string_catalog.strings:
            self._string_catalog.add_locale(DEFAULT_LOCALE)
            self._refresh_string_tab()

        try:
            self._string_catalog.add_key(key, "")
        except ValueError as e:
            QMessageBox.warning(self, "Invalid Key", str(e))
            return

        self._refresh_string_tab()
        self.resource_imported.emit()

    def _on_rename_string_key(self):
        """Rename the selected string key across all locales."""
        row = self._string_table.currentRow()
        if row < 0:
            return
        key_item = self._string_table.item(row, 0)
        if key_item is None:
            return
        old_key = key_item.text().strip()
        if not old_key:
            return

        new_key, ok = QInputDialog.getText(
            self,
            "Rename String Key",
            "Enter new string key name:",
            text=old_key,
        )
        if not ok:
            return
        new_key = new_key.strip()
        if not new_key or new_key == old_key:
            return
        usages = list(self._resource_usage_index.get(("string", old_key), []))
        confirmed = self._confirm_reference_impact(
            "Rename String Key",
            old_key,
            usages,
            "",
            f"Renaming it to '{new_key}' will update those string references.",
            "Rename",
        )
        if not confirmed:
            return

        try:
            self._string_catalog.rename_key(old_key, new_key)
        except ValueError as exc:
            QMessageBox.warning(self, "Invalid Key", str(exc))
            return

        self._refresh_string_tab()
        self._select_resource_item("string", new_key)
        self.string_key_renamed.emit(old_key, new_key)
        self.resource_imported.emit()

    def _on_remove_string_key(self):
        """Remove the selected string key from all locales."""
        row = self._string_table.currentRow()
        if row < 0:
            return
        key_item = self._string_table.item(row, 0)
        if not key_item:
            return
        key = key_item.text()
        usages = list(self._resource_usage_index.get(("string", key), []))
        replacement_text = self._string_catalog.get(key, DEFAULT_LOCALE)
        rewrite_text = "convert those references to the default-locale literal text" if replacement_text else "clear those references"
        confirmed = self._confirm_reference_impact(
            "Remove String Key",
            key,
            usages,
            f"Remove key '{key}' from all locales?",
            f"Removing it will {rewrite_text}.",
            "Remove",
        )
        if not confirmed:
            return
        self._string_catalog.remove_key(key)
        self._refresh_string_tab()
        self.string_key_deleted.emit(key, replacement_text)
        self.resource_imported.emit()

    def _on_add_locale(self):
        """Add a new locale (e.g. 'zh', 'ja', 'fr')."""
        locale, ok = QInputDialog.getText(
            self, "Add Locale",
            "Enter locale code (e.g. zh, ja, fr, de):"
        )
        if not ok or not locale:
            return
        locale = locale.strip().lower()
        if not locale:
            return

        # Ensure default locale exists first
        if DEFAULT_LOCALE not in self._string_catalog.strings:
            self._string_catalog.add_locale(DEFAULT_LOCALE)

        self._string_catalog.add_locale(locale)
        self._refresh_string_tab()

        # Select the new locale
        idx = self._locale_combo.findData(locale)
        if idx >= 0:
            self._locale_combo.setCurrentIndex(idx)

        self.resource_imported.emit()

    def _on_remove_locale(self):
        """Remove the currently selected locale."""
        locale = self._get_selected_locale()
        if locale == DEFAULT_LOCALE:
            QMessageBox.information(
                self, "Cannot Remove",
                "The default locale cannot be removed."
            )
            return
        reply = QMessageBox.question(
            self, "Remove Locale",
            f"Remove locale '{locale}' and all its translations?",
            QMessageBox.Yes | QMessageBox.No,
        )
        if reply != QMessageBox.Yes:
            return
        self._string_catalog.remove_locale(locale)
        self._refresh_string_tab()
        self.resource_imported.emit()

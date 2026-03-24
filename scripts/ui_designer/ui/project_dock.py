"""Project Explorer dock — lists pages and resources.

Provides a tree view showing:
  Pages/
    main_page
    settings_page
  Resources/
    images/
    fonts/

Context menus allow adding, deleting, renaming, and copying pages,
as well as setting the startup page.
"""

import os
from PyQt5.QtWidgets import (
    QDockWidget, QWidget, QVBoxLayout, QTreeWidget, QTreeWidgetItem,
    QMenu, QAction, QInputDialog, QMessageBox, QLabel, QHBoxLayout,
    QPushButton, QComboBox, QGroupBox,
)
from PyQt5.QtCore import pyqtSignal, Qt
from PyQt5.QtGui import QIcon, QFont

# Page names that collide with egui internal module names.
# A page named "test" generates egui_test_init() which conflicts with
# src/test/egui_test.h's egui_test_init(void).
_RESERVED_PAGE_NAMES = {
    "activity", "animation", "api", "background", "canvas", "common",
    "config", "core", "dialog", "display_driver", "dlist", "fixmath",
    "focus", "font", "i18n", "image", "input", "interpolator",
    "key_event", "mask", "motion_event", "oop", "page_base", "pfb_manager",
    "platform", "region", "resource", "rotation", "scroller", "shadow",
    "sprite", "slist", "style", "test", "theme", "timer", "toast",
    "touch_driver", "utils", "view",
}


class ProjectExplorerDock(QDockWidget):
    """Dock widget showing project pages and resources.

    Signals:
        page_selected(str):    page name selected (filename without ext)
        page_added(str):       new page name
        page_duplicated(str,str): (source_name, new page name)
        page_removed(str):     removed page name
        page_renamed(str,str): (old_name, new_name)
        startup_changed(str):  new startup page name
        page_mode_changed(str): "easy_page" or "activity"
    """

    page_selected = pyqtSignal(str)
    page_added = pyqtSignal(str)
    page_duplicated = pyqtSignal(str, str)
    page_removed = pyqtSignal(str)
    page_renamed = pyqtSignal(str, str)
    startup_changed = pyqtSignal(str)
    page_mode_changed = pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__("Project Explorer", parent)
        self.setAllowedAreas(Qt.AllDockWidgetAreas)
        self.setMinimumWidth(180)

        self._project = None
        self._current_page_name = None
        self._dirty_pages = set()
        self._init_ui()

    def _init_ui(self):
        container = QWidget()
        layout = QVBoxLayout(container)
        layout.setContentsMargins(4, 4, 4, 4)
        layout.setSpacing(4)

        # Project settings group
        settings_group = QGroupBox("Project")
        # settings_group.setStyleSheet(_GROUPBOX_STYLE)
        settings_layout = QVBoxLayout(settings_group)
        settings_layout.setContentsMargins(4, 4, 4, 4)

        # Page mode selector
        mode_layout = QHBoxLayout()
        mode_layout.addWidget(QLabel("Mode:"))
        self._mode_combo = QComboBox()
        self._mode_combo.addItems(["easy_page", "activity"])
        self._mode_combo.currentTextChanged.connect(self._on_mode_changed)
        mode_layout.addWidget(self._mode_combo)
        settings_layout.addLayout(mode_layout)
        layout.addWidget(settings_group)

        # Page tree
        pages_label = QLabel("Pages")
        pages_label.setFont(QFont("", -1, QFont.Bold))
        layout.addWidget(pages_label)

        self._page_tree = QTreeWidget()
        self._page_tree.setHeaderHidden(True)
        self._page_tree.setContextMenuPolicy(Qt.CustomContextMenu)
        self._page_tree.customContextMenuRequested.connect(self._on_page_context_menu)
        self._page_tree.currentItemChanged.connect(self._on_page_item_changed)
        layout.addWidget(self._page_tree)

        # Add page button
        add_btn = QPushButton("+ New Page")
        add_btn.clicked.connect(self._on_add_page)
        layout.addWidget(add_btn)

        layout.addStretch()

        self.setWidget(container)

    # ── Public API ─────────────────────────────────────────────────

    def set_project(self, project):
        """Refresh the explorer from the given Project."""
        self._project = project
        self._mode_combo.blockSignals(True)
        if project:
            self._mode_combo.setCurrentText(project.page_mode)
        else:
            self._mode_combo.setCurrentIndex(0)
        self._mode_combo.blockSignals(False)
        self._rebuild_page_tree()
        self._rebuild_resource_tree()

    def set_current_page(self, page_name):
        """Highlight the current page in the tree."""
        self._current_page_name = page_name
        for i in range(self._page_tree.topLevelItemCount()):
            item = self._page_tree.topLevelItem(i)
            name = item.data(0, Qt.UserRole)
            font = item.font(0)
            font.setBold(name == page_name)
            item.setFont(0, font)

    def set_dirty_pages(self, page_names):
        self._dirty_pages = set(page_names or [])
        for i in range(self._page_tree.topLevelItemCount()):
            item = self._page_tree.topLevelItem(i)
            name = item.data(0, Qt.UserRole)
            if name:
                item.setText(0, self._page_item_text(name))

    # ── Internal ───────────────────────────────────────────────────

    def _rebuild_page_tree(self):
        self._page_tree.clear()
        if not self._project:
            return
        for page in self._project.pages:
            name = page.name
            item = QTreeWidgetItem([self._page_item_text(name)])
            item.setData(0, Qt.UserRole, name)
            if name == self._current_page_name:
                font = item.font(0)
                font.setBold(True)
                item.setFont(0, font)
            self._page_tree.addTopLevelItem(item)

    def _page_item_text(self, page_name):
        startup = self._project.startup_page if self._project else ""
        dirty_suffix = "*" if page_name in self._dirty_pages else ""
        return f"▶ {page_name}{dirty_suffix}" if page_name == startup else f"  {page_name}{dirty_suffix}"

    def _rebuild_resource_tree(self):
        # Resources are managed by the independent ResourcePanel dock
        pass

    def _on_page_item_changed(self, current, previous):
        if current is None:
            return
        name = current.data(0, Qt.UserRole)
        if name:
            self.page_selected.emit(name)

    def _on_page_context_menu(self, pos):
        item = self._page_tree.itemAt(pos)
        menu = QMenu(self)

        if item:
            name = item.data(0, Qt.UserRole)

            rename_act = QAction("Rename", self)
            rename_act.triggered.connect(lambda: self._rename_page(name))
            menu.addAction(rename_act)

            dup_act = QAction("Duplicate", self)
            dup_act.triggered.connect(lambda: self._duplicate_page(name))
            menu.addAction(dup_act)

            startup_act = QAction("Set as Startup Page", self)
            startup_act.triggered.connect(lambda: self._set_startup(name))
            menu.addAction(startup_act)

            menu.addSeparator()

            del_act = QAction("Delete", self)
            del_act.triggered.connect(lambda: self._delete_page(name))
            menu.addAction(del_act)
        else:
            add_act = QAction("New Page...", self)
            add_act.triggered.connect(self._on_add_page)
            menu.addAction(add_act)

        menu.exec_(self._page_tree.viewport().mapToGlobal(pos))

    def _on_add_page(self):
        name, ok = QInputDialog.getText(
            self, "New Page", "Page name (e.g. settings_page):"
        )
        if ok and name:
            # Sanitize: remove extension, replace spaces
            name = name.replace(" ", "_").replace(".xml", "")
            if name in _RESERVED_PAGE_NAMES:
                QMessageBox.warning(
                    self, "Reserved Name",
                    f"'{name}' is a reserved egui module name and cannot be used as a page name.\n"
                    f"Please choose a different name (e.g. '{name}_page')."
                )
                return
            if self._project and self._project.get_page_by_name(name):
                QMessageBox.warning(self, "Error", f"Page '{name}' already exists.")
                return
            self.page_added.emit(name)

    def _rename_page(self, old_name):
        new_name, ok = QInputDialog.getText(
            self, "Rename Page", "New name:", text=old_name
        )
        if ok and new_name and new_name != old_name:
            new_name = new_name.replace(" ", "_").replace(".xml", "")
            if new_name in _RESERVED_PAGE_NAMES:
                QMessageBox.warning(
                    self, "Reserved Name",
                    f"'{new_name}' is a reserved egui module name and cannot be used as a page name.\n"
                    f"Please choose a different name (e.g. '{new_name}_page')."
                )
                return
            if self._project and self._project.get_page_by_name(new_name):
                QMessageBox.warning(self, "Error", f"Page '{new_name}' already exists.")
                return
            self.page_renamed.emit(old_name, new_name)

    def _duplicate_page(self, name):
        new_name = f"{name}_copy"
        counter = 1
        while self._project and self._project.get_page_by_name(new_name):
            counter += 1
            new_name = f"{name}_copy{counter}"
        self.page_duplicated.emit(name, new_name)

    def _delete_page(self, name):
        if self._project and len(self._project.pages) <= 1:
            QMessageBox.warning(self, "Error", "Cannot delete the last page.")
            return
        reply = QMessageBox.question(
            self, "Delete Page",
            f"Delete page '{name}'? This cannot be undone.",
            QMessageBox.Yes | QMessageBox.No,
        )
        if reply == QMessageBox.Yes:
            self.page_removed.emit(name)

    def _set_startup(self, name):
        self.startup_changed.emit(name)

    def _on_mode_changed(self, mode):
        self.page_mode_changed.emit(mode)

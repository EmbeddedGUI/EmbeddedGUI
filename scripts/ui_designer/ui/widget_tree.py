"""Widget tree panel for EmbeddedGUI Designer."""

import re

from PyQt5.QtWidgets import (
    QWidget, QVBoxLayout, QTreeWidget, QTreeWidgetItem,
    QPushButton, QHBoxLayout, QMenu, QAction, QInputDialog, QAbstractItemView, QMessageBox, QLineEdit,
)
from PyQt5.QtCore import pyqtSignal, Qt, QItemSelectionModel

from ..model.widget_name import resolve_widget_name
from ..model.widget_model import WidgetModel
from ..model.widget_registry import WidgetRegistry


def _get_addable_types():
    """Get addable widget types from the registry."""
    return WidgetRegistry.instance().addable_types()


def _get_container_types():
    """Get container widget types from the registry."""
    return WidgetRegistry.instance().container_types()


class WidgetTreePanel(QWidget):
    """Tree view showing the widget hierarchy."""

    widget_selected = pyqtSignal(object)  # emits WidgetModel or None
    selection_changed = pyqtSignal(list, object)  # widgets, primary
    tree_changed = pyqtSignal()  # emits when tree structure changes
    feedback_message = pyqtSignal(str)  # emits user-facing status messages

    def __init__(self, parent=None):
        super().__init__(parent)
        self.project = None
        self._widget_map = {}  # QTreeWidgetItem -> WidgetModel
        self._item_map = {}  # widget id -> QTreeWidgetItem
        self._building = False
        self._syncing_selection = False
        self._expanded_widgets = set()
        self._suppress_expansion_tracking = False
        self._default_expand_next_rebuild = True
        self._init_ui()

    def _init_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(4, 4, 4, 4)

        # Button bar
        btn_layout = QHBoxLayout()
        self.add_btn = QPushButton("Add")
        self.add_btn.clicked.connect(self._on_add_clicked)
        self.del_btn = QPushButton("Delete")
        self.del_btn.clicked.connect(self._on_delete_clicked)
        self.expand_btn = QPushButton("Expand")
        self.expand_btn.clicked.connect(self._expand_all_items)
        self.collapse_btn = QPushButton("Collapse")
        self.collapse_btn.clicked.connect(self._collapse_all_items)
        btn_layout.addWidget(self.add_btn)
        btn_layout.addWidget(self.del_btn)
        btn_layout.addWidget(self.expand_btn)
        btn_layout.addWidget(self.collapse_btn)
        layout.addLayout(btn_layout)

        self.filter_edit = QLineEdit()
        self.filter_edit.setPlaceholderText("Filter widgets by name or type")
        self.filter_edit.textChanged.connect(self._apply_tree_filter)
        layout.addWidget(self.filter_edit)

        # Tree
        self.tree = QTreeWidget()
        self.tree.setHeaderLabels(["Widget", "Type"])
        self.tree.setColumnWidth(0, 140)
        self.tree.setSelectionMode(QAbstractItemView.ExtendedSelection)
        self.tree.setContextMenuPolicy(Qt.CustomContextMenu)
        self.tree.customContextMenuRequested.connect(self._on_context_menu)
        self.tree.itemSelectionChanged.connect(self._on_selection_changed)
        self.tree.itemExpanded.connect(self._on_item_expanded)
        self.tree.itemCollapsed.connect(self._on_item_collapsed)
        layout.addWidget(self.tree)

    def set_project(self, project):
        self.project = project
        self._expanded_widgets = set()
        self._default_expand_next_rebuild = True
        self.rebuild_tree()

    def rebuild_tree(self):
        self._expanded_widgets = self._collect_expanded_widget_ids()
        default_expand = self._default_expand_next_rebuild
        self._default_expand_next_rebuild = False
        self._building = True
        self.tree.clear()
        self._widget_map = {}
        self._item_map = {}
        if self.project:
            for root_widget in self.project.root_widgets:
                self._add_widget_to_tree(root_widget, None)
        self._building = False
        self._apply_tree_filter(default_expand=default_expand)

    def _add_widget_to_tree(self, widget, parent_item):
        item = QTreeWidgetItem()
        item.setText(0, self._display_name(widget))
        item.setText(1, widget.widget_type)
        self._widget_map[id(item)] = widget
        self._item_map[id(widget)] = item

        if parent_item is None:
            self.tree.addTopLevelItem(item)
        else:
            parent_item.addChild(item)

        for child in widget.children:
            self._add_widget_to_tree(child, item)

        return item

    def _display_name(self, widget):
        prefix = []
        if getattr(widget, "designer_locked", False):
            prefix.append("[L]")
        if getattr(widget, "designer_hidden", False):
            prefix.append("[H]")
        if prefix:
            return f"{' '.join(prefix)} {widget.name}"
        return widget.name

    def _on_selection_changed(self):
        if self._building or self._syncing_selection:
            return
        widgets = self.selected_widgets()
        primary = self._widget_map.get(id(self.tree.currentItem())) if self.tree.currentItem() else None
        if primary is None and widgets:
            primary = widgets[-1]
        self.widget_selected.emit(primary)
        self.selection_changed.emit(widgets, primary)

    def _get_selected_widget(self):
        item = self.tree.currentItem()
        if item is None:
            return None
        return self._widget_map.get(id(item))

    def selected_widgets(self):
        widgets = []
        for item in self.tree.selectedItems():
            widget = self._widget_map.get(id(item))
            if widget is not None:
                widgets.append(widget)
        return widgets

    def set_selected_widgets(self, widgets, primary=None):
        self._syncing_selection = True
        try:
            self.tree.clearSelection()
            widgets = [widget for widget in (widgets or []) if widget is not None]
            current_item = None
            for widget in widgets:
                item = self._item_map.get(id(widget))
                if item is not None:
                    item.setSelected(True)
            if primary is not None:
                item = self._item_map.get(id(primary))
                if item is not None:
                    self.tree.setCurrentItem(item, 0, QItemSelectionModel.NoUpdate)
                    current_item = item
            elif widgets:
                item = self._item_map.get(id(widgets[-1]))
                if item is not None:
                    self.tree.setCurrentItem(item, 0, QItemSelectionModel.NoUpdate)
                    current_item = item
            else:
                self.tree.setCurrentItem(None)
            if current_item is not None:
                self._reveal_item(current_item)
        finally:
            self._syncing_selection = False

    def _iter_widgets(self):
        if not self.project:
            return
        for root_widget in self.project.root_widgets:
            yield root_widget
            yield from root_widget.get_all_widgets_flat()[1:]

    def _existing_widget_names(self, exclude_widget=None):
        names = set()
        for widget in self._iter_widgets() or []:
            if widget is exclude_widget:
                continue
            if widget.name:
                names.add(widget.name)
        return names

    def _make_unique_widget_name(self, base_name, exclude_widget=None):
        candidate = (base_name or "").strip().replace(" ", "_")
        if not candidate:
            return ""
        existing = self._existing_widget_names(exclude_widget=exclude_widget)
        if candidate not in existing:
            return candidate

        match = re.match(r"^(.*?)(?:_(\d+))?$", candidate)
        stem = candidate
        suffix = 2
        if match:
            stem = match.group(1) or candidate
            if match.group(2):
                suffix = int(match.group(2)) + 1

        while f"{stem}_{suffix}" in existing:
            suffix += 1
        return f"{stem}_{suffix}"

    def _on_add_clicked(self):
        menu = QMenu(self)
        for display_name, type_name in _get_addable_types():
            action = QAction(display_name, self)
            action.setData(type_name)
            action.triggered.connect(lambda checked, t=type_name: self._add_widget(t))
            menu.addAction(action)
        menu.exec_(self.add_btn.mapToGlobal(self.add_btn.rect().bottomLeft()))

    def _add_widget(self, widget_type):
        if not self.project:
            return

        widget = WidgetModel(widget_type)
        widget.name = self._make_unique_widget_name(widget.name)

        # Find selected container to add to
        selected = self._get_selected_widget()
        if selected and selected.is_container:
            selected.add_child(widget)
        elif selected and selected.parent and selected.parent.is_container:
            selected.parent.add_child(widget)
        elif self.project.root_widgets:
            # Add to first root if it's a container
            root = self.project.root_widgets[0]
            if root.is_container:
                root.add_child(widget)
            else:
                self.project.root_widgets.append(widget)
        else:
            self.project.root_widgets.append(widget)

        self.rebuild_tree()
        self.tree_changed.emit()

    def _on_delete_clicked(self):
        widgets = self.selected_widgets()
        if not widgets:
            return

        deletable = [widget for widget in widgets if not getattr(widget, "designer_locked", False)]
        locked_count = len(widgets) - len(deletable)
        if not deletable:
            if locked_count:
                self.feedback_message.emit(f"Cannot delete selection: {self._locked_widget_summary(locked_count)}.")
            return

        deleted_count = 0
        for widget in self._top_level_selected_widgets(deletable):
            if widget.parent:
                widget.parent.remove_child(widget)
            elif widget in self.project.root_widgets:
                self.project.root_widgets.remove(widget)
            deleted_count += 1

        self.rebuild_tree()
        self.tree_changed.emit()
        if locked_count:
            self.feedback_message.emit(f"Deleted {deleted_count} widget(s); skipped {self._locked_widget_summary(locked_count)}")

    def _on_context_menu(self, pos):
        item = self.tree.itemAt(pos)
        if item is None:
            return

        widget = self._widget_map.get(id(item))
        if widget is None:
            return

        menu = QMenu(self)

        # Rename
        rename_action = QAction("Rename", self)
        rename_action.triggered.connect(lambda: self._rename_widget(widget))
        menu.addAction(rename_action)

        # Add child (if container)
        if widget.is_container:
            add_menu = menu.addMenu("Add Child")
            for display_name, type_name in _get_addable_types():
                action = QAction(display_name, self)
                action.triggered.connect(
                    lambda checked, w=widget, t=type_name: self._add_child_to(w, t)
                )
                add_menu.addAction(action)

        # Delete
        del_action = QAction("Delete", self)
        del_action.triggered.connect(lambda: self._delete_widget(widget))
        menu.addAction(del_action)

        menu.exec_(self.tree.viewport().mapToGlobal(pos))

    def _rename_widget(self, widget):
        new_name, ok = QInputDialog.getText(
            self, "Rename Widget", "New name:", text=widget.name
        )
        if ok and new_name:
            valid, resolved_name, message = resolve_widget_name(widget, new_name)
            if not valid:
                QMessageBox.warning(self, "Invalid Widget Name", message)
                return
            widget.name = resolved_name
            self.rebuild_tree()
            self.tree_changed.emit()

    def _add_child_to(self, parent, widget_type):
        child = WidgetModel(widget_type)
        child.name = self._make_unique_widget_name(child.name)
        parent.add_child(child)
        self.rebuild_tree()
        self.tree_changed.emit()

    def _delete_widget(self, widget):
        if getattr(widget, "designer_locked", False):
            self.feedback_message.emit(f"Cannot delete widget: {widget.name} is locked.")
            return
        if widget.parent:
            widget.parent.remove_child(widget)
        elif widget in self.project.root_widgets:
            self.project.root_widgets.remove(widget)
        self.rebuild_tree()
        self.tree_changed.emit()

    def _top_level_selected_widgets(self, widgets):
        selected_ids = {id(widget) for widget in widgets}
        result = []
        for widget in widgets:
            parent = widget.parent
            skip = False
            while parent is not None:
                if id(parent) in selected_ids:
                    skip = True
                    break
                parent = parent.parent
            if not skip:
                result.append(widget)
        return result

    def _expand_all_items(self):
        self._suppress_expansion_tracking = True
        try:
            self.tree.expandAll()
        finally:
            self._suppress_expansion_tracking = False
        self._expanded_widgets = self._collect_expanded_widget_ids()

    def _collapse_all_items(self):
        self._suppress_expansion_tracking = True
        try:
            self.tree.collapseAll()
        finally:
            self._suppress_expansion_tracking = False
        self._expanded_widgets = set()

    def _apply_tree_filter(self, _text="", default_expand=False):
        query = self.filter_edit.text().strip().lower()
        self._suppress_expansion_tracking = True
        try:
            for index in range(self.tree.topLevelItemCount()):
                item = self.tree.topLevelItem(index)
                if query:
                    self._apply_filter_to_item(item, query)
                else:
                    self._clear_item_filter(item)
            if not query:
                if default_expand:
                    self.tree.expandAll()
                    self._expanded_widgets = self._collect_expanded_widget_ids()
                else:
                    for index in range(self.tree.topLevelItemCount()):
                        self._restore_item_expansion(self.tree.topLevelItem(index))
        finally:
            self._suppress_expansion_tracking = False

    def _apply_filter_to_item(self, item, query):
        widget = self._widget_map.get(id(item))
        name = (widget.name if widget is not None else item.text(0)).lower()
        type_name = (widget.widget_type if widget is not None else item.text(1)).lower()
        own_match = query in name or query in type_name
        child_match = False
        for index in range(item.childCount()):
            child = item.child(index)
            if self._apply_filter_to_item(child, query):
                child_match = True
        visible = own_match or child_match
        item.setHidden(not visible)
        if visible and item.childCount():
            item.setExpanded(child_match)
        return visible

    def _clear_item_filter(self, item):
        item.setHidden(False)
        for index in range(item.childCount()):
            self._clear_item_filter(item.child(index))

    def _restore_item_expansion(self, item):
        widget = self._widget_map.get(id(item))
        item.setExpanded(widget is not None and id(widget) in self._expanded_widgets)
        for index in range(item.childCount()):
            self._restore_item_expansion(item.child(index))

    def _collect_expanded_widget_ids(self):
        expanded = set()
        for index in range(self.tree.topLevelItemCount()):
            self._collect_expanded_widget_ids_from_item(self.tree.topLevelItem(index), expanded)
        return expanded

    def _collect_expanded_widget_ids_from_item(self, item, expanded):
        widget = self._widget_map.get(id(item))
        if widget is not None and item.isExpanded():
            expanded.add(id(widget))
        for index in range(item.childCount()):
            self._collect_expanded_widget_ids_from_item(item.child(index), expanded)

    def _on_item_expanded(self, item):
        if self._building or self._suppress_expansion_tracking or self.filter_edit.text().strip():
            return
        widget = self._widget_map.get(id(item))
        if widget is not None:
            self._expanded_widgets.add(id(widget))

    def _on_item_collapsed(self, item):
        if self._building or self._suppress_expansion_tracking or self.filter_edit.text().strip():
            return
        widget = self._widget_map.get(id(item))
        if widget is not None:
            self._expanded_widgets.discard(id(widget))

    def _reveal_item(self, item):
        parent = item.parent()
        while parent is not None:
            parent.setExpanded(True)
            parent = parent.parent()
        self.tree.scrollToItem(item, QAbstractItemView.PositionAtCenter)

    def _locked_widget_summary(self, count):
        noun = "widget" if count == 1 else "widgets"
        return f"{count} locked {noun}"

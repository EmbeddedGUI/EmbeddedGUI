"""Editor panel for page-level user fields."""

from __future__ import annotations

from PyQt5.QtCore import Qt, pyqtSignal
from PyQt5.QtWidgets import (
    QAbstractItemView,
    QHBoxLayout,
    QHeaderView,
    QLabel,
    QPushButton,
    QTableWidget,
    QTableWidgetItem,
    QVBoxLayout,
    QWidget,
)

from ..model.page_fields import COMMON_PAGE_FIELD_TYPES, normalize_page_fields, suggest_page_field_name, validate_page_fields


class PageFieldsPanel(QWidget):
    """Editable list of page-level fields stored in Page.user_fields."""

    fields_changed = pyqtSignal(list)
    validation_message = pyqtSignal(str)
    user_code_section_requested = pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self._page = None
        self._fields = []
        self._updating = False
        self._init_ui()
        self.clear()

    def _init_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(8, 8, 8, 8)
        layout.setSpacing(6)

        self._summary_label = QLabel("")
        self._hint_label = QLabel(
            "Page fields become generated struct members. Default is treated as a raw C expression and is applied on page open."
        )
        self._hint_label.setWordWrap(True)

        self._code_hint_label = QLabel("Page lifecycle hooks live in {page}.c. Open a section to edit page-level setup and teardown logic.")
        self._code_hint_label.setWordWrap(True)

        code_buttons = QHBoxLayout()
        code_buttons.setContentsMargins(0, 0, 0, 0)
        code_buttons.setSpacing(6)
        self._open_on_open_button = QPushButton("Open on_open")
        self._open_on_close_button = QPushButton("Open on_close")
        self._open_init_button = QPushButton("Open init")
        self._open_on_open_button.clicked.connect(lambda: self._request_section("on_open"))
        self._open_on_close_button.clicked.connect(lambda: self._request_section("on_close"))
        self._open_init_button.clicked.connect(lambda: self._request_section("init"))
        code_buttons.addWidget(self._open_on_open_button)
        code_buttons.addWidget(self._open_on_close_button)
        code_buttons.addWidget(self._open_init_button)
        code_buttons.addStretch(1)

        self._table = QTableWidget(0, 3, self)
        self._table.setHorizontalHeaderLabels(["Name", "Type", "Default"])
        self._table.setSelectionBehavior(QAbstractItemView.SelectRows)
        self._table.setSelectionMode(QAbstractItemView.SingleSelection)
        self._table.verticalHeader().setVisible(False)
        self._table.horizontalHeader().setSectionResizeMode(0, QHeaderView.Stretch)
        self._table.horizontalHeader().setSectionResizeMode(1, QHeaderView.ResizeToContents)
        self._table.horizontalHeader().setSectionResizeMode(2, QHeaderView.Stretch)
        self._table.itemChanged.connect(self._on_item_changed)
        self._table.itemSelectionChanged.connect(self._update_actions)

        buttons = QHBoxLayout()
        buttons.setContentsMargins(0, 0, 0, 0)
        buttons.setSpacing(6)
        self._add_button = QPushButton("Add Field")
        self._remove_button = QPushButton("Remove Field")
        self._add_button.clicked.connect(self._on_add_field)
        self._remove_button.clicked.connect(self._on_remove_field)
        buttons.addWidget(self._add_button)
        buttons.addWidget(self._remove_button)
        buttons.addStretch(1)

        layout.addWidget(self._summary_label)
        layout.addWidget(self._hint_label)
        layout.addWidget(self._code_hint_label)
        layout.addLayout(code_buttons)
        layout.addWidget(self._table, 1)
        layout.addLayout(buttons)

    def clear(self):
        self.set_page(None)

    def set_page(self, page):
        self._page = page
        self._fields = normalize_page_fields(getattr(page, "user_fields", []))
        self._rebuild_table()

    def _rebuild_table(self):
        self._updating = True
        self._table.setRowCount(len(self._fields))
        for row, field in enumerate(self._fields):
            self._table.setItem(row, 0, QTableWidgetItem(field.get("name", "")))
            self._table.setItem(row, 1, QTableWidgetItem(field.get("type", "")))
            self._table.setItem(row, 2, QTableWidgetItem(field.get("default", "")))
        self._updating = False
        self._update_summary()
        self._update_actions()

    def _update_summary(self):
        if self._page is None:
            self._summary_label.setText("Page Fields: no active page")
            return
        count = len(self._fields)
        noun = "field" if count == 1 else "fields"
        self._summary_label.setText(f"Page Fields: {count} {noun} on {self._page.name}")

    def _update_actions(self):
        has_page = self._page is not None
        has_selection = bool(self._table.selectionModel() and self._table.selectionModel().selectedRows())
        self._table.setEnabled(has_page)
        self._add_button.setEnabled(has_page)
        self._remove_button.setEnabled(has_page and has_selection)
        self._open_on_open_button.setEnabled(has_page)
        self._open_on_close_button.setEnabled(has_page)
        self._open_init_button.setEnabled(has_page)

    def _table_fields(self):
        fields = []
        for row in range(self._table.rowCount()):
            name_item = self._table.item(row, 0)
            type_item = self._table.item(row, 1)
            default_item = self._table.item(row, 2)
            fields.append(
                {
                    "name": name_item.text() if name_item is not None else "",
                    "type": type_item.text() if type_item is not None else "",
                    "default": default_item.text() if default_item is not None else "",
                }
            )
        return fields

    def _commit_table_fields(self):
        fields = self._table_fields()
        ok, normalized_fields, message = validate_page_fields(self._page, fields)
        if not ok:
            self.validation_message.emit(message)
            self._rebuild_table()
            return

        normalization_changed = normalized_fields != fields
        changed = normalized_fields != self._fields
        self._fields = normalized_fields

        if normalization_changed:
            self._rebuild_table()

        self._update_summary()
        if changed:
            self.fields_changed.emit(list(self._fields))

    def _on_item_changed(self, item):
        del item
        if self._updating or self._page is None:
            return
        self._commit_table_fields()

    def _on_add_field(self):
        if self._page is None:
            return
        field_name = suggest_page_field_name(self._page, self._fields, base_name="field")
        default_value = "0"
        field_type = COMMON_PAGE_FIELD_TYPES[0] if COMMON_PAGE_FIELD_TYPES else "int"
        self._fields = self._fields + [{"name": field_name, "type": field_type, "default": default_value}]
        self._rebuild_table()
        self.fields_changed.emit(list(self._fields))
        row = len(self._fields) - 1
        if row >= 0:
            self._table.selectRow(row)

    def _on_remove_field(self):
        if self._page is None:
            return
        selected_rows = self._table.selectionModel().selectedRows() if self._table.selectionModel() else []
        if not selected_rows:
            return
        row = selected_rows[0].row()
        self._fields = [field for index, field in enumerate(self._fields) if index != row]
        self._rebuild_table()
        self.fields_changed.emit(list(self._fields))

    def _request_section(self, section_name):
        if self._page is None or not section_name:
            return
        self.user_code_section_requested.emit(section_name)

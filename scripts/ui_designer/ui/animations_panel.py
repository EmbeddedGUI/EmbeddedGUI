"""Editor panel for widget animations."""

from __future__ import annotations

from PyQt5.QtCore import Qt, pyqtSignal
from PyQt5.QtWidgets import (
    QAbstractItemView,
    QCheckBox,
    QComboBox,
    QFormLayout,
    QGroupBox,
    QHeaderView,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QPushButton,
    QSpinBox,
    QTableWidget,
    QTableWidgetItem,
    QVBoxLayout,
    QWidget,
)

from ..model.widget_animations import (
    ANIMATION_REPEAT_MODES,
    animation_interpolator_names,
    animation_param_choices,
    animation_param_defaults,
    animation_type_names,
    clone_animation,
    create_default_animation,
    normalize_widget_animations,
)


class AnimationsPanel(QWidget):
    """Editable list of animations for the currently selected widget."""

    animations_changed = pyqtSignal(list)

    def __init__(self, parent=None):
        super().__init__(parent)
        self._selection = []
        self._primary_widget = None
        self._animations = []
        self._updating = False
        self._init_ui()
        self.clear()

    def _init_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(8, 8, 8, 8)
        layout.setSpacing(6)

        self._summary_label = QLabel("")
        self._hint_label = QLabel(
            "Animations are stored on the selected widget and compiled into the generated layout source."
        )
        self._hint_label.setWordWrap(True)

        self._table = QTableWidget(0, 4, self)
        self._table.setHorizontalHeaderLabels(["Type", "Duration", "Interpolator", "Auto Start"])
        self._table.setSelectionBehavior(QAbstractItemView.SelectRows)
        self._table.setSelectionMode(QAbstractItemView.SingleSelection)
        self._table.setEditTriggers(QAbstractItemView.NoEditTriggers)
        self._table.verticalHeader().setVisible(False)
        self._table.horizontalHeader().setSectionResizeMode(0, QHeaderView.ResizeToContents)
        self._table.horizontalHeader().setSectionResizeMode(1, QHeaderView.ResizeToContents)
        self._table.horizontalHeader().setSectionResizeMode(2, QHeaderView.Stretch)
        self._table.horizontalHeader().setSectionResizeMode(3, QHeaderView.ResizeToContents)
        self._table.itemSelectionChanged.connect(self._on_row_selected)

        buttons = QHBoxLayout()
        buttons.setContentsMargins(0, 0, 0, 0)
        buttons.setSpacing(6)
        self._add_button = QPushButton("Add Animation")
        self._duplicate_button = QPushButton("Duplicate")
        self._remove_button = QPushButton("Remove")
        self._add_button.clicked.connect(self._on_add_animation)
        self._duplicate_button.clicked.connect(self._on_duplicate_animation)
        self._remove_button.clicked.connect(self._on_remove_animation)
        buttons.addWidget(self._add_button)
        buttons.addWidget(self._duplicate_button)
        buttons.addWidget(self._remove_button)
        buttons.addStretch(1)

        self._detail_group = QGroupBox("Selected Animation")
        self._detail_form = QFormLayout()
        self._detail_group.setLayout(self._detail_form)

        layout.addWidget(self._summary_label)
        layout.addWidget(self._hint_label)
        layout.addWidget(self._table, 1)
        layout.addLayout(buttons)
        layout.addWidget(self._detail_group)

    def clear(self):
        self.set_selection([], primary=None)

    def set_selection(self, widgets, primary=None):
        widgets = [widget for widget in (widgets or []) if widget is not None]
        self._selection = list(widgets)
        if primary is None or all(widget is not primary for widget in widgets):
            primary = widgets[-1] if widgets else None
        self._primary_widget = primary if len(widgets) == 1 else None
        selected_row = self._selected_row()
        self._animations = normalize_widget_animations(getattr(self._primary_widget, "animations", []))
        self._rebuild_table(selected_row=selected_row)

    def refresh(self):
        self.set_selection(self._selection, primary=self._primary_widget)

    def _selected_row(self):
        selection_model = self._table.selectionModel()
        if selection_model is None:
            return -1
        rows = selection_model.selectedRows()
        if not rows:
            return -1
        return rows[0].row()

    def _clear_form_layout(self):
        while self._detail_form.rowCount():
            self._detail_form.removeRow(0)

    def _rebuild_table(self, selected_row=-1):
        self._updating = True
        self._table.setRowCount(len(self._animations))
        for row, animation in enumerate(self._animations):
            values = [
                animation.anim_type,
                str(animation.duration),
                animation.interpolator,
                "Yes" if animation.auto_start else "No",
            ]
            for column, value in enumerate(values):
                item = QTableWidgetItem(value)
                item.setFlags(item.flags() & ~Qt.ItemIsEditable)
                self._table.setItem(row, column, item)
        self._updating = False

        if self._primary_widget is not None and self._animations:
            row = selected_row if 0 <= selected_row < len(self._animations) else 0
            self._table.selectRow(row)
        self._update_summary()
        self._update_actions()
        self._rebuild_detail_form()

    def _update_summary(self):
        if not self._selection:
            self._summary_label.setText("Animations: no widget selected")
            return
        if len(self._selection) > 1 or self._primary_widget is None:
            self._summary_label.setText(f"Animations: select a single widget ({len(self._selection)} selected)")
            return

        count = len(self._animations)
        noun = "animation" if count == 1 else "animations"
        self._summary_label.setText(
            f"Animations: {count} {noun} on {self._primary_widget.widget_type} {self._primary_widget.name}"
        )

    def _update_actions(self):
        has_widget = self._primary_widget is not None
        has_selection = self._selected_row() >= 0
        self._table.setEnabled(has_widget)
        self._detail_group.setEnabled(has_widget and has_selection)
        self._add_button.setEnabled(has_widget)
        self._duplicate_button.setEnabled(has_widget and has_selection)
        self._remove_button.setEnabled(has_widget and has_selection)

    def _rebuild_detail_form(self):
        self._clear_form_layout()

        if not self._selection:
            self._detail_form.addRow(QLabel("Select a widget to edit animations."))
            return

        if len(self._selection) > 1 or self._primary_widget is None:
            self._detail_form.addRow(QLabel("Animation editing is available for a single selected widget only."))
            return

        row = self._selected_row()
        if row < 0 or row >= len(self._animations):
            if self._animations:
                self._detail_form.addRow(QLabel("Select an animation from the table above."))
            else:
                self._detail_form.addRow(QLabel("No animations on the selected widget."))
            return

        animation = self._animations[row]

        type_combo = QComboBox()
        type_combo.addItems(animation_type_names())
        type_combo.setCurrentText(animation.anim_type)
        type_combo.currentTextChanged.connect(lambda value, index=row: self._on_type_changed(index, value))
        self._detail_form.addRow("Type:", type_combo)

        duration_spin = QSpinBox()
        duration_spin.setRange(0, 600000)
        duration_spin.setValue(animation.duration)
        duration_spin.valueChanged.connect(lambda value, index=row: self._on_duration_changed(index, value))
        self._detail_form.addRow("Duration (ms):", duration_spin)

        interpolator_combo = QComboBox()
        interpolator_combo.addItems(animation_interpolator_names())
        interpolator_combo.setCurrentText(animation.interpolator)
        interpolator_combo.currentTextChanged.connect(lambda value, index=row: self._on_interpolator_changed(index, value))
        self._detail_form.addRow("Interpolator:", interpolator_combo)

        repeat_count_spin = QSpinBox()
        repeat_count_spin.setRange(0, 9999)
        repeat_count_spin.setValue(animation.repeat_count)
        repeat_count_spin.valueChanged.connect(lambda value, index=row: self._on_repeat_count_changed(index, value))
        self._detail_form.addRow("Repeat Count:", repeat_count_spin)

        repeat_mode_combo = QComboBox()
        repeat_mode_combo.addItems(ANIMATION_REPEAT_MODES)
        repeat_mode_combo.setCurrentText(animation.repeat_mode)
        repeat_mode_combo.currentTextChanged.connect(lambda value, index=row: self._on_repeat_mode_changed(index, value))
        self._detail_form.addRow("Repeat Mode:", repeat_mode_combo)

        auto_start_check = QCheckBox("Start automatically")
        auto_start_check.setChecked(animation.auto_start)
        auto_start_check.toggled.connect(lambda value, index=row: self._on_auto_start_changed(index, value))
        self._detail_form.addRow(auto_start_check)

        for param_name, default_value in animation_param_defaults(animation.anim_type).items():
            editor = self._create_param_editor(row, param_name, animation.params.get(param_name, default_value))
            self._detail_form.addRow(self._label_for_param(param_name), editor)

    def _label_for_param(self, param_name):
        name = str(param_name or "")
        if name.startswith("from_"):
            return "From " + name[len("from_"):].replace("_", " ").title() + ":"
        if name.startswith("to_"):
            return "To " + name[len("to_"):].replace("_", " ").title() + ":"
        return name.replace("_", " ").title() + ":"

    def _create_param_editor(self, row, param_name, value):
        choices = animation_param_choices(param_name)
        if choices:
            editor = QComboBox()
            editor.setEditable(param_name != "mode")
            editor.addItems(choices)
            if editor.findText(str(value)) < 0:
                editor.addItem(str(value))
            editor.setCurrentText(str(value))
            editor.currentTextChanged.connect(lambda text, index=row, key=param_name: self._on_param_changed(index, key, text))
            return editor

        editor = QLineEdit()
        editor.setText(str(value))
        editor.editingFinished.connect(lambda index=row, key=param_name, line_edit=editor: self._on_param_changed(index, key, line_edit.text()))
        return editor

    def _emit_changed(self):
        if self._primary_widget is None:
            return
        self.animations_changed.emit([clone_animation(animation) for animation in self._animations])

    def _on_row_selected(self):
        if self._updating:
            return
        self._update_actions()
        self._rebuild_detail_form()

    def _on_add_animation(self):
        if self._primary_widget is None:
            return
        self._animations.append(create_default_animation())
        self._rebuild_table(selected_row=len(self._animations) - 1)
        self._emit_changed()

    def _on_duplicate_animation(self):
        row = self._selected_row()
        if self._primary_widget is None or row < 0 or row >= len(self._animations):
            return
        self._animations.insert(row + 1, clone_animation(self._animations[row]))
        self._rebuild_table(selected_row=row + 1)
        self._emit_changed()

    def _on_remove_animation(self):
        row = self._selected_row()
        if self._primary_widget is None or row < 0 or row >= len(self._animations):
            return
        del self._animations[row]
        next_row = min(row, len(self._animations) - 1)
        self._rebuild_table(selected_row=next_row)
        self._emit_changed()

    def _on_type_changed(self, row, anim_type):
        if self._updating or row < 0 or row >= len(self._animations):
            return
        current = self._animations[row]
        replacement = create_default_animation(anim_type)
        replacement.duration = current.duration
        replacement.interpolator = current.interpolator
        replacement.repeat_count = current.repeat_count
        replacement.repeat_mode = current.repeat_mode
        replacement.auto_start = current.auto_start
        self._animations[row] = replacement
        self._rebuild_table(selected_row=row)
        self._emit_changed()

    def _on_duration_changed(self, row, value):
        if row < 0 or row >= len(self._animations):
            return
        self._animations[row].duration = int(value)
        self._rebuild_table(selected_row=row)
        self._emit_changed()

    def _on_interpolator_changed(self, row, value):
        if row < 0 or row >= len(self._animations):
            return
        self._animations[row].interpolator = str(value)
        self._rebuild_table(selected_row=row)
        self._emit_changed()

    def _on_repeat_count_changed(self, row, value):
        if row < 0 or row >= len(self._animations):
            return
        self._animations[row].repeat_count = int(value)
        self._emit_changed()

    def _on_repeat_mode_changed(self, row, value):
        if row < 0 or row >= len(self._animations):
            return
        self._animations[row].repeat_mode = str(value)
        self._emit_changed()

    def _on_auto_start_changed(self, row, value):
        if row < 0 or row >= len(self._animations):
            return
        self._animations[row].auto_start = bool(value)
        self._rebuild_table(selected_row=row)
        self._emit_changed()

    def _on_param_changed(self, row, param_name, value):
        if row < 0 or row >= len(self._animations):
            return
        default_value = animation_param_defaults(self._animations[row].anim_type).get(param_name, "0")
        text = str(value).strip() or str(default_value)
        self._animations[row].params[param_name] = text
        self._emit_changed()

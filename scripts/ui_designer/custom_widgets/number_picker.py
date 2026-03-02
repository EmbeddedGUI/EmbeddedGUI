"""NumberPicker widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="number_picker",
    descriptor={
        "c_type": "egui_view_number_picker_t",
        "init_func": "egui_view_number_picker_init_with_params",
        "params_macro": "EGUI_VIEW_NUMBER_PICKER_PARAMS_INIT",
        "params_type": "egui_view_number_picker_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "value": {
                "type": "int", "default": 0, "min": -9999, "max": 9999,
                "code_gen": {"kind": "setter", "func": "egui_view_number_picker_set_value"},
            },
            "min_value": {
                "type": "int", "default": -9999, "min": -9999, "max": 9999,
                "ui_group": "range",
                "code_gen": {"kind": "multi_setter",
                             "func": "egui_view_number_picker_set_range",
                             "args": "{min_value}, {max_value}",
                             "group": "range"},
            },
            "max_value": {
                "type": "int", "default": 9999, "min": -9999, "max": 9999,
                "ui_group": "range",
                "code_gen": None,
            },
            "step": {
                "type": "int", "default": 1, "min": 1, "max": 1000,
                "code_gen": {"kind": "setter",
                             "func": "egui_view_number_picker_set_step",
                             "skip_default": True},
            },
        },
        "events": {
            "onValueChanged": {
                "setter": "egui_view_number_picker_set_on_value_changed_listener",
                "signature": "void {func_name}(egui_view_t *self, int16_t value)",
            },
        },
    },
    xml_tag="NumberPicker",
    display_name="NumberPicker",
)

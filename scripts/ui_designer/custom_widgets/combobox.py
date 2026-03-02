"""Combobox widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="combobox",
    descriptor={
        "c_type": "egui_view_combobox_t",
        "init_func": "egui_view_combobox_init_with_params",
        "params_macro": "EGUI_VIEW_COMBOBOX_PARAMS_INIT",
        "params_type": "egui_view_combobox_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        # items/item_count are pointer-based, set in user C code
        "properties": {
            "current_index": {
                "type": "int", "default": 0, "min": 0, "max": 255,
                "code_gen": {"kind": "setter", "func": "egui_view_combobox_set_current_index",
                             "skip_default": True},
            },
        },
        "events": {
            "onSelected": {
                "setter": "egui_view_combobox_set_on_selected_listener",
                "signature": "void {func_name}(egui_view_t *self, uint8_t index)",
            },
        },
    },
    xml_tag="Combobox",
    display_name="Combobox",
)

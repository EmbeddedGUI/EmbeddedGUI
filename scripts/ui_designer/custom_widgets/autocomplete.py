"""Autocomplete widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="autocomplete",
    descriptor={
        "c_type": "egui_view_autocomplete_t",
        "init_func": "egui_view_autocomplete_init_with_params",
        "params_macro": "EGUI_VIEW_AUTOCOMPLETE_PARAMS_INIT",
        "params_type": "egui_view_autocomplete_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "current_index": {
                "type": "int",
                "default": 0,
                "min": 0,
                "max": 30,
                "code_gen": {
                    "kind": "setter",
                    "func": "egui_view_autocomplete_set_current_index",
                    "skip_default": True,
                },
            },
            "max_visible_items": {
                "type": "int",
                "default": 5,
                "min": 1,
                "max": 20,
                "code_gen": {
                    "kind": "setter",
                    "func": "egui_view_autocomplete_set_max_visible_items",
                    "skip_default": True,
                },
            },
        },
        "events": {
            "onSelected": {
                "setter": "egui_view_autocomplete_set_on_selected_listener",
                "signature": "void {func_name}(egui_view_t *self, uint8_t index)",
            },
        },
    },
    xml_tag="Autocomplete",
    display_name="Autocomplete",
)

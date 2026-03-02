"""ToggleButton widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="toggle_button",
    descriptor={
        "c_type": "egui_view_toggle_button_t",
        "init_func": "egui_view_toggle_button_init_with_params",
        "params_macro": "EGUI_VIEW_TOGGLE_BUTTON_PARAMS_INIT",
        "params_type": "egui_view_toggle_button_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "text": {
                "type": "string", "default": "Toggle",
                "code_gen": {"kind": "text_setter", "func": "egui_view_toggle_button_set_text"},
            },
            "is_toggled": {
                "type": "bool", "default": False,
                "code_gen": {"kind": "setter", "func": "egui_view_toggle_button_set_toggled",
                             "bool_to_int": True, "skip_default": True},
            },
        },
        "events": {
            "onToggled": {
                "setter": "egui_view_toggle_button_set_on_toggled_listener",
                "signature": "void {func_name}(egui_view_t *self, uint8_t is_toggled)",
            },
        },
    },
    xml_tag="ToggleButton",
    display_name="ToggleButton",
)

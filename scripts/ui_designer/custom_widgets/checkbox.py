"""Checkbox widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="checkbox",
    descriptor={
        "c_type": "egui_view_checkbox_t",
        "init_func": "egui_view_checkbox_init_with_params",
        "params_macro": "EGUI_VIEW_CHECKBOX_PARAMS_INIT",
        "params_type": "egui_view_checkbox_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "text": {
                "type": "string", "default": "",
                "code_gen": {"kind": "text_setter", "func": "egui_view_checkbox_set_text"},
            },
            "text_color": {
                "type": "color", "default": "EGUI_THEME_TEXT",
                "code_gen": {"kind": "setter", "func": "egui_view_checkbox_set_text_color"},
            },
            "is_checked": {
                "type": "bool", "default": False,
                "code_gen": {"kind": "setter", "func": "egui_view_checkbox_set_checked",
                             "bool_to_int": True, "skip_default": True},
            },
        },
        "events": {
            "onCheckedChanged": {
                "setter": "egui_view_checkbox_set_on_checked_listener",
                "signature": "void {func_name}(egui_view_t *self, uint8_t is_checked)",
            },
        },
    },
    xml_tag="Checkbox",
    display_name="Checkbox",
)

"""Divider widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="divider",
    descriptor={
        "c_type": "egui_view_divider_t",
        "init_func": "egui_view_divider_init_with_params",
        "params_macro": "EGUI_VIEW_DIVIDER_PARAMS_INIT",
        "params_type": "egui_view_divider_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "color": {
                "type": "color", "default": "EGUI_COLOR_WHITE",
                "code_gen": {"kind": "setter", "func": "egui_view_divider_set_color"},
            },
        },
    },
    xml_tag="Divider",
    display_name="Divider",
)

"""Spinner widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="spinner",
    descriptor={
        "c_type": "egui_view_spinner_t",
        "init_func": "egui_view_spinner_init_with_params",
        "params_macro": "EGUI_VIEW_SPINNER_PARAMS_INIT",
        "params_type": "egui_view_spinner_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "color": {
                "type": "color", "default": "EGUI_COLOR_WHITE",
                "code_gen": {"kind": "setter", "func": "egui_view_spinner_set_color"},
            },
        },
    },
    xml_tag="Spinner",
    display_name="Spinner",
)

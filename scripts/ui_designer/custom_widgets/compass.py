"""Compass widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="compass",
    descriptor={
        "c_type": "egui_view_compass_t",
        "init_func": "egui_view_compass_init_with_params",
        "params_macro": "EGUI_VIEW_COMPASS_PARAMS_INIT",
        "params_type": "egui_view_compass_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "heading": {
                "type": "int", "default": 0, "min": 0, "max": 359,
                "code_gen": {"kind": "setter", "func": "egui_view_compass_set_heading"},
            },
        },
    },
    xml_tag="Compass",
    display_name="Compass",
)

"""HeartRate widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="heart_rate",
    descriptor={
        "c_type": "egui_view_heart_rate_t",
        "init_func": "egui_view_heart_rate_init_with_params",
        "params_macro": "EGUI_VIEW_HEART_RATE_PARAMS_INIT",
        "params_type": "egui_view_heart_rate_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "bpm": {
                "type": "int", "default": 0, "min": 0, "max": 255,
                "code_gen": {"kind": "setter", "func": "egui_view_heart_rate_set_bpm"},
            },
        },
    },
    xml_tag="HeartRate",
    display_name="HeartRate",
)

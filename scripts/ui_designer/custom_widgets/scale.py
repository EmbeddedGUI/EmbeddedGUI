"""Scale widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="scale",
    descriptor={
        "c_type": "egui_view_scale_t",
        "init_func": "egui_view_scale_init_with_params",
        "params_macro": "EGUI_VIEW_SCALE_PARAMS_INIT",
        "params_type": "egui_view_scale_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "range_min": {
                "type": "int", "default": 0,
                "code_gen": {"kind": "param"},
            },
            "range_max": {
                "type": "int", "default": 100,
                "code_gen": {"kind": "param"},
            },
            "major_tick_count": {
                "type": "int", "default": 6,
                "code_gen": {"kind": "param"},
            },
            "value": {
                "type": "int", "default": 0,
                "code_gen": {"kind": "setter", "func": "egui_view_scale_set_value"},
            },
            "orientation": {
                "type": "int", "default": 1,
                "code_gen": {"kind": "setter", "func": "egui_view_scale_set_orientation"},
            },
            "show_labels": {
                "type": "int", "default": 1,
                "code_gen": {"kind": "setter", "func": "egui_view_scale_show_labels"},
            },
            "show_indicator": {
                "type": "int", "default": 0,
                "code_gen": {"kind": "setter", "func": "egui_view_scale_show_indicator"},
            },
        },
    },
    xml_tag="Scale",
    display_name="Scale",
)

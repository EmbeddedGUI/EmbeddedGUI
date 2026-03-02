"""Line widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="line",
    descriptor={
        "c_type": "egui_view_line_t",
        "init_func": "egui_view_line_init_with_params",
        "params_macro": "EGUI_VIEW_LINE_PARAMS_INIT",
        "params_type": "egui_view_line_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "line_width": {
                "type": "int", "default": 2, "min": 1, "max": 20,
                "code_gen": {"kind": "setter", "func": "egui_view_line_set_line_width",
                             "skip_default": True},
            },
            "line_color": {
                "type": "color", "default": "",
                "ui_group": "style",
                "code_gen": {"kind": "setter", "func": "egui_view_line_set_line_color",
                             "skip_default": True},
            },
            "use_round_cap": {
                "type": "bool", "default": False,
                "code_gen": {"kind": "setter", "func": "egui_view_line_set_use_round_cap",
                             "bool_to_int": True, "skip_default": True},
            },
        },
    },
    xml_tag="Line",
    display_name="Line",
)

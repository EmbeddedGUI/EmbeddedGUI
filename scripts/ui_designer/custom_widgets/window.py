"""Window widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="window",
    descriptor={
        "c_type": "egui_view_window_t",
        "init_func": "egui_view_window_init_with_params",
        "params_macro": "EGUI_VIEW_WINDOW_PARAMS_INIT",
        "params_type": "egui_view_window_params_t",
        "is_container": True,
        "add_child_func": "egui_view_window_add_content",
        "properties": {
            "title": {
                "type": "string",
                "default": "Window",
                "code_gen": {"kind": "text_setter", "func": "egui_view_window_set_title"},
            },
            "header_height": {
                "type": "int",
                "default": 30,
                "min": 10,
                "max": 100,
                "code_gen": {"kind": "setter", "func": "egui_view_window_set_header_height"},
            },
        },
    },
    xml_tag="Window",
    display_name="Window",
)

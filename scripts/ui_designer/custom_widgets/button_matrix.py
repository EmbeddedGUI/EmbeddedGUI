"""ButtonMatrix widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="button_matrix",
    descriptor={
        "c_type": "egui_view_button_matrix_t",
        "init_func": "egui_view_button_matrix_init_with_params",
        "params_macro": "EGUI_VIEW_BUTTON_MATRIX_PARAMS_INIT",
        "params_type": "egui_view_button_matrix_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "cols": {
                "type": "int", "default": 4, "min": 1, "max": 16,
                "code_gen": {"kind": "param_only"},
            },
            "gap": {
                "type": "int", "default": 2, "min": 0, "max": 32,
                "code_gen": {"kind": "param_only"},
            },
            "cornerRadius": {
                "type": "int", "default": 4, "min": 0, "max": 32,
                "code_gen": {"kind": "setter", "func": "egui_view_button_matrix_set_corner_radius"},
            },
        },
    },
    xml_tag="ButtonMatrix",
    display_name="ButtonMatrix",
)

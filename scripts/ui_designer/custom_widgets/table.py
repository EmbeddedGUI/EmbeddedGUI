"""Table widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="table",
    descriptor={
        "c_type": "egui_view_table_t",
        "init_func": "egui_view_table_init_with_params",
        "params_macro": "EGUI_VIEW_TABLE_PARAMS_INIT",
        "params_type": "egui_view_table_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "row_count": {
                "type": "int", "default": 0, "min": 0, "max": 16,
                "code_gen": {"kind": "param_only"},
            },
            "col_count": {
                "type": "int", "default": 0, "min": 0, "max": 8,
                "code_gen": {"kind": "param_only"},
            },
            "header_rows": {
                "type": "int", "default": 1, "min": 0, "max": 16,
                "code_gen": {"kind": "setter", "func": "egui_view_table_set_header_rows"},
            },
            "row_height": {
                "type": "int", "default": 20, "min": 1, "max": 200,
                "code_gen": {"kind": "setter", "func": "egui_view_table_set_row_height"},
            },
            "show_grid": {
                "type": "int", "default": 1, "min": 0, "max": 1,
                "code_gen": {"kind": "setter", "func": "egui_view_table_set_show_grid"},
            },
        },
    },
    xml_tag="Table",
    display_name="Table",
)

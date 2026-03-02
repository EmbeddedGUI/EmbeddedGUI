"""GridLayout widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="gridlayout",
    descriptor={
        "c_type": "egui_view_gridlayout_t",
        "init_func": "egui_view_gridlayout_init_with_params",
        "params_macro": "EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT",
        "params_type": "egui_view_gridlayout_params_t",
        "is_container": True,
        "add_child_func": "egui_view_group_add_child",
        "layout_func": "egui_view_gridlayout_layout_childs",
        "properties": {
            "col_count": {
                "type": "int", "default": 2, "min": 1, "max": 20,
                "code_gen": {"kind": "setter", "func": "egui_view_gridlayout_set_col_count"},
            },
            "align_type": {
                "type": "align", "default": "EGUI_ALIGN_CENTER",
                "code_gen": {"kind": "setter", "func": "egui_view_gridlayout_set_align_type"},
            },
        },
    },
    xml_tag="GridLayout",
    display_name="GridLayout",
)

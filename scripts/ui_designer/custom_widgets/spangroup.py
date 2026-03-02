"""Spangroup widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="spangroup",
    descriptor={
        "c_type": "egui_view_spangroup_t",
        "init_func": "egui_view_spangroup_init_with_params",
        "params_macro": "EGUI_VIEW_SPANGROUP_PARAMS_INIT",
        "params_type": "egui_view_spangroup_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "align": {
                "type": "align", "default": "EGUI_ALIGN_LEFT",
                "ui_group": "main",
                "code_gen": {"kind": "setter", "func": "egui_view_spangroup_set_align"},
            },
            "line_spacing": {
                "type": "int", "default": 2,
                "ui_group": "main",
                "code_gen": {"kind": "setter", "func": "egui_view_spangroup_set_line_spacing"},
            },
        },
    },
    xml_tag="Spangroup",
    display_name="Spangroup",
)

"""List widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="list",
    descriptor={
        "c_type": "egui_view_list_t",
        "init_func": "egui_view_list_init_with_params",
        "params_macro": "EGUI_VIEW_LIST_PARAMS_INIT",
        "params_type": "egui_view_list_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "item_height": {
                "type": "int", "default": 30,
                "ui_group": "main",
                "code_gen": {"kind": "setter", "func": "egui_view_list_set_item_height"},
            },
        },
    },
    xml_tag="List",
    display_name="List",
)

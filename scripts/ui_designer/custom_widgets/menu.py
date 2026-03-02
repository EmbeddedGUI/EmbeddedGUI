from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="menu",
    descriptor={
        "c_type": "egui_view_menu_t",
        "init_func": "egui_view_menu_init_with_params",
        "params_macro": "EGUI_VIEW_MENU_PARAMS_INIT",
        "params_type": "egui_view_menu_params_t",
        "is_container": False,
        "properties": {
            "header_height": {
                "type": "int",
                "default": 30,
                "code_gen": {"kind": "setter", "func": "egui_view_menu_set_header_height"},
            },
            "item_height": {
                "type": "int",
                "default": 30,
                "code_gen": {"kind": "setter", "func": "egui_view_menu_set_item_height"},
            },
        },
    },
    xml_tag="Menu",
    display_name="Menu",
)

"""ImageButton widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="image_button",
    descriptor={
        "c_type": "egui_view_image_button_t",
        "init_func": "egui_view_image_button_init_with_params",
        "params_macro": "EGUI_VIEW_IMAGE_BUTTON_PARAMS_INIT",
        "params_type": "egui_view_image_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "image_file": {
                "type": "image_file", "default": "",
                "ui_group": "main",
                "code_gen": {"kind": "image_setter", "func": "egui_view_image_set_image",
                             "cast": "(egui_image_t *)"},
            },
            "image_format": {
                "type": "image_format", "default": "rgb565",
                "ui_group": "image_config",
                "ui_visible_when": {"image_file": "!empty"},
                "code_gen": None,
            },
            "image_alpha": {
                "type": "image_alpha", "default": "4",
                "ui_group": "image_config",
                "ui_visible_when": {"image_file": "!empty"},
                "code_gen": None,
            },
        },
    },
    xml_tag="ImageButton",
    display_name="ImageButton",
)

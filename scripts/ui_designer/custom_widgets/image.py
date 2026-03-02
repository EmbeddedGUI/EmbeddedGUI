"""Image widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="image",
    descriptor={
        "c_type": "egui_view_image_t",
        "init_func": "egui_view_image_init_with_params",
        "params_macro": "EGUI_VIEW_IMAGE_PARAMS_INIT",
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
            "image_dim": {
                "type": "string", "default": "",
                "ui_group": "image_config",
                "ui_visible_when": {"image_file": "!empty"},
                "code_gen": None,
            },
            "image_external": {
                "type": "image_external", "default": "0",
                "ui_group": "image_config",
                "ui_visible_when": {"image_file": "!empty"},
                "code_gen": None,
            },
            "image_swap": {
                "type": "bool", "default": False,
                "ui_group": "image_config",
                "ui_visible_when": {"image_file": "!empty"},
                "code_gen": None,
            },
            "image_rot": {
                "type": "int", "default": 0, "min": 0, "max": 359,
                "ui_group": "image_config",
                "ui_visible_when": {"image_file": "!empty"},
                "code_gen": None,
            },
            "image_resize": {
                "type": "bool", "default": False,
                "ui_group": "image_config",
                "ui_visible_when": {"image_file": "!empty"},
                "code_gen": {"kind": "setter",
                             "func": "egui_view_image_set_image_type",
                             "skip_default": True,
                             "value_map": {True: "EGUI_VIEW_IMAGE_TYPE_RESIZE",
                                           False: "EGUI_VIEW_IMAGE_TYPE_NORMAL"}},
            },
            "image_color": {
                "type": "color", "default": "",
                "ui_group": "image_config",
                "ui_visible_when": {"image_file": "!empty"},
                "code_gen": {"kind": "multi_setter",
                             "func": "egui_view_image_set_image_color",
                             "args": "{image_color}, {image_color_alpha}",
                             "group": "image_color",
                             "skip_default": True},
            },
            "image_color_alpha": {
                "type": "alpha", "default": "",
                "ui_group": "image_config",
                "ui_visible_when": {"image_file": "!empty"},
                "code_gen": None,
            },
        },
    },
    xml_tag="Image",
    display_name="Image",
)

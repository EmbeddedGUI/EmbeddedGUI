"""Textblock widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="textblock",
    descriptor={
        "c_type": "egui_view_textblock_t",
        "init_func": "egui_view_textblock_init_with_params",
        "params_macro": "EGUI_VIEW_TEXTBLOCK_PARAMS_INIT",
        "params_type": "egui_view_textblock_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "text": {
                "type": "string", "default": "Text",
                "ui_group": "main",
                "code_gen": {"kind": "text_setter", "func": "egui_view_textblock_set_text"},
            },
            "font_file": {
                "type": "font_file", "default": "",
                "ui_group": "font_config",
                "code_gen": {"kind": "derived_setter", "func": "egui_view_textblock_set_font",
                             "derive": "font", "cast": "(egui_font_t *)"},
            },
            "font_builtin": {
                "type": "font", "default": "EGUI_CONFIG_FONT_DEFAULT",
                "ui_group": "font_config",
                "ui_visible_when": {"font_file": "empty"},
                "code_gen": None,
            },
            "font_pixelsize": {
                "type": "font_pixelsize", "default": "16",
                "ui_group": "font_config",
                "ui_visible_when": {"font_file": "!empty"},
                "code_gen": None,
            },
            "font_fontbitsize": {
                "type": "font_fontbitsize", "default": "4",
                "ui_group": "font_config",
                "ui_visible_when": {"font_file": "!empty"},
                "code_gen": None,
            },
            "font_external": {
                "type": "font_external", "default": "0",
                "ui_group": "font_config",
                "ui_visible_when": {"font_file": "!empty"},
                "code_gen": None,
            },
            "font_text": {
                "type": "string", "default": "",
                "ui_group": "font_config",
                "ui_visible_when": {"font_file": "!empty"},
                "code_gen": None,
            },
            "font_text_file": {
                "type": "text_file", "default": "",
                "ui_group": "font_config",
                "ui_visible_when": {"font_file": "!empty"},
                "code_gen": None,
            },
            "color": {
                "type": "color", "default": "EGUI_COLOR_WHITE",
                "ui_group": "main",
                "code_gen": {"kind": "multi_setter", "func": "egui_view_textblock_set_font_color",
                             "args": "{color}, {alpha}", "group": "font_color"},
            },
            "alpha": {
                "type": "alpha", "default": "EGUI_ALPHA_100",
                "ui_group": "main",
                "code_gen": None,
            },
            "align_type": {
                "type": "align", "default": "EGUI_ALIGN_CENTER",
                "ui_group": "main",
                "code_gen": {"kind": "setter", "func": "egui_view_textblock_set_align_type"},
            },
        },
    },
    xml_tag="Textblock",
    display_name="Textblock",
)

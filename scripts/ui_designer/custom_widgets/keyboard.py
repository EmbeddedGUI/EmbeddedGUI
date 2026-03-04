"""Keyboard widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="keyboard",
    descriptor={
        "c_type": "egui_view_keyboard_t",
        "init_func": "egui_view_keyboard_init",
        "is_container": False,
        "addable": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "mode": {
                "type": "enum", "default": "EGUI_KEYBOARD_MODE_LOWERCASE",
                "options": [
                    "EGUI_KEYBOARD_MODE_LOWERCASE",
                    "EGUI_KEYBOARD_MODE_UPPERCASE",
                    "EGUI_KEYBOARD_MODE_SYMBOLS",
                ],
                "code_gen": {
                    "kind": "setter",
                    "func": "egui_view_keyboard_set_mode",
                    "skip_default": True,
                },
            },
            "font_file": {
                "type": "font_file", "default": "",
                "ui_group": "font_config",
                "code_gen": {
                    "kind": "derived_setter",
                    "func": "egui_view_keyboard_set_font",
                    "derive": "font",
                    "cast": "(egui_font_t *)",
                },
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
        },
    },
    xml_tag="Keyboard",
    display_name="Keyboard",
)

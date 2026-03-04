"""Textinput widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="textinput",
    descriptor={
        "c_type": "egui_view_textinput_t",
        "init_func": "egui_view_textinput_init",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "text": {
                "type": "string", "default": "",
                "ui_group": "main",
                "code_gen": {"kind": "text_setter", "func": "egui_view_textinput_set_text"},
            },
            "placeholder": {
                "type": "string", "default": "",
                "ui_group": "main",
                "code_gen": {"kind": "text_setter", "func": "egui_view_textinput_set_placeholder"},
            },
            "text_color": {
                "type": "color", "default": "EGUI_COLOR_WHITE",
                "ui_group": "main",
                "code_gen": {"kind": "multi_setter", "func": "egui_view_textinput_set_text_color",
                             "args": "{text_color}, {text_alpha}", "group": "text_color"},
            },
            "text_alpha": {
                "type": "alpha", "default": "EGUI_ALPHA_100",
                "ui_group": "main",
                "code_gen": None,
            },
            "placeholder_color": {
                "type": "color", "default": "EGUI_COLOR_GRAY",
                "ui_group": "main",
                "code_gen": {"kind": "multi_setter", "func": "egui_view_textinput_set_placeholder_color",
                             "args": "{placeholder_color}, {placeholder_alpha}", "group": "placeholder_color"},
            },
            "placeholder_alpha": {
                "type": "alpha", "default": "EGUI_ALPHA_100",
                "ui_group": "main",
                "code_gen": None,
            },
            "cursor_color": {
                "type": "color", "default": "EGUI_COLOR_WHITE",
                "ui_group": "main",
                "code_gen": {"kind": "setter", "func": "egui_view_textinput_set_cursor_color"},
            },
            "align_type": {
                "type": "align", "default": "EGUI_ALIGN_LEFT",
                "ui_group": "main",
                "code_gen": {"kind": "setter", "func": "egui_view_textinput_set_align_type"},
            },
            "max_length": {
                "type": "int", "default": 32, "min": 1, "max": 255,
                "ui_group": "main",
                "code_gen": {"kind": "setter", "func": "egui_view_textinput_set_max_length", "skip_default": True},
            },
            "font_file": {
                "type": "font_file", "default": "",
                "ui_group": "font_config",
                "code_gen": {"kind": "derived_setter", "func": "egui_view_textinput_set_font",
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
        },
        "events": {
            "onTextChanged": {
                "setter": "egui_view_textinput_set_on_text_changed",
                "signature": "void {func_name}(egui_view_t *self, const char *text)",
            },
            "onSubmit": {
                "setter": "egui_view_textinput_set_on_submit",
                "signature": "void {func_name}(egui_view_t *self, const char *text)",
            },
        },
    },
    xml_tag="Textinput",
    display_name="Textinput",
)

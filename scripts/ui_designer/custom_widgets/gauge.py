"""Gauge widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="gauge",
    descriptor={
        "c_type": "egui_view_gauge_t",
        "init_func": "egui_view_gauge_init_with_params",
        "params_macro": "EGUI_VIEW_GAUGE_PARAMS_INIT",
        "params_type": "egui_view_gauge_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "value": {
                "type": "int", "default": 0, "min": 0, "max": 100,
                "code_gen": {"kind": "setter", "func": "egui_view_gauge_set_value"},
            },
            "text_color": {
                "type": "color", "default": "EGUI_COLOR_WHITE",
                "code_gen": {"kind": "setter", "func": "egui_view_gauge_set_text_color"},
            },
            "font_file": {
                "type": "font_file", "default": "",
                "ui_group": "font_config",
                "code_gen": {"kind": "derived_setter", "func": "egui_view_gauge_set_font",
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
    },
    xml_tag="Gauge",
    display_name="Gauge",
)

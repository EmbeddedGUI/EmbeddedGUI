"""Pie Chart widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="chart_pie",
    descriptor={
        "c_type": "egui_view_chart_pie_t",
        "init_func": "egui_view_chart_pie_init_with_params",
        "params_macro": "EGUI_VIEW_CHART_PIE_PARAMS_INIT",
        "params_type": "egui_view_chart_pie_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            # --- legend ---
            "legend_pos": {
                "type": "enum", "default": "EGUI_CHART_LEGEND_NONE",
                "options": ["EGUI_CHART_LEGEND_NONE",
                            "EGUI_CHART_LEGEND_TOP",
                            "EGUI_CHART_LEGEND_BOTTOM",
                            "EGUI_CHART_LEGEND_RIGHT"],
                "ui_group": "style",
                "code_gen": {"kind": "setter",
                             "func": "egui_view_chart_pie_set_legend_pos",
                             "skip_default": True},
            },
            # --- colors (pie only has bg + text) ---
            "bg_color": {
                "type": "color", "default": "EGUI_COLOR_BLACK",
                "ui_group": "style",
                "code_gen": {"kind": "multi_setter",
                             "func": "egui_view_chart_pie_set_colors",
                             "args": "{bg_color}, {text_color}",
                             "group": "chart_colors"},
            },
            "text_color": {
                "type": "color", "default": "EGUI_COLOR_WHITE",
                "ui_group": "style",
                "code_gen": None,
            },
            # --- font ---
            "font_file": {
                "type": "font_file", "default": "",
                "ui_group": "font_config",
                "code_gen": {"kind": "derived_setter",
                             "func": "egui_view_chart_pie_set_font",
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
                "type": "string", "default": "0123456789.-%",
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
    xml_tag="ChartPie",
    display_name="Chart Pie",
)

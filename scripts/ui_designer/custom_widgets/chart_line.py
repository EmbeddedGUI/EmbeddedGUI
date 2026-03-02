"""Line Chart widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="chart_line",
    descriptor={
        "c_type": "egui_view_chart_line_t",
        "init_func": "egui_view_chart_line_init_with_params",
        "params_macro": "EGUI_VIEW_CHART_LINE_PARAMS_INIT",
        "params_type": "egui_view_chart_line_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            # --- axis x ---
            "axis_x_min": {
                "type": "int", "default": 0, "min": -32768, "max": 32767,
                "ui_group": "axis_x",
                "code_gen": {"kind": "multi_setter",
                             "func": "egui_view_chart_axis_set_axis_x",
                             "args": "{axis_x_min}, {axis_x_max}, "
                                     "{axis_x_tick_step}",
                             "group": "axis_x"},
            },
            "axis_x_max": {
                "type": "int", "default": 100, "min": -32768, "max": 32767,
                "ui_group": "axis_x",
                "code_gen": None,
            },
            "axis_x_tick_step": {
                "type": "int", "default": 10, "min": 0, "max": 32767,
                "ui_group": "axis_x",
                "code_gen": None,
            },
            # --- axis y ---
            "axis_y_min": {
                "type": "int", "default": 0, "min": -32768, "max": 32767,
                "ui_group": "axis_y",
                "code_gen": {"kind": "multi_setter",
                             "func": "egui_view_chart_axis_set_axis_y",
                             "args": "{axis_y_min}, {axis_y_max}, "
                                     "{axis_y_tick_step}",
                             "group": "axis_y"},
            },
            "axis_y_max": {
                "type": "int", "default": 100, "min": -32768, "max": 32767,
                "ui_group": "axis_y",
                "code_gen": None,
            },
            "axis_y_tick_step": {
                "type": "int", "default": 10, "min": 0, "max": 32767,
                "ui_group": "axis_y",
                "code_gen": None,
            },
            # --- legend ---
            "legend_pos": {
                "type": "enum", "default": "EGUI_CHART_LEGEND_NONE",
                "options": ["EGUI_CHART_LEGEND_NONE",
                            "EGUI_CHART_LEGEND_TOP",
                            "EGUI_CHART_LEGEND_BOTTOM",
                            "EGUI_CHART_LEGEND_RIGHT"],
                "ui_group": "style",
                "code_gen": {"kind": "setter",
                             "func": "egui_view_chart_axis_set_legend_pos",
                             "skip_default": True},
            },
            # --- colors ---
            "bg_color": {
                "type": "color", "default": "EGUI_COLOR_BLACK",
                "ui_group": "style",
                "code_gen": {"kind": "multi_setter",
                             "func": "egui_view_chart_axis_set_colors",
                             "args": "{bg_color}, {axis_color}, "
                                     "{grid_color}, {text_color}",
                             "group": "chart_colors"},
            },
            "axis_color": {
                "type": "color", "default": "EGUI_COLOR_WHITE",
                "ui_group": "style",
                "code_gen": None,
            },
            "grid_color": {
                "type": "color", "default": "EGUI_COLOR_DARK_GREY",
                "ui_group": "style",
                "code_gen": None,
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
                             "func": "egui_view_chart_axis_set_font",
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
                "type": "string", "default": "0123456789.-",
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
            # --- line-specific ---
            "line_width": {
                "type": "int", "default": 2, "min": 1, "max": 10,
                "ui_group": "style",
                "code_gen": {"kind": "setter",
                             "func": "egui_view_chart_line_set_line_width",
                             "skip_default": True},
            },
            "point_radius": {
                "type": "int", "default": 3, "min": 0, "max": 10,
                "ui_group": "style",
                "code_gen": {"kind": "setter",
                             "func": "egui_view_chart_line_set_point_radius",
                             "skip_default": True},
            },
        },
    },
    xml_tag="ChartLine",
    display_name="Chart Line",
)

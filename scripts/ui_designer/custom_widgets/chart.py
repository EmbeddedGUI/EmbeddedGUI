"""Chart widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="chart",
    descriptor={
        "c_type": "egui_view_chart_line_t",
        "init_func": "egui_view_chart_line_init_with_params",
        "params_macro": "EGUI_VIEW_CHART_LINE_PARAMS_INIT",
        "params_type": "egui_view_chart_line_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        # Backward-compatible alias: Chart now maps to line chart defaults.
        # For richer configs, use ChartLine/ChartBar/ChartScatter/ChartPie.
        "properties": {},
    },
    xml_tag="Chart",
    display_name="Chart",
)

"""Chart widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="chart",
    descriptor={
        "c_type": "egui_view_chart_t",
        "init_func": "egui_view_chart_init_with_params",
        "params_macro": "EGUI_VIEW_CHART_PARAMS_INIT",
        "params_type": "egui_view_chart_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        # series/axis/slices are complex structs, set in user C code
        "properties": {},
    },
    xml_tag="Chart",
    display_name="Chart",
)

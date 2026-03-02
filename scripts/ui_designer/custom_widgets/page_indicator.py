"""PageIndicator widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="page_indicator",
    descriptor={
        "c_type": "egui_view_page_indicator_t",
        "init_func": "egui_view_page_indicator_init_with_params",
        "params_macro": "EGUI_VIEW_PAGE_INDICATOR_PARAMS_INIT",
        "params_type": "egui_view_page_indicator_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "total_count": {
                "type": "int", "default": 3, "min": 1, "max": 20,
                "code_gen": {"kind": "setter", "func": "egui_view_page_indicator_set_total_count"},
            },
            "current_index": {
                "type": "int", "default": 0, "min": 0, "max": 19,
                "code_gen": {"kind": "setter", "func": "egui_view_page_indicator_set_current_index",
                             "skip_default": True},
            },
        },
    },
    xml_tag="PageIndicator",
    display_name="PageIndicator",
)

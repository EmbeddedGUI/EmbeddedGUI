"""Scroll widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="scroll",
    descriptor={
        "c_type": "egui_view_scroll_t",
        "init_func": "egui_view_scroll_init_with_params",
        "params_macro": "EGUI_VIEW_SCROLL_PARAMS_INIT",
        "params_type": "egui_view_scroll_params_t",
        "is_container": True,
        "add_child_func": "egui_view_scroll_add_child",
        "layout_func": "egui_view_scroll_layout_childs",
        "properties": {},
    },
    xml_tag="Scroll",
    display_name="Scroll",
)

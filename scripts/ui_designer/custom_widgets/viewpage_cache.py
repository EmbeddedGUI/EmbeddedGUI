"""ViewPageCache widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="viewpage_cache",
    descriptor={
        "c_type": "egui_view_viewpage_cache_t",
        "init_func": "egui_view_viewpage_cache_init_with_params",
        "params_macro": "EGUI_VIEW_VIEWPAGE_CACHE_PARAMS_INIT",
        "params_type": "egui_view_viewpage_cache_params_t",
        "is_container": True,
        "add_child_func": "egui_view_viewpage_add_child",
        "layout_func": "egui_view_viewpage_layout_childs",
        "properties": {},
    },
    xml_tag="ViewPageCache",
    display_name="ViewPageCache",
)

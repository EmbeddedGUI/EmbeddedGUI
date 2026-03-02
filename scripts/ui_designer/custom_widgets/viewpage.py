"""ViewPage widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="viewpage",
    descriptor={
        "c_type": "egui_view_viewpage_t",
        "init_func": "egui_view_viewpage_init_with_params",
        "params_macro": "EGUI_VIEW_VIEWPAGE_PARAMS_INIT",
        "params_type": "egui_view_viewpage_params_t",
        "is_container": True,
        "add_child_func": "egui_view_viewpage_add_child",
        "layout_func": "egui_view_viewpage_layout_childs",
        "properties": {
            "current_page": {
                "type": "int", "default": 0, "min": 0, "max": 255,
                "code_gen": {"kind": "setter",
                             "func": "egui_view_viewpage_set_current_page",
                             "skip_default": True},
            },
        },
    },
    xml_tag="ViewPage",
    display_name="ViewPage",
)

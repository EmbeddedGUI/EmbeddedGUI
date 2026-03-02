"""Group widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="group",
    descriptor={
        "c_type": "egui_view_group_t",
        "init_func": "egui_view_group_init_with_params",
        "params_macro": "EGUI_VIEW_GROUP_PARAMS_INIT",
        "params_type": "egui_view_group_params_t",
        "is_container": True,
        "add_child_func": "egui_view_group_add_child",
        "layout_func": None,
        "properties": {},
    },
    xml_tag="Group",
    display_name="Group",
)

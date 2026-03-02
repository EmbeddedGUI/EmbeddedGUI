"""Switch widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="switch",
    descriptor={
        "c_type": "egui_view_switch_t",
        "init_func": "egui_view_switch_init_with_params",
        "params_macro": "EGUI_VIEW_SWITCH_PARAMS_INIT",
        "params_type": "egui_view_switch_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "is_checked": {
                "type": "bool", "default": False,
                "code_gen": {"kind": "setter", "func": "egui_view_switch_set_checked",
                             "bool_to_int": True, "skip_default": True},
            },
        },
        "events": {
            "onCheckedChanged": {
                "setter": "egui_view_switch_set_on_checked_listener",
                "signature": "void {func_name}(egui_view_t *self, int is_checked)",
            },
        },
    },
    xml_tag="Switch",
    display_name="Switch",
)

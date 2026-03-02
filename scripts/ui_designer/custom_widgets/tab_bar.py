"""TabBar widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="tab_bar",
    descriptor={
        "c_type": "egui_view_tab_bar_t",
        "init_func": "egui_view_tab_bar_init_with_params",
        "params_macro": "EGUI_VIEW_TAB_BAR_PARAMS_INIT",
        "params_type": "egui_view_tab_bar_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        # tab_texts/tab_count are pointer-based, set in user C code
        "properties": {
            "current_index": {
                "type": "int", "default": 0, "min": 0, "max": 20,
                "code_gen": {"kind": "setter", "func": "egui_view_tab_bar_set_current_index",
                             "skip_default": True},
            },
        },
        "events": {
            "onTabChanged": {
                "setter": "egui_view_tab_bar_set_on_tab_changed_listener",
                "signature": "void {func_name}(egui_view_t *self, uint8_t index)",
            },
        },
    },
    xml_tag="TabBar",
    display_name="TabBar",
)

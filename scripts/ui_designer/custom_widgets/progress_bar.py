"""ProgressBar widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="progress_bar",
    descriptor={
        "c_type": "egui_view_progress_bar_t",
        "init_func": "egui_view_progress_bar_init_with_params",
        "params_macro": "EGUI_VIEW_PROGRESS_BAR_PARAMS_INIT",
        "params_type": "egui_view_progress_bar_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "value": {
                "type": "int", "default": 50, "min": 0, "max": 100,
                "code_gen": {"kind": "setter", "func": "egui_view_progress_bar_set_process"},
            },
        },
        "events": {
            "onProgressChanged": {
                "setter": "egui_view_progress_bar_set_on_progress_listener",
                "signature": "void {func_name}(egui_view_t *self, uint8_t progress)",
            },
        },
    },
    xml_tag="ProgressBar",
    display_name="ProgressBar",
)

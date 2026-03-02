"""ArcSlider widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="arc_slider",
    descriptor={
        "c_type": "egui_view_arc_slider_t",
        "init_func": "egui_view_arc_slider_init_with_params",
        "params_macro": "EGUI_VIEW_ARC_SLIDER_PARAMS_INIT",
        "params_type": "egui_view_arc_slider_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "value": {
                "type": "int", "default": 0, "min": 0, "max": 255,
                "code_gen": {"kind": "setter", "func": "egui_view_arc_slider_set_value"},
            },
        },
        "events": {
            "onValueChanged": {
                "setter": "egui_view_arc_slider_set_on_value_changed_listener",
                "signature": "void {func_name}(egui_view_t *self, uint8_t value)",
            },
        },
    },
    xml_tag="ArcSlider",
    display_name="ArcSlider",
)

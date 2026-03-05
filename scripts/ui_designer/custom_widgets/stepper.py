"""Stepper widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="stepper",
    descriptor={
        "c_type": "egui_view_stepper_t",
        "init_func": "egui_view_stepper_init_with_params",
        "params_macro": "EGUI_VIEW_STEPPER_PARAMS_INIT",
        "params_type": "egui_view_stepper_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "total_steps": {
                "type": "int",
                "default": 4,
                "min": 1,
                "max": 20,
                "code_gen": {
                    "kind": "setter",
                    "func": "egui_view_stepper_set_total_steps",
                    "skip_default": True,
                },
            },
            "current_step": {
                "type": "int",
                "default": 0,
                "min": 0,
                "max": 20,
                "code_gen": {
                    "kind": "setter",
                    "func": "egui_view_stepper_set_current_step",
                    "skip_default": True,
                },
            },
        },
        "events": {},
    },
    xml_tag="Stepper",
    display_name="Stepper",
)

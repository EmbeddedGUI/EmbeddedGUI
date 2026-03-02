"""AnimatedImage widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="animated_image",
    descriptor={
        "c_type": "egui_view_animated_image_t",
        "init_func": "egui_view_animated_image_init_with_params",
        "params_macro": "EGUI_VIEW_ANIMATED_IMAGE_PARAMS_INIT",
        "params_type": "egui_view_animated_image_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "frame_interval_ms": {
                "type": "int", "default": 100, "min": 1, "max": 65535,
                "code_gen": {"kind": "setter",
                             "func": "egui_view_animated_image_set_interval",
                             "skip_default": True},
            },
            "is_loop": {
                "type": "bool", "default": True,
                "code_gen": {"kind": "setter",
                             "func": "egui_view_animated_image_set_loop",
                             "skip_default": True,
                             "value_map": {True: "1", False: "0"}},
            },
        },
    },
    xml_tag="AnimatedImage",
    display_name="AnimatedImage",
)

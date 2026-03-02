from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="activity_ring",
    descriptor={
        "c_type": "egui_view_activity_ring_t",
        "init_func": "egui_view_activity_ring_init_with_params",
        "params_macro": "EGUI_VIEW_ACTIVITY_RING_PARAMS_INIT",
        "params_type": "egui_view_activity_ring_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "ring_count": {
                "type": "int",
                "default": 3,
                "code_gen": {"kind": "setter", "func": "egui_view_activity_ring_set_ring_count"},
            },
            "stroke_width": {
                "type": "int",
                "default": 8,
                "code_gen": {"kind": "setter", "func": "egui_view_activity_ring_set_stroke_width"},
            },
            "ring_gap": {
                "type": "int",
                "default": 4,
                "code_gen": {"kind": "setter", "func": "egui_view_activity_ring_set_ring_gap"},
            },
        },
    },
    xml_tag="ActivityRing",
    display_name="ActivityRing",
)

from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="analog_clock",
    descriptor={
        "c_type": "egui_view_analog_clock_t",
        "init_func": "egui_view_analog_clock_init_with_params",
        "params_macro": "EGUI_VIEW_ANALOG_CLOCK_PARAMS_INIT",
        "params_type": "egui_view_analog_clock_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "hour": {
                "type": "int",
                "default": 0,
                "code_gen": {"kind": "param", "index": 5},
            },
            "minute": {
                "type": "int",
                "default": 0,
                "code_gen": {"kind": "param", "index": 6},
            },
            "second": {
                "type": "int",
                "default": 0,
                "code_gen": {"kind": "param", "index": 7},
            },
            "show_second": {
                "type": "int",
                "default": 1,
                "code_gen": {"kind": "setter", "func": "egui_view_analog_clock_show_second"},
            },
            "show_ticks": {
                "type": "int",
                "default": 1,
                "code_gen": {"kind": "setter", "func": "egui_view_analog_clock_show_ticks"},
            },
        },
    },
    xml_tag="AnalogClock",
    display_name="AnalogClock",
)

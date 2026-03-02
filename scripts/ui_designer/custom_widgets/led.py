"""LED widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="led",
    descriptor={
        "c_type": "egui_view_led_t",
        "init_func": "egui_view_led_init_with_params",
        "params_macro": "EGUI_VIEW_LED_PARAMS_INIT",
        "params_type": "egui_view_led_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "is_on": {
                "type": "bool", "default": False,
                "code_gen": {"kind": "setter", "func": "egui_view_led_set_on",
                             "bool_to_int": True, "skip_default": True},
            },
            "on_color": {
                "type": "color", "default": "",
                "ui_group": "style",
                "code_gen": {"kind": "multi_setter",
                             "func": "egui_view_led_set_colors",
                             "args": "{on_color}, {off_color}",
                             "group": "led_colors",
                             "skip_default": True},
            },
            "off_color": {
                "type": "color", "default": "",
                "ui_group": "style",
                "code_gen": None,
            },
            "blink_period": {
                "type": "int", "default": 0, "min": 0, "max": 10000,
                "ui_group": "style",
                "code_gen": {"kind": "setter",
                             "func": "egui_view_led_set_blink",
                             "skip_default": True},
            },
        },
    },
    xml_tag="Led",
    display_name="LED",
)

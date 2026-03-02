"""MiniCalendar widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="mini_calendar",
    descriptor={
        "c_type": "egui_view_mini_calendar_t",
        "init_func": "egui_view_mini_calendar_init_with_params",
        "params_macro": "EGUI_VIEW_MINI_CALENDAR_PARAMS_INIT",
        "params_type": "egui_view_mini_calendar_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "year": {
                "type": "int", "default": 2026, "min": 1970, "max": 2099,
                "code_gen": {"kind": "multi_setter",
                             "func": "egui_view_mini_calendar_set_date",
                             "args": ["{year}", "{month}", "{day}"]},
            },
            "month": {
                "type": "int", "default": 1, "min": 1, "max": 12,
                "code_gen": {"kind": "param_only"},
            },
            "day": {
                "type": "int", "default": 1, "min": 1, "max": 31,
                "code_gen": {"kind": "param_only"},
            },
        },
    },
    xml_tag="MiniCalendar",
    display_name="MiniCalendar",
)

"""NotificationBadge widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="notification_badge",
    descriptor={
        "c_type": "egui_view_notification_badge_t",
        "init_func": "egui_view_notification_badge_init_with_params",
        "params_macro": "EGUI_VIEW_NOTIFICATION_BADGE_PARAMS_INIT",
        "params_type": "egui_view_notification_badge_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "count": {
                "type": "int", "default": 0, "min": 0, "max": 999,
                "code_gen": {"kind": "setter", "func": "egui_view_notification_badge_set_count"},
            },
            "max_display": {
                "type": "int", "default": 99, "min": 1, "max": 255,
                "code_gen": {"kind": "setter",
                             "func": "egui_view_notification_badge_set_max_display",
                             "skip_default": True},
            },
        },
    },
    xml_tag="NotificationBadge",
    display_name="NotificationBadge",
)

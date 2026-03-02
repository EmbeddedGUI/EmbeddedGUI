"""RadioButton widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="radio_button",
    descriptor={
        "c_type": "egui_view_radio_button_t",
        "init_func": "egui_view_radio_button_init_with_params",
        "params_macro": "EGUI_VIEW_RADIO_BUTTON_PARAMS_INIT",
        "params_type": "egui_view_radio_button_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "text": {
                "type": "string", "default": "",
                "code_gen": {"kind": "text_setter", "func": "egui_view_radio_button_set_text"},
            },
            "text_color": {
                "type": "color", "default": "EGUI_THEME_TEXT",
                "code_gen": {"kind": "setter", "func": "egui_view_radio_button_set_text_color"},
            },
            "is_checked": {
                "type": "bool", "default": False,
                "code_gen": {"kind": "setter", "func": "egui_view_radio_button_set_checked",
                             "bool_to_int": True, "skip_default": True},
            },
        },
    },
    xml_tag="RadioButton",
    display_name="RadioButton",
)

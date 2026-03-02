"""Card widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="card",
    descriptor={
        "c_type": "egui_view_card_t",
        "init_func": "egui_view_card_init_with_params",
        "params_macro": "EGUI_VIEW_CARD_PARAMS_INIT",
        "params_type": "egui_view_card_params_t",
        "is_container": True,
        "add_child_func": "egui_view_card_add_child",
        "layout_func": "egui_view_card_layout_childs",
        "layout_func_args": "{orientation_value}, {align_type}",
        "properties": {
            "corner_radius": {
                "type": "int", "default": 8, "min": 0, "max": 100,
                "code_gen": {"kind": "setter", "func": "egui_view_card_set_corner_radius"},
            },
            "bg_color": {
                "type": "color", "default": "",
                "ui_group": "card_bg",
                "code_gen": {"kind": "multi_setter",
                             "func": "egui_view_card_set_bg_color",
                             "args": "{bg_color}, {bg_alpha}",
                             "group": "card_bg",
                             "skip_default": True},
            },
            "bg_alpha": {
                "type": "alpha", "default": "",
                "ui_group": "card_bg",
                "code_gen": None,
            },
            "border_width": {
                "type": "int", "default": 0, "min": 0, "max": 20,
                "ui_group": "card_border",
                "code_gen": {"kind": "multi_setter",
                             "func": "egui_view_card_set_border",
                             "args": "{border_width}, {border_color}",
                             "group": "card_border",
                             "skip_default": True},
            },
            "border_color": {
                "type": "color", "default": "",
                "ui_group": "card_border",
                "code_gen": None,
            },
            "orientation": {
                "type": "orientation", "default": "vertical",
                "code_gen": None,
            },
            "align_type": {
                "type": "align", "default": "EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP",
                "code_gen": None,
            },
        },
    },
    xml_tag="Card",
    display_name="Card",
)

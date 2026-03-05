"""Chips widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="chips",
    descriptor={
        "c_type": "egui_view_chips_t",
        "init_func": "egui_view_chips_init_with_params",
        "params_macro": "EGUI_VIEW_CHIPS_PARAMS_INIT",
        "params_type": "egui_view_chips_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "gap": {
                "type": "int",
                "default": 6,
                "min": 0,
                "max": 24,
                "code_gen": {
                    "kind": "setter",
                    "func": "egui_view_chips_set_gap",
                    "skip_default": True,
                },
            },
            "corner_radius": {
                "type": "int",
                "default": 8,
                "min": 0,
                "max": 30,
                "code_gen": {
                    "kind": "setter",
                    "func": "egui_view_chips_set_corner_radius",
                    "skip_default": True,
                },
            },
            "selected_index": {
                "type": "int",
                "default": 0,
                "min": 0,
                "max": 15,
                "code_gen": {
                    "kind": "setter",
                    "func": "egui_view_chips_set_selected_index",
                    "skip_default": False,
                },
            },
            "bg_color": {
                "type": "color",
                "default": "",
                "code_gen": {
                    "kind": "setter",
                    "func": "egui_view_chips_set_bg_color",
                    "skip_default": True,
                },
            },
            "selected_bg_color": {
                "type": "color",
                "default": "",
                "code_gen": {
                    "kind": "setter",
                    "func": "egui_view_chips_set_selected_bg_color",
                    "skip_default": True,
                },
            },
            "text_color": {
                "type": "color",
                "default": "",
                "code_gen": {
                    "kind": "setter",
                    "func": "egui_view_chips_set_text_color",
                    "skip_default": True,
                },
            },
            "border_color": {
                "type": "color",
                "default": "",
                "code_gen": {
                    "kind": "setter",
                    "func": "egui_view_chips_set_border_color",
                    "skip_default": True,
                },
            },
        },
        "events": {
            "onChipSelected": {
                "setter": "egui_view_chips_set_on_selected_listener",
                "signature": "void {func_name}(egui_view_t *self, uint8_t index)",
            },
        },
    },
    xml_tag="Chips",
    display_name="Chips",
)

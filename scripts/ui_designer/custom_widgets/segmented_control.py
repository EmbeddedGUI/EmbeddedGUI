"""SegmentedControl widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="segmented_control",
    descriptor={
        "c_type": "egui_view_segmented_control_t",
        "init_func": "egui_view_segmented_control_init_with_params",
        "params_macro": "EGUI_VIEW_SEGMENTED_CONTROL_PARAMS_INIT",
        "params_type": "egui_view_segmented_control_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        # segment_texts/segment_count are pointer-based, set in user C code
        "properties": {
            "current_index": {
                "type": "int",
                "default": 0,
                "min": 0,
                "max": 20,
                "code_gen": {
                    "kind": "setter",
                    "func": "egui_view_segmented_control_set_current_index",
                    "skip_default": True,
                },
            },
            "segment_gap": {
                "type": "int",
                "default": 2,
                "min": 0,
                "max": 20,
                "code_gen": {
                    "kind": "setter",
                    "func": "egui_view_segmented_control_set_segment_gap",
                    "skip_default": True,
                },
            },
            "horizontal_padding": {
                "type": "int",
                "default": 2,
                "min": 0,
                "max": 20,
                "code_gen": {
                    "kind": "setter",
                    "func": "egui_view_segmented_control_set_horizontal_padding",
                    "skip_default": True,
                },
            },
            "corner_radius": {
                "type": "int",
                "default": 10,
                "min": 0,
                "max": 40,
                "code_gen": {
                    "kind": "setter",
                    "func": "egui_view_segmented_control_set_corner_radius",
                    "skip_default": True,
                },
            },
            "bg_color": {
                "type": "color",
                "default": "",
                "code_gen": {
                    "kind": "setter",
                    "func": "egui_view_segmented_control_set_bg_color",
                    "skip_default": True,
                },
            },
            "selected_bg_color": {
                "type": "color",
                "default": "",
                "code_gen": {
                    "kind": "setter",
                    "func": "egui_view_segmented_control_set_selected_bg_color",
                    "skip_default": True,
                },
            },
            "text_color": {
                "type": "color",
                "default": "",
                "code_gen": {
                    "kind": "setter",
                    "func": "egui_view_segmented_control_set_text_color",
                    "skip_default": True,
                },
            },
            "selected_text_color": {
                "type": "color",
                "default": "",
                "code_gen": {
                    "kind": "setter",
                    "func": "egui_view_segmented_control_set_selected_text_color",
                    "skip_default": True,
                },
            },
            "border_color": {
                "type": "color",
                "default": "",
                "code_gen": {
                    "kind": "setter",
                    "func": "egui_view_segmented_control_set_border_color",
                    "skip_default": True,
                },
            },
        },
        "events": {
            "onSegmentChanged": {
                "setter": "egui_view_segmented_control_set_on_segment_changed_listener",
                "signature": "void {func_name}(egui_view_t *self, uint8_t index)",
            },
        },
    },
    xml_tag="SegmentedControl",
    display_name="SegmentedControl",
)

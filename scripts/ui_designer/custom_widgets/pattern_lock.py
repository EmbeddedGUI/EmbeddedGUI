"""PatternLock widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="pattern_lock",
    descriptor={
        "c_type": "egui_view_pattern_lock_t",
        "init_func": "egui_view_pattern_lock_init_with_params",
        "params_macro": "EGUI_VIEW_PATTERN_LOCK_PARAMS_INIT",
        "params_type": "egui_view_pattern_lock_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "min_nodes": {
                "type": "int",
                "default": 4,
                "min": 1,
                "max": 9,
                "code_gen": {
                    "kind": "setter",
                    "func": "egui_view_pattern_lock_set_min_nodes",
                    "skip_default": False,
                },
            },
            "touch_expand": {
                "type": "int",
                "default": 5,
                "min": 0,
                "max": 24,
                "code_gen": {
                    "kind": "setter",
                    "func": "egui_view_pattern_lock_set_touch_expand",
                    "skip_default": False,
                },
            },
            "bg_color": {
                "type": "color",
                "default": "",
                "code_gen": {
                    "kind": "setter",
                    "func": "egui_view_pattern_lock_set_bg_color",
                    "skip_default": True,
                },
            },
            "border_color": {
                "type": "color",
                "default": "",
                "code_gen": {
                    "kind": "setter",
                    "func": "egui_view_pattern_lock_set_border_color",
                    "skip_default": True,
                },
            },
            "node_color": {
                "type": "color",
                "default": "",
                "code_gen": {
                    "kind": "setter",
                    "func": "egui_view_pattern_lock_set_node_color",
                    "skip_default": True,
                },
            },
            "active_node_color": {
                "type": "color",
                "default": "",
                "code_gen": {
                    "kind": "setter",
                    "func": "egui_view_pattern_lock_set_active_node_color",
                    "skip_default": True,
                },
            },
            "line_color": {
                "type": "color",
                "default": "",
                "code_gen": {
                    "kind": "setter",
                    "func": "egui_view_pattern_lock_set_line_color",
                    "skip_default": True,
                },
            },
            "error_color": {
                "type": "color",
                "default": "",
                "code_gen": {
                    "kind": "setter",
                    "func": "egui_view_pattern_lock_set_error_color",
                    "skip_default": True,
                },
            },
        },
        "events": {
            "onPatternComplete": {
                "setter": "egui_view_pattern_lock_set_on_pattern_complete_listener",
                "signature": "void {func_name}(egui_view_t *self, uint8_t node_count)",
            },
            "onPatternFinish": {
                "setter": "egui_view_pattern_lock_set_on_pattern_finish_listener",
                "signature": "void {func_name}(egui_view_t *self, const uint8_t *nodes, uint8_t node_count, uint8_t valid)",
            },
        },
    },
    xml_tag="PatternLock",
    display_name="PatternLock",
)

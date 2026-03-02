"""LinearLayout widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="linearlayout",
    descriptor={
        "c_type": "egui_view_linearlayout_t",
        "init_func": "egui_view_linearlayout_init_with_params",
        "params_macro": "EGUI_VIEW_LINEARLAYOUT_PARAMS_INIT",
        "params_type": "egui_view_linearlayout_params_t",
        "is_container": True,
        "add_child_func": "egui_view_group_add_child",
        "layout_func": "egui_view_linearlayout_layout_childs",
        "properties": {
            "align_type": {
                "type": "align", "default": "EGUI_ALIGN_CENTER",
                "code_gen": {"kind": "setter", "func": "egui_view_linearlayout_set_align_type"},
            },
            "orientation": {
                "type": "orientation", "default": "vertical",
                "code_gen": {"kind": "setter", "func": "egui_view_linearlayout_set_orientation",
                             "value_map": {"horizontal": "1", "vertical": "0"},
                             "skip_values": ["vertical"]},
            },
            "auto_width": {
                "type": "bool", "default": False,
                "code_gen": {"kind": "setter",
                             "func": "egui_view_linearlayout_set_auto_width",
                             "bool_to_int": True, "skip_default": True},
            },
            "auto_height": {
                "type": "bool", "default": False,
                "code_gen": {"kind": "setter",
                             "func": "egui_view_linearlayout_set_auto_height",
                             "bool_to_int": True, "skip_default": True},
            },
        },
    },
    xml_tag="LinearLayout",
    display_name="LinearLayout",
)

"""MP4 widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="mp4",
    descriptor={
        "c_type": "egui_view_mp4_t",
        "init_func": "egui_view_mp4_init_with_params",
        "params_macro": "EGUI_VIEW_MP4_PARAMS_INIT",
        "params_type": "egui_view_mp4_params_t",
        "is_container": False,
        "add_child_func": None,
        "layout_func": None,
        "properties": {
            "align_type": {
                "type": "align", "default": "EGUI_ALIGN_CENTER",
                "code_gen": {"kind": "setter", "func": "egui_view_mp4_set_align_type"},
            },
        },
    },
    xml_tag="Mp4",
    display_name="Mp4",
)

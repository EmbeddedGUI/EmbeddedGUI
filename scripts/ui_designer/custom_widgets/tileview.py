"""TileView widget plugin for EmbeddedGUI Designer."""
from ui_designer.model.widget_registry import WidgetRegistry

WidgetRegistry.instance().register(
    type_name="tileview",
    descriptor={
        "c_type": "egui_view_tileview_t",
        "init_func": "egui_view_tileview_init_with_params",
        "params_macro": "EGUI_VIEW_TILEVIEW_PARAMS_INIT",
        "params_type": "egui_view_tileview_params_t",
        "is_container": True,
        "properties": {
            "current_col": {
                "type": "int", "default": 0, "min": 0, "max": 255,
                "code_gen": {"kind": "setter",
                             "func": "egui_view_tileview_set_current",
                             "skip_default": True},
            },
        },
    },
    xml_tag="TileView",
    display_name="TileView",
)

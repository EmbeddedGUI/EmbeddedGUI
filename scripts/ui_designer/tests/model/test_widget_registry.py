"""Tests for WidgetRegistry."""

import os
import tempfile
import pytest

from ui_designer.model.widget_registry import WidgetRegistry

# Core widget types that should be loaded from custom_widgets/ plugins
_EXPECTED_TYPES = {
    "label", "button", "image", "group", "linearlayout", "scroll",
    "viewpage", "viewpage_cache", "switch", "progress_bar",
    "checkbox", "radio_button", "slider", "arc_slider",
    "circular_progress_bar", "spinner", "led", "toggle_button",
    "image_button", "textblock", "dynamic_label", "number_picker",
    "combobox", "roller", "page_indicator", "tab_bar", "chart",
    "card", "gridlayout", "divider", "gauge", "mp4", "textinput", "keyboard",
}

_EXPECTED_TAGS = {
    "Label": "label", "Button": "button", "Image": "image",
    "Group": "group", "LinearLayout": "linearlayout", "Scroll": "scroll",
    "ViewPage": "viewpage", "ViewPageCache": "viewpage_cache",
    "Switch": "switch", "ProgressBar": "progress_bar",
    "Checkbox": "checkbox", "RadioButton": "radio_button",
    "Slider": "slider", "ArcSlider": "arc_slider",
    "CircularProgressBar": "circular_progress_bar", "Spinner": "spinner",
    "Led": "led", "ToggleButton": "toggle_button",
    "ImageButton": "image_button", "Textblock": "textblock",
    "DynamicLabel": "dynamic_label", "NumberPicker": "number_picker",
    "Combobox": "combobox", "Roller": "roller",
    "PageIndicator": "page_indicator", "TabBar": "tab_bar",
    "Chart": "chart", "Card": "card", "GridLayout": "gridlayout",
    "Divider": "divider", "Gauge": "gauge", "Mp4": "mp4", "Textinput": "textinput", "Keyboard": "keyboard",
}


class TestWidgetRegistryBuiltins:
    """Test that built-in widgets are loaded correctly."""

    def setup_method(self):
        WidgetRegistry.reset()

    def test_instance_is_singleton(self):
        r1 = WidgetRegistry.instance()
        r2 = WidgetRegistry.instance()
        assert r1 is r2

    def test_reset_creates_new_instance(self):
        r1 = WidgetRegistry.instance()
        WidgetRegistry.reset()
        r2 = WidgetRegistry.instance()
        assert r1 is not r2

    def test_all_builtin_types_loaded(self):
        reg = WidgetRegistry.instance()
        for type_name in _EXPECTED_TYPES:
            assert reg.has(type_name), f"Missing type: {type_name}"

    def test_get_returns_descriptor(self):
        reg = WidgetRegistry.instance()
        info = reg.get("label")
        assert info["c_type"] == "egui_view_label_t"
        assert info["is_container"] is False

    def test_get_unknown_returns_empty_dict(self):
        reg = WidgetRegistry.instance()
        assert reg.get("nonexistent") == {}

    def test_has_returns_false_for_unknown(self):
        reg = WidgetRegistry.instance()
        assert reg.has("nonexistent") is False

    def test_tag_to_type_matches_builtin(self):
        reg = WidgetRegistry.instance()
        for tag, type_name in _EXPECTED_TAGS.items():
            assert reg.tag_to_type(tag) == type_name

    def test_type_to_tag_matches_builtin(self):
        reg = WidgetRegistry.instance()
        for tag, type_name in _EXPECTED_TAGS.items():
            assert reg.type_to_tag(type_name) == tag

    def test_addable_types_contains_all_builtins(self):
        reg = WidgetRegistry.instance()
        addable = reg.addable_types()
        type_names = [tn for _, tn in addable]
        for type_name in _EXPECTED_TYPES:
            if type_name == "keyboard":
                continue
            assert type_name in type_names, f"Missing from addable: {type_name}"

    def test_keyboard_is_not_addable_by_default(self):
        reg = WidgetRegistry.instance()
        type_names = [tn for _, tn in reg.addable_types()]
        assert "keyboard" not in type_names

    def test_container_types(self):
        reg = WidgetRegistry.instance()
        containers = reg.container_types()
        assert "group" in containers
        assert "linearlayout" in containers
        assert "scroll" in containers
        assert "viewpage" in containers
        assert "label" not in containers
        assert "button" not in containers


class TestWidgetRegistryCustom:
    """Test custom widget registration."""

    def setup_method(self):
        WidgetRegistry.reset()

    def test_register_custom_widget(self):
        reg = WidgetRegistry.instance()
        reg.register("gauge", {
            "c_type": "egui_view_gauge_t",
            "init_func": "egui_view_gauge_init",
            "is_container": False,
            "add_child_func": None,
            "layout_func": None,
            "properties": {
                "value": {"type": "int", "default": 0, "min": 0, "max": 360},
            },
        }, xml_tag="Gauge", display_name="Gauge")

        assert reg.has("gauge")
        assert reg.get("gauge")["c_type"] == "egui_view_gauge_t"
        assert reg.tag_to_type("Gauge") == "gauge"
        assert reg.type_to_tag("gauge") == "Gauge"

    def test_custom_widget_in_addable(self):
        reg = WidgetRegistry.instance()
        reg.register("gauge", {
            "c_type": "egui_view_gauge_t",
            "is_container": False,
            "properties": {},
        }, display_name="Gauge")

        addable = reg.addable_types()
        type_names = [tn for _, tn in addable]
        assert "gauge" in type_names

    def test_custom_container_in_container_types(self):
        reg = WidgetRegistry.instance()
        reg.register("custom_panel", {
            "c_type": "custom_panel_t",
            "is_container": True,
            "add_child_func": "custom_panel_add_child",
            "properties": {},
        })

        assert "custom_panel" in reg.container_types()

    def test_register_not_addable(self):
        reg = WidgetRegistry.instance()
        reg.register("internal_widget", {
            "c_type": "internal_t",
            "is_container": False,
            "addable": False,
            "properties": {},
        })

        assert reg.has("internal_widget")
        type_names = [tn for _, tn in reg.addable_types()]
        assert "internal_widget" not in type_names

    def test_auto_xml_tag_from_type_name(self):
        reg = WidgetRegistry.instance()
        reg.register("arc_slider", {
            "c_type": "egui_view_arc_slider_t",
            "is_container": False,
            "properties": {},
        })

        assert reg.type_to_tag("arc_slider") == "ArcSlider"
        assert reg.tag_to_type("ArcSlider") == "arc_slider"

    def test_re_register_updates_descriptor(self):
        reg = WidgetRegistry.instance()
        reg.register("gauge", {"c_type": "old_t", "is_container": False, "properties": {}})
        reg.register("gauge", {"c_type": "new_t", "is_container": False, "properties": {}})

        assert reg.get("gauge")["c_type"] == "new_t"
        # Should not duplicate in addable list
        type_names = [tn for _, tn in reg.addable_types()]
        assert type_names.count("gauge") == 1


class TestLoadCustomWidgets:
    """Test loading custom widget plugins from directory."""

    def setup_method(self):
        WidgetRegistry.reset()

    def test_load_from_directory(self):
        reg = WidgetRegistry.instance()

        with tempfile.TemporaryDirectory() as tmpdir:
            plugin = os.path.join(tmpdir, "test_gauge.py")
            with open(plugin, "w") as f:
                f.write(
                    "from ui_designer.model.widget_registry import WidgetRegistry\n"
                    "WidgetRegistry.instance().register('test_gauge', {\n"
                    "    'c_type': 'test_gauge_t',\n"
                    "    'is_container': False,\n"
                    "    'properties': {'value': {'type': 'int', 'default': 0}},\n"
                    "}, xml_tag='TestGauge')\n"
                )

            reg.load_custom_widgets(tmpdir)

        assert reg.has("test_gauge")
        assert reg.get("test_gauge")["c_type"] == "test_gauge_t"

    def test_load_skips_underscore_files(self):
        reg = WidgetRegistry.instance()

        with tempfile.TemporaryDirectory() as tmpdir:
            plugin = os.path.join(tmpdir, "_internal.py")
            with open(plugin, "w") as f:
                f.write(
                    "from ui_designer.model.widget_registry import WidgetRegistry\n"
                    "WidgetRegistry.instance().register('should_not_load', {\n"
                    "    'c_type': 'x', 'is_container': False, 'properties': {},\n"
                    "})\n"
                )

            reg.load_custom_widgets(tmpdir)

        assert not reg.has("should_not_load")

    def test_load_nonexistent_dir_no_error(self):
        reg = WidgetRegistry.instance()
        reg.load_custom_widgets("/nonexistent/path")
        # Should not raise

    def test_load_bad_plugin_does_not_crash(self):
        reg = WidgetRegistry.instance()

        with tempfile.TemporaryDirectory() as tmpdir:
            plugin = os.path.join(tmpdir, "bad.py")
            with open(plugin, "w") as f:
                f.write("raise RuntimeError('intentional error')\n")

            reg.load_custom_widgets(tmpdir)
            # Should not raise, just log the error


class TestCustomWidgetCodeGen:
    """Test that custom widgets generate correct C code."""

    def setup_method(self):
        WidgetRegistry.reset()

    def test_custom_widget_init_lines(self):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.generator.code_generator import _gen_widget_init_lines

        reg = WidgetRegistry.instance()
        reg.register("gauge", {
            "c_type": "egui_view_gauge_t",
            "init_func": "egui_view_gauge_init_with_params",
            "is_container": False,
            "add_child_func": None,
            "layout_func": None,
            "properties": {
                "value": {
                    "type": "int", "default": 0, "min": 0, "max": 360,
                    "code_gen": {"kind": "setter", "func": "egui_view_gauge_set_value"},
                },
                "color": {
                    "type": "color", "default": "EGUI_COLOR_WHITE",
                    "code_gen": {"kind": "setter", "func": "egui_view_gauge_set_color"},
                },
            },
        }, xml_tag="Gauge")

        w = WidgetModel("gauge", name="gauge_1", x=10, y=20, width=100, height=100)
        w.properties["value"] = 180
        w.properties["color"] = "EGUI_COLOR_RED"

        lines = _gen_widget_init_lines(w)
        code = "\n".join(lines)

        assert "egui_view_gauge_init(" in code
        assert "egui_view_set_position" in code
        assert "egui_view_gauge_set_value((egui_view_t *)&local->gauge_1, 180);" in code
        assert "egui_view_gauge_set_color((egui_view_t *)&local->gauge_1, EGUI_COLOR_RED);" in code

    def test_custom_widget_skip_default(self):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.generator.code_generator import _gen_widget_init_lines

        reg = WidgetRegistry.instance()
        reg.register("toggle", {
            "c_type": "toggle_t",
            "init_func": "toggle_init_with_params",
            "is_container": False,
            "add_child_func": None,
            "layout_func": None,
            "properties": {
                "is_on": {
                    "type": "bool", "default": False,
                    "code_gen": {"kind": "setter", "func": "toggle_set_on",
                                 "bool_to_int": True, "skip_default": True},
                },
            },
        })

        # Default value — should be skipped
        w = WidgetModel("toggle", name="t1")
        lines = _gen_widget_init_lines(w)
        code = "\n".join(lines)
        assert "toggle_set_on" not in code

        # Non-default value — should be emitted
        w2 = WidgetModel("toggle", name="t2")
        w2.properties["is_on"] = True
        lines2 = _gen_widget_init_lines(w2)
        code2 = "\n".join(lines2)
        assert "toggle_set_on((egui_view_t *)&local->t2, 1);" in code2

    def test_custom_widget_xml_round_trip(self):
        import xml.etree.ElementTree as ET
        from ui_designer.model.widget_model import WidgetModel

        reg = WidgetRegistry.instance()
        reg.register("gauge", {
            "c_type": "egui_view_gauge_t",
            "init_func": "egui_view_gauge_init",
            "is_container": False,
            "properties": {
                "value": {"type": "int", "default": 0, "min": 0, "max": 360},
                "color": {"type": "color", "default": "EGUI_COLOR_WHITE"},
            },
        }, xml_tag="Gauge")

        w = WidgetModel("gauge", name="g1", x=5, y=10, width=80, height=80)
        w.properties["value"] = 90
        w.properties["color"] = "EGUI_COLOR_BLUE"

        elem = w.to_xml_element()
        assert elem.tag == "Gauge"

        w2 = WidgetModel.from_xml_element(elem)
        assert w2.widget_type == "gauge"
        assert w2.name == "g1"
        assert w2.properties["value"] == 90
        assert w2.properties["color"] == "EGUI_COLOR_BLUE"

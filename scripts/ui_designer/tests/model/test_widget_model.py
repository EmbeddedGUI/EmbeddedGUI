"""Tests for ui_designer.model.widget_model — WidgetModel, BackgroundModel, helpers."""

import pytest
import xml.etree.ElementTree as ET

from ui_designer.model.widget_model import (
    WidgetModel,
    BackgroundModel,
    AnimationModel,
    ANIMATION_TYPES,
    INTERPOLATOR_TYPES,
    format_file_name,
    derive_image_c_expr,
    derive_font_c_expr,
    parse_legacy_image_expr,
    parse_legacy_font_expr,
)
from ui_designer.model.widget_registry import WidgetRegistry


# ── TestWidgetModelCreation ──────────────────────────────────────


class TestWidgetModelCreation:
    """Widget construction and auto-naming."""

    def test_default_properties_for_label(self):
        w = WidgetModel("label", name="lbl")
        assert w.properties["text"] == "Label"
        assert w.properties["color"] == "EGUI_COLOR_WHITE"
        assert w.properties["align_type"] == "EGUI_ALIGN_CENTER"

    def test_default_properties_for_button(self):
        w = WidgetModel("button", name="btn")
        assert w.properties["text"] == "Button"
        assert w.properties["color"] == "EGUI_COLOR_BLACK"

    def test_default_position_and_size(self):
        w = WidgetModel("label", name="lbl")
        assert w.x == 0
        assert w.y == 0
        assert w.width == 100
        assert w.height == 40

    def test_auto_name_generates_sequential_names(self):
        w1 = WidgetModel("label")
        w2 = WidgetModel("label")
        assert w1.name == "label_1"
        assert w2.name == "label_2"

    def test_auto_name_counter_per_type(self):
        l1 = WidgetModel("label")
        b1 = WidgetModel("button")
        assert l1.name == "label_1"
        assert b1.name == "button_1"

    def test_reset_counter(self):
        WidgetModel("label")  # label_1
        WidgetModel.reset_counter()
        w = WidgetModel("label")
        assert w.name == "label_1"

    def test_is_container_true_for_group(self):
        w = WidgetModel("group", name="g")
        assert w.is_container is True

    def test_is_container_true_for_linearlayout(self):
        w = WidgetModel("linearlayout", name="ll")
        assert w.is_container is True

    def test_is_container_false_for_label(self):
        w = WidgetModel("label", name="lbl")
        assert w.is_container is False

    def test_is_container_false_for_image(self):
        w = WidgetModel("image", name="img")
        assert w.is_container is False

    def test_margin_default_zero(self):
        w = WidgetModel("label", name="lbl")
        assert w.margin == 0

    def test_on_click_default_empty(self):
        w = WidgetModel("label", name="lbl")
        assert w.on_click == ""


# ── TestWidgetModelTree ──────────────────────────────────────────


class TestWidgetModelTree:
    """Parent/child tree operations."""

    def test_add_child_sets_parent(self):
        parent = WidgetModel("group", name="g")
        child = WidgetModel("label", name="c")
        parent.add_child(child)
        assert child.parent is parent
        assert child in parent.children

    def test_remove_child_clears_parent(self):
        parent = WidgetModel("group", name="g")
        child = WidgetModel("label", name="c")
        parent.add_child(child)
        parent.remove_child(child)
        assert child.parent is None
        assert child not in parent.children

    def test_get_all_widgets_flat_single(self):
        w = WidgetModel("label", name="solo")
        flat = w.get_all_widgets_flat()
        assert flat == [w]

    def test_get_all_widgets_flat_hierarchy(self):
        root = WidgetModel("group", name="root")
        c1 = WidgetModel("label", name="c1")
        c2 = WidgetModel("group", name="c2")
        c2_1 = WidgetModel("button", name="c2_1")
        root.add_child(c1)
        root.add_child(c2)
        c2.add_child(c2_1)
        flat = root.get_all_widgets_flat()
        assert len(flat) == 4
        assert flat[0] is root
        assert flat[1] is c1
        assert flat[2] is c2
        assert flat[3] is c2_1


# ── TestWidgetModelXmlRoundTrip ──────────────────────────────────


class TestWidgetModelXmlRoundTrip:
    """Serialize to XML and back; verify all fields survive."""

    @staticmethod
    def _round_trip(widget):
        """Serialize widget to XML element and parse it back."""
        elem = widget.to_xml_element()
        return WidgetModel.from_xml_element(elem)

    def test_label_round_trip(self):
        orig = WidgetModel("label", name="my_label", x=5, y=10, width=120, height=30)
        orig.properties["text"] = "Hello"
        restored = self._round_trip(orig)
        assert restored.widget_type == "label"
        assert restored.name == "my_label"
        assert restored.x == 5
        assert restored.y == 10
        assert restored.width == 120
        assert restored.height == 30
        assert restored.properties["text"] == "Hello"

    def test_button_round_trip(self):
        orig = WidgetModel("button", name="btn", x=0, y=0, width=80, height=40)
        orig.properties["text"] = "Click"
        restored = self._round_trip(orig)
        assert restored.widget_type == "button"
        assert restored.properties["text"] == "Click"

    def test_image_round_trip(self):
        orig = WidgetModel("image", name="img", x=0, y=0, width=64, height=64)
        orig.properties["image_file"] = "star.png"
        orig.properties["image_format"] = "rgb32"
        orig.properties["image_alpha"] = "8"
        restored = self._round_trip(orig)
        assert restored.properties["image_file"] == "star.png"
        assert restored.properties["image_format"] == "rgb32"
        assert restored.properties["image_alpha"] == "8"

    def test_group_round_trip_with_children(self, container_group):
        restored = self._round_trip(container_group)
        assert restored.widget_type == "group"
        assert len(restored.children) == 2
        assert restored.children[0].widget_type == "label"
        assert restored.children[1].widget_type == "button"
        # Children's parent should be set
        assert restored.children[0].parent is restored

    def test_linearlayout_round_trip(self):
        orig = WidgetModel("linearlayout", name="ll", x=0, y=0, width=200, height=300)
        orig.properties["orientation"] = "horizontal"
        orig.properties["align_type"] = "EGUI_ALIGN_LEFT"
        restored = self._round_trip(orig)
        assert restored.widget_type == "linearlayout"
        assert restored.properties["orientation"] == "horizontal"
        assert restored.properties["align_type"] == "EGUI_ALIGN_LEFT"

    def test_margin_preserved(self):
        orig = WidgetModel("label", name="m", x=0, y=0, width=100, height=40)
        orig.margin = 5
        restored = self._round_trip(orig)
        assert restored.margin == 5

    def test_on_click_preserved(self):
        orig = WidgetModel("button", name="btn")
        orig.on_click = "on_my_click"
        restored = self._round_trip(orig)
        assert restored.on_click == "on_my_click"

    def test_default_properties_not_emitted_in_xml(self):
        """Default property values should NOT appear as XML attributes."""
        orig = WidgetModel("label", name="lbl")
        elem = orig.to_xml_element()
        # "text" default is "Label", so it should NOT be in attributes
        assert "text" not in elem.attrib
        assert "color" not in elem.attrib

    def test_non_default_properties_emitted_in_xml(self):
        orig = WidgetModel("label", name="lbl")
        orig.properties["text"] = "Custom"
        elem = orig.to_xml_element()
        assert elem.get("text") == "Custom"

    def test_margin_zero_not_emitted(self):
        orig = WidgetModel("label", name="lbl")
        elem = orig.to_xml_element()
        assert "margin" not in elem.attrib

    def test_on_click_empty_not_emitted(self):
        orig = WidgetModel("label", name="lbl")
        elem = orig.to_xml_element()
        assert "onClick" not in elem.attrib

    def test_xml_tag_mapping(self):
        """Widget type maps to correct XML tag."""
        reg = WidgetRegistry.instance()
        for wtype, tag in reg._rev_tag_map.items():
            w = WidgetModel(wtype, name="test")
            elem = w.to_xml_element()
            assert elem.tag == tag, f"{wtype} should produce tag {tag}"

    def test_switch_bool_round_trip(self):
        orig = WidgetModel("switch", name="sw")
        orig.properties["is_checked"] = True
        restored = self._round_trip(orig)
        assert restored.properties["is_checked"] is True

    def test_progress_bar_int_round_trip(self):
        orig = WidgetModel("progress_bar", name="pb")
        orig.properties["value"] = 75
        restored = self._round_trip(orig)
        assert restored.properties["value"] == 75

    def test_designer_flags_round_trip_via_dict_and_xml(self):
        orig = WidgetModel("label", name="flagged", x=12, y=24, width=80, height=20)
        orig.designer_locked = True
        orig.designer_hidden = True

        restored_from_dict = WidgetModel.from_dict(orig.to_dict())
        assert restored_from_dict.designer_locked is True
        assert restored_from_dict.designer_hidden is True

        restored_from_xml = self._round_trip(orig)
        assert restored_from_xml.designer_locked is True
        assert restored_from_xml.designer_hidden is True


# ── TestBackgroundModel ──────────────────────────────────────────


class TestBackgroundModel:
    """BackgroundModel XML and dict serialization."""

    def test_solid_bg_xml_round_trip(self):
        bg = BackgroundModel()
        bg.bg_type = "solid"
        bg.color = "EGUI_COLOR_RED"
        bg.alpha = "EGUI_ALPHA_80"
        elem = bg.to_xml_element()
        restored = BackgroundModel.from_xml_element(elem)
        assert restored.bg_type == "solid"
        assert restored.color == "EGUI_COLOR_RED"
        assert restored.alpha == "EGUI_ALPHA_80"

    def test_round_rectangle_bg_xml_round_trip(self):
        bg = BackgroundModel()
        bg.bg_type = "round_rectangle"
        bg.radius = 10
        elem = bg.to_xml_element()
        restored = BackgroundModel.from_xml_element(elem)
        assert restored.bg_type == "round_rectangle"
        assert restored.radius == 10

    def test_round_rectangle_corners_bg_xml_round_trip(self):
        bg = BackgroundModel()
        bg.bg_type = "round_rectangle_corners"
        bg.radius_left_top = 5
        bg.radius_left_bottom = 10
        bg.radius_right_top = 15
        bg.radius_right_bottom = 20
        elem = bg.to_xml_element()
        restored = BackgroundModel.from_xml_element(elem)
        assert restored.radius_left_top == 5
        assert restored.radius_left_bottom == 10
        assert restored.radius_right_top == 15
        assert restored.radius_right_bottom == 20

    def test_stroke_emitted_when_nonzero(self):
        bg = BackgroundModel()
        bg.bg_type = "solid"
        bg.stroke_width = 2
        bg.stroke_color = "EGUI_COLOR_BLUE"
        bg.stroke_alpha = "EGUI_ALPHA_50"
        elem = bg.to_xml_element()
        assert elem.get("stroke_width") == "2"
        restored = BackgroundModel.from_xml_element(elem)
        assert restored.stroke_width == 2
        assert restored.stroke_color == "EGUI_COLOR_BLUE"
        assert restored.stroke_alpha == "EGUI_ALPHA_50"

    def test_stroke_not_emitted_when_zero(self):
        bg = BackgroundModel()
        bg.bg_type = "solid"
        bg.stroke_width = 0
        elem = bg.to_xml_element()
        assert "stroke_width" not in elem.attrib

    def test_pressed_state_round_trip(self):
        bg = BackgroundModel()
        bg.bg_type = "solid"
        bg.has_pressed = True
        bg.pressed_color = "EGUI_COLOR_GREEN"
        bg.pressed_alpha = "EGUI_ALPHA_60"
        elem = bg.to_xml_element()
        restored = BackgroundModel.from_xml_element(elem)
        assert restored.has_pressed is True
        assert restored.pressed_color == "EGUI_COLOR_GREEN"
        assert restored.pressed_alpha == "EGUI_ALPHA_60"

    def test_disabled_state_round_trip(self):
        bg = BackgroundModel()
        bg.bg_type = "solid"
        bg.has_disabled = True
        bg.disabled_color = "EGUI_COLOR_DARK_GREY"
        bg.disabled_alpha = "EGUI_ALPHA_40"
        elem = bg.to_xml_element()
        restored = BackgroundModel.from_xml_element(elem)
        assert restored.has_disabled is True
        assert restored.disabled_color == "EGUI_COLOR_DARK_GREY"
        assert restored.disabled_alpha == "EGUI_ALPHA_40"

    def test_dict_round_trip(self):
        bg = BackgroundModel()
        bg.bg_type = "round_rectangle"
        bg.color = "EGUI_COLOR_NAVY"
        bg.radius = 8
        bg.stroke_width = 1
        d = bg.to_dict()
        restored = BackgroundModel.from_dict(d)
        assert restored.bg_type == "round_rectangle"
        assert restored.color == "EGUI_COLOR_NAVY"
        assert restored.radius == 8
        assert restored.stroke_width == 1

    def test_widget_with_background_xml_round_trip(self):
        """Background attached to a widget survives XML round trip."""
        w = WidgetModel("button", name="btn", x=0, y=0, width=80, height=40)
        bg = BackgroundModel()
        bg.bg_type = "solid"
        bg.color = "EGUI_COLOR_BLUE"
        w.background = bg
        elem = w.to_xml_element()
        restored = WidgetModel.from_xml_element(elem)
        assert restored.background is not None
        assert restored.background.bg_type == "solid"
        assert restored.background.color == "EGUI_COLOR_BLUE"

    def test_none_bg_not_emitted(self):
        """Background with type 'none' should not produce a child element."""
        w = WidgetModel("label", name="lbl")
        w.background = BackgroundModel()  # default bg_type = "none"
        elem = w.to_xml_element()
        assert elem.find("Background") is None


# ── TestHelpers ──────────────────────────────────────────────────


class TestHelpers:
    """Helper functions for C name derivation and legacy parsing."""

    # -- format_file_name --

    def test_format_file_name_basic(self):
        assert format_file_name("star.png") == "star_png"

    def test_format_file_name_dashes_and_spaces(self):
        assert format_file_name("my-image file.bmp") == "my_image_file_bmp"

    def test_format_file_name_path_separators(self):
        result = format_file_name("path/to\\file.png")
        assert "/" not in result
        assert "\\" not in result

    def test_format_file_name_double_underscores_collapsed(self):
        # parentheses + dots can produce consecutive underscores
        result = format_file_name("a((b)).png")
        assert "__" not in result

    def test_format_file_name_lowercase(self):
        assert format_file_name("MyImage.PNG") == "myimage_png"

    # -- derive_image_c_expr --

    def test_derive_image_c_expr_with_file(self):
        w = WidgetModel("image", name="img")
        w.properties["image_file"] = "star.png"
        w.properties["image_format"] = "rgb565"
        w.properties["image_alpha"] = "4"
        assert derive_image_c_expr(w) == "&egui_res_image_star_rgb565_4"

    def test_derive_image_c_expr_null_when_no_file(self):
        w = WidgetModel("image", name="img")
        w.properties["image_file"] = ""
        assert derive_image_c_expr(w) == "NULL"

    def test_derive_image_c_expr_different_format(self):
        w = WidgetModel("image", name="img")
        w.properties["image_file"] = "icon.bmp"
        w.properties["image_format"] = "rgb32"
        w.properties["image_alpha"] = "8"
        assert derive_image_c_expr(w) == "&egui_res_image_icon_rgb32_8"

    # -- derive_font_c_expr --

    def test_derive_font_c_expr_builtin(self):
        w = WidgetModel("label", name="lbl")
        w.properties["font_file"] = ""
        w.properties["font_builtin"] = "EGUI_CONFIG_FONT_DEFAULT"
        assert derive_font_c_expr(w) == "EGUI_CONFIG_FONT_DEFAULT"

    def test_derive_font_c_expr_custom_file(self):
        w = WidgetModel("label", name="lbl")
        w.properties["font_file"] = "myfont.ttf"
        w.properties["font_pixelsize"] = "24"
        w.properties["font_fontbitsize"] = "4"
        assert derive_font_c_expr(w) == "&egui_res_font_myfont_24_4"

    # -- parse_legacy_image_expr --

    def test_parse_legacy_image_expr_valid(self):
        result = parse_legacy_image_expr("&egui_res_image_star_rgb565_4")
        assert result == {"name": "star", "format": "rgb565", "alpha": "4"}

    def test_parse_legacy_image_expr_rgb32(self):
        result = parse_legacy_image_expr("&egui_res_image_icon_rgb32_8")
        assert result == {"name": "icon", "format": "rgb32", "alpha": "8"}

    def test_parse_legacy_image_expr_invalid(self):
        assert parse_legacy_image_expr("NULL") is None
        assert parse_legacy_image_expr("random_string") is None

    # -- parse_legacy_font_expr --

    def test_parse_legacy_font_expr_valid(self):
        result = parse_legacy_font_expr("&egui_res_font_montserrat_16_4")
        assert result == {"name": "montserrat", "pixelsize": "16", "fontbitsize": "4"}

    def test_parse_legacy_font_expr_custom(self):
        result = parse_legacy_font_expr("&egui_res_font_myfont_24_8")
        assert result == {"name": "myfont", "pixelsize": "24", "fontbitsize": "8"}

    def test_parse_legacy_font_expr_invalid(self):
        assert parse_legacy_font_expr("EGUI_CONFIG_FONT_DEFAULT") is None
        assert parse_legacy_font_expr("not_a_font") is None


# ── TestConstants ────────────────────────────────────────────────


class TestConstants:
    """Verify tag/type mappings are consistent in the registry."""

    def test_tag_to_type_and_type_to_tag_are_inverses(self):
        reg = WidgetRegistry.instance()
        for tag, wtype in reg._tag_map.items():
            assert reg.type_to_tag(wtype) == tag

    def test_all_widget_types_have_tag(self):
        reg = WidgetRegistry.instance()
        for wtype in reg.all_types():
            assert reg.type_to_tag(wtype), f"widget type '{wtype}' missing tag"


# ── TestAnimationModel ─────────────────────────────────────────────


class TestAnimationModel:
    """AnimationModel creation, serialization, and XML round-trip."""

    def test_default_values(self):
        a = AnimationModel()
        assert a.anim_type == "alpha"
        assert a.duration == 500
        assert a.interpolator == "linear"
        assert a.repeat_count == 0
        assert a.repeat_mode == "restart"
        assert a.auto_start is True
        assert a.params == {}

    def test_dict_round_trip(self):
        a = AnimationModel()
        a.anim_type = "translate"
        a.duration = 1000
        a.interpolator = "bounce"
        a.repeat_count = 3
        a.repeat_mode = "reverse"
        a.auto_start = False
        a.params = {"from_x": "0", "to_x": "100", "from_y": "0", "to_y": "50"}
        d = a.to_dict()
        a2 = AnimationModel.from_dict(d)
        assert a2.anim_type == "translate"
        assert a2.duration == 1000
        assert a2.interpolator == "bounce"
        assert a2.repeat_count == 3
        assert a2.repeat_mode == "reverse"
        assert a2.auto_start is False
        assert a2.params == {"from_x": "0", "to_x": "100", "from_y": "0", "to_y": "50"}

    def test_xml_round_trip_alpha(self):
        a = AnimationModel()
        a.anim_type = "alpha"
        a.duration = 300
        a.params = {"from_alpha": "0", "to_alpha": "255"}
        elem = a.to_xml_element()
        assert elem.tag == "Animation"
        assert elem.get("type") == "alpha"
        assert elem.get("duration") == "300"
        a2 = AnimationModel.from_xml_element(elem)
        assert a2.anim_type == "alpha"
        assert a2.duration == 300
        assert a2.params["from_alpha"] == "0"
        assert a2.params["to_alpha"] == "255"

    def test_xml_round_trip_scale(self):
        a = AnimationModel()
        a.anim_type = "scale"
        a.duration = 800
        a.interpolator = "overshoot"
        a.params = {"from_scale": "EGUI_FLOAT_VALUE(0.5)", "to_scale": "EGUI_FLOAT_VALUE(1.0)"}
        elem = a.to_xml_element()
        a2 = AnimationModel.from_xml_element(elem)
        assert a2.anim_type == "scale"
        assert a2.interpolator == "overshoot"
        assert a2.params["from_scale"] == "EGUI_FLOAT_VALUE(0.5)"

    def test_xml_optional_attrs_omitted(self):
        """Default repeat_count/mode/auto_start should not appear in XML."""
        a = AnimationModel()
        elem = a.to_xml_element()
        assert "repeat_count" not in elem.attrib
        assert "repeat_mode" not in elem.attrib
        assert "auto_start" not in elem.attrib

    def test_xml_non_default_attrs_present(self):
        a = AnimationModel()
        a.repeat_count = 5
        a.repeat_mode = "reverse"
        a.auto_start = False
        elem = a.to_xml_element()
        assert elem.get("repeat_count") == "5"
        assert elem.get("repeat_mode") == "reverse"
        assert elem.get("auto_start") == "false"

    def test_widget_with_animation_xml_round_trip(self):
        w = WidgetModel("label", name="title", x=10, y=20, width=100, height=30)
        anim = AnimationModel()
        anim.anim_type = "alpha"
        anim.duration = 500
        anim.params = {"from_alpha": "0", "to_alpha": "255"}
        w.animations.append(anim)
        elem = w.to_xml_element()
        anim_elems = elem.findall("Animation")
        assert len(anim_elems) == 1
        w2 = WidgetModel.from_xml_element(elem)
        assert len(w2.animations) == 1
        assert w2.animations[0].anim_type == "alpha"
        assert w2.animations[0].duration == 500

    def test_animation_types_registry(self):
        assert "alpha" in ANIMATION_TYPES
        assert "translate" in ANIMATION_TYPES
        assert "scale" in ANIMATION_TYPES
        for atype, info in ANIMATION_TYPES.items():
            assert "c_type" in info
            assert "init_func" in info
            assert "params" in info

    def test_interpolator_types_registry(self):
        assert len(INTERPOLATOR_TYPES) == 9
        assert "linear" in INTERPOLATOR_TYPES
        assert "bounce" in INTERPOLATOR_TYPES


# ── TestEventCallbacks ─────────────────────────────────────────────


class TestEventCallbacks:
    """Tests for widget event callback support."""

    def test_events_default_empty(self):
        w = WidgetModel("slider", name="s1")
        assert w.events == {}

    def test_events_dict_round_trip(self):
        w = WidgetModel("slider", name="s1")
        w.events = {"onValueChanged": "on_slider_changed"}
        d = w.to_dict()
        assert d["events"] == {"onValueChanged": "on_slider_changed"}
        w2 = WidgetModel.from_dict(d)
        assert w2.events == {"onValueChanged": "on_slider_changed"}

    def test_events_xml_round_trip(self):
        w = WidgetModel("slider", name="s1", x=0, y=0, width=200, height=30)
        w.events = {"onValueChanged": "my_slider_cb"}
        elem = w.to_xml_element()
        assert elem.get("onValueChanged") == "my_slider_cb"
        w2 = WidgetModel.from_xml_element(elem)
        assert w2.events.get("onValueChanged") == "my_slider_cb"

    def test_events_xml_omitted_when_empty(self):
        w = WidgetModel("slider", name="s1")
        elem = w.to_xml_element()
        assert "onValueChanged" not in elem.attrib

    def test_switch_event_round_trip(self):
        w = WidgetModel("switch", name="sw1")
        w.events = {"onCheckedChanged": "on_switch_toggled"}
        elem = w.to_xml_element()
        assert elem.get("onCheckedChanged") == "on_switch_toggled"
        w2 = WidgetModel.from_xml_element(elem)
        assert w2.events["onCheckedChanged"] == "on_switch_toggled"

    def test_multiple_widgets_events_registry(self):
        """Verify that event-supporting widgets have events in their registry."""
        reg = WidgetRegistry.instance()
        event_widgets = [
            "slider", "arc_slider", "number_picker",
            "switch", "checkbox", "toggle_button",
            "combobox", "roller", "tab_bar",
            "progress_bar", "circular_progress_bar",
        ]
        for wt in event_widgets:
            info = reg.get(wt)
            assert "events" in info, f"{wt} missing events in registry"
            assert len(info["events"]) > 0, f"{wt} has empty events"

"""Tests for resource binding helpers."""

from ui_designer.model.resource_binding import (
    assign_resource_to_widget,
    find_widget_resource_property,
    resource_property_type,
)
from ui_designer.model.widget_model import WidgetModel


class TestResourceBinding:
    def test_resource_property_type_maps_known_types(self):
        assert resource_property_type("image") == "image_file"
        assert resource_property_type("font") == "font_file"
        assert resource_property_type("text") == "text_file"
        assert resource_property_type("unknown") == ""

    def test_find_widget_resource_property_prefers_exact_match(self):
        image = WidgetModel("image", name="hero")

        assert find_widget_resource_property(image, "image") == "image_file"

    def test_find_widget_resource_property_falls_back_to_font_text_file(self):
        label = WidgetModel("label", name="title")

        assert find_widget_resource_property(label, "text") == "font_text_file"

    def test_assign_resource_to_widget_updates_matching_property(self):
        label = WidgetModel("label", name="title")

        prop_name = assign_resource_to_widget(label, "text", "chars.txt")

        assert prop_name == "font_text_file"
        assert label.properties["font_text_file"] == "chars.txt"

    def test_assign_resource_to_widget_returns_empty_when_widget_has_no_match(self):
        group = WidgetModel("group", name="root_group")

        prop_name = assign_resource_to_widget(group, "text", "chars.txt")

        assert prop_name == ""

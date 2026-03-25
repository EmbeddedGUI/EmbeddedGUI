"""Tests for ui_designer.model.page.Page."""

import os

import pytest

from ui_designer.model.page import Page
from ui_designer.model.widget_model import WidgetModel


class TestPageNameProperties:
    """Tests for name, c_struct_name, c_prefix derived from file_path."""

    def test_name_from_file_path(self):
        page = Page(file_path="layout/main_page.xml")
        assert page.name == "main_page"

    def test_c_struct_name(self):
        page = Page(file_path="layout/main_page.xml")
        assert page.c_struct_name == "egui_main_page_t"

    def test_c_prefix(self):
        page = Page(file_path="layout/main_page.xml")
        assert page.c_prefix == "egui_main_page"


class TestCreateDefault:
    """Tests for Page.create_default factory."""

    def test_create_default(self):
        page = Page.create_default("home_screen", screen_width=320, screen_height=480)

        assert page.root_widget is not None
        assert page.root_widget.widget_type == "group"
        assert page.root_widget.width == 320
        assert page.root_widget.height == 480
        assert page.file_path == "layout/home_screen.xml"
        assert page.name == "home_screen"
        assert page.dirty is True


class TestXmlSerialization:
    """Tests for XML round-trip serialization."""

    def test_to_xml_from_xml_round_trip(self):
        root = WidgetModel("group", name="root_group", x=0, y=0, width=240, height=320)
        child = WidgetModel("label", name="title_label", x=10, y=10, width=200, height=30)
        child.properties["text"] = "Hello World"
        root.add_child(child)

        original = Page(file_path="layout/test_page.xml", root_widget=root)
        xml_str = original.to_xml_string()

        restored = Page.from_xml_string(xml_str, file_path="layout/test_page.xml")

        assert restored.name == "test_page"
        assert restored.root_widget is not None
        assert restored.root_widget.widget_type == "group"
        assert len(restored.root_widget.children) == 1
        assert restored.root_widget.children[0].name == "title_label"
        assert restored.root_widget.children[0].properties["text"] == "Hello World"

    def test_user_fields_preserved(self):
        root = WidgetModel("group", name="root_group", x=0, y=0, width=240, height=320)
        page = Page(file_path="layout/test_page.xml", root_widget=root)
        page.user_fields = [
            {"name": "counter", "type": "int", "default": "0"},
            {"name": "timer_id", "type": "int"},
        ]

        xml_str = page.to_xml_string()
        restored = Page.from_xml_string(xml_str, file_path="layout/test_page.xml")

        assert len(restored.user_fields) == 2
        assert restored.user_fields[0]["name"] == "counter"
        assert restored.user_fields[0]["type"] == "int"
        assert restored.user_fields[0]["default"] == "0"
        assert restored.user_fields[1]["name"] == "timer_id"
        assert restored.user_fields[1]["type"] == "int"

    def test_timers_preserved(self):
        root = WidgetModel("group", name="root_group", x=0, y=0, width=240, height=320)
        page = Page(file_path="layout/test_page.xml", root_widget=root)
        page.timers = [
            {
                "name": "refresh_timer",
                "callback": "tick_refresh",
                "delay_ms": "500",
                "period_ms": "1000",
                "auto_start": True,
            }
        ]

        xml_str = page.to_xml_string()
        restored = Page.from_xml_string(xml_str, file_path="layout/test_page.xml")

        assert restored.timers == [
            {
                "name": "refresh_timer",
                "callback": "tick_refresh",
                "delay_ms": "500",
                "period_ms": "1000",
                "auto_start": True,
            }
        ]

    def test_mockup_attributes_preserved(self):
        root = WidgetModel("group", name="root_group", x=0, y=0, width=240, height=320)
        page = Page(file_path="layout/test_page.xml", root_widget=root)
        page.mockup_image_path = "mockup/design.png"
        page.mockup_image_visible = False
        page.mockup_image_opacity = 0.5

        xml_str = page.to_xml_string()
        restored = Page.from_xml_string(xml_str, file_path="layout/test_page.xml")

        assert restored.mockup_image_path == "mockup/design.png"
        assert restored.mockup_image_visible is False
        assert restored.mockup_image_opacity == pytest.approx(0.5)


class TestGetAllWidgets:
    """Tests for get_all_widgets."""

    def test_get_all_widgets(self):
        root = WidgetModel("group", name="root_group", x=0, y=0, width=240, height=320)
        label = WidgetModel("label", name="lbl", x=0, y=0, width=100, height=30)
        button = WidgetModel("button", name="btn", x=0, y=40, width=100, height=40)
        root.add_child(label)
        root.add_child(button)

        page = Page(file_path="layout/test_page.xml", root_widget=root)
        all_widgets = page.get_all_widgets()

        # root + 2 children = 3
        assert len(all_widgets) == 3


class TestFileIO:
    """Tests for save/load file operations."""

    @pytest.mark.integration
    def test_save_load_round_trip(self, tmp_path):
        root = WidgetModel("group", name="root_group", x=0, y=0, width=240, height=320)
        label = WidgetModel("label", name="my_label", x=5, y=5, width=100, height=30)
        label.properties["text"] = "Saved Label"
        root.add_child(label)

        original = Page(file_path="layout/round_trip.xml", root_widget=root)
        original.save(str(tmp_path))

        loaded = Page.load(str(tmp_path), "layout/round_trip.xml")

        assert loaded.name == "round_trip"
        assert loaded.root_widget is not None
        assert loaded.root_widget.widget_type == "group"
        assert len(loaded.root_widget.children) == 1
        assert loaded.root_widget.children[0].properties["text"] == "Saved Label"


class TestDirtyTracking:
    """Tests for the dirty flag."""

    def test_dirty_tracking(self, tmp_path):
        page = Page(file_path="layout/dirty_test.xml")
        assert page.dirty is False

        page.dirty = True
        assert page.dirty is True

        # Saving should clear the dirty flag
        page.root_widget = WidgetModel("group", name="root_group", x=0, y=0, width=240, height=320)
        page.save(str(tmp_path))
        assert page.dirty is False

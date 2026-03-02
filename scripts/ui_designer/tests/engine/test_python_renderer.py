"""Tests for the Python preview renderer."""

import pytest
from PIL import Image

from ui_designer.engine.python_renderer import (
    _resolve_color,
    _resolve_alpha,
    render_page,
    render_page_to_bytes,
)
from ui_designer.model.widget_model import WidgetModel, BackgroundModel
from ui_designer.model.page import Page


class TestResolveColor:
    def test_named_color(self):
        assert _resolve_color("EGUI_COLOR_RED") == (255, 0, 0)

    def test_hex_color(self):
        assert _resolve_color("EGUI_COLOR_HEX(0x123456)") == (18, 52, 86)

    def test_unknown_color(self):
        assert _resolve_color("UNKNOWN") == (200, 200, 200)

    def test_empty(self):
        assert _resolve_color("") == (200, 200, 200)
        assert _resolve_color(None) == (200, 200, 200)


class TestResolveAlpha:
    def test_alpha_100(self):
        assert _resolve_alpha("EGUI_ALPHA_100") == 255

    def test_alpha_50(self):
        assert _resolve_alpha("EGUI_ALPHA_50") == 127

    def test_alpha_0(self):
        assert _resolve_alpha("EGUI_ALPHA_0") == 0

    def test_empty(self):
        assert _resolve_alpha("") == 255
        assert _resolve_alpha(None) == 255


class TestRenderPage:
    def test_empty_page(self):
        page = Page(file_path="test.xml", root_widget=None)
        img = render_page(page, 240, 320)
        assert isinstance(img, Image.Image)
        assert img.size == (240, 320)
        assert img.mode == "RGBA"

    def test_page_with_label(self):
        root = WidgetModel("group", name="root", x=0, y=0, width=240, height=320)
        label = WidgetModel("label", name="lbl", x=10, y=10, width=100, height=30)
        label.properties["text"] = "Hello"
        root.add_child(label)
        page = Page(file_path="test.xml", root_widget=root)
        img = render_page(page, 240, 320)
        assert img.size == (240, 320)

    def test_page_with_button(self):
        root = WidgetModel("group", name="root", x=0, y=0, width=240, height=320)
        btn = WidgetModel("button", name="btn", x=10, y=50, width=100, height=40)
        btn.properties["text"] = "Click"
        root.add_child(btn)
        page = Page(file_path="test.xml", root_widget=root)
        img = render_page(page, 240, 320)
        assert img.size == (240, 320)

    def test_page_with_progress_bar(self):
        root = WidgetModel("group", name="root", x=0, y=0, width=240, height=320)
        pb = WidgetModel("progress_bar", name="pb", x=10, y=100, width=200, height=20)
        pb.properties["value"] = 75
        root.add_child(pb)
        page = Page(file_path="test.xml", root_widget=root)
        img = render_page(page, 240, 320)
        assert img.size == (240, 320)

    def test_page_with_background(self):
        root = WidgetModel("group", name="root", x=0, y=0, width=240, height=320)
        root.background = BackgroundModel()
        root.background.bg_type = "round_rectangle"
        root.background.color = "EGUI_COLOR_BLUE"
        root.background.radius = 10
        page = Page(file_path="test.xml", root_widget=root)
        img = render_page(page, 240, 320)
        assert img.size == (240, 320)

    def test_render_to_bytes(self):
        root = WidgetModel("group", name="root", x=0, y=0, width=240, height=320)
        page = Page(file_path="test.xml", root_widget=root)
        data = render_page_to_bytes(page, 240, 320)
        assert isinstance(data, bytes)
        assert len(data) > 0
        # Verify it's valid PNG
        assert data[:4] == b'\x89PNG'

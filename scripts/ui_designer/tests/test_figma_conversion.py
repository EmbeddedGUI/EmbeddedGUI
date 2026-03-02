"""Tests for Figma conversion precision improvements in html2egui_helper.py."""

import os
import sys
import pytest

# Add parent paths so we can import the helper module
sys.path.insert(0, os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "..")))

import html2egui_helper as h


class TestFigmaColorToHex:
    """Test _figma_color_to_hex precision."""

    def test_pure_red(self):
        assert h._figma_color_to_hex({"r": 1.0, "g": 0.0, "b": 0.0}) == "FF0000"

    def test_pure_white(self):
        assert h._figma_color_to_hex({"r": 1.0, "g": 1.0, "b": 1.0}) == "FFFFFF"

    def test_pure_black(self):
        assert h._figma_color_to_hex({"r": 0.0, "g": 0.0, "b": 0.0}) == "000000"

    def test_rounding_precision(self):
        # 0.502 * 255 = 128.01 -> should round to 128 = 0x80
        result = h._figma_color_to_hex({"r": 0.502, "g": 0.502, "b": 0.502})
        assert result == "808080"

    def test_high_precision_rounding(self):
        # 0.498 * 255 = 126.99 -> should round to 127 = 0x7F
        result = h._figma_color_to_hex({"r": 0.498, "g": 0.498, "b": 0.498})
        assert result == "7F7F7F"

    def test_none_input(self):
        assert h._figma_color_to_hex(None) is None

    def test_empty_dict(self):
        assert h._figma_color_to_hex({}) == "000000"


class TestFigmaOpacityToAlpha:
    """Test _figma_opacity_to_alpha precision."""

    def test_full_opacity(self):
        assert h._figma_opacity_to_alpha(1.0) == "EGUI_ALPHA_100"

    def test_none_opacity(self):
        assert h._figma_opacity_to_alpha(None) == "EGUI_ALPHA_100"

    def test_half_opacity(self):
        assert h._figma_opacity_to_alpha(0.5) == "EGUI_ALPHA_50"

    def test_zero_opacity(self):
        assert h._figma_opacity_to_alpha(0.0) == "EGUI_ALPHA_0"

    def test_75_percent(self):
        # 0.75 should map to nearest 10% = 80
        result = h._figma_opacity_to_alpha(0.75)
        assert result == "EGUI_ALPHA_80"


class TestFigmaColorWithAlpha:
    """Test _figma_color_with_alpha for RGBA precision."""

    def test_solid_color(self):
        color = {"r": 1.0, "g": 0.0, "b": 0.0}
        hex_str, alpha = h._figma_color_with_alpha(color, 1.0)
        assert hex_str == "FF0000"
        assert alpha == "EGUI_ALPHA_100"

    def test_semi_transparent(self):
        color = {"r": 0.0, "g": 0.0, "b": 1.0}
        hex_str, alpha = h._figma_color_with_alpha(color, 0.5)
        assert hex_str == "0000FF"
        assert alpha == "EGUI_ALPHA_50"

    def test_color_with_alpha_channel(self):
        color = {"r": 1.0, "g": 1.0, "b": 1.0, "a": 0.5}
        hex_str, alpha = h._figma_color_with_alpha(color, 1.0)
        assert hex_str == "FFFFFF"
        assert alpha == "EGUI_ALPHA_50"

    def test_combined_alpha(self):
        # color.a=0.5 * opacity=0.5 = 0.25 -> round(2.5)=2 -> EGUI_ALPHA_20
        color = {"r": 1.0, "g": 0.0, "b": 0.0, "a": 0.5}
        hex_str, alpha = h._figma_color_with_alpha(color, 0.5)
        assert hex_str == "FF0000"
        assert alpha == "EGUI_ALPHA_20"


class TestFigmaAutoLayoutPadding:
    """Test auto-layout padding extraction from Figma nodes."""

    def test_uniform_padding(self):
        node = {
            "type": "FRAME",
            "name": "container",
            "absoluteBoundingBox": {"x": 0, "y": 0, "width": 200, "height": 100},
            "layoutMode": "VERTICAL",
            "paddingLeft": 16, "paddingRight": 16,
            "paddingTop": 16, "paddingBottom": 16,
            "itemSpacing": 8,
            "children": [],
        }
        xml = h._figma_node_to_xml(node)
        assert 'padding_left="16"' in xml
        assert 'padding_right="16"' in xml
        assert 'padding_top="16"' in xml
        assert 'padding_bottom="16"' in xml

    def test_asymmetric_padding(self):
        node = {
            "type": "FRAME",
            "name": "card",
            "absoluteBoundingBox": {"x": 0, "y": 0, "width": 200, "height": 100},
            "layoutMode": "HORIZONTAL",
            "paddingLeft": 12, "paddingRight": 12,
            "paddingTop": 8, "paddingBottom": 8,
            "itemSpacing": 4,
            "children": [],
        }
        xml = h._figma_node_to_xml(node)
        assert 'padding_left="12"' in xml
        assert 'padding_top="8"' in xml


class TestFigmaVariantDetection:
    """Test Figma component variant detection."""

    def test_detect_button_variants(self):
        node = {
            "type": "COMPONENT_SET",
            "name": "Button",
            "absoluteBoundingBox": {"x": 0, "y": 0, "width": 120, "height": 40},
            "children": [
                {
                    "type": "COMPONENT",
                    "name": "State=Default",
                    "absoluteBoundingBox": {"x": 0, "y": 0, "width": 120, "height": 40},
                    "fills": [{"visible": True, "type": "SOLID", "color": {"r": 0.0, "g": 0.5, "b": 1.0}}],
                    "children": [],
                },
                {
                    "type": "COMPONENT",
                    "name": "State=Pressed",
                    "absoluteBoundingBox": {"x": 0, "y": 0, "width": 120, "height": 40},
                    "fills": [{"visible": True, "type": "SOLID", "color": {"r": 0.0, "g": 0.4, "b": 0.8}}],
                    "children": [],
                },
            ],
        }
        variants = h._figma_detect_variants(node)
        assert "default" in variants
        assert "pressed" in variants

    def test_no_variants(self):
        node = {
            "type": "FRAME",
            "name": "Simple",
            "absoluteBoundingBox": {"x": 0, "y": 0, "width": 100, "height": 50},
            "children": [],
        }
        variants = h._figma_detect_variants(node)
        assert variants == {}


class TestFigmaGradientSupport:
    """Test Figma gradient to EGUI mapping."""

    def test_linear_gradient_horizontal(self):
        fill = {
            "type": "GRADIENT_LINEAR",
            "visible": True,
            "gradientHandlePositions": [
                {"x": 0.0, "y": 0.5},
                {"x": 1.0, "y": 0.5},
            ],
            "gradientStops": [
                {"position": 0.0, "color": {"r": 1.0, "g": 0.0, "b": 0.0, "a": 1.0}},
                {"position": 1.0, "color": {"r": 0.0, "g": 0.0, "b": 1.0, "a": 1.0}},
            ],
        }
        result = h._figma_gradient_to_egui(fill)
        assert result is not None
        assert result["type"] == "linear_gradient"
        assert result["start_color"] == "FF0000"
        assert result["end_color"] == "0000FF"

    def test_non_gradient_returns_none(self):
        fill = {"type": "SOLID", "color": {"r": 1.0, "g": 0.0, "b": 0.0}}
        result = h._figma_gradient_to_egui(fill)
        assert result is None


class TestFigmaShadowMapping:
    """Test Figma drop-shadow to EGUI shadow parameters."""

    def test_basic_shadow(self):
        effect = {
            "type": "DROP_SHADOW",
            "visible": True,
            "color": {"r": 0.0, "g": 0.0, "b": 0.0, "a": 0.25},
            "offset": {"x": 0, "y": 4},
            "radius": 8,
        }
        result = h._figma_shadow_to_egui(effect)
        assert result is not None
        assert result["offset_x"] == 0
        assert result["offset_y"] == 4
        assert result["blur"] == 8
        assert result["color"] == "000000"

    def test_invisible_shadow_ignored(self):
        effect = {
            "type": "DROP_SHADOW",
            "visible": False,
            "color": {"r": 0.0, "g": 0.0, "b": 0.0, "a": 0.25},
            "offset": {"x": 0, "y": 4},
            "radius": 8,
        }
        result = h._figma_shadow_to_egui(effect)
        assert result is None

    def test_non_shadow_effect(self):
        effect = {"type": "INNER_SHADOW", "visible": True}
        result = h._figma_shadow_to_egui(effect)
        assert result is None

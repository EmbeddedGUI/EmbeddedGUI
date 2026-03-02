"""Tests for HTML parsing completeness improvements in html2egui_helper.py."""

import os
import sys
import pytest

sys.path.insert(0, os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "..")))

import html2egui_helper as h


class TestClassifyLayout:
    """Test _classify_layout with extended Tailwind classes."""

    def test_flex_row(self):
        assert h._classify_layout("flex gap-4") == "flex-row-start"

    def test_flex_col(self):
        assert h._classify_layout("flex flex-col gap-2") == "flex-col-start"

    def test_flex_justify_between(self):
        assert h._classify_layout("flex justify-between") == "flex-row-between"

    def test_grid_2col(self):
        assert h._classify_layout("grid grid-cols-2 gap-4") == "grid-2col"

    def test_grid_3col(self):
        assert h._classify_layout("grid grid-cols-3") == "grid-3col"

    def test_block(self):
        assert h._classify_layout("p-4 bg-gray-800") == "block"

    def test_flex_wrap(self):
        result = h._classify_layout("flex flex-wrap gap-2")
        assert "wrap" in result

    def test_flex_justify_center(self):
        result = h._classify_layout("flex justify-center items-center")
        assert "center" in result

    def test_flex_justify_end(self):
        result = h._classify_layout("flex justify-end")
        assert "end" in result


class TestExtractTwColor:
    """Test _extract_tw_color with extended patterns."""

    def test_standard_color(self):
        result = h._extract_tw_color("text-cyan-400")
        assert result is not None
        assert result["name"] == "cyan-400"

    def test_hex_color(self):
        result = h._extract_tw_color("text-[#FF0000]")
        assert result is not None
        assert result["hex"] == "FF0000"

    def test_white(self):
        result = h._extract_tw_color("text-white font-bold")
        assert result is not None
        assert result["name"] == "white"

    def test_opacity_modifier(self):
        result = h._extract_tw_color("text-cyan-400/50")
        assert result is not None
        assert result["opacity"] == 50

    def test_no_color(self):
        result = h._extract_tw_color("font-bold p-4")
        assert result is None


class TestExtractTwBg:
    """Test _extract_tw_bg with extended patterns."""

    def test_standard_bg(self):
        result = h._extract_tw_bg("bg-gray-800 p-4")
        assert result is not None
        assert result["name"] == "gray-800"

    def test_hex_bg(self):
        result = h._extract_tw_bg("bg-[#1a1a2e]")
        assert result is not None
        assert result["hex"] == "1A1A2E"

    def test_bg_opacity(self):
        result = h._extract_tw_bg("bg-gray-800/50")
        assert result is not None
        assert result["opacity"] == 50

    def test_no_bg(self):
        result = h._extract_tw_bg("text-white p-4")
        assert result is None


class TestExtractTwFont:
    """Test _extract_tw_font with extended patterns."""

    def test_text_sm(self):
        result = h._extract_tw_font("text-sm text-white")
        assert result is not None
        assert result["size_px"] == 14

    def test_text_xl(self):
        result = h._extract_tw_font("text-xl font-bold")
        assert result is not None
        assert result["size_px"] == 20
        assert result["weight"] == "bold"

    def test_custom_size(self):
        result = h._extract_tw_font("text-[18px]")
        assert result is not None
        assert result["size_px"] == 18

    def test_no_font(self):
        result = h._extract_tw_font("bg-gray-800 p-4")
        assert result is None


class TestExtractTwRadius:
    """Test _extract_tw_radius."""

    def test_rounded_lg(self):
        assert h._extract_tw_radius("rounded-lg bg-gray-800") == 8

    def test_rounded_full(self):
        assert h._extract_tw_radius("rounded-full") == 9999

    def test_rounded_custom_px(self):
        assert h._extract_tw_radius("rounded-[12px]") == 12

    def test_rounded_custom_rem(self):
        assert h._extract_tw_radius("rounded-[0.5rem]") == 8

    def test_no_radius(self):
        assert h._extract_tw_radius("bg-gray-800 p-4") is None


class TestTwToPx:
    """Test _tw_to_px spacing conversion."""

    def test_standard_values(self):
        assert h._tw_to_px("4") == 16
        assert h._tw_to_px("8") == 32
        assert h._tw_to_px("0") == 0

    def test_bracket_px(self):
        assert h._tw_to_px("[16px]") == 16

    def test_unknown(self):
        assert h._tw_to_px("unknown") is None


class TestCssInJsParsing:
    """Test CSS-in-JS style={{}} inline style parsing."""

    def test_parse_inline_style(self):
        style_str = "backgroundColor: '#1a1a2e', color: '#ffffff', fontSize: '14px'"
        result = h._parse_css_in_js(style_str)
        assert result.get("backgroundColor") == "#1a1a2e"
        assert result.get("color") == "#ffffff"
        assert result.get("fontSize") == "14px"

    def test_empty_style(self):
        result = h._parse_css_in_js("")
        assert result == {}

    def test_numeric_values(self):
        style_str = "width: 100, height: 200, padding: 16"
        result = h._parse_css_in_js(style_str)
        assert result.get("width") == "100"
        assert result.get("height") == "200"

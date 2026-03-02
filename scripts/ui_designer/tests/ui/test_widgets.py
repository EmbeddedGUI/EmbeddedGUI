"""Tests for color_picker and font_selector utility functions.

Qt-independent tests run always. Qt-dependent tests are skipped if PyQt5 is missing.
"""

import re
import pytest


# ── Qt-independent tests (regex patterns) ─────────────────────

# Duplicate the regex patterns to avoid importing PyQt5-dependent modules
_HEX_RE = re.compile(r'^EGUI_COLOR_HEX\(\s*0x([0-9A-Fa-f]{6})\s*\)$')
_FONT_RE = re.compile(r'&egui_res_font_(\w+?)_(\d+)_(\d+)$')


def _font_display_info_local(font_expr):
    m = _FONT_RE.match(font_expr)
    if m:
        return m.group(1), m.group(2), m.group(3)
    return None


class TestHexRegex:
    """Test the EGUI_COLOR_HEX regex pattern."""

    def test_valid_hex(self):
        assert _HEX_RE.match("EGUI_COLOR_HEX(0xABCDEF)")

    def test_valid_hex_with_spaces(self):
        assert _HEX_RE.match("EGUI_COLOR_HEX( 0xABCDEF )")

    def test_invalid_short(self):
        assert _HEX_RE.match("EGUI_COLOR_HEX(0xABC)") is None

    def test_invalid_no_prefix(self):
        assert _HEX_RE.match("EGUI_COLOR_HEX(ABCDEF)") is None

    def test_extracts_hex_value(self):
        m = _HEX_RE.match("EGUI_COLOR_HEX(0xFF8000)")
        assert m.group(1) == "FF8000"

    def test_lowercase_hex(self):
        m = _HEX_RE.match("EGUI_COLOR_HEX(0xabcdef)")
        assert m.group(1) == "abcdef"


class TestFontDisplayInfo:
    """Test _font_display_info parsing."""

    def test_standard_font(self):
        assert _font_display_info_local("&egui_res_font_montserrat_14_4") == ("montserrat", "14", "4")

    def test_custom_font(self):
        assert _font_display_info_local("&egui_res_font_roboto_12_8") == ("roboto", "12", "8")

    def test_default_font_returns_none(self):
        assert _font_display_info_local("EGUI_CONFIG_FONT_DEFAULT") is None

    def test_empty_returns_none(self):
        assert _font_display_info_local("") is None

    def test_arbitrary_string_returns_none(self):
        assert _font_display_info_local("some_random_string") is None


# ── Qt-dependent tests ────────────────────────────────────────

try:
    import PyQt5
    _has_pyqt5 = True
except ImportError:
    _has_pyqt5 = False

_skip_no_qt = pytest.mark.skipif(not _has_pyqt5, reason="PyQt5 not available")


@_skip_no_qt
class TestEguiColorToQColor:
    """Test egui_color_to_qcolor conversion (requires PyQt5)."""

    def test_named_color_black(self):
        from ui_designer.ui.widgets.color_picker import egui_color_to_qcolor
        qc = egui_color_to_qcolor("EGUI_COLOR_BLACK")
        assert qc is not None
        assert (qc.red(), qc.green(), qc.blue()) == (0, 0, 0)

    def test_hex_color(self):
        from ui_designer.ui.widgets.color_picker import egui_color_to_qcolor
        qc = egui_color_to_qcolor("EGUI_COLOR_HEX(0xFF8000)")
        assert (qc.red(), qc.green(), qc.blue()) == (255, 128, 0)

    def test_empty_returns_none(self):
        from ui_designer.ui.widgets.color_picker import egui_color_to_qcolor
        assert egui_color_to_qcolor("") is None

    def test_unknown_returns_none(self):
        from ui_designer.ui.widgets.color_picker import egui_color_to_qcolor
        assert egui_color_to_qcolor("NOT_A_COLOR") is None


@_skip_no_qt
class TestQColorToEguiHex:
    """Test qcolor_to_egui_hex conversion (requires PyQt5)."""

    def test_red(self):
        from PyQt5.QtGui import QColor
        from ui_designer.ui.widgets.color_picker import qcolor_to_egui_hex
        assert qcolor_to_egui_hex(QColor(255, 0, 0)) == "EGUI_COLOR_HEX(0xFF0000)"

    def test_custom_color(self):
        from PyQt5.QtGui import QColor
        from ui_designer.ui.widgets.color_picker import qcolor_to_egui_hex
        assert qcolor_to_egui_hex(QColor(18, 52, 86)) == "EGUI_COLOR_HEX(0x123456)"

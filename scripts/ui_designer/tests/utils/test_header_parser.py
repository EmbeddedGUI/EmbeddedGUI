"""Tests for ui_designer.utils.header_parser module."""

import pytest
import os
import tempfile

from ui_designer.utils.header_parser import (
    parse_header,
    parse_widget_dir,
    generate_registration_template,
    _split_type_name,
    _infer_code_gen,
    _infer_property_type,
    _snake_to_camel,
    _snake_to_pascal,
    WidgetHeaderInfo,
)


# ── Test data: minimal C header ──────────────────────────────────

SLIDER_HEADER = """\
#ifndef _EGUI_VIEW_SLIDER_H_
#define _EGUI_VIEW_SLIDER_H_

#include "egui_view.h"

typedef void (*egui_view_on_value_changed_listener_t)(egui_view_t *self, uint8_t value);

typedef struct egui_view_slider_params egui_view_slider_params_t;
struct egui_view_slider_params {
    egui_region_t region;
    uint8_t value;
};

typedef struct egui_view_slider egui_view_slider_t;

#define EGUI_VIEW_SLIDER_PARAMS_INIT(_name, _x, _y, _w, _h, _val) \\
    static const egui_view_slider_params_t _name = { .region = {{(_x), (_y)}, {(_w), (_h)}}, .value = (_val) }

void egui_view_slider_init(egui_view_t *self);
void egui_view_slider_init_with_params(egui_view_t *self, const egui_view_slider_params_t *params);
void egui_view_slider_set_value(egui_view_t *self, uint8_t value);
void egui_view_slider_set_on_value_changed_listener(egui_view_t *self, egui_view_on_value_changed_listener_t listener);

#endif
"""

LABEL_HEADER = """\
#ifndef _EGUI_VIEW_LABEL_H_
#define _EGUI_VIEW_LABEL_H_

typedef struct egui_view_label egui_view_label_t;

void egui_view_label_init(egui_view_t *self);
void egui_view_label_set_text(egui_view_t *self, const char *text);
void egui_view_label_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_label_set_font_color(egui_view_t *self, egui_color_t color, egui_alpha_t alpha);

#endif
"""


# ── Helpers ──────────────────────────────────────────────────────


@pytest.fixture
def slider_header_file(tmp_path):
    p = tmp_path / "egui_view_slider.h"
    p.write_text(SLIDER_HEADER, encoding="utf-8")
    return p


@pytest.fixture
def label_header_file(tmp_path):
    p = tmp_path / "egui_view_label.h"
    p.write_text(LABEL_HEADER, encoding="utf-8")
    return p


@pytest.fixture
def widget_dir(tmp_path, slider_header_file, label_header_file):
    return tmp_path


# ── TestSplitTypeName ────────────────────────────────────────────


class TestSplitTypeName:
    def test_simple_type(self):
        assert _split_type_name("uint8_t value") == ("uint8_t", "value")

    def test_pointer_type(self):
        assert _split_type_name("const char *text") == ("const char *", "text")

    def test_struct_pointer(self):
        assert _split_type_name("const egui_font_t *font") == ("const egui_font_t *", "font")


# ── TestInferCodeGen ─────────────────────────────────────────────


class TestInferCodeGen:
    def test_simple_setter(self):
        cg = _infer_code_gen("egui_view_slider_set_value", "uint8_t value")
        assert cg["kind"] == "setter"
        assert cg["func"] == "egui_view_slider_set_value"

    def test_text_setter(self):
        cg = _infer_code_gen("egui_view_label_set_text", "const char *text")
        assert cg["kind"] == "text_setter"

    def test_font_derived_setter(self):
        cg = _infer_code_gen("egui_view_label_set_font", "const egui_font_t *font")
        assert cg["kind"] == "derived_setter"
        assert cg["derive"] == "font"

    def test_multi_param_setter(self):
        cg = _infer_code_gen("egui_view_label_set_font_color",
                             "egui_color_t color, egui_alpha_t alpha")
        assert cg["kind"] == "multi_setter"
        assert "{color}" in cg["args"]
        assert "{alpha}" in cg["args"]

    def test_no_params(self):
        cg = _infer_code_gen("egui_view_led_set_on", "")
        assert cg["kind"] == "setter"


# ── TestInferPropertyType ────────────────────────────────────────


class TestInferPropertyType:
    def test_uint8(self):
        assert _infer_property_type("uint8_t") == "int"

    def test_int(self):
        assert _infer_property_type("int") == "int"

    def test_string(self):
        assert _infer_property_type("const char *") == "string"

    def test_color(self):
        assert _infer_property_type("egui_color_t") == "color"


# ── TestSnakeConversions ─────────────────────────────────────────


class TestSnakeConversions:
    def test_snake_to_camel(self):
        assert _snake_to_camel("value_changed") == "ValueChanged"

    def test_snake_to_pascal(self):
        assert _snake_to_pascal("progress_bar") == "ProgressBar"

    def test_single_word(self):
        assert _snake_to_camel("toggled") == "Toggled"


# ── TestParseHeader ──────────────────────────────────────────────


class TestParseHeader:
    def test_parse_slider(self, slider_header_file):
        info = parse_header(slider_header_file)
        assert info is not None
        assert info.c_type == "egui_view_slider_t"
        assert info.struct_name == "egui_view_slider"
        assert info.widget_name == "slider"
        assert info.init_func == "egui_view_slider_init"
        assert info.init_with_params == "egui_view_slider_init_with_params"
        assert info.params_type == "egui_view_slider_params_t"

    def test_params_macro(self, slider_header_file):
        info = parse_header(slider_header_file)
        assert "EGUI_VIEW_SLIDER_PARAMS_INIT" in info.params_macros

    def test_setters(self, slider_header_file):
        info = parse_header(slider_header_file)
        setter_names = [s[0] for s in info.setters]
        assert "egui_view_slider_set_value" in setter_names
        # set_on_*_listener should be excluded from setters
        assert "egui_view_slider_set_on_value_changed_listener" not in setter_names

    def test_listener_typedef(self, slider_header_file):
        info = parse_header(slider_header_file)
        assert "egui_view_on_value_changed_listener_t" in info.listener_typedefs
        assert "uint8_t value" in info.listener_typedefs["egui_view_on_value_changed_listener_t"]

    def test_set_listener(self, slider_header_file):
        info = parse_header(slider_header_file)
        assert len(info.set_listeners) == 1
        assert info.set_listeners[0][0] == "egui_view_slider_set_on_value_changed_listener"

    def test_parse_label(self, label_header_file):
        info = parse_header(label_header_file)
        assert info.c_type == "egui_view_label_t"
        setter_names = [s[0] for s in info.setters]
        assert "egui_view_label_set_text" in setter_names
        assert "egui_view_label_set_font" in setter_names
        assert "egui_view_label_set_font_color" in setter_names

    def test_parse_nonwidget_returns_none(self, tmp_path):
        p = tmp_path / "egui_core.h"
        p.write_text("#ifndef X\n#define X\n#endif\n", encoding="utf-8")
        assert parse_header(p) is None


# ── TestParseWidgetDir ───────────────────────────────────────────


class TestParseWidgetDir:
    def test_parse_dir(self, widget_dir):
        results = parse_widget_dir(widget_dir)
        names = [r.widget_name for r in results]
        assert "slider" in names
        assert "label" in names

    def test_parse_real_widget_dir(self):
        """Parse the actual project widget headers (integration test)."""
        widget_dir = os.path.join(
            os.path.dirname(__file__), "..", "..", "..", "..", "src", "widget"
        )
        widget_dir = os.path.normpath(widget_dir)
        if not os.path.isdir(widget_dir):
            pytest.skip("Widget source directory not found")
        results = parse_widget_dir(widget_dir)
        names = [r.widget_name for r in results]
        assert "slider" in names
        assert "label" in names
        assert len(results) >= 10


# ── TestGenerateTemplate ─────────────────────────────────────────


class TestGenerateTemplate:
    def test_generate_slider_template(self, slider_header_file):
        info = parse_header(slider_header_file)
        template = generate_registration_template(info)
        assert 'type_name="slider"' in template
        assert '"c_type": "egui_view_slider_t"' in template
        assert '"init_func": "egui_view_slider_init_with_params"' in template
        assert '"value"' in template
        assert '"onValueChanged"' in template
        assert "WidgetRegistry.instance().register(" in template

    def test_generate_label_template(self, label_header_file):
        info = parse_header(label_header_file)
        template = generate_registration_template(info)
        assert 'type_name="label"' in template
        assert '"text"' in template
        assert '"font"' in template
        assert '"font_color"' in template

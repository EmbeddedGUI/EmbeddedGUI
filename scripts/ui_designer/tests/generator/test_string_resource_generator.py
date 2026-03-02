"""Tests for ui_designer.generator.string_resource_generator module."""

import pytest

from ui_designer.generator.string_resource_generator import (
    _key_to_enum,
    _locale_to_enum,
    _locale_to_array_name,
    _escape_c_string,
    generate_strings_header,
    generate_strings_source,
    generate_string_files,
)
from ui_designer.model.string_resource import StringResourceCatalog


# ── Fixture helpers ───────────────────────────────────────────────


def _make_catalog(keys_and_values=None, locales=None):
    """Create a StringResourceCatalog with test data.

    Args:
        keys_and_values: dict mapping locale -> dict(key -> value).
            If None, creates a default catalog with two keys and two locales.
        locales: ignored if keys_and_values is provided.
    """
    cat = StringResourceCatalog()
    if keys_and_values is None:
        keys_and_values = {
            "": {"app_name": "My App", "greeting": "Hello"},
            "zh": {"app_name": "Wo De App", "greeting": "Ni Hao"},
        }
    for locale, kv in keys_and_values.items():
        for key, value in kv.items():
            cat.set(key, value, locale)
    return cat


# ======================================================================
# TestHelpers
# ======================================================================


class TestHelpers:
    """Tests for internal helper functions."""

    def test_key_to_enum(self):
        assert _key_to_enum("app_name") == "EGUI_STR_APP_NAME"

    def test_locale_to_enum_default(self):
        assert _locale_to_enum("") == "EGUI_LOCALE_DEFAULT"

    def test_locale_to_enum_zh(self):
        assert _locale_to_enum("zh") == "EGUI_LOCALE_ZH"

    def test_locale_to_array_name_default(self):
        assert _locale_to_array_name("") == "s_strings_default"

    def test_locale_to_array_name_zh(self):
        assert _locale_to_array_name("zh") == "s_strings_zh"

    def test_escape_c_string(self):
        assert _escape_c_string('say "hi"') == 'say \\"hi\\"'
        assert _escape_c_string("back\\slash") == "back\\\\slash"
        assert _escape_c_string("line\nbreak") == "line\\nbreak"
        assert _escape_c_string("a\ttab") == "a\\ttab"


# ======================================================================
# TestGenerateStrings
# ======================================================================


class TestGenerateStrings:
    """Tests for generate_strings_header, generate_strings_source,
    and generate_string_files."""

    def test_header_contains_enums(self):
        cat = _make_catalog()
        output = generate_strings_header(cat)
        # String ID enum
        assert "EGUI_STR_APP_NAME" in output
        assert "EGUI_STR_GREETING" in output
        assert "EGUI_STR_COUNT" in output
        # Locale enum
        assert "EGUI_LOCALE_DEFAULT" in output
        assert "EGUI_LOCALE_ZH" in output
        assert "EGUI_LOCALE_COUNT" in output

    def test_source_contains_string_tables(self):
        cat = _make_catalog()
        output = generate_strings_source(cat)
        assert "s_strings_default" in output
        assert "s_strings_zh" in output
        assert '"My App"' in output
        assert '"Hello"' in output
        assert '"Wo De App"' in output
        assert '"Ni Hao"' in output

    def test_source_contains_init(self):
        cat = _make_catalog()
        output = generate_strings_source(cat)
        assert "void egui_strings_init(void)" in output
        assert "egui_i18n_init" in output
        assert "EGUI_LOCALE_COUNT" in output

    def test_generate_string_files_empty(self):
        cat = StringResourceCatalog()
        files = generate_string_files(cat)
        assert files == {}

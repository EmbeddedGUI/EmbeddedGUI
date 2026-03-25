"""Tests for ui_designer.model.string_resource."""

import os

import pytest

from ui_designer.model.string_resource import (
    parse_string_ref,
    make_string_ref,
    StringResourceCatalog,
    DEFAULT_LOCALE,
)


class TestParseStringRef:
    """Tests for the parse_string_ref helper function."""

    def test_parse_string_ref_valid(self):
        assert parse_string_ref("@string/hello") == "hello"

    def test_parse_string_ref_invalid(self):
        assert parse_string_ref("hello") is None

    def test_parse_string_ref_empty(self):
        assert parse_string_ref("") is None

    def test_parse_string_ref_with_underscores(self):
        assert parse_string_ref("@string/my_app_name") == "my_app_name"

    def test_parse_string_ref_none_input(self):
        assert parse_string_ref(None) is None


class TestMakeStringRef:
    """Tests for the make_string_ref helper function."""

    def test_make_string_ref(self):
        assert make_string_ref("hello") == "@string/hello"


class TestStringResourceCatalogGetSet:
    """Tests for basic get/set operations."""

    def test_set_and_get(self):
        cat = StringResourceCatalog()
        cat.set("greeting", "Hello", DEFAULT_LOCALE)
        assert cat.get("greeting", DEFAULT_LOCALE) == "Hello"

    def test_get_fallback_to_default(self, string_catalog):
        # string_catalog has "greeting" = "Hello" in default, "Ni Hao" in "zh"
        # Remove the zh entry for a new key to test fallback
        string_catalog.set("farewell", "Goodbye", DEFAULT_LOCALE)
        # "farewell" is not in "zh" locale, should fall back to default
        assert string_catalog.get("farewell", "zh") == "Goodbye"

    def test_get_missing_key_returns_empty(self):
        cat = StringResourceCatalog()
        assert cat.get("nonexistent") == ""


class TestKeyManagement:
    """Tests for add_key, remove_key, rename_key."""

    def test_add_key_populates_all_locales(self, string_catalog):
        string_catalog.add_key("new_key", "New Value")

        assert string_catalog.get("new_key", DEFAULT_LOCALE) == "New Value"
        # Non-default locales get empty string
        assert string_catalog.get("new_key", "zh") == ""

    def test_add_key_invalid_raises(self):
        cat = StringResourceCatalog()
        cat.set("dummy", "val", DEFAULT_LOCALE)  # Ensure default locale exists
        with pytest.raises(ValueError):
            cat.add_key("123invalid")

    def test_add_key_invalid_with_spaces_raises(self):
        cat = StringResourceCatalog()
        cat.set("dummy", "val", DEFAULT_LOCALE)
        with pytest.raises(ValueError):
            cat.add_key("has space")

    def test_remove_key(self, string_catalog):
        assert string_catalog.get("greeting", DEFAULT_LOCALE) == "Hello"
        string_catalog.remove_key("greeting")
        assert string_catalog.get("greeting", DEFAULT_LOCALE) == ""
        assert string_catalog.get("greeting", "zh") == ""

    def test_rename_key(self, string_catalog):
        string_catalog.rename_key("greeting", "salutation")
        assert string_catalog.get("salutation", DEFAULT_LOCALE) == "Hello"
        assert string_catalog.get("salutation", "zh") == "Ni Hao"
        # Old key should be gone
        assert string_catalog.get("greeting", DEFAULT_LOCALE) == ""

    def test_rename_key_invalid_raises(self, string_catalog):
        with pytest.raises(ValueError):
            string_catalog.rename_key("greeting", "bad-key!")

    def test_rename_key_duplicate_raises(self, string_catalog):
        with pytest.raises(ValueError):
            string_catalog.rename_key("greeting", "app_name")


class TestProperties:
    """Tests for all_keys, has_strings, locales."""

    def test_all_keys_sorted(self, string_catalog):
        keys = string_catalog.all_keys
        assert keys == sorted(keys)
        assert "app_name" in keys
        assert "greeting" in keys

    def test_has_strings_empty(self):
        cat = StringResourceCatalog()
        assert cat.has_strings is False

    def test_has_strings_with_data(self, string_catalog):
        assert string_catalog.has_strings is True

    def test_locales_default_first(self, string_catalog):
        locales = string_catalog.locales
        assert len(locales) >= 2
        assert locales[0] == DEFAULT_LOCALE
        assert "zh" in locales


class TestCollectAllChars:
    """Tests for collect_all_chars."""

    def test_collect_all_chars(self, string_catalog):
        chars = string_catalog.collect_all_chars()
        assert isinstance(chars, set)
        # "Hello", "My App", "My App ZH", "Ni Hao" should contribute characters
        assert "H" in chars
        assert "e" in chars
        assert "l" in chars
        assert "o" in chars
        assert " " in chars


class TestLocaleManagement:
    """Tests for add_locale, remove_locale."""

    def test_add_locale(self, string_catalog):
        string_catalog.add_locale("ja")
        assert "ja" in string_catalog.locales
        # All existing keys should be present with empty values
        for key in string_catalog.all_keys:
            val = string_catalog.strings["ja"].get(key)
            assert val == ""

    def test_add_locale_already_exists(self, string_catalog):
        # Adding an existing locale should be a no-op
        original_data = dict(string_catalog.strings["zh"])
        string_catalog.add_locale("zh")
        assert string_catalog.strings["zh"] == original_data

    def test_remove_locale(self, string_catalog):
        assert "zh" in string_catalog.locales
        string_catalog.remove_locale("zh")
        assert "zh" not in string_catalog.locales

    def test_remove_locale_default_raises(self, string_catalog):
        with pytest.raises(ValueError):
            string_catalog.remove_locale(DEFAULT_LOCALE)


class TestResolve:
    """Tests for resolve method."""

    def test_resolve_literal(self, string_catalog):
        result = string_catalog.resolve("plain text")
        assert result == "plain text"

    def test_resolve_string_ref(self, string_catalog):
        result = string_catalog.resolve("@string/greeting", DEFAULT_LOCALE)
        assert result == "Hello"

    def test_resolve_string_ref_locale(self, string_catalog):
        result = string_catalog.resolve("@string/greeting", "zh")
        assert result == "Ni Hao"


class TestSaveLoad:
    """Tests for file I/O round-trip."""

    @pytest.mark.integration
    def test_save_load_round_trip(self, tmp_path):
        cat = StringResourceCatalog()
        cat.set("app_name", "Test App", DEFAULT_LOCALE)
        cat.set("greeting", "Hello", DEFAULT_LOCALE)
        cat.set("app_name", "Test App ZH", "zh")
        cat.set("greeting", "Ni Hao", "zh")

        src_dir = str(tmp_path / "resources")
        os.makedirs(src_dir, exist_ok=True)
        cat.save(src_dir)

        # Verify files were created
        assert os.path.isfile(os.path.join(src_dir, "values", "strings.xml"))
        assert os.path.isfile(os.path.join(src_dir, "values-zh", "strings.xml"))

        loaded = StringResourceCatalog.scan_and_load(src_dir)

        assert loaded.has_strings is True
        assert loaded.get("app_name", DEFAULT_LOCALE) == "Test App"
        assert loaded.get("greeting", DEFAULT_LOCALE) == "Hello"
        assert loaded.get("app_name", "zh") == "Test App ZH"
        assert loaded.get("greeting", "zh") == "Ni Hao"
        assert set(loaded.locales) == {DEFAULT_LOCALE, "zh"}

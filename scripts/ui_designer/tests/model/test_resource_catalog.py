"""Tests for ResourceCatalog model."""

import os
import json
import pytest

from ui_designer.model.resource_catalog import (
    ResourceCatalog,
    IMAGE_EXTENSIONS,
    FONT_EXTENSIONS,
    TEXT_EXTENSIONS,
)


class TestResourceCatalogInit:
    """Test initial state of a new catalog."""

    def test_empty_catalog(self):
        cat = ResourceCatalog()
        assert cat.images == []
        assert cat.fonts == []
        assert cat.text_files == []


class TestImageManagement:
    """Test image add/remove/has operations."""

    def test_add_image(self):
        cat = ResourceCatalog()
        cat.add_image("star.png")
        assert cat.has_image("star.png")
        assert cat.images == ["star.png"]

    def test_add_image_sorted(self):
        cat = ResourceCatalog()
        cat.add_image("zebra.png")
        cat.add_image("alpha.png")
        assert cat.images == ["alpha.png", "zebra.png"]

    def test_add_image_no_duplicate(self):
        cat = ResourceCatalog()
        cat.add_image("star.png")
        cat.add_image("star.png")
        assert cat.images == ["star.png"]

    def test_remove_image(self):
        cat = ResourceCatalog()
        cat.add_image("star.png")
        cat.remove_image("star.png")
        assert not cat.has_image("star.png")
        assert cat.images == []

    def test_remove_nonexistent_image(self):
        cat = ResourceCatalog()
        cat.remove_image("nope.png")  # should not raise
        assert cat.images == []

    def test_has_image_false(self):
        cat = ResourceCatalog()
        assert not cat.has_image("missing.png")


class TestFontManagement:
    """Test font add/remove/has operations."""

    def test_add_font(self):
        cat = ResourceCatalog()
        cat.add_font("test.ttf")
        assert cat.has_font("test.ttf")

    def test_add_font_sorted(self):
        cat = ResourceCatalog()
        cat.add_font("z.otf")
        cat.add_font("a.ttf")
        assert cat.fonts == ["a.ttf", "z.otf"]

    def test_add_font_no_duplicate(self):
        cat = ResourceCatalog()
        cat.add_font("test.ttf")
        cat.add_font("test.ttf")
        assert cat.fonts == ["test.ttf"]

    def test_remove_font(self):
        cat = ResourceCatalog()
        cat.add_font("test.ttf")
        cat.remove_font("test.ttf")
        assert not cat.has_font("test.ttf")

    def test_remove_nonexistent_font(self):
        cat = ResourceCatalog()
        cat.remove_font("nope.ttf")
        assert cat.fonts == []


class TestTextFileManagement:
    """Test text file add/remove/has operations."""

    def test_add_text_file(self):
        cat = ResourceCatalog()
        cat.add_text_file("supported_text.txt")
        assert cat.has_text_file("supported_text.txt")

    def test_add_text_file_sorted(self):
        cat = ResourceCatalog()
        cat.add_text_file("z.txt")
        cat.add_text_file("a.txt")
        assert cat.text_files == ["a.txt", "z.txt"]

    def test_add_text_file_no_duplicate(self):
        cat = ResourceCatalog()
        cat.add_text_file("a.txt")
        cat.add_text_file("a.txt")
        assert cat.text_files == ["a.txt"]

    def test_remove_text_file(self):
        cat = ResourceCatalog()
        cat.add_text_file("a.txt")
        cat.remove_text_file("a.txt")
        assert not cat.has_text_file("a.txt")


class TestAutoDetectAddFile:
    """Test add_file() auto-detection by extension."""

    @pytest.mark.parametrize("filename", ["icon.png", "photo.jpg", "bg.bmp", "pic.jpeg", "anim.gif"])
    def test_add_image_by_extension(self, filename):
        cat = ResourceCatalog()
        cat.add_file(filename)
        assert cat.has_image(filename)
        assert cat.fonts == []
        assert cat.text_files == []

    @pytest.mark.parametrize("filename", ["font.ttf", "font.otf"])
    def test_add_font_by_extension(self, filename):
        cat = ResourceCatalog()
        cat.add_file(filename)
        assert cat.has_font(filename)
        assert cat.images == []

    def test_add_text_by_extension(self):
        cat = ResourceCatalog()
        cat.add_file("chars.txt")
        assert cat.has_text_file("chars.txt")
        assert cat.images == []

    def test_add_unknown_extension_ignored(self):
        cat = ResourceCatalog()
        cat.add_file("readme.md")
        assert cat.images == []
        assert cat.fonts == []
        assert cat.text_files == []

    def test_remove_file_from_any_category(self):
        cat = ResourceCatalog()
        cat.add_image("star.png")
        cat.remove_file("star.png")
        assert not cat.has_image("star.png")


class TestXmlSerialization:
    """Test save/load XML round-trip."""

    def test_save_and_load_roundtrip(self, tmp_path):
        cat = ResourceCatalog()
        cat.add_image("star.png")
        cat.add_image("icon_wifi.png")
        cat.add_font("test.ttf")
        cat.add_text_file("supported_text.txt")

        cat.save(str(tmp_path))

        loaded = ResourceCatalog.load(str(tmp_path))
        assert loaded is not None
        assert loaded.images == ["icon_wifi.png", "star.png"]
        assert loaded.fonts == ["test.ttf"]
        assert loaded.text_files == ["supported_text.txt"]

    def test_save_empty_catalog(self, tmp_path):
        cat = ResourceCatalog()
        cat.save(str(tmp_path))

        loaded = ResourceCatalog.load(str(tmp_path))
        assert loaded is not None
        assert loaded.images == []
        assert loaded.fonts == []
        assert loaded.text_files == []

    def test_load_nonexistent_returns_none(self, tmp_path):
        result = ResourceCatalog.load(str(tmp_path / "nonexistent"))
        assert result is None

    def test_save_creates_xml_file(self, tmp_path):
        cat = ResourceCatalog()
        cat.add_image("test.png")
        cat.save(str(tmp_path))
        assert os.path.isfile(os.path.join(str(tmp_path), "resources.xml"))


class TestFromDirectory:
    """Test from_directory() filesystem scanning."""

    def test_scan_flat_directory(self, tmp_path):
        # Create flat layout (legacy)
        (tmp_path / "star.png").write_bytes(b"")
        (tmp_path / "font.ttf").write_bytes(b"")
        (tmp_path / "chars.txt").write_text("abc")

        cat = ResourceCatalog.from_directory(str(tmp_path))
        assert cat.has_image("star.png")
        assert cat.has_font("font.ttf")
        assert cat.has_text_file("chars.txt")

    def test_scan_structured_directory(self, tmp_path):
        # Create structured layout (.eguiproject/resources/)
        images_dir = tmp_path / "images"
        images_dir.mkdir()
        (images_dir / "icon.png").write_bytes(b"")
        (tmp_path / "font.ttf").write_bytes(b"")

        cat = ResourceCatalog.from_directory(str(tmp_path))
        assert cat.has_image("icon.png")
        assert cat.has_font("font.ttf")

    def test_scan_nonexistent_directory(self):
        cat = ResourceCatalog.from_directory("/nonexistent/path")
        assert cat.images == []
        assert cat.fonts == []

    def test_scan_ignores_subdirectories(self, tmp_path):
        subdir = tmp_path / "subdir"
        subdir.mkdir()
        (subdir / "hidden.png").write_bytes(b"")

        cat = ResourceCatalog.from_directory(str(tmp_path))
        # subdir itself is not a file, hidden.png is in subdir not images/
        assert not cat.has_image("hidden.png")

    def test_no_duplicate_from_images_subdir_and_root(self, tmp_path):
        # Same file in both images/ and root
        images_dir = tmp_path / "images"
        images_dir.mkdir()
        (images_dir / "star.png").write_bytes(b"")
        (tmp_path / "star.png").write_bytes(b"")

        cat = ResourceCatalog.from_directory(str(tmp_path))
        assert cat.images.count("star.png") == 1


class TestFromResourceConfig:
    """Test from_resource_config() migration."""

    def test_from_config_data(self, tmp_path):
        config_data = {
            "img": [
                {"file": "star.png", "format": "rgb565"},
                {"file": "icon.png", "format": "alpha"},
            ],
            "font": [
                {"file": "test.ttf", "pixelsize": "16", "text": "chars.txt"},
            ],
        }
        cat = ResourceCatalog.from_resource_config(str(tmp_path), config_data)
        assert cat.has_image("star.png")
        assert cat.has_image("icon.png")
        assert cat.has_font("test.ttf")
        assert cat.has_text_file("chars.txt")

    def test_from_config_deduplicates(self, tmp_path):
        config_data = {
            "img": [
                {"file": "star.png"},
                {"file": "star.png"},
            ],
            "font": [],
        }
        cat = ResourceCatalog.from_resource_config(str(tmp_path), config_data)
        assert cat.images.count("star.png") == 1

    def test_from_config_file_on_disk(self, tmp_path):
        config_data = {"img": [{"file": "a.png"}], "font": []}
        config_path = tmp_path / "app_resource_config.json"
        config_path.write_text(json.dumps(config_data))

        cat = ResourceCatalog.from_resource_config(str(tmp_path))
        assert cat.has_image("a.png")

    def test_from_config_fallback_to_directory(self, tmp_path):
        # No config file, should fall back to from_directory
        (tmp_path / "star.png").write_bytes(b"")
        cat = ResourceCatalog.from_resource_config(str(tmp_path))
        assert cat.has_image("star.png")

    def test_from_config_also_scans_directory(self, tmp_path):
        # Config has one image, directory has another
        config_data = {"img": [{"file": "a.png"}], "font": []}
        (tmp_path / "b.png").write_bytes(b"")
        cat = ResourceCatalog.from_resource_config(str(tmp_path), config_data)
        assert cat.has_image("a.png")
        assert cat.has_image("b.png")

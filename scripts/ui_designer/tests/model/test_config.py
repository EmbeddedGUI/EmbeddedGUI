"""Tests for DesignerConfig model."""

import json
import os
import pytest
from unittest.mock import patch

from ui_designer.model.config import DesignerConfig, _get_config_dir, _get_config_path
from ui_designer.model.workspace import normalize_path


@pytest.fixture(autouse=True)
def reset_singleton():
    """Reset the singleton instance before each test."""
    DesignerConfig._instance = None
    yield
    DesignerConfig._instance = None


@pytest.fixture
def config():
    """Create a fresh DesignerConfig (not singleton)."""
    return DesignerConfig()


class TestDefaults:
    """Test default configuration values."""

    def test_default_values(self, config):
        assert config.sdk_root == ""
        assert config.egui_root == ""
        assert config.last_app == "HelloDesigner"
        assert config.recent_projects == []
        assert config.recent_apps == []
        assert config.theme == "dark"
        assert config.auto_compile is True
        assert config.overlay_mode == "horizontal"
        assert config.overlay_flipped is True
        assert config.font_size_px == 0
        assert config.show_all_examples is False
        assert config.window_geometry == ""
        assert config.window_state == ""


class TestSaveLoad:
    """Test save/load JSON round-trip."""

    def test_save_and_load_roundtrip(self, config, tmp_path):
        config.sdk_root = "/some/path"
        config.last_app = "MyApp"
        config.theme = "light"
        config.auto_compile = False
        config.font_size_px = 14

        config_path = tmp_path / "config.json"
        with patch("ui_designer.model.config._get_config_path", return_value=str(config_path)):
            with patch("ui_designer.model.config._get_config_dir", return_value=str(tmp_path)):
                config.save()

        loaded = DesignerConfig()
        with patch("ui_designer.model.config._get_config_path", return_value=str(config_path)):
            loaded.load()

        assert loaded.sdk_root == normalize_path("/some/path")
        assert loaded.egui_root == normalize_path("/some/path")
        assert loaded.last_app == "MyApp"
        assert loaded.theme == "light"
        assert loaded.auto_compile is False
        assert loaded.font_size_px == 14

    def test_load_nonexistent_file(self, config, tmp_path):
        config_path = tmp_path / "nonexistent.json"
        with patch("ui_designer.model.config._get_config_path", return_value=str(config_path)):
            config.load()
        # Should keep defaults
        assert config.last_app == "HelloDesigner"
        assert config.theme == "dark"

    def test_load_corrupted_file(self, config, tmp_path):
        config_path = tmp_path / "config.json"
        config_path.write_text("not valid json {{{")
        with patch("ui_designer.model.config._get_config_path", return_value=str(config_path)):
            config.load()  # should not raise
        assert config.last_app == "HelloDesigner"

    def test_save_creates_directory(self, config, tmp_path):
        nested = tmp_path / "sub" / "dir"
        config_path = nested / "config.json"
        with patch("ui_designer.model.config._get_config_path", return_value=str(config_path)):
            with patch("ui_designer.model.config._get_config_dir", return_value=str(nested)):
                config.save()
        assert config_path.is_file()


class TestRecentApps:
    """Test recent apps MRU list management."""

    def test_add_recent_app(self, config, tmp_path):
        config_path = tmp_path / "config.json"
        with patch("ui_designer.model.config._get_config_path", return_value=str(config_path)):
            with patch("ui_designer.model.config._get_config_dir", return_value=str(tmp_path)):
                config.add_recent_app("App1", "/root1")
        assert config.recent_apps == [("App1", normalize_path("/root1"))]
        assert config.recent_projects[0]["display_name"] == "App1"

    def test_add_recent_app_moves_to_front(self, config, tmp_path):
        config_path = tmp_path / "config.json"
        with patch("ui_designer.model.config._get_config_path", return_value=str(config_path)):
            with patch("ui_designer.model.config._get_config_dir", return_value=str(tmp_path)):
                config.add_recent_app("App1", "/root")
                config.add_recent_app("App2", "/root")
                config.add_recent_app("App1", "/root")
        assert config.recent_apps[0] == ("App1", normalize_path("/root"))
        assert len(config.recent_apps) == 2

    def test_add_recent_app_max_10(self, config, tmp_path):
        config_path = tmp_path / "config.json"
        with patch("ui_designer.model.config._get_config_path", return_value=str(config_path)):
            with patch("ui_designer.model.config._get_config_dir", return_value=str(tmp_path)):
                for i in range(15):
                    config.add_recent_app(f"App{i}", "/root")
        assert len(config.recent_apps) == 10
        assert config.recent_apps[0] == ("App14", normalize_path("/root"))

    def test_add_recent_deduplicates(self, config, tmp_path):
        config_path = tmp_path / "config.json"
        with patch("ui_designer.model.config._get_config_path", return_value=str(config_path)):
            with patch("ui_designer.model.config._get_config_dir", return_value=str(tmp_path)):
                config.add_recent_app("App1", "/root")
                config.add_recent_app("App1", "/root")
        assert len(config.recent_apps) == 1

    def test_remove_recent_project_updates_legacy_recent_apps(self, config, tmp_path):
        config_path = tmp_path / "config.json"
        with patch("ui_designer.model.config._get_config_path", return_value=str(config_path)):
            with patch("ui_designer.model.config._get_config_dir", return_value=str(tmp_path)):
                config.add_recent_project("/root/App1/App1.egui", "/root", "App1")
                config.add_recent_project("/root/App2/App2.egui", "/root", "App2")

                removed = config.remove_recent_project("/root/App1/App1.egui")

        assert removed is True
        assert [item["display_name"] for item in config.recent_projects] == ["App2"]
        assert config.recent_apps == [("App2", normalize_path("/root"))]


class TestPathManagement:
    """Test path helper methods."""

    def test_get_app_dir(self, config):
        config.egui_root = "/project"
        config.last_app = "MyApp"
        result = config.get_app_dir()
        assert result == os.path.join(normalize_path("/project"), "example", "MyApp")

    def test_get_app_dir_with_args(self, config):
        result = config.get_app_dir("TestApp", "/other/root")
        assert result == os.path.join(normalize_path("/other/root"), "example", "TestApp")

    def test_get_app_dir_empty_root(self, config):
        config.egui_root = ""
        assert config.get_app_dir() == ""

    def test_get_project_path(self, config):
        config.egui_root = "/project"
        config.last_app = "MyApp"
        result = config.get_project_path()
        assert result == os.path.join(normalize_path("/project"), "example", "MyApp", "MyApp.egui")

    def test_get_project_path_empty(self, config):
        config.egui_root = ""
        assert config.get_project_path() == ""

    def test_list_available_apps(self, config, tmp_path):
        config.sdk_root = str(tmp_path)
        example_dir = tmp_path / "example"
        example_dir.mkdir()

        # Create valid app (has build.mk)
        app1 = example_dir / "ValidApp"
        app1.mkdir()
        (app1 / "build.mk").write_text("")
        (app1 / "ValidApp.egui").write_text("")

        # Create invalid app (no build.mk)
        app2 = example_dir / "InvalidApp"
        app2.mkdir()

        apps = config.list_available_apps()
        assert "ValidApp" in apps
        assert "InvalidApp" not in apps

    def test_list_available_apps_empty_root(self, config):
        config.sdk_root = ""
        assert config.list_available_apps() == []

    def test_list_available_apps_no_example_dir(self, config, tmp_path):
        config.sdk_root = str(tmp_path)
        assert config.list_available_apps() == []


class TestSingleton:
    """Test singleton pattern."""

    def test_instance_returns_same_object(self, tmp_path):
        config_path = tmp_path / "config.json"
        with patch("ui_designer.model.config._get_config_path", return_value=str(config_path)):
            inst1 = DesignerConfig.instance()
            inst2 = DesignerConfig.instance()
        assert inst1 is inst2

"""Global configuration management for EmbeddedGUI Designer."""

from __future__ import annotations

import json
import os
import sys

from .workspace import normalize_path, resolve_available_sdk_root


def _get_config_dir():
    """Get the configuration directory path."""
    if getattr(sys, "frozen", False):
        if sys.platform == "win32":
            base = os.environ.get("APPDATA", os.path.expanduser("~"))
        elif sys.platform == "darwin":
            base = os.path.join(os.path.expanduser("~"), "Library", "Application Support")
        else:
            base = os.environ.get("XDG_CONFIG_HOME", os.path.join(os.path.expanduser("~"), ".config"))
        return os.path.join(base, "EmbeddedGUI-Designer")

    base = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    return os.path.join(base, ".config")


def _get_config_path():
    """Get the full path to the config file."""
    return os.path.join(_get_config_dir(), "config.json")


class DesignerConfig:
    """Manages global designer configuration."""

    _instance = None

    def __init__(self):
        self.sdk_root = ""
        self.egui_root = ""
        self.last_app = "HelloDesigner"
        self.last_project_path = ""
        self.recent_projects = []
        self.recent_apps = []
        self.theme = "dark"
        self.auto_compile = True
        self.overlay_mode = "horizontal"
        self.overlay_flipped = True
        self.show_grid = True
        self.grid_size = 8
        self.font_size_px = 0
        self.show_all_examples = False
        self.window_geometry = ""
        self.window_state = ""
        self.sdk_setup_prompted = False

    @property
    def egui_root(self):
        """Legacy alias for ``sdk_root``."""
        return self.sdk_root

    @egui_root.setter
    def egui_root(self, value):
        self.sdk_root = normalize_path(value)

    @classmethod
    def instance(cls):
        """Get the singleton config instance."""
        if cls._instance is None:
            cls._instance = cls()
            cls._instance.load()
        return cls._instance

    def _normalize_recent_projects(self, recent_projects, recent_apps, fallback_sdk_root):
        normalized = []

        for item in recent_projects or []:
            if not isinstance(item, dict):
                continue
            project_path = normalize_path(item.get("project_path", ""))
            sdk_root = normalize_path(item.get("sdk_root", item.get("egui_root", "")))
            display_name = item.get("display_name", "")
            if not project_path:
                continue
            if not display_name:
                display_name = os.path.splitext(os.path.basename(project_path))[0]
            normalized.append(
                {
                    "project_path": project_path,
                    "sdk_root": sdk_root,
                    "display_name": display_name,
                }
            )

        for app_name, egui_root in recent_apps or []:
            sdk_root = normalize_path(egui_root or fallback_sdk_root)
            if not sdk_root or not app_name:
                continue
            project_path = normalize_path(os.path.join(sdk_root, "example", app_name, f"{app_name}.egui"))
            normalized.append(
                {
                    "project_path": project_path,
                    "sdk_root": sdk_root,
                    "display_name": app_name,
                }
            )

        deduped = []
        seen = set()
        for item in normalized:
            key = item["project_path"]
            if not key or key in seen:
                continue
            seen.add(key)
            deduped.append(item)
        return deduped[:10]

    def _legacy_recent_apps_from_projects(self):
        result = []
        for item in self.recent_projects[:10]:
            app_name = item.get("display_name") or os.path.splitext(os.path.basename(item.get("project_path", "")))[0]
            sdk_root = item.get("sdk_root", "")
            if app_name and sdk_root:
                result.append((app_name, sdk_root))
        return result

    def _default_cached_sdk_root(self):
        return normalize_path(os.path.join(_get_config_dir(), "sdk", "EmbeddedGUI"))

    def _resolve_sdk_root(self, sdk_root=""):
        return resolve_available_sdk_root(
            sdk_root,
            self.sdk_root,
            self.egui_root,
            cached_sdk_root=self._default_cached_sdk_root(),
        )

    def load(self):
        """Load configuration from file."""
        config_path = _get_config_path()
        if not os.path.isfile(config_path):
            return

        try:
            with open(config_path, "r", encoding="utf-8") as f:
                data = json.load(f)

            self.sdk_root = normalize_path(data.get("sdk_root", data.get("egui_root", "")))
            self.egui_root = self.sdk_root
            self.last_app = data.get("last_app", "HelloDesigner")
            self.last_project_path = normalize_path(data.get("last_project_path", ""))
            self.recent_projects = self._normalize_recent_projects(
                data.get("recent_projects", []),
                data.get("recent_apps", []),
                self.sdk_root,
            )
            self.recent_apps = self._legacy_recent_apps_from_projects()
            self.theme = data.get("theme", "dark")
            self.auto_compile = data.get("auto_compile", True)
            self.overlay_mode = data.get("overlay_mode", "horizontal")
            self.overlay_flipped = data.get("overlay_flipped", True)
            self.show_grid = data.get("show_grid", True)
            self.grid_size = int(data.get("grid_size", 8))
            self.font_size_px = data.get("font_size_px", 0)
            self.show_all_examples = data.get("show_all_examples", False)
            self.window_geometry = data.get("window_geometry", "")
            self.window_state = data.get("window_state", "")
            self.sdk_setup_prompted = data.get("sdk_setup_prompted", False)
        except Exception as e:
            print(f"Warning: Failed to load config: {e}")

    def save(self):
        """Save configuration to file."""
        config_dir = _get_config_dir()
        config_path = _get_config_path()

        try:
            os.makedirs(config_dir, exist_ok=True)
            data = {
                "sdk_root": self.sdk_root,
                "egui_root": self.sdk_root,
                "last_app": self.last_app,
                "last_project_path": self.last_project_path,
                "recent_projects": self.recent_projects,
                "recent_apps": self._legacy_recent_apps_from_projects(),
                "theme": self.theme,
                "auto_compile": self.auto_compile,
                "overlay_mode": self.overlay_mode,
                "overlay_flipped": self.overlay_flipped,
                "show_grid": self.show_grid,
                "grid_size": self.grid_size,
                "font_size_px": self.font_size_px,
                "show_all_examples": self.show_all_examples,
                "window_geometry": self.window_geometry,
                "window_state": self.window_state,
                "sdk_setup_prompted": self.sdk_setup_prompted,
            }
            with open(config_path, "w", encoding="utf-8") as f:
                json.dump(data, f, indent=2, ensure_ascii=False)
        except Exception as e:
            print(f"Warning: Failed to save config: {e}")

    def add_recent_project(self, project_path, sdk_root="", display_name=""):
        """Add a project to the MRU list."""
        project_path = normalize_path(project_path)
        sdk_root = normalize_path(sdk_root)
        if not project_path:
            return
        if not display_name:
            display_name = os.path.splitext(os.path.basename(project_path))[0]

        self.recent_projects = [item for item in self.recent_projects if item.get("project_path") != project_path]
        self.recent_projects.insert(
            0,
            {
                "project_path": project_path,
                "sdk_root": sdk_root,
                "display_name": display_name,
            },
        )
        self.recent_projects = self.recent_projects[:10]
        self.recent_apps = self._legacy_recent_apps_from_projects()
        self.save()

    def remove_recent_project(self, project_path):
        """Remove a project from the MRU list."""
        project_path = normalize_path(project_path)
        if not project_path:
            return False

        original_len = len(self.recent_projects)
        self.recent_projects = [item for item in self.recent_projects if item.get("project_path") != project_path]
        removed = len(self.recent_projects) != original_len
        if removed:
            if self.last_project_path == project_path:
                self.last_project_path = ""
            self.recent_apps = self._legacy_recent_apps_from_projects()
            self.save()
        return removed

    def add_recent_app(self, app_name, egui_root):
        """Legacy MRU helper kept for compatibility."""
        if not app_name:
            return
        sdk_root = normalize_path(egui_root)
        project_path = ""
        if sdk_root:
            project_path = os.path.join(sdk_root, "example", app_name, f"{app_name}.egui")
        self.add_recent_project(project_path, sdk_root, app_name)

    def get_app_dir(self, app_name=None, sdk_root=None):
        """Get the default SDK example directory for an app."""
        app_name = app_name or self.last_app
        sdk_root = self._resolve_sdk_root(sdk_root)
        if not sdk_root or not app_name:
            return ""
        return os.path.join(sdk_root, "example", app_name)

    def get_project_path(self, app_name=None, sdk_root=None):
        """Get the default SDK example project path for an app."""
        app_dir = self.get_app_dir(app_name, sdk_root)
        if not app_dir:
            return ""
        app_name = app_name or self.last_app
        return os.path.join(app_dir, f"{app_name}.egui")

    def list_available_app_entries(self, sdk_root=None, include_legacy=False):
        """List all available app entries in the SDK ``example/`` directory."""
        sdk_root = self._resolve_sdk_root(sdk_root)
        if not sdk_root:
            return []

        example_dir = os.path.join(sdk_root, "example")
        if not os.path.isdir(example_dir):
            return []

        entries = []
        for name in os.listdir(example_dir):
            app_path = os.path.join(example_dir, name)
            if not os.path.isdir(app_path):
                continue
            if not os.path.isfile(os.path.join(app_path, "build.mk")):
                continue

            project_path = os.path.join(app_path, f"{name}.egui")
            has_project = os.path.isfile(project_path)
            if not has_project and not include_legacy:
                continue

            entries.append(
                {
                    "app_name": name,
                    "app_dir": app_path,
                    "project_path": project_path if has_project else "",
                    "has_project": has_project,
                    "is_legacy": not has_project,
                }
            )
        return sorted(entries, key=lambda item: item["app_name"].lower())

    def list_available_apps(self, sdk_root=None, include_legacy=False):
        """Return app names for SDK examples."""
        entries = self.list_available_app_entries(sdk_root=sdk_root, include_legacy=include_legacy)
        return [entry["app_name"] for entry in entries]


def get_config():
    """Get the global config instance."""
    return DesignerConfig.instance()

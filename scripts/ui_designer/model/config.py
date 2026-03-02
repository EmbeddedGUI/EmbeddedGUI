"""Global configuration management for EmbeddedGUI Designer.

Stores user preferences and recent paths in a JSON config file.
Config file location: .config/config.json next to the running script or exe.
"""

import os
import json
import sys


def _get_config_dir():
    """Get the configuration directory path.

    When running as a frozen exe (PyInstaller), uses the directory of the exe.
    Otherwise, uses the directory of this script (scripts/ui_designer/model/).
    The config is stored in a .config/ subdirectory next to the entry point.
    """
    if getattr(sys, 'frozen', False):
        # PyInstaller frozen exe
        base = os.path.dirname(sys.executable)
    else:
        # Running as script — go up from model/ to ui_designer/
        base = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    return os.path.join(base, ".config")


def _get_config_path():
    """Get the full path to the config file."""
    return os.path.join(_get_config_dir(), "config.json")


class DesignerConfig:
    """Manages global designer configuration.

    Attributes:
        egui_root: Path to EmbeddedGUI project root
        last_app: Name of last used app (e.g., "HelloDesigner")
        recent_apps: List of recently opened apps [(name, egui_root), ...]
        theme: Current theme ("dark" or "light")
        auto_compile: Whether auto-compile is enabled
        window_geometry: Saved window geometry (bytes, base64 encoded)
        window_state: Saved window state (bytes, base64 encoded)
    """

    _instance = None

    def __init__(self):
        self.egui_root = ""
        self.last_app = "HelloDesigner"
        self.last_project_path = ""  # Full path to last opened .egui file
        self.recent_apps = []  # [(app_name, egui_root), ...]
        self.theme = "dark"
        self.auto_compile = True
        self.overlay_mode = "horizontal"   # "vertical", "horizontal", "hidden"
        self.overlay_flipped = True        # True = wireframe first (left/top)
        self.font_size_px = 0              # 0 = use system default
        self.window_geometry = ""
        self.window_state = ""

    @classmethod
    def instance(cls):
        """Get the singleton config instance."""
        if cls._instance is None:
            cls._instance = cls()
            cls._instance.load()
        return cls._instance

    def load(self):
        """Load configuration from file."""
        config_path = _get_config_path()
        if not os.path.isfile(config_path):
            return

        try:
            with open(config_path, "r", encoding="utf-8") as f:
                data = json.load(f)

            self.egui_root = data.get("egui_root", "")
            self.last_app = data.get("last_app", "HelloDesigner")
            self.last_project_path = data.get("last_project_path", "")
            self.recent_apps = data.get("recent_apps", [])
            self.theme = data.get("theme", "dark")
            self.auto_compile = data.get("auto_compile", True)
            self.overlay_mode = data.get("overlay_mode", "horizontal")
            self.overlay_flipped = data.get("overlay_flipped", True)
            self.font_size_px = data.get("font_size_px", 0)
            self.window_geometry = data.get("window_geometry", "")
            self.window_state = data.get("window_state", "")
        except Exception as e:
            print(f"Warning: Failed to load config: {e}")

    def save(self):
        """Save configuration to file."""
        config_dir = _get_config_dir()
        config_path = _get_config_path()

        try:
            os.makedirs(config_dir, exist_ok=True)
            data = {
                "egui_root": self.egui_root,
                "last_app": self.last_app,
                "last_project_path": self.last_project_path,
                "recent_apps": self.recent_apps,
                "theme": self.theme,
                "auto_compile": self.auto_compile,
                "overlay_mode": self.overlay_mode,
                "overlay_flipped": self.overlay_flipped,
                "font_size_px": self.font_size_px,
                "window_geometry": self.window_geometry,
                "window_state": self.window_state,
            }
            with open(config_path, "w", encoding="utf-8") as f:
                json.dump(data, f, indent=2, ensure_ascii=False)
        except Exception as e:
            print(f"Warning: Failed to save config: {e}")

    def add_recent_app(self, app_name, egui_root):
        """Add an app to the recent list."""
        # Remove if already exists
        self.recent_apps = [
            (n, r) for (n, r) in self.recent_apps
            if not (n == app_name and r == egui_root)
        ]
        # Add to front
        self.recent_apps.insert(0, (app_name, egui_root))
        # Keep only last 10
        self.recent_apps = self.recent_apps[:10]
        self.save()

    def get_app_dir(self, app_name=None, egui_root=None):
        """Get the app directory path.

        Args:
            app_name: App name (default: self.last_app)
            egui_root: EmbeddedGUI root (default: self.egui_root)

        Returns:
            Path to app directory (e.g., .../example/HelloDesigner/)
        """
        app_name = app_name or self.last_app
        egui_root = egui_root or self.egui_root
        if not egui_root or not app_name:
            return ""
        return os.path.join(egui_root, "example", app_name)

    def get_project_path(self, app_name=None, egui_root=None):
        """Get the project file path (.egui) for an app.

        Args:
            app_name: App name (default: self.last_app)
            egui_root: EmbeddedGUI root (default: self.egui_root)

        Returns:
            Path to .egui project file
        """
        app_dir = self.get_app_dir(app_name, egui_root)
        if not app_dir:
            return ""
        app_name = app_name or self.last_app
        return os.path.join(app_dir, f"{app_name}.egui")

    def list_available_apps(self, egui_root=None):
        """List all available apps in the example/ directory.

        Returns:
            List of app names that have build.mk files
        """
        egui_root = egui_root or self.egui_root
        if not egui_root:
            return []

        example_dir = os.path.join(egui_root, "example")
        if not os.path.isdir(example_dir):
            return []

        apps = []
        for name in os.listdir(example_dir):
            app_path = os.path.join(example_dir, name)
            if os.path.isdir(app_path):
                # Check if it has build.mk (valid app)
                if os.path.isfile(os.path.join(app_path, "build.mk")):
                    apps.append(name)
        return sorted(apps)


# Convenience function
def get_config():
    """Get the global config instance."""
    return DesignerConfig.instance()

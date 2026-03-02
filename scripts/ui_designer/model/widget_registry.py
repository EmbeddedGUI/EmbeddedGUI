"""Centralized widget type registry for EmbeddedGUI Designer.

Provides a single source of truth for widget type descriptors, replacing
scattered hardcoded lists across widget_tree.py, code_generator.py, and
property_panel.py.

Built-in widgets are loaded from WIDGET_TYPES on first access.
Custom widgets can be registered at runtime via .py plugin files.
"""

import os
import importlib.util
import logging

logger = logging.getLogger(__name__)


class WidgetRegistry:
    """Central registry for widget type descriptors.

    Usage::

        reg = WidgetRegistry.instance()
        reg.get("label")           # -> descriptor dict
        reg.addable_types()        # -> [("Label", "label"), ...]
        reg.container_types()      # -> {"group", "linearlayout", ...}
        reg.tag_to_type("Label")   # -> "label"
        reg.type_to_tag("label")   # -> "Label"
    """

    _instance = None

    def __init__(self):
        self._types = {}        # type_name -> descriptor dict
        self._tag_map = {}      # XML tag -> type_name
        self._rev_tag_map = {}  # type_name -> XML tag
        self._addable = []      # [(display_name, type_name), ...]

    @classmethod
    def instance(cls):
        """Return the singleton registry, creating it on first call."""
        if cls._instance is None:
            cls._instance = cls()
            cls._instance._load_builtins()
        return cls._instance

    @classmethod
    def reset(cls):
        """Reset the singleton (for testing)."""
        cls._instance = None

    def _load_builtins(self):
        """Load all widget types from the custom_widgets/ plugin directory."""
        # custom_widgets/ is a sibling package of model/
        pkg_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        widgets_dir = os.path.join(pkg_dir, "custom_widgets")
        self.load_custom_widgets(widgets_dir)

    def register(self, type_name, descriptor, xml_tag=None, display_name=None):
        """Register a widget type.

        Args:
            type_name:    Internal type key (e.g., "label", "my_gauge").
            descriptor:   Dict with c_type, init_func, properties, etc.
            xml_tag:      XML tag for serialization (default: TitleCase of type_name).
            display_name: Human-readable name for UI menus (default: same as xml_tag).
        """
        self._types[type_name] = descriptor

        # Derive XML tag if not provided
        if xml_tag is None:
            xml_tag = type_name.replace("_", " ").title().replace(" ", "")
        self._tag_map[xml_tag] = type_name
        self._rev_tag_map[type_name] = xml_tag

        # Display name for menus
        if display_name is None:
            display_name = xml_tag

        # Add to addable list (avoid duplicates on re-register)
        addable = descriptor.get("addable", True)
        # Remove existing entry if re-registering
        self._addable = [(dn, tn) for dn, tn in self._addable if tn != type_name]
        if addable:
            self._addable.append((display_name, type_name))

    def get(self, type_name):
        """Get descriptor for a widget type. Returns empty dict if unknown."""
        return self._types.get(type_name, {})

    def has(self, type_name):
        """Check if a widget type is registered."""
        return type_name in self._types

    def tag_to_type(self, tag):
        """Convert XML tag to internal type name."""
        return self._tag_map.get(tag, tag.lower())

    def type_to_tag(self, type_name):
        """Convert internal type name to XML tag."""
        return self._rev_tag_map.get(type_name, type_name)

    def addable_types(self):
        """Return list of (display_name, type_name) for the Add Widget menu."""
        return list(self._addable)

    def container_types(self):
        """Return set of type names that are containers."""
        return {tn for tn, desc in self._types.items() if desc.get("is_container")}

    def all_types(self):
        """Return dict of all registered type descriptors."""
        return dict(self._types)

    def load_custom_widgets(self, *dirs):
        """Scan directories for custom widget .py plugin files and execute them.

        Each .py file is expected to call ``WidgetRegistry.instance().register(...)``
        to register one or more custom widget types.

        Files starting with ``_`` are skipped.

        Args:
            *dirs: Directory paths to scan for .py files.
        """
        for d in dirs:
            if not d or not os.path.isdir(d):
                continue
            for fname in sorted(os.listdir(d)):
                if not fname.endswith(".py") or fname.startswith("_"):
                    continue
                path = os.path.join(d, fname)
                try:
                    mod_name = f"custom_widget_{fname[:-3]}"
                    spec = importlib.util.spec_from_file_location(mod_name, path)
                    if spec and spec.loader:
                        mod = importlib.util.module_from_spec(spec)
                        spec.loader.exec_module(mod)
                        logger.info("Loaded custom widget plugin: %s", path)
                except Exception:
                    logger.exception("Failed to load custom widget plugin: %s", path)

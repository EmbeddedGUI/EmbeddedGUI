"""Android-style multi-language string resource management.

Directory convention (inside ``.eguiproject/resources/``):

    .eguiproject/resources/
        values/
            strings.xml          <- default locale
        values-zh/
            strings.xml          <- Chinese
        values-ja/
            strings.xml          <- Japanese
        ...

Legacy fallback: ``resource/src/values*/strings.xml``

Each ``strings.xml`` follows Android format::

    <?xml version="1.0" encoding="utf-8"?>
    <resources>
        <string name="app_name">My App</string>
        <string name="hello">Hello World</string>
    </resources>

Locale discovery is **automatic**: any ``values*/strings.xml`` found under
the given base directory is loaded.  ``values/`` is the default locale
(empty locale code ``""``).  ``values-xx/`` maps to locale code ``xx``.

Usage in widget text properties:
    - Literal text:   ``"Hello"``       -> emitted as C string literal
    - String ref:     ``"@string/hello"`` -> emitted as ``egui_i18n_get(EGUI_STR_HELLO)``
"""

import os
import re
import xml.etree.ElementTree as ET
from collections import OrderedDict

# Pattern to detect a string resource reference in a widget text property.
# e.g. "@string/hello_world"
STRING_REF_RE = re.compile(r'^@string/([A-Za-z_][A-Za-z0-9_]*)$')

# Pattern for valid string key names
_VALID_KEY_RE = re.compile(r'^[A-Za-z_][A-Za-z0-9_]*$')

# Pattern to detect values directories: "values" or "values-xx"
_VALUES_DIR_RE = re.compile(r'^values(?:-([a-zA-Z]{2,8}))?$')

# Default locale identifier (the "values/" directory)
DEFAULT_LOCALE = ""


def parse_string_ref(text):
    """Parse ``@string/key`` reference from widget text.

    Returns the key string if *text* is a valid ``@string/`` reference,
    otherwise returns ``None``.
    """
    if not text:
        return None
    m = STRING_REF_RE.match(text.strip())
    return m.group(1) if m else None


def make_string_ref(key):
    """Create an ``@string/key`` reference string."""
    return f"@string/{key}"


class StringResourceCatalog:
    """Manages multi-locale string resources using Android-style directory layout.

    Attributes:
        strings: OrderedDict[str, OrderedDict[str, str]]
            Maps locale_code -> OrderedDict[key -> value].
            locale_code ``""`` = default (``values/``).
            locale_code ``"zh"`` = Chinese (``values-zh/``), etc.
    """

    def __init__(self):
        self.strings = OrderedDict()  # locale -> {key -> value}

    # ── Properties ─────────────────────────────────────────────────

    @property
    def locales(self):
        """List of locale codes (default locale ``""`` first)."""
        result = []
        if DEFAULT_LOCALE in self.strings:
            result.append(DEFAULT_LOCALE)
        for loc in self.strings:
            if loc != DEFAULT_LOCALE:
                result.append(loc)
        return result

    @property
    def locale_display_names(self):
        """Human-readable locale names for UI display."""
        names = {}
        for loc in self.locales:
            if loc == DEFAULT_LOCALE:
                names[loc] = "Default"
            else:
                names[loc] = loc
        return names

    @property
    def all_keys(self):
        """Sorted list of all unique string keys across all locales."""
        keys = set()
        for locale_dict in self.strings.values():
            keys.update(locale_dict.keys())
        return sorted(keys)

    @property
    def has_strings(self):
        """True if any strings are defined in any locale."""
        return any(bool(d) for d in self.strings.values())

    # ── Key management ─────────────────────────────────────────────

    def get(self, key, locale=DEFAULT_LOCALE):
        """Get string value for a key in a specific locale.

        Falls back to default locale if the key is not found in the
        requested locale.
        """
        locale_dict = self.strings.get(locale, {})
        value = locale_dict.get(key)
        if value is not None:
            return value
        # Fallback to default locale
        if locale != DEFAULT_LOCALE:
            default_dict = self.strings.get(DEFAULT_LOCALE, {})
            return default_dict.get(key, "")
        return ""

    def set(self, key, value, locale=DEFAULT_LOCALE):
        """Set string value for a key in a specific locale."""
        if locale not in self.strings:
            self.strings[locale] = OrderedDict()
        self.strings[locale][key] = value

    def add_key(self, key, default_value=""):
        """Add a new string key with a default value.

        The key is added to all existing locales (empty value for
        non-default locales).
        """
        if not _VALID_KEY_RE.match(key):
            raise ValueError(
                f"Invalid string key '{key}': must be [A-Za-z_][A-Za-z0-9_]*"
            )
        for locale in self.locales:
            if locale == DEFAULT_LOCALE:
                self.strings[locale][key] = default_value
            else:
                if key not in self.strings[locale]:
                    self.strings[locale][key] = ""

    def remove_key(self, key):
        """Remove a string key from all locales."""
        for locale_dict in self.strings.values():
            locale_dict.pop(key, None)

    def rename_key(self, old_key, new_key):
        """Rename a string key across all locales."""
        if not _VALID_KEY_RE.match(new_key):
            raise ValueError(
                f"Invalid string key '{new_key}': must be [A-Za-z_][A-Za-z0-9_]*"
            )
        for locale_dict in self.strings.values():
            if old_key in locale_dict:
                value = locale_dict.pop(old_key)
                locale_dict[new_key] = value

    # ── Locale management ──────────────────────────────────────────

    def add_locale(self, locale_code):
        """Add a new locale. Creates empty entries for all existing keys."""
        if locale_code in self.strings:
            return  # Already exists
        new_dict = OrderedDict()
        # Pre-populate with all existing keys (empty values)
        for key in self.all_keys:
            new_dict[key] = ""
        self.strings[locale_code] = new_dict

    def remove_locale(self, locale_code):
        """Remove a locale (cannot remove default)."""
        if locale_code == DEFAULT_LOCALE:
            raise ValueError("Cannot remove the default locale")
        self.strings.pop(locale_code, None)

    # ── Collect all text for font glyph coverage ───────────────────

    def collect_all_chars(self):
        """Return set of all characters used across all locales and keys.

        This is used by the font glyph generator to ensure all needed
        characters are included in generated fonts.
        """
        chars = set()
        for locale_dict in self.strings.values():
            for value in locale_dict.values():
                chars.update(value)
        return chars

    # ── Directory I/O ──────────────────────────────────────────────

    @classmethod
    def _locale_to_dir_name(cls, locale_code):
        """Convert locale code to directory name."""
        if locale_code == DEFAULT_LOCALE:
            return "values"
        return f"values-{locale_code}"

    @classmethod
    def _dir_name_to_locale(cls, dir_name):
        """Convert directory name to locale code, or None if not a values dir."""
        m = _VALUES_DIR_RE.match(dir_name)
        if not m:
            return None
        return m.group(1) or DEFAULT_LOCALE

    @classmethod
    def scan_and_load(cls, src_dir):
        """Scan ``resource/src/`` for ``values*/strings.xml`` and load all.

        Args:
            src_dir: Path to ``resource/src/`` directory.

        Returns:
            StringResourceCatalog instance (may be empty if no values/ found).
        """
        catalog = cls()
        if not os.path.isdir(src_dir):
            return catalog

        for entry in sorted(os.listdir(src_dir)):
            entry_path = os.path.join(src_dir, entry)
            if not os.path.isdir(entry_path):
                continue
            locale = cls._dir_name_to_locale(entry)
            if locale is None:
                continue
            strings_xml = os.path.join(entry_path, "strings.xml")
            if os.path.isfile(strings_xml):
                locale_dict = cls._load_strings_xml(strings_xml)
                catalog.strings[locale] = locale_dict

        return catalog

    @classmethod
    def _load_strings_xml(cls, filepath):
        """Parse a single ``strings.xml`` file.

        Returns:
            OrderedDict[str, str]: key -> value
        """
        result = OrderedDict()
        try:
            tree = ET.parse(filepath)
            root = tree.getroot()
            for elem in root.findall("string"):
                name = elem.get("name", "").strip()
                value = (elem.text or "").strip()
                if name:
                    result[name] = value
        except Exception as e:
            print(f"Warning: Failed to parse {filepath}: {e}")
        return result

    def save(self, src_dir):
        """Save all locales to ``resource/src/values*/strings.xml``.

        Creates directories as needed. Removes directories for locales
        that have been deleted.

        Args:
            src_dir: Path to ``resource/src/`` directory.
        """
        os.makedirs(src_dir, exist_ok=True)

        # Write each locale
        written_dirs = set()
        for locale_code, locale_dict in self.strings.items():
            dir_name = self._locale_to_dir_name(locale_code)
            dir_path = os.path.join(src_dir, dir_name)
            os.makedirs(dir_path, exist_ok=True)
            written_dirs.add(dir_name)

            strings_xml = os.path.join(dir_path, "strings.xml")
            self._save_strings_xml(strings_xml, locale_dict)

        # Optionally clean up stale values-xx/ directories that are
        # no longer in our model (only if they had a strings.xml).
        # We do NOT auto-delete to be safe — user can manually clean up.

    @classmethod
    def _save_strings_xml(cls, filepath, locale_dict):
        """Write a single ``strings.xml`` file."""
        root = ET.Element("resources")
        for key, value in locale_dict.items():
            elem = ET.SubElement(root, "string")
            elem.set("name", key)
            elem.text = value

        ET.indent(root, space="    ")
        tree = ET.ElementTree(root)
        with open(filepath, "w", encoding="utf-8") as f:
            f.write('<?xml version="1.0" encoding="utf-8"?>\n')
            tree.write(f, encoding="unicode", xml_declaration=False)
            f.write("\n")

    # ── Resolve reference ──────────────────────────────────────────

    def resolve(self, text, locale=DEFAULT_LOCALE):
        """Resolve a text value: if it's ``@string/key``, return the
        localized value; otherwise return the text as-is.

        Args:
            text: Widget text property value.
            locale: Locale code to resolve in.

        Returns:
            The resolved display text.
        """
        key = parse_string_ref(text)
        if key is not None:
            return self.get(key, locale)
        return text

"""Page model for EmbeddedGUI Designer.

Each Page corresponds to a single XML layout file (e.g., layout/main_page.xml).
The page name is derived from the filename — no separate 'name' attribute is stored.
"""

import os
import xml.etree.ElementTree as ET

from .widget_model import WidgetModel


class Page:
    """Represents a single page/screen in the application.

    Page name is derived from file_path:
        'layout/main_page.xml' -> 'main_page'
    """

    def __init__(self, file_path="", root_widget=None):
        self.file_path = file_path  # relative path like 'layout/main_page.xml'
        self.root_widget = root_widget  # root WidgetModel for this page
        self.user_fields = []  # [{name, type, default?}, ...]
        self.timers = []  # [{name, callback, delay_ms, period_ms, auto_start}, ...]
        self._dirty = False
        # Background mockup image (per-page, design-time only)
        self.mockup_image_path = ""      # relative path in .eguiproject/ (e.g., "mockup/main.png")
        self.mockup_image_visible = True
        self.mockup_image_opacity = 0.3  # 0.0 ~ 1.0

    @property
    def name(self):
        """Derive page name from file path (filename without extension)."""
        return os.path.splitext(os.path.basename(self.file_path))[0]

    @property
    def c_struct_name(self):
        """C struct type name: egui_{page_name}_t"""
        return f"egui_{self.name}_t"

    @property
    def c_prefix(self):
        """C function prefix: egui_{page_name}"""
        return f"egui_{self.name}"

    @property
    def dirty(self):
        return self._dirty

    @dirty.setter
    def dirty(self, value):
        self._dirty = value

    def get_all_widgets(self):
        """Return flat list of all widgets in this page."""
        if self.root_widget:
            return self.root_widget.get_all_widgets_flat()
        return []

    # ── XML Serialization ──────────────────────────────────────────

    def to_xml_string(self):
        """Serialize page to XML string."""
        root_elem = ET.Element("Page")

        # Mockup image attributes (only if set)
        if self.mockup_image_path:
            root_elem.set("mockup_image", self.mockup_image_path)
            root_elem.set("mockup_visible", str(self.mockup_image_visible).lower())
            root_elem.set("mockup_opacity", str(self.mockup_image_opacity))

        # Root widget
        if self.root_widget:
            root_elem.append(self.root_widget.to_xml_element())

        # User-defined fields (timers, counters, etc.)
        if self.user_fields:
            fields_elem = ET.SubElement(root_elem, "UserFields")
            for field in self.user_fields:
                f_elem = ET.SubElement(fields_elem, "Field")
                f_elem.set("name", field.get("name", ""))
                f_elem.set("type", field.get("type", ""))
                if "default" in field:
                    f_elem.set("default", str(field["default"]))

        if self.timers:
            timers_elem = ET.SubElement(root_elem, "Timers")
            for timer in self.timers:
                t_elem = ET.SubElement(timers_elem, "Timer")
                t_elem.set("name", timer.get("name", ""))
                t_elem.set("callback", timer.get("callback", ""))
                t_elem.set("delay_ms", str(timer.get("delay_ms", "")))
                t_elem.set("period_ms", str(timer.get("period_ms", "")))
                t_elem.set("auto_start", str(bool(timer.get("auto_start", False))).lower())

        return _indent_xml(root_elem)

    @classmethod
    def from_xml_string(cls, xml_string, file_path="", src_dir=None):
        """Parse page from XML string.

        Args:
            xml_string: XML content to parse.
            file_path: Relative file path for this page.
            src_dir: Optional resource/src/ path for legacy migration.

        Returns Page on success, raises ValueError on parse error.
        """
        root_elem = ET.fromstring(xml_string)
        return cls._from_element(root_elem, file_path, src_dir=src_dir)

    @classmethod
    def _from_element(cls, root_elem, file_path, src_dir=None):
        page = cls(file_path=file_path)

        # Read mockup image attributes
        page.mockup_image_path = root_elem.get("mockup_image", "")
        page.mockup_image_visible = root_elem.get("mockup_visible", "true").lower() == "true"
        try:
            page.mockup_image_opacity = float(root_elem.get("mockup_opacity", "0.3"))
        except ValueError:
            page.mockup_image_opacity = 0.3

        for child in root_elem:
            if child.tag == "UserFields":
                for f_elem in child:
                    field = {
                        "name": f_elem.get("name", ""),
                        "type": f_elem.get("type", ""),
                    }
                    if "default" in f_elem.attrib:
                        field["default"] = f_elem.get("default")
                    page.user_fields.append(field)
            elif child.tag == "Timers":
                for t_elem in child:
                    timer = {
                        "name": t_elem.get("name", ""),
                        "callback": t_elem.get("callback", ""),
                        "delay_ms": t_elem.get("delay_ms", ""),
                        "period_ms": t_elem.get("period_ms", ""),
                        "auto_start": t_elem.get("auto_start", "false").lower() == "true",
                    }
                    page.timers.append(timer)
            else:
                # First non-UserFields child is the root widget
                if page.root_widget is None:
                    page.root_widget = WidgetModel.from_xml_element(child, src_dir=src_dir)

        return page

    # ── File I/O ───────────────────────────────────────────────────

    def save(self, base_dir):
        """Save page XML to file under base_dir."""
        full_path = os.path.join(base_dir, self.file_path)
        os.makedirs(os.path.dirname(full_path), exist_ok=True)
        xml_str = self.to_xml_string()
        with open(full_path, "w", encoding="utf-8") as f:
            f.write(xml_str)
        self._dirty = False

    @classmethod
    def load(cls, base_dir, file_path, src_dir=None):
        """Load page from XML file.

        Args:
            base_dir: Base directory for the project.
            file_path: Relative path to the page XML file.
            src_dir: Optional resource/src/ path for legacy migration.
        """
        full_path = os.path.join(base_dir, file_path)
        with open(full_path, "r", encoding="utf-8") as f:
            xml_string = f.read()
        return cls.from_xml_string(xml_string, file_path, src_dir=src_dir)

    @classmethod
    def create_default(cls, page_name, screen_width=240, screen_height=320):
        """Create a new page with a default root group container."""
        file_path = f"layout/{page_name}.xml"
        root = WidgetModel(
            "group", name="root_group",
            x=0, y=0, width=screen_width, height=screen_height,
        )
        page = cls(file_path=file_path, root_widget=root)
        page._dirty = True
        return page


def _indent_xml(elem):
    """Pretty-print XML with indentation."""
    ET.indent(elem, space="    ")
    return '<?xml version="1.0" encoding="utf-8"?>\n' + ET.tostring(
        elem, encoding="unicode"
    )

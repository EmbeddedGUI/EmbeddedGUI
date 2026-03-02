"""Adobe XD file (.xd) importer.

Parses XD files (which are ZIP archives containing JSON) into
a DesignTree for conversion to EGUI XML.
"""

import json
import os
import zipfile

from .base_importer import BaseImporter, DesignNode, DesignTree


class XDImporter(BaseImporter):
    """Import .xd files into the design pipeline."""

    def parse(self, input_path):
        """Parse a .xd file and return a DesignTree."""
        with zipfile.ZipFile(input_path, "r") as zf:
            manifest = json.loads(zf.read("manifest"))
            # Use first artboard
            ab_entry = manifest["children"][0]
            ab_path = ab_entry["path"]
            ab_name = ab_entry.get("name", "Screen")
            ab_data = json.loads(zf.read(ab_path))

        ab_info = ab_data.get("artboard", {})
        root = DesignNode(
            node_type="frame",
            name=ab_name,
            x=0,
            y=0,
            width=int(ab_info.get("width", 240)),
            height=int(ab_info.get("height", 320)),
        )

        for child_data in ab_data.get("children", []):
            child = self._parse_element(child_data)
            if child:
                root.add_child(child)

        return DesignTree(root=root, source_format="xd")

    def export_assets(self, design_tree, output_dir):
        """Export image assets from the design tree."""
        os.makedirs(output_dir, exist_ok=True)

    # ------------------------------------------------------------------
    # Internal helpers
    # ------------------------------------------------------------------

    def _parse_element(self, elem):
        elem_type = elem.get("type", "")
        name = elem.get("name", elem_type)
        transform = elem.get("transform", {})
        x = int(transform.get("tx", 0))
        y = int(transform.get("ty", 0))

        # Determine dimensions
        width, height = self._get_dimensions(elem)
        node_type = self._type_to_node_type(elem_type)

        node = DesignNode(
            node_type=node_type, name=name, x=x, y=y, width=width, height=height
        )

        # Extract text
        text_info = elem.get("text", {})
        if text_info.get("rawText"):
            node.set_property("text", text_info["rawText"])

        # Extract font size
        style = elem.get("style", {})
        font = style.get("font", {})
        if font.get("size"):
            node.set_property("font_size", int(font["size"]))

        # Recurse into group children
        group = elem.get("group", {})
        for child_data in group.get("children", []):
            child = self._parse_element(child_data)
            if child:
                node.add_child(child)

        return node

    @staticmethod
    def _get_dimensions(elem):
        shape = elem.get("shape", {})
        if shape:
            return int(shape.get("width", 0)), int(shape.get("height", 0))
        # Fallback: try uxdesign bounds or default
        return 0, 0

    @staticmethod
    def _type_to_node_type(xd_type):
        mapping = {
            "artboard": "frame",
            "group": "group",
            "text": "text",
            "shape": "shape",
            "bitmap": "image",
        }
        return mapping.get(xd_type, "shape")

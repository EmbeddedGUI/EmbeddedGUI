"""Sketch file (.sketch) importer.

Parses Sketch files (which are ZIP archives containing JSON) into
a DesignTree for conversion to EGUI XML.
"""

import json
import os
import zipfile

from .base_importer import BaseImporter, DesignNode, DesignTree


class SketchImporter(BaseImporter):
    """Import .sketch files into the design pipeline."""

    def parse(self, input_path):
        """Parse a .sketch file and return a DesignTree."""
        with zipfile.ZipFile(input_path, "r") as zf:
            doc = json.loads(zf.read("document.json"))
            # Use first page
            page_ref = doc["pages"][0]["_ref"]
            page_data = json.loads(zf.read(f"{page_ref}.json"))

        page_name = page_data.get("name", "Screen")
        frame = page_data.get("frame", {})
        root = DesignNode(
            node_type="frame",
            name=page_name,
            x=int(frame.get("x", 0)),
            y=int(frame.get("y", 0)),
            width=int(frame.get("width", 240)),
            height=int(frame.get("height", 320)),
        )

        for layer in page_data.get("layers", []):
            child = self._parse_layer(layer)
            if child:
                root.add_child(child)

        return DesignTree(root=root, source_format="sketch")

    def export_assets(self, design_tree, output_dir):
        """Export image assets from the design tree."""
        os.makedirs(output_dir, exist_ok=True)

    # ------------------------------------------------------------------
    # Internal helpers
    # ------------------------------------------------------------------

    def _parse_layer(self, layer):
        cls = layer.get("_class", "")
        frame = layer.get("frame", {})
        name = layer.get("name", cls)

        node_type = self._class_to_type(cls)
        node = DesignNode(
            node_type=node_type,
            name=name,
            x=int(frame.get("x", 0)),
            y=int(frame.get("y", 0)),
            width=int(frame.get("width", 0)),
            height=int(frame.get("height", 0)),
        )

        # Extract text content
        attr_str = layer.get("attributedString", {})
        if attr_str.get("string"):
            node.set_property("text", attr_str["string"])

        # Recurse into sub-layers (groups, artboards)
        for sub in layer.get("layers", []):
            child = self._parse_layer(sub)
            if child:
                node.add_child(child)

        return node

    @staticmethod
    def _class_to_type(cls):
        mapping = {
            "artboard": "frame",
            "group": "group",
            "text": "text",
            "rectangle": "rectangle",
            "oval": "shape",
            "shapePath": "shape",
            "bitmap": "image",
            "symbolInstance": "group",
        }
        return mapping.get(cls, "rectangle")

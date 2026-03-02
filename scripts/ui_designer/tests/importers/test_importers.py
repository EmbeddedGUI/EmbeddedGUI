"""Tests for the importer abstraction layer.

Tests BaseImporter interface, SketchImporter, and XDImporter.
"""

import os
import sys
import json
import zipfile
import tempfile
import pytest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", ".."))

from ui_designer.importers.base_importer import BaseImporter, DesignTree, DesignNode
from ui_designer.importers.sketch_importer import SketchImporter
from ui_designer.importers.xd_importer import XDImporter


# ---------------------------------------------------------------------------
# DesignNode / DesignTree tests
# ---------------------------------------------------------------------------

class TestDesignNode:
    def test_create_node(self):
        node = DesignNode(node_type="frame", name="root", x=0, y=0, width=240, height=320)
        assert node.node_type == "frame"
        assert node.name == "root"
        assert node.width == 240
        assert node.height == 320
        assert node.children == []
        assert node.properties == {}

    def test_add_child(self):
        parent = DesignNode(node_type="frame", name="parent", x=0, y=0, width=240, height=320)
        child = DesignNode(node_type="text", name="label1", x=10, y=10, width=100, height=20)
        parent.add_child(child)
        assert len(parent.children) == 1
        assert parent.children[0].name == "label1"

    def test_set_property(self):
        node = DesignNode(node_type="text", name="lbl", x=0, y=0, width=100, height=20)
        node.set_property("text", "Hello")
        node.set_property("font_size", 14)
        assert node.properties["text"] == "Hello"
        assert node.properties["font_size"] == 14

    def test_to_dict(self):
        node = DesignNode(node_type="frame", name="root", x=0, y=0, width=240, height=320)
        child = DesignNode(node_type="text", name="lbl", x=10, y=10, width=80, height=20)
        child.set_property("text", "Hi")
        node.add_child(child)
        d = node.to_dict()
        assert d["node_type"] == "frame"
        assert d["name"] == "root"
        assert len(d["children"]) == 1
        assert d["children"][0]["properties"]["text"] == "Hi"


class TestDesignTree:
    def test_create_tree(self):
        root = DesignNode(node_type="frame", name="screen", x=0, y=0, width=240, height=320)
        tree = DesignTree(root=root, source_format="sketch", metadata={"version": "1.0"})
        assert tree.root.name == "screen"
        assert tree.source_format == "sketch"
        assert tree.metadata["version"] == "1.0"

    def test_flatten_nodes(self):
        root = DesignNode(node_type="frame", name="root", x=0, y=0, width=240, height=320)
        c1 = DesignNode(node_type="text", name="a", x=0, y=0, width=50, height=20)
        c2 = DesignNode(node_type="frame", name="b", x=0, y=0, width=100, height=100)
        c3 = DesignNode(node_type="text", name="c", x=0, y=0, width=50, height=20)
        c2.add_child(c3)
        root.add_child(c1)
        root.add_child(c2)
        tree = DesignTree(root=root, source_format="test")
        flat = tree.flatten_nodes()
        names = [n.name for n in flat]
        assert "root" in names
        assert "a" in names
        assert "b" in names
        assert "c" in names
        assert len(flat) == 4


# ---------------------------------------------------------------------------
# BaseImporter abstract interface tests
# ---------------------------------------------------------------------------

class TestBaseImporter:
    def test_cannot_instantiate(self):
        with pytest.raises(TypeError):
            BaseImporter()

    def test_subclass_must_implement_parse(self):
        class Incomplete(BaseImporter):
            def to_xml(self, design_tree):
                return ""
            def export_assets(self, design_tree, output_dir):
                pass
        with pytest.raises(TypeError):
            Incomplete()

    def test_to_xml_default(self):
        """BaseImporter.to_xml has a default implementation that produces XML."""
        root = DesignNode(node_type="frame", name="screen", x=0, y=0, width=240, height=320)
        child = DesignNode(node_type="text", name="title", x=10, y=10, width=100, height=24)
        child.set_property("text", "Hello")
        root.add_child(child)
        tree = DesignTree(root=root, source_format="test")

        class Concrete(BaseImporter):
            def parse(self, input_path):
                return tree
            def export_assets(self, design_tree, output_dir):
                pass

        imp = Concrete()
        xml = imp.to_xml(tree)
        assert "<Group" in xml or "<frame" in xml.lower() or "screen" in xml


# ---------------------------------------------------------------------------
# SketchImporter tests
# ---------------------------------------------------------------------------

class TestSketchImporter:
    def _make_sketch_file(self, tmp_path, pages_data):
        """Create a minimal .sketch zip with document.json and pages."""
        sketch_path = os.path.join(str(tmp_path), "test.sketch")
        with zipfile.ZipFile(sketch_path, "w") as zf:
            doc = {"pages": [{"_ref": f"pages/{pid}"} for pid in pages_data]}
            zf.writestr("document.json", json.dumps(doc))
            for pid, page_content in pages_data.items():
                zf.writestr(f"pages/{pid}.json", json.dumps(page_content))
        return sketch_path

    def test_parse_sketch_file(self, tmp_path):
        pages = {
            "page1": {
                "name": "MainScreen",
                "frame": {"x": 0, "y": 0, "width": 240, "height": 320},
                "layers": [
                    {
                        "_class": "text",
                        "name": "title",
                        "frame": {"x": 10, "y": 10, "width": 100, "height": 24},
                        "attributedString": {"string": "Hello"},
                    }
                ],
            }
        }
        sketch_path = self._make_sketch_file(tmp_path, pages)
        imp = SketchImporter()
        tree = imp.parse(sketch_path)
        assert tree is not None
        assert tree.source_format == "sketch"
        assert tree.root.name == "MainScreen"

    def test_parse_sketch_layers(self, tmp_path):
        pages = {
            "p1": {
                "name": "Screen",
                "frame": {"x": 0, "y": 0, "width": 240, "height": 320},
                "layers": [
                    {
                        "_class": "rectangle",
                        "name": "bg",
                        "frame": {"x": 0, "y": 0, "width": 240, "height": 320},
                    },
                    {
                        "_class": "text",
                        "name": "label",
                        "frame": {"x": 20, "y": 50, "width": 80, "height": 20},
                        "attributedString": {"string": "Test"},
                    },
                ],
            }
        }
        sketch_path = self._make_sketch_file(tmp_path, pages)
        imp = SketchImporter()
        tree = imp.parse(sketch_path)
        assert len(tree.root.children) == 2
        assert tree.root.children[1].properties.get("text") == "Test"

    def test_parse_nested_groups(self, tmp_path):
        pages = {
            "p1": {
                "name": "Screen",
                "frame": {"x": 0, "y": 0, "width": 240, "height": 320},
                "layers": [
                    {
                        "_class": "group",
                        "name": "header",
                        "frame": {"x": 0, "y": 0, "width": 240, "height": 60},
                        "layers": [
                            {
                                "_class": "text",
                                "name": "title",
                                "frame": {"x": 10, "y": 10, "width": 100, "height": 24},
                                "attributedString": {"string": "Title"},
                            }
                        ],
                    }
                ],
            }
        }
        sketch_path = self._make_sketch_file(tmp_path, pages)
        imp = SketchImporter()
        tree = imp.parse(sketch_path)
        header = tree.root.children[0]
        assert header.node_type == "group"
        assert len(header.children) == 1
        assert header.children[0].properties.get("text") == "Title"

    def test_export_assets(self, tmp_path):
        root = DesignNode(node_type="frame", name="screen", x=0, y=0, width=240, height=320)
        tree = DesignTree(root=root, source_format="sketch")
        imp = SketchImporter()
        out_dir = os.path.join(str(tmp_path), "assets")
        # Should not raise
        imp.export_assets(tree, out_dir)
        assert os.path.isdir(out_dir)


# ---------------------------------------------------------------------------
# XDImporter tests
# ---------------------------------------------------------------------------

class TestXDImporter:
    def _make_xd_file(self, tmp_path, manifest, artboards):
        """Create a minimal .xd zip with manifest and artboard JSON."""
        xd_path = os.path.join(str(tmp_path), "test.xd")
        with zipfile.ZipFile(xd_path, "w") as zf:
            zf.writestr("manifest", json.dumps(manifest))
            for ab_path, ab_data in artboards.items():
                zf.writestr(ab_path, json.dumps(ab_data))
        return xd_path

    def test_parse_xd_file(self, tmp_path):
        manifest = {
            "children": [
                {"path": "artboard-1/graphics/graphicContent.agc", "name": "Screen1"}
            ]
        }
        artboard = {
            "children": [
                {
                    "type": "text",
                    "name": "greeting",
                    "transform": {"tx": 10, "ty": 20},
                    "text": {"rawText": "Hello XD"},
                    "style": {"font": {"size": 16}},
                }
            ],
            "artboard": {"width": 240, "height": 320},
        }
        xd_path = self._make_xd_file(
            tmp_path, manifest, {"artboard-1/graphics/graphicContent.agc": artboard}
        )
        imp = XDImporter()
        tree = imp.parse(xd_path)
        assert tree is not None
        assert tree.source_format == "xd"
        assert tree.root.name == "Screen1"

    def test_parse_xd_children(self, tmp_path):
        manifest = {
            "children": [
                {"path": "ab/graphics/graphicContent.agc", "name": "Main"}
            ]
        }
        artboard = {
            "children": [
                {
                    "type": "shape",
                    "name": "bg_rect",
                    "transform": {"tx": 0, "ty": 0},
                    "shape": {"width": 240, "height": 320, "type": "rect"},
                },
                {
                    "type": "text",
                    "name": "label",
                    "transform": {"tx": 20, "ty": 50},
                    "text": {"rawText": "XD Label"},
                    "style": {"font": {"size": 14}},
                },
            ],
            "artboard": {"width": 240, "height": 320},
        }
        xd_path = self._make_xd_file(
            tmp_path, manifest, {"ab/graphics/graphicContent.agc": artboard}
        )
        imp = XDImporter()
        tree = imp.parse(xd_path)
        assert len(tree.root.children) == 2
        assert tree.root.children[1].properties.get("text") == "XD Label"

    def test_parse_xd_nested_groups(self, tmp_path):
        manifest = {
            "children": [
                {"path": "ab/graphics/graphicContent.agc", "name": "Main"}
            ]
        }
        artboard = {
            "children": [
                {
                    "type": "group",
                    "name": "header",
                    "transform": {"tx": 0, "ty": 0},
                    "group": {"children": [
                        {
                            "type": "text",
                            "name": "title",
                            "transform": {"tx": 10, "ty": 5},
                            "text": {"rawText": "Title"},
                            "style": {"font": {"size": 18}},
                        }
                    ]},
                }
            ],
            "artboard": {"width": 240, "height": 320},
        }
        xd_path = self._make_xd_file(
            tmp_path, manifest, {"ab/graphics/graphicContent.agc": artboard}
        )
        imp = XDImporter()
        tree = imp.parse(xd_path)
        header = tree.root.children[0]
        assert header.node_type == "group"
        assert header.children[0].properties.get("text") == "Title"

    def test_export_assets(self, tmp_path):
        root = DesignNode(node_type="frame", name="screen", x=0, y=0, width=240, height=320)
        tree = DesignTree(root=root, source_format="xd")
        imp = XDImporter()
        out_dir = os.path.join(str(tmp_path), "assets")
        imp.export_assets(tree, out_dir)
        assert os.path.isdir(out_dir)

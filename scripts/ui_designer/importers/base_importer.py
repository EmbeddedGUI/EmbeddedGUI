"""Base importer abstraction for design file formats.

Provides DesignNode, DesignTree, and the abstract BaseImporter class
that all format-specific importers must implement.
"""

from abc import ABC, abstractmethod


class DesignNode:
    """A node in the design tree representing a UI element."""

    __slots__ = ("node_type", "name", "x", "y", "width", "height", "children", "properties")

    def __init__(self, node_type, name, x, y, width, height):
        self.node_type = node_type
        self.name = name
        self.x = x
        self.y = y
        self.width = width
        self.height = height
        self.children = []
        self.properties = {}

    def add_child(self, child):
        self.children.append(child)

    def set_property(self, key, value):
        self.properties[key] = value

    def to_dict(self):
        return {
            "node_type": self.node_type,
            "name": self.name,
            "x": self.x,
            "y": self.y,
            "width": self.width,
            "height": self.height,
            "properties": dict(self.properties),
            "children": [c.to_dict() for c in self.children],
        }


class DesignTree:
    """Container for a parsed design file."""

    __slots__ = ("root", "source_format", "metadata")

    def __init__(self, root, source_format, metadata=None):
        self.root = root
        self.source_format = source_format
        self.metadata = metadata or {}

    def flatten_nodes(self):
        """Return all nodes in the tree as a flat list (pre-order)."""
        result = []
        stack = [self.root]
        while stack:
            node = stack.pop()
            result.append(node)
            # Reverse so left children are visited first
            stack.extend(reversed(node.children))
        return result


class BaseImporter(ABC):
    """Abstract base class for design file importers.

    Subclasses must implement:
      - parse(input_path) -> DesignTree
      - export_assets(design_tree, output_dir)

    to_xml() has a default implementation that converts a DesignTree to
    a simple XML string.
    """

    @abstractmethod
    def parse(self, input_path):
        """Parse a design file and return a DesignTree."""

    def to_xml(self, design_tree):
        """Convert a DesignTree to an XML string."""
        lines = []
        self._node_to_xml(design_tree.root, lines, indent=0)
        return "\n".join(lines)

    @abstractmethod
    def export_assets(self, design_tree, output_dir):
        """Export image/font assets referenced by the design."""

    # ------------------------------------------------------------------
    # Internal helpers
    # ------------------------------------------------------------------

    def _node_to_xml(self, node, lines, indent):
        prefix = "    " * indent
        tag = self._node_type_to_tag(node)
        attrs = f' name="{node.name}" x="{node.x}" y="{node.y}" width="{node.width}" height="{node.height}"'
        for k, v in node.properties.items():
            attrs += f' {k}="{v}"'
        if node.children:
            lines.append(f"{prefix}<{tag}{attrs}>")
            for child in node.children:
                self._node_to_xml(child, lines, indent + 1)
            lines.append(f"{prefix}</{tag}>")
        else:
            lines.append(f"{prefix}<{tag}{attrs} />")

    @staticmethod
    def _node_type_to_tag(node):
        mapping = {
            "frame": "Group",
            "group": "Group",
            "text": "Label",
            "rectangle": "Group",
            "shape": "Group",
            "image": "Image",
        }
        return mapping.get(node.node_type, "Group")

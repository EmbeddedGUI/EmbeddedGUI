#!/usr/bin/env python3
"""Stage 2c: Generate EGUI XML layout + C code from Figma Make TSX project.

Orchestrates the full conversion:
  1. Parse TSX project structure (figmamake_parser)
  2. Extract animations (figmamake_anim_extractor)
  3. Generate XML layout files with embedded animations
  4. Create .egui project file
  5. Invoke existing code_generator to produce C files

Usage:
    python scripts/figmamake/figmamake_codegen.py \
        --project-dir /path/to/figmamake/project \
        --app HelloBattery --width 320 --height 240
"""

import argparse
import json
import os
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPTS_DIR = os.path.dirname(SCRIPT_DIR)
sys.path.insert(0, SCRIPT_DIR)
sys.path.insert(0, SCRIPTS_DIR)

from html2egui_helper import (
    _discover_figmamake_pages,
    _extract_lucide_imports,
    _extract_jsx_return,
    _extract_jsx_comments,
    _jsx_to_pseudo_html,
    _lucide_name_to_material,
    _extract_tw_color,
    _find_egui_root,
    _build_egui_project_xml,
)
from figmamake_anim_extractor import AnimExtractor


# ── Tailwind → pixel converters ──────────────────────────────────

_TW_SPACING = {
    "0": 0, "0.5": 2, "1": 4, "1.5": 6, "2": 8, "2.5": 10,
    "3": 12, "3.5": 14, "4": 16, "5": 20, "6": 24, "7": 28,
    "8": 32, "9": 36, "10": 40, "11": 44, "12": 48,
    "14": 56, "16": 64, "20": 80, "24": 96,
}

_TW_TEXT_SIZE = {
    "xs": 10, "sm": 12, "base": 14, "lg": 16, "xl": 20,
    "2xl": 24, "3xl": 28, "4xl": 32, "5xl": 40, "6xl": 48,
}

_TW_ROUNDED = {
    "none": 0, "sm": 2, "": 4, "md": 6, "lg": 8,
    "xl": 12, "2xl": 16, "3xl": 24, "full": 9999,
}


def _tw_px(val):
    """Convert Tailwind spacing value to pixels."""
    if val in _TW_SPACING:
        return _TW_SPACING[val]
    m = re.match(r'\[(\d+)px\]', val)
    if m:
        return int(m.group(1))
    return None


def _parse_cls_size(cls, prefix):
    """Extract pixel size from w-[Npx] or w-N class."""
    m = re.search(rf'{prefix}-\[(\d+)px\]', cls)
    if m:
        return int(m.group(1))
    m = re.search(rf'{prefix}-(\d+(?:\.\d+)?)\b', cls)
    if m and m.group(1) in _TW_SPACING:
        return _TW_SPACING[m.group(1)]
    if re.search(rf'{prefix}-full', cls):
        return -1  # sentinel: fill parent
    return None


def _parse_cls_color(cls, prefix="bg"):
    """Extract hex color from bg-[#RRGGBB] or text-[#RRGGBB]."""
    m = re.search(rf'{prefix}-\[#([0-9a-fA-F]{{6}})\]', cls)
    return m.group(1).upper() if m else None


def _parse_cls_text_size(cls):
    """Extract font pixel size from text-xs/sm/base/lg/xl or text-[14px]."""
    m = re.search(r'text-\[(\d+)px\]', cls)
    if m:
        return int(m.group(1))
    for name, px in _TW_TEXT_SIZE.items():
        if re.search(rf'\btext-{re.escape(name)}\b', cls):
            return px
    return None


def _parse_cls_rounded(cls):
    """Extract corner radius from rounded-* class."""
    m = re.search(r'rounded-(\w+)', cls)
    if m:
        return _TW_ROUNDED.get(m.group(1), 4)
    if "rounded" in cls.split():
        return 4
    return 0


def _parse_cls_gap(cls):
    """Extract gap from gap-N class."""
    m = re.search(r'gap-(\d+(?:\.\d+)?)', cls)
    if m and m.group(1) in _TW_SPACING:
        return _TW_SPACING[m.group(1)]
    m = re.search(r'gap-\[(\d+)px\]', cls)
    if m:
        return int(m.group(1))
    return 0


def _parse_cls_padding(cls):
    """Extract padding from p-N, px-N, py-N classes."""
    pad = {"top": 0, "right": 0, "bottom": 0, "left": 0}
    m = re.search(r'\bp-(\d+(?:\.\d+)?)\b', cls)
    if m and m.group(1) in _TW_SPACING:
        v = _TW_SPACING[m.group(1)]
        pad = {"top": v, "right": v, "bottom": v, "left": v}
    m = re.search(r'\bpx-(\d+(?:\.\d+)?)\b', cls)
    if m and m.group(1) in _TW_SPACING:
        v = _TW_SPACING[m.group(1)]
        pad["left"] = v
        pad["right"] = v
    m = re.search(r'\bpy-(\d+(?:\.\d+)?)\b', cls)
    if m and m.group(1) in _TW_SPACING:
        v = _TW_SPACING[m.group(1)]
        pad["top"] = v
        pad["bottom"] = v
    return pad


def _is_flex_col(cls):
    return "flex-col" in cls


def _is_flex_row(cls):
    return "flex" in cls and "flex-col" not in cls


# ── HTML parser for pseudo-HTML from JSX ─────────────────────────

from html.parser import HTMLParser


class _NodeParser(HTMLParser):
    """Parse pseudo-HTML into a node tree for XML generation."""

    def __init__(self):
        super().__init__()
        self._stack = []
        self._root = None

    def handle_starttag(self, tag, attrs):
        node = {"tag": tag, "attrs": dict(attrs), "children": [], "text": ""}
        if self._stack:
            self._stack[-1]["children"].append(node)
        self._stack.append(node)

    def handle_endtag(self, tag):
        if self._stack:
            node = self._stack.pop()
            if not self._stack:
                self._root = node

    def handle_startendtag(self, tag, attrs):
        node = {"tag": tag, "attrs": dict(attrs), "children": [], "text": ""}
        if self._stack:
            self._stack[-1]["children"].append(node)
        elif not self._root:
            self._root = node

    def handle_data(self, data):
        text = data.strip()
        if text and self._stack:
            self._stack[-1]["text"] += text


# ── Node → XML element conversion ───────────────────────────────

class XMLLayoutGenerator:
    """Convert parsed TSX nodes into EGUI XML layout elements."""

    def __init__(self, screen_w, screen_h, root_bg="060608"):
        self.screen_w = screen_w
        self.screen_h = screen_h
        self.root_bg = root_bg
        self._id_counter = {}
        self._animations = {}  # target_id -> list of animation dicts

    def set_animations(self, anims_list):
        """Set animation descriptions from AnimExtractor."""
        for anim in anims_list:
            tid = anim.get("target_id", "")
            if tid:
                self._animations.setdefault(tid, []).append(anim)

    def _unique_id(self, prefix):
        """Generate unique widget ID."""
        count = self._id_counter.get(prefix, 0)
        self._id_counter[prefix] = count + 1
        if count == 0:
            return prefix
        return f"{prefix}_{count}"

    def generate_page_xml(self, page_node, page_name):
        """Generate complete page XML from parsed node tree."""
        root = ET.Element("Page")
        group = ET.SubElement(root, "Group")
        group.set("id", "root")
        group.set("x", "0")
        group.set("y", "0")
        group.set("width", str(self.screen_w))
        group.set("height", str(self.screen_h))

        # Background
        if self.root_bg:
            bg = ET.SubElement(group, "Background")
            bg.set("type", "solid")
            bg.set("color", f"EGUI_COLOR_HEX(0x{self.root_bg})")
            bg.set("alpha", "EGUI_ALPHA_100")

        # Convert children
        if page_node and page_node.get("children"):
            self._convert_children(page_node["children"], group,
                                   0, 0, self.screen_w, self.screen_h)

        ET.indent(root, space="    ")
        return root

    def _convert_children(self, children, parent_elem,
                          parent_x, parent_y, parent_w, parent_h):
        """Recursively convert child nodes to XML elements."""
        cls_str = " ".join(
            parent_elem.get("_classes", "").split()
        ) if parent_elem.get("_classes") else ""

        is_col = _is_flex_col(cls_str)
        gap = _parse_cls_gap(cls_str)
        pad = _parse_cls_padding(cls_str)

        cursor_x = pad.get("left", 0)
        cursor_y = pad.get("top", 0)
        avail_w = parent_w - pad.get("left", 0) - pad.get("right", 0)
        avail_h = parent_h - pad.get("top", 0) - pad.get("bottom", 0)

        for child in children:
            tag = child.get("tag", "div")
            attrs = child.get("attrs", {})
            cls = attrs.get("class", attrs.get("classname", ""))
            text = child.get("text", "").strip()
            sub_children = child.get("children", [])

            # Skip non-visual elements
            if tag in ("style", "script", "link", "meta"):
                continue

            # Determine dimensions
            w = _parse_cls_size(cls, "w")
            h = _parse_cls_size(cls, "h")
            if w == -1:
                w = avail_w
            if h == -1:
                h = avail_h
            if w is None:
                w = avail_w
            if h is None:
                h = 20  # default height

            # Determine position
            x = cursor_x
            y = cursor_y

            # Check for absolute positioning
            if "absolute" in cls:
                top = _parse_cls_size(cls, "top")
                left = _parse_cls_size(cls, "left")
                right = _parse_cls_size(cls, "right")
                bottom = _parse_cls_size(cls, "bottom")
                if top is not None:
                    y = top
                if left is not None:
                    x = left
                if right is not None and w:
                    x = parent_w - w - right
                if bottom is not None and h:
                    y = parent_h - h - bottom
            else:
                # Advance cursor for flex layout
                if is_col:
                    cursor_y += h + gap
                else:
                    cursor_x += w + gap

            # Determine element type
            bg_color = _parse_cls_color(cls, "bg")
            text_color = _parse_cls_color(cls, "text")
            font_size = _parse_cls_text_size(cls)
            radius = _parse_cls_rounded(cls)

            # Is this a text element?
            is_text = bool(text) and not sub_children
            # Is this an SVG/icon?
            is_svg = tag == "svg" or tag == "icon"
            # Is this a container?
            is_container = bool(sub_children) and not is_text

            if is_svg:
                # Skip SVGs for now — icons handled separately
                continue

            if is_text and not is_container:
                elem = self._make_label(text, x, y, w, h,
                                        text_color, font_size, cls)
                parent_elem.append(elem)
            elif is_container or bg_color:
                elem = self._make_group_or_card(
                    x, y, w, h, bg_color, radius, cls, tag)
                parent_elem.append(elem)
                # Recurse into children
                if sub_children:
                    self._convert_children(
                        sub_children, elem, x, y, w, h)
            elif text:
                # Text node with children — treat as group with label
                elem = self._make_group_or_card(
                    x, y, w, h, bg_color, radius, cls, tag)
                parent_elem.append(elem)
                label = self._make_label(text, 0, 0, w, h,
                                         text_color, font_size, cls)
                elem.append(label)
                if sub_children:
                    self._convert_children(
                        sub_children, elem, 0, 0, w, h)

    def _make_label(self, text, x, y, w, h, color, font_size, cls):
        """Create a Label XML element."""
        label_id = self._unique_id(
            re.sub(r'[^a-z0-9]', '_', text[:12].lower()).strip('_') or "label"
        )
        elem = ET.Element("Label")
        elem.set("id", label_id)
        elem.set("x", str(x))
        elem.set("y", str(y))
        elem.set("width", str(w))
        elem.set("height", str(h))
        elem.set("text", text)

        if font_size and font_size >= 20:
            elem.set("font_builtin",
                      f"&egui_res_font_montserrat_{font_size}_4")
        elif font_size:
            elem.set("font_file", "simhei.ttf")
            elem.set("font_pixelsize", str(font_size))
            elem.set("font_fontbitsize", "4")

        if color:
            elem.set("color", f"EGUI_COLOR_HEX(0x{color})")
        else:
            elem.set("color", "EGUI_COLOR_WHITE")
        elem.set("alpha", "EGUI_ALPHA_100")

        # Alignment from Tailwind
        align = self._parse_alignment(cls)
        if align:
            elem.set("align_type", align)

        # Attach animations
        self._attach_animations(elem, label_id)

        return elem

    def _make_group_or_card(self, x, y, w, h, bg_color, radius, cls, tag):
        """Create a Group or Card XML element."""
        prefix = "card" if (bg_color and radius > 0) else "group"
        elem_id = self._unique_id(prefix)

        if bg_color and radius > 0:
            elem = ET.Element("Card")
            elem.set("corner_radius", str(radius))
        else:
            elem = ET.Element("Group")

        elem.set("id", elem_id)
        elem.set("x", str(x))
        elem.set("y", str(y))
        elem.set("width", str(w))
        elem.set("height", str(h))

        # Store classes for child layout computation
        elem.set("_classes", cls)

        if bg_color:
            bg = ET.SubElement(elem, "Background")
            if radius > 0:
                bg.set("type", "round_rectangle")
                bg.set("radius", str(radius))
            else:
                bg.set("type", "solid")
            bg.set("color", f"EGUI_COLOR_HEX(0x{bg_color})")
            bg.set("alpha", "EGUI_ALPHA_100")

            # Check for border
            border_color = _parse_cls_color(cls, "border")
            if border_color:
                bg.set("stroke_width", "1")
                bg.set("stroke_color", f"EGUI_COLOR_HEX(0x{border_color})")
                bg.set("stroke_alpha", "EGUI_ALPHA_30")

        # Attach animations
        self._attach_animations(elem, elem_id)

        return elem

    def _attach_animations(self, elem, widget_id):
        """Attach animation XML elements to a widget."""
        anims = self._animations.get(widget_id, [])
        for anim in anims:
            anim_elem = ET.SubElement(elem, "Animation")
            anim_elem.set("type", anim["type"])
            anim_elem.set("duration", str(anim.get("duration", 500)))
            anim_elem.set("interpolator", anim.get("interpolator", "decelerate"))

            delay = anim.get("delay", 0)
            if delay > 0:
                anim_elem.set("start_delay", str(delay))

            anim_elem.set("auto_start", "true")

            # Type-specific params
            params = anim.get("params", {})
            for k, v in params.items():
                anim_elem.set(k, str(v))

    def _parse_alignment(self, cls):
        """Parse Tailwind alignment classes to EGUI align_type."""
        parts = []
        if "text-center" in cls:
            parts.append("EGUI_ALIGN_HCENTER")
        elif "text-right" in cls:
            parts.append("EGUI_ALIGN_RIGHT")
        else:
            parts.append("EGUI_ALIGN_LEFT")

        if "items-center" in cls:
            parts.append("EGUI_ALIGN_VCENTER")

        return " | ".join(parts) if parts else None

    def cleanup_xml(self, root):
        """Remove internal attributes like _classes before serialization."""
        for elem in root.iter():
            if "_classes" in elem.attrib:
                del elem.attrib["_classes"]


# ── Main orchestrator ────────────────────────────────────────────

class FigmaMakeCodegen:
    """Full Figma Make → EGUI code generation pipeline."""

    def __init__(self, app_name, width=320, height=240):
        self.app_name = app_name
        self.width = width
        self.height = height

    def run(self, project_dir, skip_c_gen=False):
        """Run the full codegen pipeline.

        Returns dict with generated file paths.
        """
        egui_root = _find_egui_root()
        app_dir = os.path.join(egui_root, "example", self.app_name)

        # 1. Discover project structure
        print(f"[1/5] Discovering Figma Make project: {project_dir}")
        discovery = _discover_figmamake_pages(project_dir)
        if not discovery["pages"]:
            print("ERROR: No pages found.", file=sys.stderr)
            sys.exit(1)

        root_bg = discovery.get("root_bg_color", "060608")
        print(f"  Found {len(discovery['pages'])} pages, "
              f"bg=#{root_bg}")

        # 2. Extract animations
        print(f"[2/5] Extracting animations...")
        anim_extractor = AnimExtractor()
        anim_result = anim_extractor.extract_all(project_dir)
        total_anims = sum(
            len(p["animations"]) for p in anim_result["pages"]
        )
        print(f"  Found {total_anims} animations")

        if anim_result["has_extensions_needed"]:
            ext_count = len(anim_result["extensions_needed"])
            print(f"  WARNING: {ext_count} animations need framework extensions")

        # 3. Generate XML layout files
        print(f"[3/5] Generating XML layouts...")
        os.makedirs(os.path.join(app_dir, ".eguiproject", "layout"),
                    exist_ok=True)
        os.makedirs(os.path.join(app_dir, ".eguiproject", "resources", "images"),
                    exist_ok=True)
        os.makedirs(os.path.join(app_dir, "resource", "src"), exist_ok=True)

        page_names = []
        for page_info in discovery["pages"]:
            page_file = page_info["file"]
            if not os.path.isfile(page_file):
                continue

            page_name = re.sub(
                r'(?<!^)(?=[A-Z])', '_', page_info["name"]
            ).lower()
            page_names.append(page_name)

            with open(page_file, "r", encoding="utf-8") as f:
                tsx_content = f.read()

            # Parse TSX to node tree
            lucide_imports = _extract_lucide_imports(tsx_content)
            jsx_text = _extract_jsx_return(tsx_content, page_info["name"])
            if not jsx_text:
                print(f"  WARNING: No JSX in {page_file}", file=sys.stderr)
                continue

            jsx_text, _ = _extract_jsx_comments(jsx_text)
            pseudo_html = _jsx_to_pseudo_html(jsx_text, lucide_imports)

            parser = _NodeParser()
            parser.feed(pseudo_html)
            root_node = parser._root
            if not root_node and parser._stack:
                root_node = parser._stack[0]
            if not root_node:
                print(f"  WARNING: Empty parse for {page_name}",
                      file=sys.stderr)
                continue

            # Build XML
            gen = XMLLayoutGenerator(self.width, self.height, root_bg)

            # Attach page animations
            page_anims = [
                p for p in anim_result["pages"]
                if p["page"] == page_name
            ]
            if page_anims:
                gen.set_animations(page_anims[0]["animations"])

            xml_root = gen.generate_page_xml(root_node, page_name)
            gen.cleanup_xml(xml_root)

            # Write XML
            xml_path = os.path.join(
                app_dir, ".eguiproject", "layout", f"{page_name}.xml"
            )
            tree = ET.ElementTree(xml_root)
            with open(xml_path, "w", encoding="utf-8", newline="\n") as f:
                f.write('<?xml version="1.0" encoding="utf-8"?>\n')
                tree.write(f, encoding="unicode", xml_declaration=False)
            print(f"  Generated: {page_name}.xml")

        if not page_names:
            print("ERROR: No pages generated.", file=sys.stderr)
            sys.exit(1)

        # 4. Create .egui project file
        print(f"[4/5] Creating project file...")
        egui_root_rel = os.path.relpath(egui_root, app_dir).replace("\\", "/")
        project_xml = _build_egui_project_xml(
            app_name=self.app_name,
            width=self.width,
            height=self.height,
            egui_root=egui_root_rel,
            pages=page_names,
        )
        egui_file = os.path.join(app_dir, f"{self.app_name}.egui")
        with open(egui_file, "w", encoding="utf-8", newline="\n") as f:
            f.write(project_xml)
        print(f"  Created: {self.app_name}.egui")

        # Write resources.xml
        res_xml_path = os.path.join(
            app_dir, ".eguiproject", "resources", "resources.xml"
        )
        if not os.path.exists(res_xml_path):
            with open(res_xml_path, "w", encoding="utf-8", newline="\n") as f:
                f.write('<?xml version="1.0" encoding="utf-8"?>\n')
                f.write("<Resources>\n    <Images />\n    <Fonts />\n</Resources>\n")

        # 5. Generate C code
        if skip_c_gen:
            print("[5/5] Skipping C code generation (--skip-c-gen)")
        else:
            print(f"[5/5] Generating C code...")
            self._generate_c_code(egui_root, app_dir)

        print(f"\nDone! App directory: {app_dir}")
        return {
            "app_dir": app_dir,
            "pages": page_names,
            "animations": total_anims,
            "extensions_needed": anim_result.get("extensions_needed", []),
        }

    def _generate_c_code(self, egui_root, app_dir):
        """Invoke the existing code generator."""
        ui_designer_dir = os.path.join(egui_root, "scripts")
        if ui_designer_dir not in sys.path:
            sys.path.insert(0, ui_designer_dir)

        from ui_designer.model.project import Project
        from ui_designer.generator.code_generator import generate_all_files_preserved

        project = Project.load(app_dir)
        print(f"  Loaded: {len(project.pages)} pages")

        files = generate_all_files_preserved(project, app_dir, backup=False)
        for filename, content in files.items():
            filepath = os.path.join(app_dir, filename)
            os.makedirs(os.path.dirname(filepath), exist_ok=True)
            with open(filepath, "w", encoding="utf-8", newline="\n") as f:
                f.write(content)

        print(f"  Generated {len(files)} C files")


def main():
    parser = argparse.ArgumentParser(
        description="Generate EGUI code from Figma Make TSX project"
    )
    parser.add_argument("--project-dir", required=True,
                        help="Figma Make project directory")
    parser.add_argument("--app", required=True, help="EGUI app name")
    parser.add_argument("--width", type=int, default=320)
    parser.add_argument("--height", type=int, default=240)
    parser.add_argument("--skip-c-gen", action="store_true",
                        help="Skip C code generation (XML only)")
    args = parser.parse_args()

    codegen = FigmaMakeCodegen(args.app, args.width, args.height)
    result = codegen.run(args.project_dir, skip_c_gen=args.skip_c_gen)

    if result.get("extensions_needed"):
        print(f"\nWARNING: {len(result['extensions_needed'])} animations "
              "need framework extensions before full fidelity.")
        sys.exit(2)


if __name__ == "__main__":
    main()

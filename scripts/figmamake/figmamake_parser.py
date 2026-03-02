#!/usr/bin/env python3
"""Stage 2a: TSX semantic parser for Figma Make projects.

Wraps the existing figmamake-extract logic from html2egui_helper.py
and produces a layout_description.json suitable for the EGUI code generation pipeline.

Usage:
    python scripts/figmamake/figmamake_parser.py \
        --project-dir /path/to/figmamake/project \
        --output layout_description.json \
        --app-name HelloBattery \
        --width 320 --height 240
"""

import argparse
import json
import os
import re
import sys
from pathlib import Path

# Add scripts dir to path for imports
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
    _extract_figmamake_text_chars,
)


# Tailwind size class -> pixel value
_TW_SPACING = {
    "0": 0, "0.5": 2, "1": 4, "1.5": 6, "2": 8, "2.5": 10,
    "3": 12, "3.5": 14, "4": 16, "5": 20, "6": 24, "7": 28,
    "8": 32, "9": 36, "10": 40, "11": 44, "12": 48,
    "14": 56, "16": 64, "20": 80, "24": 96, "28": 112,
    "32": 128, "36": 144, "40": 160, "44": 176, "48": 192,
}

# Tailwind text size -> pixel size
_TW_TEXT_SIZE = {
    "xs": 10, "sm": 12, "base": 14, "lg": 16, "xl": 20,
    "2xl": 24, "3xl": 28, "4xl": 32, "5xl": 40, "6xl": 48,
}

# Tailwind rounded -> corner radius
_TW_ROUNDED = {
    "none": 0, "sm": 2, "": 4, "md": 6, "lg": 8,
    "xl": 12, "2xl": 16, "3xl": 24, "full": 9999,
}


def _parse_tw_color(cls_str: str, prefix: str = "text") -> str | None:
    """Extract hex color from Tailwind class like text-[#00f3ff] or bg-[#060608]."""
    m = re.search(rf'{prefix}-\[#([0-9a-fA-F]{{6}})\]', cls_str)
    if m:
        return m.group(1).upper()
    return None


def _parse_tw_size(cls_str: str, prefix: str = "w") -> int | None:
    """Extract pixel size from Tailwind class like w-[304px] or w-8."""
    # Arbitrary value: w-[304px]
    m = re.search(rf'{prefix}-\[(\d+)px\]', cls_str)
    if m:
        return int(m.group(1))
    # Standard spacing: w-8
    m = re.search(rf'{prefix}-(\d+(?:\.\d+)?)', cls_str)
    if m and m.group(1) in _TW_SPACING:
        return _TW_SPACING[m.group(1)]
    # Full: w-full
    if re.search(rf'{prefix}-full', cls_str):
        return -1  # Sentinel for "full width/height"
    return None


def _parse_tw_text_size(cls_str: str) -> int | None:
    """Extract font pixel size from Tailwind text-xs/sm/base/lg/xl or text-[14px]."""
    m = re.search(r'text-\[(\d+)px\]', cls_str)
    if m:
        return int(m.group(1))
    for name, px in _TW_TEXT_SIZE.items():
        if re.search(rf'\btext-{re.escape(name)}\b', cls_str):
            return px
    return None


def _parse_tw_rounded(cls_str: str) -> int:
    """Extract corner radius from Tailwind rounded-* class."""
    m = re.search(r'rounded-(\w+)', cls_str)
    if m:
        return _TW_ROUNDED.get(m.group(1), 4)
    if "rounded" in cls_str.split():
        return 4
    return 0


def _parse_tw_gap(cls_str: str) -> int:
    """Extract gap value from Tailwind gap-N class."""
    m = re.search(r'gap-(\d+(?:\.\d+)?)', cls_str)
    if m and m.group(1) in _TW_SPACING:
        return _TW_SPACING[m.group(1)]
    m = re.search(r'gap-\[(\d+)px\]', cls_str)
    if m:
        return int(m.group(1))
    return 0


def _parse_tw_padding(cls_str: str) -> dict:
    """Extract padding from Tailwind p-N, px-N, py-N, pt-N etc."""
    result = {"top": 0, "right": 0, "bottom": 0, "left": 0}
    # p-N (all sides)
    m = re.search(r'\bp-(\d+(?:\.\d+)?)\b', cls_str)
    if m and m.group(1) in _TW_SPACING:
        val = _TW_SPACING[m.group(1)]
        result = {"top": val, "right": val, "bottom": val, "left": val}
    # px-N (horizontal)
    m = re.search(r'\bpx-(\d+(?:\.\d+)?)\b', cls_str)
    if m and m.group(1) in _TW_SPACING:
        val = _TW_SPACING[m.group(1)]
        result["left"] = val
        result["right"] = val
    # py-N (vertical)
    m = re.search(r'\bpy-(\d+(?:\.\d+)?)\b', cls_str)
    if m and m.group(1) in _TW_SPACING:
        val = _TW_SPACING[m.group(1)]
        result["top"] = val
        result["bottom"] = val
    return result


def _is_flex_col(cls_str: str) -> bool:
    return "flex-col" in cls_str


def _is_flex_row(cls_str: str) -> bool:
    return "flex" in cls_str and "flex-col" not in cls_str


class FigmaMakeParser:
    """Parse Figma Make TSX project into layout_description.json."""

    def __init__(self, app_name: str, width: int = 320, height: int = 240):
        self.app_name = app_name
        self.width = width
        self.height = height

    def parse_project(self, project_dir: str) -> dict:
        """Parse entire Figma Make project, return layout description."""
        discovery = _discover_figmamake_pages(project_dir)

        if not discovery["pages"]:
            raise ValueError(f"No page components found in {project_dir}")

        # Override screen size if specified
        screen_w = self.width or discovery["screen"]["width"]
        screen_h = self.height or discovery["screen"]["height"]
        root_bg = discovery.get("root_bg_color", "060608")

        pages = []
        all_icons = []
        all_texts = set()
        seen_icons = set()

        for page_info in discovery["pages"]:
            page_file = page_info["file"]
            if not os.path.isfile(page_file):
                continue

            with open(page_file, "r", encoding="utf-8") as f:
                tsx_content = f.read()

            # Extract icons
            lucide_imports = _extract_lucide_imports(tsx_content)
            for local_name, lucide_name in lucide_imports.items():
                material_name, _ = _lucide_name_to_material(lucide_name)
                if material_name not in seen_icons:
                    seen_icons.add(material_name)
                    all_icons.append({
                        "lucide": lucide_name,
                        "material": material_name,
                    })

            # Extract text characters
            all_texts.update(_extract_figmamake_text_chars(tsx_content))

            # Convert page name to snake_case
            page_name = re.sub(r'(?<!^)(?=[A-Z])', '_', page_info["name"]).lower()

            pages.append({
                "name": page_name,
                "component": page_info["name"],
                "route": page_info["path"],
                "file": page_info["file"],
                "icons": [ic["material"] for ic in all_icons if ic["lucide"] in lucide_imports.values()],
            })

        result = {
            "app_name": self.app_name,
            "source": "figma-make",
            "screen": {"width": screen_w, "height": screen_h},
            "root_bg_color": root_bg,
            "pages": pages,
            "icons": all_icons,
            "text_chars": "".join(sorted(all_texts)),
        }

        return result


def main():
    parser = argparse.ArgumentParser(description="Parse Figma Make TSX project")
    parser.add_argument("--project-dir", required=True, help="Figma Make project directory")
    parser.add_argument("--output", default=None, help="Output JSON file path")
    parser.add_argument("--app-name", default="HelloApp", help="EGUI app name")
    parser.add_argument("--width", type=int, default=320, help="Screen width")
    parser.add_argument("--height", type=int, default=240, help="Screen height")
    args = parser.parse_args()

    p = FigmaMakeParser(args.app_name, args.width, args.height)
    result = p.parse_project(args.project_dir)

    output_json = json.dumps(result, indent=2, ensure_ascii=False)
    if args.output:
        Path(args.output).parent.mkdir(parents=True, exist_ok=True)
        with open(args.output, "w", encoding="utf-8", newline="\n") as f:
            f.write(output_json)
        print(f"Layout description written to: {args.output}", file=sys.stderr)
    else:
        print(output_json)


if __name__ == "__main__":
    main()

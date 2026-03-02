#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""html2egui_helper.py — Helper script for HTML-to-EmbeddedGUI conversion.

Provides sub-commands:
    scaffold        Create UI Designer project structure (.egui + XML layout)
    export-icons    Extract Material Symbols from HTML, render as PNG
    export-svgs     Extract inline SVGs from HTML, convert to PNG (via CairoSVG)
    extract-text    Extract unique characters from HTML for font generation
    gen-resource    Run resource generation (wraps make resource)
    extract-layout  Parse HTML and output structured JSON layout analysis
    generate-code   Generate C code from UI Designer XML project
    verify          Build + runtime check in one step
    figma2xml       Convert Figma design to EGUI XML layout via REST API

Usage:
    python scripts/html2egui_helper.py scaffold --app HelloSmartHome --width 320 --height 480
    python scripts/html2egui_helper.py export-icons --input code.html --output example/HelloSmartHome/.eguiproject/resources/images/ --size 24 --auto-color
    python scripts/html2egui_helper.py gen-resource --app HelloSmartHome
    python scripts/html2egui_helper.py extract-layout --input code.html
    python scripts/html2egui_helper.py generate-code --app HelloSmartHome
    python scripts/html2egui_helper.py verify --app HelloSmartHome
"""

import argparse
from html.parser import HTMLParser
import json
import os
import re
import subprocess
import sys


# ── Path helpers ──────────────────────────────────────────────────

def _find_egui_root():
    """Find EmbeddedGUI root directory (where Makefile and src/ live)."""
    # Try relative to this script: scripts/html2egui_helper.py -> root
    script_dir = os.path.dirname(os.path.abspath(__file__))
    candidate = os.path.dirname(script_dir)
    if os.path.isfile(os.path.join(candidate, "Makefile")) and os.path.isdir(os.path.join(candidate, "src")):
        return candidate
    # Try CWD
    if os.path.isfile("Makefile") and os.path.isdir("src"):
        return os.path.abspath(".")
    print("ERROR: Cannot find EmbeddedGUI root directory. Run from project root or scripts/ dir.")
    sys.exit(1)


# ── Sub-command: scaffold ─────────────────────────────────────────

APP_EGUI_CONFIG_TEMPLATE = """\
#ifndef _APP_EGUI_CONFIG_H_
#define _APP_EGUI_CONFIG_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {{
#endif

#define EGUI_CONFIG_SCEEN_WIDTH  {width}
#define EGUI_CONFIG_SCEEN_HEIGHT {height}

#define EGUI_CONFIG_PFB_WIDTH  ({width} / 8)
#define EGUI_CONFIG_PFB_HEIGHT ({height} / 8)

#define EGUI_CONFIG_COLOR_DEPTH {color_depth}

#define EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE {circle_radius}

#define EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW 1

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}}
#endif

#endif /* _APP_EGUI_CONFIG_H_ */
"""

BUILD_MK_TEMPLATE = """\
EGUI_CODE_SRC\t\t+= $(EGUI_APP_PATH)
EGUI_CODE_SRC\t\t+= $(EGUI_APP_PATH)/resource
EGUI_CODE_SRC\t\t+= $(EGUI_APP_PATH)/resource/img
EGUI_CODE_SRC\t\t+= $(EGUI_APP_PATH)/resource/font

EGUI_CODE_INCLUDE\t+= $(EGUI_APP_PATH)
EGUI_CODE_INCLUDE\t+= $(EGUI_APP_PATH)/resource
EGUI_CODE_INCLUDE\t+= $(EGUI_APP_PATH)/resource/img
EGUI_CODE_INCLUDE\t+= $(EGUI_APP_PATH)/resource/font
"""

# UI Designer project file template (use _build_egui_project_xml for multi-page)
EGUI_PROJECT_TEMPLATE = """\
<?xml version="1.0" encoding="utf-8"?>
<Project app_name="{app_name}" screen_width="{width}" screen_height="{height}" page_mode="easy_page" startup="{startup}" egui_root="{egui_root}">
    <Pages>
{page_refs}
    </Pages>
    <Resources catalog="resources.xml" />
</Project>
"""


def _build_egui_project_xml(app_name, width, height, egui_root, pages=None):
    """Build .egui project XML with multiple page refs."""
    if pages is None:
        pages = ["main_page"]
    page_refs = "\n".join(
        f'        <PageRef file="layout/{p}.xml" />' for p in pages
    )
    return EGUI_PROJECT_TEMPLATE.format(
        app_name=app_name, width=width, height=height,
        startup=pages[0], egui_root=egui_root, page_refs=page_refs
    )

# Empty page layout template
PAGE_XML_TEMPLATE = """\
<?xml version="1.0" encoding="utf-8"?>
<Page>
    <Group id="root" x="0" y="0" width="{width}" height="{height}" />
</Page>
"""

# Resource catalog template
RESOURCES_XML_TEMPLATE = """\
<?xml version="1.0" encoding="utf-8"?>
<Resources>
    <Images />
</Resources>
"""

RESOURCE_CONFIG_TEMPLATE = """\
{
    "img": [
    ],
    "font": [
    ]
}
"""


def cmd_scaffold(args):
    """Create UI Designer project directory structure and template files."""
    root = _find_egui_root()
    app_dir = os.path.join(root, "example", args.app)

    if os.path.exists(app_dir) and not args.force:
        print(f"ERROR: Directory already exists: {app_dir}")
        print("Use --force to overwrite template files (layout XML is preserved).")
        sys.exit(1)

    width = args.width
    height = args.height
    color_depth = args.color_depth
    # Circle radius support: half of the smaller dimension
    circle_radius = min(width, height) // 2

    # Compute relative egui_root path from app_dir
    egui_root = os.path.relpath(root, app_dir).replace("\\", "/")

    # Create directories
    config_dir = os.path.join(app_dir, ".eguiproject")
    dirs = [
        app_dir,
        os.path.join(config_dir, "layout"),
        os.path.join(config_dir, "resources", "images"),
        os.path.join(app_dir, "resource"),
        os.path.join(app_dir, "resource", "src"),
        os.path.join(app_dir, "resource", "img"),
        os.path.join(app_dir, "resource", "font"),
    ]
    for d in dirs:
        os.makedirs(d, exist_ok=True)

    # Write template files (always overwrite config/build)
    _write_file(os.path.join(app_dir, "app_egui_config.h"),
                APP_EGUI_CONFIG_TEMPLATE.format(
                    width=width, height=height,
                    color_depth=color_depth,
                    circle_radius=circle_radius))
    _write_file(os.path.join(app_dir, "build.mk"), BUILD_MK_TEMPLATE)

    # .egui project file (always overwrite)
    pages = [p.strip() for p in args.pages.split(",")] if args.pages else ["main_page"]
    _write_file(os.path.join(app_dir, f"{args.app}.egui"),
                _build_egui_project_xml(
                    app_name=args.app, width=width, height=height,
                    egui_root=egui_root, pages=pages))

    # resources.xml (always overwrite)
    _write_file(os.path.join(config_dir, "resources", "resources.xml"),
                RESOURCES_XML_TEMPLATE)

    # Page XML templates — only create if they do not exist (user/AI-owned)
    for page_name in pages:
        page_xml_path = os.path.join(config_dir, "layout", f"{page_name}.xml")
        if not os.path.exists(page_xml_path):
            _write_file(page_xml_path,
                         PAGE_XML_TEMPLATE.format(width=width, height=height))
            print(f"  Created: {page_name}.xml (empty template)")
        else:
            print(f"  Skipped: {page_name}.xml (already exists)")

    # Resource config — only create if it does not exist
    rc_path = os.path.join(app_dir, "resource", "src", "app_resource_config.json")
    if not os.path.exists(rc_path):
        _write_file(rc_path, RESOURCE_CONFIG_TEMPLATE)
        print(f"  Created: resource/src/app_resource_config.json")
    else:
        print(f"  Skipped: resource/src/app_resource_config.json (already exists)")

    print(f"\nScaffold complete: {app_dir}")
    print(f"  Screen: {width}x{height}, color depth: {color_depth}")
    print(f"\nNext steps:")
    print(f"  1. Extract layout:   python scripts/html2egui_helper.py extract-layout --input <html>")
    print(f"  2. Export icons:     python scripts/html2egui_helper.py export-icons --input <html> --app {args.app}")
    print(f"  3. Write XML layout: Edit .eguiproject/layout/main_page.xml")
    print(f"  4. Generate code:    python scripts/html2egui_helper.py generate-code --app {args.app}")
    print(f"  5. Gen resources:    python scripts/html2egui_helper.py gen-resource --app {args.app}")
    print(f"  6. Build & verify:   python scripts/html2egui_helper.py verify --app {args.app}")


def _write_file(path, content):
    """Write content to file, creating parent dirs if needed."""
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(content)
    print(f"  Created: {os.path.basename(path)}")


# ── Sub-command: export-icons ─────────────────────────────────────

def _extract_material_icons(html_path):
    """Extract Material Symbols icon names from HTML file.

    Looks for: <span class="material-symbols-outlined ...">icon_name</span>
    Returns a sorted list of unique icon names.
    """
    with open(html_path, "r", encoding="utf-8") as f:
        html = f.read()

    # Pattern: <span class="material-symbols-outlined ...">TEXT</span>
    pattern = r'<span\s+class\s*=\s*"[^"]*material-symbols-outlined[^"]*"\s*>([^<]+)</span>'
    matches = re.findall(pattern, html, re.IGNORECASE)

    icons = sorted(set(name.strip() for name in matches if name.strip()))
    return icons


# Standard Tailwind color name -> hex mapping
_TAILWIND_COLORS = {
    "white": "FFFFFF", "black": "000000",
    # Slate
    "slate-50": "F8FAFC", "slate-100": "F1F5F9", "slate-200": "E2E8F0", "slate-300": "CBD5E1",
    "slate-400": "94A3B8", "slate-500": "64748B", "slate-600": "475569", "slate-700": "334155",
    "slate-800": "1E293B", "slate-900": "0F172A",
    # Gray
    "gray-50": "F9FAFB", "gray-100": "F3F4F6", "gray-200": "E5E7EB", "gray-300": "D1D5DB",
    "gray-400": "9CA3AF", "gray-500": "6B7280", "gray-600": "4B5563", "gray-700": "374151",
    "gray-800": "1F2937", "gray-900": "111827", "gray-950": "030712",
    # Red
    "red-400": "F87171", "red-500": "EF4444", "red-600": "DC2626",
    # Orange
    "orange-400": "FB923C", "orange-500": "F97316", "orange-600": "EA580C",
    # Amber
    "amber-500": "F59E0B",
    # Yellow
    "yellow-400": "FACC15", "yellow-500": "EAB308", "yellow-600": "CA8A04",
    # Green
    "green-400": "4ADE80", "green-500": "22C55E", "green-600": "16A34A",
    # Emerald / Teal
    "emerald-500": "10B981", "teal-500": "14B8A6",
    # Cyan
    "cyan-400": "22D3EE", "cyan-500": "06B6D4", "cyan-600": "0891B2",
    # Sky / Blue
    "sky-500": "0EA5E9",
    "blue-400": "60A5FA", "blue-500": "3B82F6", "blue-600": "2563EB",
    # Indigo / Violet / Purple
    "indigo-500": "6366F1", "violet-500": "8B5CF6",
    "purple-400": "C084FC", "purple-500": "A855F7", "purple-600": "9333EA",
    # Fuchsia / Pink / Rose
    "fuchsia-500": "D946EF", "pink-500": "EC4899", "rose-500": "F43F5E",
}


def _extract_material_icons_with_colors(html_path):
    """Extract Material Symbols icon names and their colors from HTML.

    Parses Tailwind color classes on icon spans and their parent elements.
    Returns a dict: {icon_name: color_hex_without_hash}.
    """
    with open(html_path, "r", encoding="utf-8") as f:
        html = f.read()

    # Extract custom colors from tailwind config
    custom_colors = {}
    config_match = re.search(r'tailwind\.config\s*=\s*\{.*?\}', html, re.DOTALL)
    if config_match:
        config_text = config_match.group()
        # Find color definitions: "name": "#hex"
        color_defs = re.findall(r'"([^"]+)"\s*:\s*"#([0-9a-fA-F]{6})"', config_text)
        for name, hex_val in color_defs:
            custom_colors[name] = hex_val.upper()

    def resolve_color(class_str):
        """Extract color hex from Tailwind class string."""
        # Check text-{color} classes
        text_colors = re.findall(r'text-(\S+?)(?:\s|")', class_str + ' ')
        for tc in text_colors:
            # Skip size classes
            if tc in ('xs', 'sm', 'base', 'lg', 'xl', '2xl', '3xl', '4xl', '5xl',
                       'center', 'left', 'right', 'justify'):
                continue
            if tc.startswith('[') and tc.endswith(']'):
                # text-[14px] etc - skip size
                continue
            # Check standard Tailwind colors
            if tc in _TAILWIND_COLORS:
                return _TAILWIND_COLORS[tc]
            # Check custom colors from tailwind config
            if tc in custom_colors:
                return custom_colors[tc]
            # Check with alpha suffix like white/70 -> just use the base color
            base = tc.split('/')[0]
            if base in _TAILWIND_COLORS:
                return _TAILWIND_COLORS[base]
            if base in custom_colors:
                return custom_colors[base]
        return None

    # Find all icon spans with their full class string and parent context
    # Pattern captures: everything before the span (to find parent color), the span's classes, icon name
    icon_colors = {}
    # Match icon spans
    icon_pattern = r'<span\s+class\s*=\s*"([^"]*material-symbols-outlined[^"]*)"\s*>([^<]+)</span>'
    for match in re.finditer(icon_pattern, html, re.IGNORECASE):
        classes = match.group(1)
        icon_name = match.group(2).strip()
        if not icon_name:
            continue

        # Try to get color from the icon's own classes
        color = resolve_color(classes)

        if color is None:
            # Look at parent element for color
            # Find the closest parent with a text-* color class
            pos = match.start()
            # Search backwards for parent elements with color
            preceding = html[max(0, pos - 500):pos]
            # Find last opening tag with text-color
            parent_tags = re.findall(r'<(?:div|a|span)\s+class\s*=\s*"([^"]*text-[^"]*)"', preceding)
            if parent_tags:
                # Use the most recent (closest) parent
                parent_color = resolve_color(parent_tags[-1])
                if parent_color:
                    color = parent_color

        if color is None:
            color = "FFFFFF"  # Default: white (dark mode text)

        # Detect filled style
        is_filled = 'filled-icon' in classes or 'filled' in classes

        # If same icon appears with different colors, use first occurrence
        if icon_name not in icon_colors:
            icon_colors[icon_name] = {"color": color, "filled": is_filled}

    return icon_colors


def _render_icon_pillow(icon_name, size, color_hex, output_path, font_path=None, filled=False):
    """Render a Material Symbols icon to PNG using Pillow + TTF font.

    Requires MaterialSymbolsOutlined-Regular.ttf in scripts/tools/ or specified path.
    Args:
        filled: If True, set FILL=1 variation axis for solid/filled icon style.
    """
    try:
        from PIL import Image, ImageDraw, ImageFont
    except ImportError:
        print("ERROR: Pillow is required. Install: pip install Pillow")
        return False

    if font_path is None:
        # Try to find the font file
        script_dir = os.path.dirname(os.path.abspath(__file__))
        candidates = [
            os.path.join(script_dir, "tools", "MaterialSymbolsOutlined-Regular.ttf"),
            os.path.join(script_dir, "tools", "MaterialSymbolsOutlined.ttf"),
            os.path.join(script_dir, "tools", "material-symbols-outlined.ttf"),
        ]
        for c in candidates:
            if os.path.isfile(c):
                font_path = c
                break

    if font_path is None or not os.path.isfile(font_path):
        print(f"  WARNING: Material Symbols TTF not found. Generating placeholder for '{icon_name}'.")
        return _render_icon_placeholder(icon_name, size, color_hex, output_path)

    # Parse color from hex string
    r = int(color_hex[0:2], 16)
    g = int(color_hex[2:4], 16)
    b = int(color_hex[4:6], 16)

    # Material Symbols uses codepoints. Map icon name to unicode.
    codepoint = _material_icon_codepoint(icon_name)

    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    try:
        font = ImageFont.truetype(font_path, size - 4)
        # Set variable font axes: Fill, Grade, Optical Size, Weight
        try:
            fill_val = 1.0 if filled else 0.0
            font.set_variation_by_axes([fill_val, 0.0, 24.0, 400.0])
        except Exception:
            pass  # Not a variable font or axes not supported
    except Exception:
        return _render_icon_placeholder(icon_name, size, color_hex, output_path)

    char = chr(codepoint) if codepoint else "?"

    # Center the glyph
    bbox = draw.textbbox((0, 0), char, font=font)
    tw = bbox[2] - bbox[0]
    th = bbox[3] - bbox[1]
    x = (size - tw) // 2 - bbox[0]
    y = (size - th) // 2 - bbox[1]

    draw.text((x, y), char, fill=(r, g, b, 255), font=font)

    img.save(output_path, "PNG")
    return True


def _render_icon_placeholder(icon_name, size, color_hex, output_path):
    """Generate a simple placeholder PNG (colored square with letter)."""
    try:
        from PIL import Image, ImageDraw, ImageFont
    except ImportError:
        return False

    r = int(color_hex[0:2], 16)
    g = int(color_hex[2:4], 16)
    b = int(color_hex[4:6], 16)

    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # Draw a simple circle with the first letter
    margin = 2
    draw.ellipse([margin, margin, size - margin, size - margin],
                 outline=(r, g, b, 200), width=2)

    # First letter of icon name
    letter = icon_name[0].upper() if icon_name else "?"
    try:
        font = ImageFont.truetype("arial.ttf", size // 2)
    except Exception:
        font = ImageFont.load_default()

    bbox = draw.textbbox((0, 0), letter, font=font)
    tw = bbox[2] - bbox[0]
    th = bbox[3] - bbox[1]
    x = (size - tw) // 2 - bbox[0]
    y = (size - th) // 2 - bbox[1]
    draw.text((x, y), letter, fill=(r, g, b, 255), font=font)

    img.save(output_path, "PNG")
    return True


# Material Symbols codepoint mapping (common icons)
# Full mapping: https://github.com/ArtifexSoftware/mupdf/blob/master/resources/fonts/urw/MaterialIcons-Regular.cmap
_MATERIAL_ICON_CODEPOINTS = {
    "wifi": 0xE63E,
    "battery_full": 0xE1A4,
    "person": 0xE7FD,
    "lightbulb": 0xE0F0,
    "ac_unit": 0xEB3B,
    "bolt": 0xEA0B,
    "shield_lock": 0xF684,
    "water_drop": 0xE798,
    "grid_view": 0xE9B0,
    "auto_awesome": 0xE65F,
    "analytics": 0xEF3E,
    "settings": 0xE8B8,
    "home": 0xE88A,
    "search": 0xE8B6,
    "menu": 0xE5D2,
    "close": 0xE5CD,
    "arrow_back": 0xE5C4,
    "arrow_forward": 0xE5C8,
    "check": 0xE5CA,
    "add": 0xE145,
    "remove": 0xE15B,
    "delete": 0xE872,
    "edit": 0xE3C9,
    "favorite": 0xE87D,
    "star": 0xE838,
    "visibility": 0xE8F4,
    "visibility_off": 0xE8F5,
    "lock": 0xE897,
    "notifications": 0xE7F4,
    "power_settings_new": 0xE8AC,
    "thermostat": 0xF076,
    "humidity_percentage": 0xF87E,
    "aq": 0xEA35,  # Use "eco" glyph as fallback (0xF55A renders as text in Pillow)
    "speed": 0xE9E4,
    "light_mode": 0xE518,
    "dark_mode": 0xE51C,
    "bluetooth": 0xE1A7,
    "signal_cellular_alt": 0xE202,
    "battery_charging_full": 0xE1A3,
    "battery_0_bar": 0xF0A5,
    "devices": 0xE1B1,
    "security": 0xE32A,
    "camera": 0xE3AF,
    "music_note": 0xE405,
    "volume_up": 0xE050,
    "play_arrow": 0xE037,
    "pause": 0xE034,
    "skip_next": 0xE044,
    "skip_previous": 0xE045,
    "refresh": 0xE5D5,
    "sync": 0xE627,
    "cloud": 0xE2BD,
    "download": 0xF090,
    "upload": 0xF09B,
    "info": 0xE88E,
    "warning": 0xE002,
    "error": 0xE000,
    "help": 0xE887,
    "schedule": 0xE8B5,
    "timer": 0xE425,
    "alarm": 0xE855,
}


def _material_icon_codepoint(icon_name):
    """Get codepoint for a Material Symbols icon name."""
    return _MATERIAL_ICON_CODEPOINTS.get(icon_name, None)


def _update_resource_config(config_path, icon_names, icon_size, image_format="rgb565", image_alpha="4", suffix=""):
    """Update app_resource_config.json with icon image entries."""
    if os.path.exists(config_path):
        with open(config_path, "r", encoding="utf-8") as f:
            content = f.read()
        # Try json5 first, fall back to json
        try:
            import json5
            config = json5.loads(content)
        except (ImportError, Exception):
            # Remove comments for plain json
            cleaned = re.sub(r'//.*?$', '', content, flags=re.MULTILINE)
            cleaned = re.sub(r'/\*.*?\*/', '', cleaned, flags=re.DOTALL)
            # Remove trailing commas
            cleaned = re.sub(r',\s*([}\]])', r'\1', cleaned)
            config = json.loads(cleaned)
    else:
        config = {"img": [], "font": []}

    if "img" not in config:
        config["img"] = []

    # Get existing icon files to avoid duplicates
    existing_files = {entry.get("file", "") for entry in config["img"]}

    added = 0
    for name in icon_names:
        filename = f"icon_{name}{suffix}.png"
        if filename not in existing_files:
            dim_str = f"{icon_size},{icon_size}"
            config["img"].append({
                "file": filename,
                "external": "0",
                "format": image_format,
                "alpha": image_alpha,
                "dim": dim_str,
            })
            added += 1

    with open(config_path, "w", encoding="utf-8", newline="\n") as f:
        json.dump(config, f, indent=4, ensure_ascii=False)

    print(f"  Updated resource config: {added} new icon(s) added, {len(config['img'])} total images")


def cmd_export_icons(args):
    """Extract Material Symbols from HTML, render as PNG, update resource config."""
    if not os.path.isfile(args.input):
        print(f"ERROR: HTML file not found: {args.input}")
        sys.exit(1)

    root = _find_egui_root()
    app_name = getattr(args, "app", None)

    # Determine output directory
    if args.output:
        output_dir = args.output
    elif app_name:
        # Default: .eguiproject/resources/images/ for designer workflow
        app_dir = os.path.join(root, "example", app_name)
        output_dir = os.path.join(app_dir, ".eguiproject", "resources", "images")
    else:
        print("ERROR: Must specify either --output or --app")
        sys.exit(1)

    os.makedirs(output_dir, exist_ok=True)

    auto_color = getattr(args, "auto_color", False)
    force_filled = getattr(args, "filled", False)

    if auto_color:
        # Extract icons with per-icon colors and filled state from HTML
        icon_info = _extract_material_icons_with_colors(args.input)
        icons = sorted(icon_info.keys())
        icon_colors = {name: info["color"] for name, info in icon_info.items()}
        icon_filled = {name: info["filled"] for name, info in icon_info.items()}
    else:
        # Extract icon names only, use single --color for all
        icons = _extract_material_icons(args.input)
        default_color = args.color.lstrip("#").upper()
        if len(default_color) < 6:
            default_color = default_color.ljust(6, "0")
        icon_colors = {name: default_color for name in icons}
        icon_filled = {name: False for name in icons}

    if not icons:
        print("No Material Symbols icons found in HTML.")
        return

    print(f"Found {len(icons)} unique icons: {', '.join(icons)}")
    if auto_color:
        for name in icons:
            filled_str = " [FILLED]" if icon_filled[name] or force_filled else ""
            print(f"  {name}: #{icon_colors[name]}{filled_str}")

    # Render each icon
    size = args.size
    suffix = args.suffix if hasattr(args, "suffix") and args.suffix else ""
    font_path = args.font if hasattr(args, "font") and args.font else None
    rendered = 0
    for name in icons:
        color = icon_colors[name]
        filled = force_filled or icon_filled.get(name, False)
        out_path = os.path.join(output_dir, f"icon_{name}{suffix}.png")
        ok = _render_icon_pillow(name, size, color, out_path, font_path, filled=filled)
        if ok:
            rendered += 1
            filled_tag = " FILLED" if filled else ""
            print(f"  Exported: icon_{name}{suffix}.png ({size}x{size}, #{color}{filled_tag})")
        else:
            print(f"  FAILED:   icon_{name}{suffix}.png")

    print(f"\nRendered {rendered}/{len(icons)} icons to {output_dir}")

    # Sync PNGs to resource/src/ if using --app (designer workflow)
    if app_name:
        app_dir = os.path.join(root, "example", app_name)
        src_dir = os.path.join(app_dir, "resource", "src")
        os.makedirs(src_dir, exist_ok=True)
        import shutil
        synced = 0
        for name in icons:
            fname = f"icon_{name}{suffix}.png"
            src_png = os.path.join(output_dir, fname)
            dst_png = os.path.join(src_dir, fname)
            if os.path.exists(src_png):
                shutil.copy2(src_png, dst_png)
                synced += 1
        print(f"  Synced {synced} icons to {src_dir}")
        config_path = os.path.join(src_dir, "app_resource_config.json")
    else:
        config_path = os.path.join(output_dir, "app_resource_config.json")

    # Update resource config
    if not os.path.exists(config_path):
        with open(config_path, "w", encoding="utf-8", newline="\n") as f:
            f.write(RESOURCE_CONFIG_TEMPLATE)
        print(f"  Created new: {config_path}")

    _update_resource_config(config_path, icons, size, image_format=getattr(args, "image_format", "alpha"), suffix=suffix)

    print(f"\nNext steps:")
    if app_name:
        print(f"  1. Write XML layout: Edit example/{app_name}/.eguiproject/layout/main_page.xml")
        print(f"  2. Generate code:    python scripts/html2egui_helper.py generate-code --app {app_name}")
        print(f"  3. Gen resources:    python scripts/html2egui_helper.py gen-resource --app {app_name}")
    else:
        print(f"  1. Run: python scripts/html2egui_helper.py gen-resource --app <AppName>")


# ── Sub-command: export-svgs ──────────────────────────────────────

def _extract_svgs_from_html(html_path):
    """Extract all inline SVG elements from HTML with contextual names.

    Returns a list of dicts:
        {"name": str, "svg": str, "index": int, "context": str}
    """
    with open(html_path, "r", encoding="utf-8") as f:
        html = f.read()

    results = []
    idx = 0
    search_start = 0

    while True:
        # Find next <svg
        svg_open = html.find("<svg", search_start)
        if svg_open == -1:
            break

        # Find matching </svg>  (handle nested svg if any)
        depth = 0
        pos = svg_open
        svg_close = -1
        while pos < len(html):
            next_open = html.find("<svg", pos + 1)
            next_close = html.find("</svg>", pos + 1)
            if next_close == -1:
                break
            if next_open != -1 and next_open < next_close:
                depth += 1
                pos = next_open
            else:
                if depth == 0:
                    svg_close = next_close + len("</svg>")
                    break
                depth -= 1
                pos = next_close

        if svg_close == -1:
            search_start = svg_open + 4
            continue

        svg_str = html[svg_open:svg_close]

        # Try to derive a name from surrounding context
        name = _derive_svg_name(html, svg_open, idx)

        results.append({
            "name": name,
            "svg": svg_str,
            "index": idx,
        })
        idx += 1
        search_start = svg_close

    return results


def _derive_svg_name(html, svg_pos, index):
    """Derive a meaningful name for an SVG from its surrounding HTML context."""
    preceding = html[max(0, svg_pos - 500):svg_pos]

    # Try HTML comment: <!-- Some Name -->
    comments = re.findall(r'<!--\s*(.+?)\s*-->', preceding)
    if comments:
        name = comments[-1].strip()
        # Sanitize to filename-safe string
        name = re.sub(r'[^a-zA-Z0-9_\- ]', '', name)
        name = re.sub(r'[\s\-]+', '_', name).strip('_').lower()
        if name:
            return name

    # Try aria-label or id on a parent
    labels = re.findall(r'(?:aria-label|id)\s*=\s*"([^"]+)"', preceding)
    if labels:
        name = labels[-1]
        name = re.sub(r'[^a-zA-Z0-9_\- ]', '', name)
        name = re.sub(r'[\s\-]+', '_', name).strip('_').lower()
        if name:
            return name

    # Fallback: svg_0, svg_1, ...
    return f"svg_{index}"


def _resolve_current_color(svg_str, html, svg_pos):
    """Replace 'currentColor' in SVG with actual color from CSS context."""
    if "currentColor" not in svg_str:
        return svg_str

    # Look at parent elements for text color
    preceding = html[max(0, svg_pos - 500):svg_pos]

    # Try Tailwind text-color classes
    color_hex = None
    text_colors = re.findall(r'text-(slate-\d+|white|black|primary|[a-z]+-\d+)', preceding)
    if text_colors:
        last = text_colors[-1]
        if last in _TAILWIND_COLORS:
            color_hex = "#" + _TAILWIND_COLORS[last]

    # Try inline style color
    if not color_hex:
        inline_colors = re.findall(r'color:\s*(#[0-9a-fA-F]{3,6})', preceding)
        if inline_colors:
            color_hex = inline_colors[-1]

    if not color_hex:
        color_hex = "#94A3B8"  # Default: slate-400

    return svg_str.replace("currentColor", color_hex)


def _ensure_svg_dimensions(svg_str, size):
    """Ensure SVG has explicit width/height attributes for CairoSVG."""
    # Add xmlns if missing
    if 'xmlns=' not in svg_str:
        svg_str = svg_str.replace('<svg ', '<svg xmlns="http://www.w3.org/2000/svg" ', 1)

    # Add or replace width/height (avoid matching stroke-width, etc.)
    svg_str = re.sub(r'(?<!-)\bwidth\s*=\s*"[^"]*"', '', svg_str, count=1)
    svg_str = re.sub(r'(?<!-)\bheight\s*=\s*"[^"]*"', '', svg_str, count=1)
    # Remove Tailwind classes that CairoSVG can't interpret
    svg_str = re.sub(r'\bclass\s*=\s*"[^"]*"', '', svg_str)

    # Insert width/height after <svg
    svg_str = svg_str.replace('<svg ', f'<svg width="{size}" height="{size}" ', 1)

    return svg_str


def _svg2png_cairosvg(svg_str, out_path, size):
    """Render SVG to PNG using CairoSVG. Returns True on success."""
    import cairosvg
    cairosvg.svg2png(
        bytestring=svg_str.encode("utf-8"),
        write_to=out_path,
        output_width=size,
        output_height=size,
        background_color="transparent",
    )
    return True


def _parse_svg_color(color_str):
    """Parse SVG color string to (R, G, B) tuple."""
    if not color_str or color_str in ("none", "transparent"):
        return None
    color_str = color_str.strip()
    if color_str.startswith("#"):
        h = color_str[1:]
        if len(h) == 3:
            h = h[0]*2 + h[1]*2 + h[2]*2
        if len(h) == 6:
            return (int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16))
    # Named colors (common subset)
    named = {"white": (255,255,255), "black": (0,0,0), "red": (255,0,0),
             "green": (0,128,0), "blue": (0,0,255), "gray": (128,128,128)}
    return named.get(color_str.lower())


def _svg2png_pillow(svg_str, out_path, size):
    """Render SVG to PNG using Pillow (basic SVG subset: circle, rect, line, ellipse, path).

    This is a fallback when CairoSVG/Cairo C library is not available.
    Handles common SVG elements found in UI design HTML exports.
    """
    import xml.etree.ElementTree as ET
    from PIL import Image, ImageDraw
    import math

    root = ET.fromstring(svg_str)
    ns = {'svg': 'http://www.w3.org/2000/svg'}

    # Parse viewBox for coordinate mapping
    vb = root.get('viewBox', root.get('viewbox', ''))
    if vb:
        parts = vb.replace(',', ' ').split()
        vx, vy, vw, vh = float(parts[0]), float(parts[1]), float(parts[2]), float(parts[3])
    else:
        vw = float(root.get('width', size))
        vh = float(root.get('height', size))
        vx, vy = 0, 0

    sx = size / vw
    sy = size / vh
    scale = min(sx, sy)
    ox = (size - vw * scale) / 2 - vx * scale
    oy = (size - vh * scale) / 2 - vy * scale

    def tx(x): return x * scale + ox
    def ty(y): return y * scale + oy

    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    def _find_all(tag):
        """Find elements with or without namespace."""
        found = root.findall(f'.//{tag}')
        found += root.findall(f'.//svg:{tag}', ns)
        return found

    # Render circles
    for el in _find_all('circle'):
        cx = float(el.get('cx', 0))
        cy = float(el.get('cy', 0))
        r = float(el.get('r', 0))
        sw = float(el.get('stroke-width', 1))
        stroke = el.get('stroke')
        fill = el.get('fill')

        bbox = [tx(cx - r), ty(cy - r), tx(cx + r), ty(cy + r)]
        fill_color = _parse_svg_color(fill)
        stroke_color = _parse_svg_color(stroke)

        if fill_color:
            draw.ellipse(bbox, fill=(*fill_color, 255))
        if stroke_color:
            # Handle stroke-dasharray for arc rendering
            da = el.get('stroke-dasharray')
            do = el.get('stroke-dashoffset', '0')
            if da:
                # Dashed arc: draw partial arc
                total = float(da)
                offset = float(do)
                if total > 0:
                    sweep = (total - offset) / total * 360
                    start = 0  # SVG default start (after -rotate-90 transform: top)
                    draw.arc(bbox, start=start, end=start + sweep,
                             fill=(*stroke_color, 255), width=max(1, int(sw * scale)))
            else:
                draw.ellipse(bbox, outline=(*stroke_color, 255), width=max(1, int(sw * scale)))

    # Render rectangles
    for el in _find_all('rect'):
        x = float(el.get('x', 0))
        y = float(el.get('y', 0))
        w = float(el.get('width', 0))
        h = float(el.get('height', 0))
        rx = float(el.get('rx', 0))
        fill = el.get('fill')
        stroke = el.get('stroke')
        sw = float(el.get('stroke-width', 1))

        bbox = [tx(x), ty(y), tx(x + w), ty(y + h)]
        fill_color = _parse_svg_color(fill)
        stroke_color = _parse_svg_color(stroke)

        if rx > 0:
            r_px = int(rx * scale)
            if fill_color:
                draw.rounded_rectangle(bbox, radius=r_px, fill=(*fill_color, 255))
            if stroke_color:
                draw.rounded_rectangle(bbox, radius=r_px, outline=(*stroke_color, 255),
                                       width=max(1, int(sw * scale)))
        else:
            if fill_color:
                draw.rectangle(bbox, fill=(*fill_color, 255))
            if stroke_color:
                draw.rectangle(bbox, outline=(*stroke_color, 255),
                               width=max(1, int(sw * scale)))

    # Render lines
    for el in _find_all('line'):
        x1 = float(el.get('x1', 0))
        y1 = float(el.get('y1', 0))
        x2 = float(el.get('x2', 0))
        y2 = float(el.get('y2', 0))
        stroke = el.get('stroke', '#000')
        sw = float(el.get('stroke-width', 1))
        stroke_color = _parse_svg_color(stroke)
        if stroke_color:
            draw.line([(tx(x1), ty(y1)), (tx(x2), ty(y2))],
                      fill=(*stroke_color, 255), width=max(1, int(sw * scale)))

    # Render ellipses
    for el in _find_all('ellipse'):
        cx = float(el.get('cx', 0))
        cy = float(el.get('cy', 0))
        rx = float(el.get('rx', 0))
        ry = float(el.get('ry', 0))
        fill = el.get('fill')
        stroke = el.get('stroke')
        sw = float(el.get('stroke-width', 1))

        bbox = [tx(cx - rx), ty(cy - ry), tx(cx + rx), ty(cy + ry)]
        fill_color = _parse_svg_color(fill)
        stroke_color = _parse_svg_color(stroke)
        if fill_color:
            draw.ellipse(bbox, fill=(*fill_color, 255))
        if stroke_color:
            draw.ellipse(bbox, outline=(*stroke_color, 255), width=max(1, int(sw * scale)))

    img.save(out_path, "PNG")
    return True


def _get_svg_renderer():
    """Detect available SVG renderer. Returns ('cairosvg'|'pillow', render_func)."""
    try:
        import cairosvg
        # Verify Cairo C library is actually loadable
        cairosvg.svg2png(bytestring=b'<svg xmlns="http://www.w3.org/2000/svg"/>', write_to=os.devnull)
        return "cairosvg", _svg2png_cairosvg
    except Exception:
        pass

    try:
        from PIL import Image, ImageDraw
        return "pillow", _svg2png_pillow
    except ImportError:
        pass

    return None, None


def cmd_export_svgs(args):
    """Extract inline SVGs from HTML, convert to PNG."""
    backend_name, render_fn = _get_svg_renderer()
    if render_fn is None:
        print("ERROR: No SVG renderer available.")
        print("  Option 1: pip install cairosvg  (+ Cairo C library)")
        print("  Option 2: pip install Pillow    (basic SVG subset)")
        sys.exit(1)

    print(f"SVG renderer: {backend_name}")

    if not os.path.isfile(args.input):
        print(f"ERROR: HTML file not found: {args.input}")
        sys.exit(1)

    with open(args.input, "r", encoding="utf-8") as f:
        html = f.read()

    root = _find_egui_root()
    app_name = getattr(args, "app", None)

    # Determine output directory
    if args.output:
        output_dir = args.output
    elif app_name:
        app_dir = os.path.join(root, "example", app_name)
        output_dir = os.path.join(app_dir, ".eguiproject", "resources", "images")
    else:
        print("ERROR: Must specify either --output or --app")
        sys.exit(1)

    os.makedirs(output_dir, exist_ok=True)

    svgs = _extract_svgs_from_html(args.input)
    if not svgs:
        print("No inline SVG elements found in HTML.")
        return

    print(f"Found {len(svgs)} SVG element(s)")

    size = args.size
    prefix = args.prefix
    rendered = 0
    exported_names = []

    for entry in svgs:
        name = entry["name"]
        svg_str = entry["svg"]

        # Resolve currentColor from CSS context
        svg_pos = html.find(svg_str)
        svg_str = _resolve_current_color(svg_str, html, svg_pos if svg_pos >= 0 else 0)

        # Ensure proper dimensions and xmlns
        svg_str = _ensure_svg_dimensions(svg_str, size)

        filename = f"{prefix}{name}.png"
        out_path = os.path.join(output_dir, filename)

        try:
            render_fn(svg_str, out_path, size)
            rendered += 1
            exported_names.append(name)
            print(f"  Exported: {filename} ({size}x{size})")
        except Exception as e:
            print(f"  FAILED:   {filename} — {e}")
            # Save the SVG for debugging
            debug_path = os.path.join(output_dir, f"{prefix}{name}.svg")
            with open(debug_path, "w", encoding="utf-8") as f:
                f.write(svg_str)
            print(f"           (debug SVG saved: {debug_path})")

    print(f"\nRendered {rendered}/{len(svgs)} SVGs to {output_dir}")

    # Sync to resource/src/ if using --app
    if app_name and exported_names:
        app_dir = os.path.join(root, "example", app_name)
        src_dir = os.path.join(app_dir, "resource", "src")
        os.makedirs(src_dir, exist_ok=True)
        import shutil
        synced = 0
        for name in exported_names:
            fname = f"{prefix}{name}.png"
            src_png = os.path.join(output_dir, fname)
            dst_png = os.path.join(src_dir, fname)
            if os.path.exists(src_png):
                shutil.copy2(src_png, dst_png)
                synced += 1
        print(f"  Synced {synced} SVG PNGs to {src_dir}")

        # Update resource config
        config_path = os.path.join(src_dir, "app_resource_config.json")
        if not os.path.exists(config_path):
            with open(config_path, "w", encoding="utf-8", newline="\n") as f:
                f.write(RESOURCE_CONFIG_TEMPLATE)
        full_names = [f"{prefix}{n}" for n in exported_names]
        _update_resource_config(config_path, full_names, size,
                                image_format=getattr(args, "image_format", "rgb565"))


# ── Sub-command: extract-text ─────────────────────────────────────

class _TextExtractHTMLParser(HTMLParser):
    """Extract visible text content from HTML, skipping non-visible elements."""

    _SKIP_TAGS = {"script", "style", "link", "meta", "title", "head"}

    def __init__(self):
        super().__init__()
        self._skip_depth = 0
        self._texts = []
        self._in_icon_span = False

    def handle_starttag(self, tag, attrs):
        if tag in self._SKIP_TAGS:
            self._skip_depth += 1
        # Skip Material Symbols icon text (icon names, not display text)
        attrs_dict = dict(attrs)
        cls = attrs_dict.get("class", "")
        if "material-symbols" in cls:
            self._in_icon_span = True

    def handle_endtag(self, tag):
        if tag in self._SKIP_TAGS and self._skip_depth > 0:
            self._skip_depth -= 1
        if self._in_icon_span and tag == "span":
            self._in_icon_span = False

    def handle_data(self, data):
        if self._skip_depth > 0 or self._in_icon_span:
            return
        text = data.strip()
        if text:
            self._texts.append(text)

    def get_all_text(self):
        return " ".join(self._texts)


def cmd_extract_text(args):
    """Extract unique characters from HTML visible text, output to .txt for font generation."""
    if not os.path.isfile(args.input):
        print(f"ERROR: HTML file not found: {args.input}")
        sys.exit(1)

    with open(args.input, "r", encoding="utf-8") as f:
        html = f.read()

    parser = _TextExtractHTMLParser()
    parser.feed(html)
    all_text = parser.get_all_text()

    # Collect unique characters
    chars = set(all_text)
    # Always include basic ASCII printable range
    for c in range(0x20, 0x7F):
        chars.add(chr(c))
    # Remove control characters
    chars = {c for c in chars if ord(c) >= 0x20}

    # Sort: ASCII first, then by codepoint
    sorted_chars = sorted(chars, key=lambda c: ord(c))
    char_str = "".join(sorted_chars)

    # Determine output path
    root = _find_egui_root()
    app_name = getattr(args, "app", None)

    if args.output:
        out_path = args.output
    elif app_name:
        app_dir = os.path.join(root, "example", app_name)
        src_dir = os.path.join(app_dir, "resource", "src")
        os.makedirs(src_dir, exist_ok=True)
        out_path = os.path.join(src_dir, "supported_text.txt")
    else:
        # Print to stdout
        sys.stdout.buffer.write(char_str.encode("utf-8"))
        sys.stdout.buffer.write(b"\n")
        print(f"\n({len(sorted_chars)} unique characters)", file=sys.stderr)
        return

    with open(out_path, "w", encoding="utf-8", newline="\n") as f:
        f.write(char_str)

    print(f"Extracted {len(sorted_chars)} unique characters to: {out_path}")

    # Show non-ASCII characters separately for review
    non_ascii = [c for c in sorted_chars if ord(c) > 0x7E]
    if non_ascii:
        display = "".join(non_ascii)
        try:
            print(f"  Non-ASCII ({len(non_ascii)}): {display}")
        except UnicodeEncodeError:
            # Windows console may not support all characters
            print(f"  Non-ASCII ({len(non_ascii)}): [contains characters not displayable in current console]")


# ── Sub-command: gen-resource ─────────────────────────────────────

def cmd_gen_resource(args):
    """Run resource generation (wraps make resource)."""
    root = _find_egui_root()
    app_name = args.app
    app_dir = os.path.join(root, "example", app_name)

    if not os.path.isdir(app_dir):
        print(f"ERROR: App directory not found: {app_dir}")
        sys.exit(1)

    resource_dir = os.path.join(app_dir, "resource")
    src_dir = os.path.join(resource_dir, "src")
    config_path = os.path.join(src_dir, "app_resource_config.json")

    if not os.path.isfile(config_path):
        print(f"ERROR: Resource config not found: {config_path}")
        print("Run 'scaffold' or 'export-icons' first.")
        sys.exit(1)

    # Call app_resource_generate.py directly
    # Remove cached files to force full regeneration (the -f flag has a parsing bug)
    for cached in ("app_egui_resource_generate.h", "app_egui_resource_generate.c"):
        cached_path = os.path.join(resource_dir, cached)
        if os.path.exists(cached_path):
            os.remove(cached_path)

    gen_script = os.path.join(root, "scripts", "tools", "app_resource_generate.py")
    output_dir = os.path.join(root, "output")
    os.makedirs(output_dir, exist_ok=True)

    cmd = [sys.executable, gen_script, "-r", resource_dir, "-o", output_dir]
    print(f"Running: {' '.join(cmd)}")

    result = subprocess.run(cmd, cwd=root)
    if result.returncode == 0:
        print(f"\nResource generation complete.")
        print(f"  C files generated in: {resource_dir}/img/ and {resource_dir}/font/")
        print(f"\nNext: make all APP={app_name} PORT=pc BITS=64")
    else:
        print(f"\nResource generation failed (exit code {result.returncode}).")
        sys.exit(1)


# ── Sub-command: generate-code ──────────────────────────────────

def _sync_font_files(project, egui_root, src_dir):
    """Copy font files referenced by widgets to resource/src/."""
    import shutil
    font_files = set()
    for page in project.pages:
        for w in page.get_all_widgets():
            ff = w.properties.get("font_file", "")
            if ff:
                font_files.add(ff)
    if not font_files:
        return
    os.makedirs(src_dir, exist_ok=True)
    tools_dir = os.path.join(egui_root, "scripts", "tools")
    synced = 0
    for fname in font_files:
        dst = os.path.join(src_dir, fname)
        if os.path.isfile(dst):
            synced += 1
            continue
        src = os.path.join(tools_dir, fname)
        if os.path.isfile(src):
            shutil.copy2(src, dst)
            synced += 1
            print(f"  Copied font: {fname}")
        else:
            print(f"  WARNING: Font file not found: {fname}")
    if synced:
        print(f"  {synced} font file(s) in resource/src/")


def cmd_generate_code(args):
    """Generate C code from UI Designer XML project."""
    root = _find_egui_root()
    app_name = args.app
    app_dir = os.path.join(root, "example", app_name)

    if not os.path.isdir(app_dir):
        print(f"ERROR: App directory not found: {app_dir}")
        sys.exit(1)

    # Find .egui project file
    egui_file = os.path.join(app_dir, f"{app_name}.egui")
    if not os.path.isfile(egui_file):
        print(f"ERROR: Project file not found: {egui_file}")
        print("Run 'scaffold' first to create the project structure.")
        sys.exit(1)

    # Add ui_designer to path
    ui_designer_dir = os.path.join(root, "scripts")
    if ui_designer_dir not in sys.path:
        sys.path.insert(0, ui_designer_dir)

    # Load project
    from ui_designer.model.project import Project
    project = Project.load(app_dir)
    print(f"Loaded project: {app_name} ({project.screen_width}x{project.screen_height})")
    print(f"  Pages: {', '.join(p.name for p in project.pages)}")

    # Sync images from .eguiproject/resources/images/ to resource/src/
    images_dir = os.path.join(app_dir, ".eguiproject", "resources", "images")
    src_dir = os.path.join(app_dir, "resource", "src")
    if os.path.isdir(images_dir):
        import shutil
        os.makedirs(src_dir, exist_ok=True)
        synced = 0
        for fname in os.listdir(images_dir):
            if fname.lower().endswith((".png", ".bmp", ".jpg")):
                shutil.copy2(os.path.join(images_dir, fname), os.path.join(src_dir, fname))
                synced += 1
        if synced:
            print(f"  Synced {synced} images to resource/src/")

    # Sync font files referenced by widgets to resource/src/
    _sync_font_files(project, root, src_dir)

    # Generate app_resource_config.json from XML
    from ui_designer.generator.resource_config_generator import ResourceConfigGenerator
    rcg = ResourceConfigGenerator()
    rcg.generate_and_save(project, src_dir)
    print(f"  Generated: resource/src/app_resource_config.json")

    # Also write generated text files to .eguiproject/resources/ for editor visibility
    eguiproject_res_dir = os.path.join(app_dir, ".eguiproject", "resources")
    os.makedirs(eguiproject_res_dir, exist_ok=True)
    import shutil as _shutil
    for fname in os.listdir(src_dir):
        if fname.startswith("_generated_text_") and fname.endswith(".txt"):
            _shutil.copy2(os.path.join(src_dir, fname), os.path.join(eguiproject_res_dir, fname))

    # Generate C code
    from ui_designer.generator.code_generator import generate_all_files_preserved
    files = generate_all_files_preserved(project, app_dir, backup=False)
    for filename, content in files.items():
        filepath = os.path.join(app_dir, filename)
        os.makedirs(os.path.dirname(filepath), exist_ok=True)
        with open(filepath, "w", encoding="utf-8", newline="\n") as f:
            f.write(content)

    print(f"\nGenerated {len(files)} C files:")
    for filename in sorted(files.keys()):
        print(f"  {filename}")
    print(f"\nNext steps:")
    print(f"  1. Gen resources:  python scripts/html2egui_helper.py gen-resource --app {app_name}")
    print(f"  2. Build & verify: python scripts/html2egui_helper.py verify --app {app_name}")


# ── Sub-command: extract-layout ──────────────────────────────────

# Tailwind spacing unit to pixels
_TW_SPACING = {
    "0": 0, "0.5": 2, "1": 4, "1.5": 6, "2": 8, "2.5": 10, "3": 12,
    "3.5": 14, "4": 16, "5": 20, "6": 24, "7": 28, "8": 32, "9": 36,
    "10": 40, "11": 44, "12": 48, "14": 56, "16": 64,
}


def _tw_to_px(val):
    """Convert a Tailwind spacing value to pixels."""
    if val in _TW_SPACING:
        return _TW_SPACING[val]
    # Try direct pixel value like [16px]
    m = re.match(r'\[(\d+)px\]', val)
    if m:
        return int(m.group(1))
    return None


def _classify_layout(classes):
    """Classify a Tailwind class string into an EGUI layout type."""
    cls = classes if isinstance(classes, str) else " ".join(classes)
    if "grid" in cls:
        m = re.search(r'grid-cols-(\d+)', cls)
        cols = int(m.group(1)) if m else 2
        return f"grid-{cols}col"
    if "flex" in cls:
        is_col = "flex-col" in cls
        is_wrap = "flex-wrap" in cls
        direction = "col" if is_col else "row"
        # Determine justify
        if "justify-between" in cls:
            justify = "between"
        elif "justify-center" in cls:
            justify = "center"
        elif "justify-end" in cls:
            justify = "end"
        elif "justify-around" in cls:
            justify = "around"
        elif "justify-evenly" in cls:
            justify = "evenly"
        else:
            justify = "start"
        suffix = f"-wrap" if is_wrap else ""
        return f"flex-{direction}-{justify}{suffix}"
    return "block"


def _extract_colors_from_tailwind_config(html):
    """Extract custom colors from tailwind.config script block."""
    colors = {}
    # Find the tailwind config script
    m = re.search(r'tailwind\.config\s*=\s*\{(.*?)\}\s*</script>', html, re.DOTALL)
    if not m:
        return colors
    config_text = m.group(1)
    # Find color definitions
    color_matches = re.findall(r'"([^"]+)"\s*:\s*"(#[0-9a-fA-F]{6})"', config_text)
    for name, hex_val in color_matches:
        colors[name] = "0x" + hex_val[1:].upper()
    return colors


class _LayoutHTMLParser(HTMLParser):
    """Parse HTML to extract structured layout information."""

    def __init__(self):
        super().__init__()
        self._stack = []  # stack of (tag, attrs_dict, children, text_parts)
        self._root = None
        self._in_main_container = False
        self._main_depth = 0

    def handle_starttag(self, tag, attrs):
        attrs_dict = dict(attrs)
        node = {
            "tag": tag,
            "attrs": attrs_dict,
            "classes": attrs_dict.get("class", ""),
            "children": [],
            "text": "",
        }
        if self._stack:
            self._stack[-1]["children"].append(node)
        self._stack.append(node)

        # Detect main container
        cls = attrs_dict.get("class", "")
        if not self._in_main_container:
            w_match = re.search(r'w-\[(\d+)px\]', cls)
            h_match = re.search(r'h-\[(\d+)px\]', cls)
            if w_match and h_match:
                self._in_main_container = True
                self._main_depth = len(self._stack)
                node["_is_main"] = True
                node["_width"] = int(w_match.group(1))
                node["_height"] = int(h_match.group(1))

    def handle_endtag(self, tag):
        if self._stack:
            node = self._stack.pop()
            if not self._stack:
                self._root = node

    def handle_data(self, data):
        text = data.strip()
        if text and self._stack:
            self._stack[-1]["text"] += (" " + text if self._stack[-1]["text"] else text)

    def get_main_container(self):
        """Find and return the main container node."""
        if self._root and self._root.get("_is_main"):
            return self._root
        return self._find_main(self._root)

    def _find_main(self, node):
        if node is None:
            return None
        if node.get("_is_main"):
            return node
        for child in node.get("children", []):
            result = self._find_main(child)
            if result:
                return result
        return None


def _node_to_section(node, index, prev_comment=""):
    """Convert a parsed HTML node into a section descriptor."""
    cls = node.get("classes", "")
    layout_type = _classify_layout(cls)

    # Try to identify from HTML comment or content
    section_id = f"section_{index}"

    # Extract padding/margin info
    padding = {}
    for prefix, key in [("px-", "x"), ("py-", "y"), ("pt-", "top"), ("pb-", "bottom"),
                         ("pl-", "left"), ("pr-", "right"), ("p-", "all")]:
        m = re.search(rf'{prefix}(\d+(?:\.\d+)?|\[\d+px\])', cls)
        if m:
            val = _tw_to_px(m.group(1).strip("[]").replace("px", ""))
            if val is not None:
                padding[key] = val

    # Extract gap
    gap = None
    m = re.search(r'gap-(\d+)', cls)
    if m:
        gap = _tw_to_px(m.group(1))

    # Extract height if specified
    height = None
    m = re.search(r'h-\[(\d+)px\]', cls)
    if m:
        height = int(m.group(1))
    elif re.search(r'h-(\d+)', cls):
        m = re.search(r'h-(\d+)', cls)
        val = _tw_to_px(m.group(1))
        if val:
            height = val

    # Extract children info
    children = []
    for child in node.get("children", []):
        child_info = _extract_child_info(child)
        if child_info:
            children.append(child_info)

    section = {
        "id": section_id,
        "classes": cls,
        "layout_type": layout_type,
        "children": children,
    }
    if prev_comment:
        section["html_comment"] = prev_comment
    if padding:
        section["padding"] = padding
    if gap is not None:
        section["gap"] = gap
    if height is not None:
        section["height"] = height

    # Check for absolute positioning
    if "absolute" in cls:
        section["position"] = "absolute"
        if "bottom-0" in cls:
            section["anchor"] = "bottom"

    # Check for navigate target (from onClick={() => navigate('/path')})
    nav_target = node.get("attrs", {}).get("data-navigate", "")
    if nav_target:
        section["navigate_to"] = nav_target

    return section


def _extract_child_info(node):
    """Extract information from a child node."""
    cls = node.get("classes", "")
    tag = node.get("tag", "")
    text = node.get("text", "").strip()

    # Skip style and script tags
    if tag in ("style", "script", "link", "meta"):
        return None

    info = {}

    # Check if it's a Material Symbols icon
    if "material-symbols-outlined" in cls:
        info["type"] = "icon"
        info["name"] = text
        # Extract size from classes
        if "text-sm" in cls:
            info["size"] = "sm"
        elif "text-lg" in cls:
            info["size"] = "lg"
        elif "text-xl" in cls:
            info["size"] = "xl"
        m = re.search(r'text-\[(\d+)px\]', cls)
        if m:
            info["size_px"] = int(m.group(1))
        # Extract color
        color_info = _extract_tw_color(cls)
        if color_info:
            info["color"] = color_info
        return info

    # Check if it's a text element
    if tag in ("span", "p", "h1", "h2", "h3", "a") and text and not node.get("children"):
        info["type"] = "text"
        info["content"] = text
        # Font info
        font_info = _extract_tw_font(cls)
        if font_info:
            info["font"] = font_info
        color_info = _extract_tw_color(cls)
        if color_info:
            info["color"] = color_info
        return info

    # Check if it's an SVG (gauge)
    if tag == "svg":
        info["type"] = "svg_gauge"
        # Try to extract stroke-dasharray and stroke-dashoffset
        for child in node.get("children", []):
            child_cls = child.get("attrs", {})
            if child.get("tag") == "circle" and child_cls.get("stroke-dasharray"):
                da = float(child_cls.get("stroke-dasharray", "0"))
                do = float(child_cls.get("stroke-dashoffset", "0"))
                if da > 0:
                    info["percent"] = round((1 - do / da) * 100)
                    info["stroke_width"] = child_cls.get("stroke-width", "6")
                    info["stroke_color"] = child_cls.get("stroke", "")
        return info

    # It's a container with children
    if node.get("children"):
        info["type"] = "container"
        info["classes"] = cls
        info["layout_type"] = _classify_layout(cls)
        info["children"] = []
        for child in node.get("children", []):
            child_info = _extract_child_info(child)
            if child_info:
                info["children"].append(child_info)
        # Add text if present alongside children
        if text:
            info["text"] = text
        # Extract bg color
        bg_info = _extract_tw_bg(cls)
        if bg_info:
            info["bg"] = bg_info
        # Extract rounded corners
        radius = _extract_tw_radius(cls)
        if radius is not None:
            info["radius"] = radius
        # Extract height
        m = re.search(r'h-(\d+)', cls)
        if m:
            h = _tw_to_px(m.group(1))
            if h:
                info["height"] = h
        # Extract navigate target
        nav_target = node.get("attrs", {}).get("data-navigate", "")
        if nav_target:
            info["navigate_to"] = nav_target
        return info

    # Leaf element with text
    if text:
        info["type"] = "text"
        info["content"] = text
        font_info = _extract_tw_font(cls)
        if font_info:
            info["font"] = font_info
        color_info = _extract_tw_color(cls)
        if color_info:
            info["color"] = color_info
        return info

    return None


def _extract_tw_color(cls):
    """Extract text/icon color from Tailwind classes.

    Supports standard Tailwind colors (text-cyan-400), custom names
    (text-primary), hex syntax (text-[#RRGGBB]), and opacity modifiers
    (text-cyan-400/50).  Returns a dict with ``name`` and optionally
    ``hex`` and ``opacity`` keys, or a simple string for legacy callers.
    """
    # Inline hex: text-[#0a0e1a]
    m = re.search(r'text-\[#([0-9a-fA-F]{6})\]', cls)
    if m:
        return {"name": f"#{m.group(1)}", "hex": m.group(1).upper(), "opacity": 100}
    # Standard pattern: text-COLOR/OPACITY  e.g. text-cyan-400/50
    m = re.search(r'text-((?:[a-z]+-)?[a-z]+-\d{2,3}|white|black|primary|accent-blue|card-dark)(?:/(\d+))?', cls)
    if m:
        name = m.group(1)
        opacity = int(m.group(2)) if m.group(2) else 100
        hex_val = _TAILWIND_COLORS.get(name)
        result = {"name": name, "opacity": opacity}
        if hex_val:
            result["hex"] = hex_val
        return result
    if "text-white" in cls:
        return {"name": "white", "hex": "FFFFFF", "opacity": 100}
    return None


def _extract_tw_bg(cls):
    """Extract background color from Tailwind classes.

    Supports standard Tailwind colors (bg-gray-800), custom names
    (bg-primary), hex syntax (bg-[#0a0e1a]), and opacity modifiers
    (bg-gray-800/50).
    """
    # Inline hex: bg-[#0a0e1a]
    m = re.search(r'bg-\[#([0-9a-fA-F]{6})\]', cls)
    if m:
        return {"name": f"#{m.group(1)}", "hex": m.group(1).upper(), "opacity": 100}
    # Standard pattern: bg-COLOR/OPACITY
    m = re.search(r'bg-((?:[a-z]+-)?[a-z]+-\d{2,3}|white|black|primary|card-dark|background-dark)(?:/(\d+))?', cls)
    if m:
        name = m.group(1)
        opacity = int(m.group(2)) if m.group(2) else 100
        hex_val = _TAILWIND_COLORS.get(name)
        result = {"name": name, "opacity": opacity}
        if hex_val:
            result["hex"] = hex_val
        return result
    return None


def _extract_tw_border_color(cls):
    """Extract border color from Tailwind classes.

    Supports border-COLOR/OPACITY syntax (e.g. border-cyan-500/30).
    """
    m = re.search(r'border-((?:[a-z]+-)?[a-z]+-\d{2,3}|white|black)(?:/(\d+))?', cls)
    if m:
        name = m.group(1)
        opacity = int(m.group(2)) if m.group(2) else 100
        hex_val = _TAILWIND_COLORS.get(name)
        result = {"name": name, "opacity": opacity}
        if hex_val:
            result["hex"] = hex_val
        return result
    return None


def _extract_tw_font(cls):
    """Extract font size and weight from Tailwind classes."""
    info = {}
    # Size
    size_map = {
        "text-xs": 12, "text-sm": 14, "text-base": 16, "text-lg": 18,
        "text-xl": 20, "text-2xl": 24, "text-3xl": 30, "text-4xl": 36,
    }
    for tw, px in size_map.items():
        if tw in cls.split():
            info["size_px"] = px
            break
    m = re.search(r'text-\[(\d+)px\]', cls)
    if m:
        info["size_px"] = int(m.group(1))
    # Weight
    for w in ("font-bold", "font-semibold", "font-medium", "font-light"):
        if w in cls:
            info["weight"] = w.replace("font-", "")
            break
    return info if info else None


def _extract_tw_radius(cls):
    """Extract border radius from Tailwind classes."""
    # Check bracket syntax first (rounded-[12px], rounded-[0.5rem])
    m = re.search(r'rounded-\[([^\]]+)\]', cls)
    if m:
        val = m.group(1)
        if val.endswith("rem"):
            return int(float(val.replace("rem", "")) * 16)
        if val.endswith("px"):
            return int(val.replace("px", ""))
    radius_map = {
        "rounded-full": 9999, "rounded-2xl": 16, "rounded-xl": 12,
        "rounded-lg": 8, "rounded-md": 6, "rounded": 4,
    }
    for tw, px in radius_map.items():
        if tw in cls:
            return px
    return None


def _parse_css_in_js(style_str):
    """Parse CSS-in-JS style={{}} inline style string to dict.

    Handles patterns like: backgroundColor: '#1a1a2e', fontSize: '14px'
    and numeric values like: width: 100, padding: 16
    """
    result = {}
    if not style_str:
        return result
    # Match key: 'value' or key: value patterns
    for m in re.finditer(r"(\w+)\s*:\s*(?:'([^']*)'|\"([^\"]*)\"|(\d+(?:\.\d+)?))", style_str):
        key = m.group(1)
        val = m.group(2) or m.group(3) or m.group(4)
        if val is not None:
            result[key] = val
    return result


def cmd_extract_layout(args):
    """Parse HTML and output structured JSON layout analysis."""
    if not os.path.isfile(args.input):
        print(f"ERROR: HTML file not found: {args.input}", file=sys.stderr)
        sys.exit(1)

    with open(args.input, "r", encoding="utf-8") as f:
        html = f.read()

    # Extract custom colors from Tailwind config
    colors = _extract_colors_from_tailwind_config(html)

    # Extract all Material Symbols icons
    icons = _extract_material_icons(args.input)

    # Parse HTML DOM
    parser = _LayoutHTMLParser()
    parser.feed(html)
    main_container = parser.get_main_container()

    if not main_container:
        print("ERROR: No main container found (looking for div with w-[Npx] h-[Npx])", file=sys.stderr)
        sys.exit(1)

    screen_w = main_container.get("_width", 320)
    screen_h = main_container.get("_height", 480)

    # Extract HTML comments for section identification
    comment_pattern = r'<!--\s*(.+?)\s*-->'
    comments = re.findall(comment_pattern, html)

    # Process sections (direct children of main container that are divs)
    sections = []
    comment_idx = 0
    for i, child in enumerate(main_container.get("children", [])):
        if child.get("tag") in ("style", "script", "link"):
            continue
        comment = ""
        # Try to match HTML comments to sections
        if comment_idx < len(comments):
            comment = comments[comment_idx]
            comment_idx += 1
        section = _node_to_section(child, i, prev_comment=comment)
        sections.append(section)

    result = {
        "screen": {"width": screen_w, "height": screen_h},
        "colors": colors,
        "icons": icons,
        "sections": sections,
    }

    output_json = json.dumps(result, indent=2, ensure_ascii=False)
    if args.output:
        with open(args.output, "w", encoding="utf-8", newline="\n") as f:
            f.write(output_json)
        print(f"Layout analysis written to: {args.output}", file=sys.stderr)
    else:
        sys.stdout.buffer.write(output_json.encode("utf-8"))
        sys.stdout.buffer.write(b"\n")


# ── Sub-command: verify ──────────────────────────────────────────

def cmd_verify(args):
    """Build and run runtime check for an app."""
    root = _find_egui_root()
    app_name = args.app
    app_dir = os.path.join(root, "example", app_name)

    if not os.path.isdir(app_dir):
        print(f"ERROR: App directory not found: {app_dir}")
        sys.exit(1)

    bits_flag = "BITS=64" if args.bits64 else ""

    # Step 1: Clean
    if not args.no_clean:
        print("=== Step 1: Clean ===")
        clean_cmd = f"make clean APP={app_name}"
        result = subprocess.run(clean_cmd, shell=True, cwd=root)
        if result.returncode != 0:
            print(f"WARNING: Clean failed (continuing anyway)")

    # Step 2: Build
    print("\n=== Step 2: Build ===")
    build_cmd = f"make all APP={app_name} PORT=pc {bits_flag}".strip()
    result = subprocess.run(build_cmd, shell=True, cwd=root)
    if result.returncode != 0:
        print(f"\nBUILD FAILED (exit code {result.returncode})")
        sys.exit(1)
    print("BUILD OK")

    # Step 3: Runtime check
    print("\n=== Step 3: Runtime Check ===")
    check_script = os.path.join(root, "scripts", "code_runtime_check.py")
    check_cmd = [sys.executable, check_script, "--app", app_name, "--keep-screenshots"]
    if args.bits64:
        check_cmd.append("--bits64")
    if args.timeout:
        check_cmd.extend(["--timeout", str(args.timeout)])
    result = subprocess.run(check_cmd, cwd=root)
    if result.returncode != 0:
        print(f"\nRUNTIME CHECK FAILED (exit code {result.returncode})")
        sys.exit(1)
    print("\nRUNTIME CHECK OK")

    # Step 4: Report screenshot location
    screenshot_dir = os.path.join(root, "runtime_check_output", app_name)
    if os.path.isdir(screenshot_dir):
        pngs = [f for f in os.listdir(screenshot_dir) if f.endswith(".png")]
        if pngs:
            print(f"\nScreenshots saved in: {screenshot_dir}")
            print(f"  Files: {', '.join(sorted(pngs)[:5])}" + (" ..." if len(pngs) > 5 else ""))

    # Step 5: Visual comparison (optional)
    design_path = getattr(args, "compare_design", None)
    if design_path and os.path.isdir(screenshot_dir):
        # Find first rendered frame
        default_dir = os.path.join(screenshot_dir, "default")
        search_dir = default_dir if os.path.isdir(default_dir) else screenshot_dir
        frames = sorted(f for f in os.listdir(search_dir) if f.endswith(".png"))
        if frames:
            rendered_path = os.path.join(search_dir, frames[0])
            compare_script = os.path.join(root, "scripts", "figma_visual_compare.py")
            output_path = os.path.join(screenshot_dir, "comparison.png")
            compare_cmd = [sys.executable, compare_script,
                           "--design", design_path,
                           "--rendered", rendered_path,
                           "--output", output_path]
            print("\n=== Step 5: Visual Comparison ===")
            subprocess.run(compare_cmd, cwd=root)


# ── Sub-command: figma2xml ────────────────────────────────────────

def _parse_figma_url(url):
    """Parse Figma URL to extract file_key and node_id.

    Supports:
        https://www.figma.com/design/FILE_KEY/Title?node-id=NODE_ID
        https://www.figma.com/file/FILE_KEY/Title?node-id=NODE_ID
    Returns (file_key, node_id) or raises ValueError.
    """
    import urllib.parse

    parsed = urllib.parse.urlparse(url)
    if "figma.com" not in parsed.netloc:
        raise ValueError(f"Not a Figma URL: {url}")

    # Extract file key from path: /design/FILE_KEY/... or /file/FILE_KEY/...
    path_parts = parsed.path.strip("/").split("/")
    if len(path_parts) < 2 or path_parts[0] not in ("design", "file"):
        raise ValueError(f"Cannot extract file key from URL: {url}")
    file_key = path_parts[1]

    # Extract node-id from query string
    query = urllib.parse.parse_qs(parsed.query)
    node_id = query.get("node-id", [None])[0]
    if node_id:
        # URL decode: 1%3A2 -> 1:2
        node_id = urllib.parse.unquote(node_id)

    return file_key, node_id


def _fetch_figma_nodes(file_key, node_ids, token):
    """Fetch node tree from Figma REST API.

    Args:
        file_key: Figma file key
        node_ids: List of node IDs to fetch (or None for entire file)
        token: Figma personal access token

    Returns parsed JSON response.
    """
    import urllib.request
    import urllib.error

    base_url = f"https://api.figma.com/v1/files/{file_key}"
    if node_ids:
        ids_param = ",".join(node_ids)
        url = f"{base_url}/nodes?ids={ids_param}"
    else:
        url = base_url

    req = urllib.request.Request(url)
    req.add_header("X-Figma-Token", token)

    try:
        with urllib.request.urlopen(req, timeout=30) as resp:
            return json.loads(resp.read().decode("utf-8"))
    except urllib.error.HTTPError as e:
        if e.code == 403:
            raise ValueError("Figma API: Invalid or expired token")
        elif e.code == 404:
            raise ValueError(f"Figma API: File or node not found: {file_key}")
        raise ValueError(f"Figma API error: {e.code} {e.reason}")


def _figma_color_to_hex(color_obj):
    """Convert Figma color {r, g, b, a} (0-1 floats) to hex string."""
    if color_obj is None:
        return None
    r = min(255, max(0, round(color_obj.get("r", 0) * 255)))
    g = min(255, max(0, round(color_obj.get("g", 0) * 255)))
    b = min(255, max(0, round(color_obj.get("b", 0) * 255)))
    return f"{r:02X}{g:02X}{b:02X}"


def _figma_opacity_to_alpha(opacity):
    """Convert Figma opacity (0-1) to EGUI alpha constant."""
    if opacity is None or opacity >= 1.0:
        return "EGUI_ALPHA_100"
    pct = round(opacity * 10) * 10  # Round to nearest 10%
    pct = max(0, min(100, pct))
    return f"EGUI_ALPHA_{pct}"


def _figma_color_with_alpha(color_obj, opacity=1.0):
    """Convert Figma color + opacity to (hex_string, alpha_constant) tuple.

    Combines the color's own alpha channel with the layer opacity.
    """
    hex_str = _figma_color_to_hex(color_obj)
    color_alpha = color_obj.get("a", 1.0) if color_obj else 1.0
    combined = color_alpha * (opacity if opacity is not None else 1.0)
    alpha = _figma_opacity_to_alpha(combined)
    return hex_str, alpha


def _figma_detect_variants(node):
    """Detect Figma component variants from a COMPONENT_SET node.

    Returns dict mapping state name -> fill color hex, e.g.
    {"default": "0080FF", "pressed": "0066CC"}.
    """
    if node.get("type") != "COMPONENT_SET":
        return {}
    variants = {}
    for child in node.get("children", []):
        if child.get("type") != "COMPONENT":
            continue
        name = child.get("name", "")
        # Parse "State=Default", "State=Pressed", etc.
        state = None
        for part in name.split(","):
            part = part.strip()
            if "=" in part:
                key, val = part.split("=", 1)
                if key.strip().lower() == "state":
                    state = val.strip().lower()
        if not state:
            state = "default"
        fills = child.get("fills", [])
        fill_color = None
        for fill in fills:
            if fill.get("visible", True) and fill.get("type") == "SOLID":
                fill_color = _figma_color_to_hex(fill.get("color"))
                break
        variants[state] = fill_color
    return variants


def _figma_gradient_to_egui(fill):
    """Convert a Figma gradient fill to EGUI gradient descriptor.

    Returns dict with type, start_color, end_color, direction or None.
    """
    if not fill or fill.get("type") not in ("GRADIENT_LINEAR", "GRADIENT_RADIAL"):
        return None
    if not fill.get("visible", True):
        return None
    stops = fill.get("gradientStops", [])
    if len(stops) < 2:
        return None
    start_color = _figma_color_to_hex(stops[0].get("color"))
    end_color = _figma_color_to_hex(stops[-1].get("color"))
    # Determine direction from handle positions
    handles = fill.get("gradientHandlePositions", [])
    direction = "horizontal"
    if len(handles) >= 2:
        dx = abs(handles[1].get("x", 0) - handles[0].get("x", 0))
        dy = abs(handles[1].get("y", 0) - handles[0].get("y", 0))
        if dy > dx:
            direction = "vertical"
    return {
        "type": "linear_gradient",
        "start_color": start_color,
        "end_color": end_color,
        "direction": direction,
    }


def _figma_shadow_to_egui(effect):
    """Convert a Figma drop-shadow effect to EGUI shadow parameters.

    Returns dict with offset_x, offset_y, blur, color or None.
    """
    if not effect or effect.get("type") != "DROP_SHADOW":
        return None
    if not effect.get("visible", True):
        return None
    offset = effect.get("offset", {})
    return {
        "offset_x": int(offset.get("x", 0)),
        "offset_y": int(offset.get("y", 0)),
        "blur": int(effect.get("radius", 0)),
        "color": _figma_color_to_hex(effect.get("color")),
    }


def _figma_align_to_egui(h_align, v_align=None):
    """Convert Figma text alignment to EGUI align_type."""
    h_map = {"LEFT": "EGUI_ALIGN_LEFT", "CENTER": "EGUI_ALIGN_HCENTER", "RIGHT": "EGUI_ALIGN_RIGHT"}
    v_map = {"TOP": "EGUI_ALIGN_TOP", "CENTER": "EGUI_ALIGN_VCENTER", "BOTTOM": "EGUI_ALIGN_BOTTOM"}

    parts = []
    if h_align:
        parts.append(h_map.get(h_align, "EGUI_ALIGN_LEFT"))
    if v_align:
        parts.append(v_map.get(v_align, "EGUI_ALIGN_VCENTER"))

    if not parts:
        return "EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER"
    return " | ".join(parts)


def _figma_font_to_egui(style):
    """Convert Figma text style to EGUI font attributes.

    Returns dict with font_builtin or font_file, font_pixelsize, etc.
    """
    result = {}
    font_size = int(style.get("fontSize", 14))
    font_family = style.get("fontFamily", "")
    font_weight = style.get("fontWeight", 400)

    # Check for Material Symbols font
    if "material" in font_family.lower() and "symbol" in font_family.lower():
        result["font_file"] = "MaterialSymbolsOutlined-Regular.ttf"
        result["font_pixelsize"] = font_size
        result["font_fontbitsize"] = "4"
        return result

    # Map to builtin Montserrat fonts
    # Available: montserrat_8_4, montserrat_10_4, montserrat_12_4, montserrat_14_4, montserrat_18_4, montserrat_30_4
    size_map = {
        8: "8", 9: "8", 10: "10", 11: "10", 12: "12", 13: "12",
        14: "14", 15: "14", 16: "14", 17: "18", 18: "18", 19: "18",
        20: "18", 21: "18", 22: "18", 23: "18", 24: "30", 25: "30",
        26: "30", 27: "30", 28: "30", 29: "30", 30: "30",
    }
    mapped_size = size_map.get(font_size, "14")
    if font_size > 30:
        mapped_size = "30"

    result["font_builtin"] = f"&amp;egui_res_font_montserrat_{mapped_size}_4"
    return result


def _escape_xml_text(text):
    """Escape text for XML content."""
    if not text:
        return ""
    text = text.replace("&", "&amp;")
    text = text.replace("<", "&lt;")
    text = text.replace(">", "&gt;")
    text = text.replace('"', "&quot;")
    return text


def _figma_autolayout_align_to_egui(primary, counter, orientation):
    """Map Figma auto-layout alignment to EGUI align type."""
    h_map = {"MIN": "EGUI_ALIGN_LEFT", "CENTER": "EGUI_ALIGN_HCENTER", "MAX": "EGUI_ALIGN_RIGHT"}
    v_map = {"MIN": "EGUI_ALIGN_TOP", "CENTER": "EGUI_ALIGN_VCENTER", "MAX": "EGUI_ALIGN_BOTTOM"}
    if orientation == "vertical":
        h = h_map.get(counter, "EGUI_ALIGN_HCENTER")
        v = v_map.get(primary, "EGUI_ALIGN_TOP")
    else:
        h = h_map.get(primary, "EGUI_ALIGN_LEFT")
        v = v_map.get(counter, "EGUI_ALIGN_VCENTER")
    return f"({h} | {v})"


def _figma_node_to_xml(node, parent_x=0, parent_y=0, scale=1.0, indent=2):
    """Convert a Figma node to EGUI XML string recursively.

    Args:
        node: Figma node dict
        parent_x, parent_y: Parent's absolute position (for relative coords)
        scale: Scale factor for coordinates
        indent: Current indentation level

    Returns XML string.
    """
    node_type = node.get("type", "")
    name = node.get("name", "element")
    node_id = name.lower().replace(" ", "_").replace("-", "_")
    node_id = re.sub(r'[^a-z0-9_]', '', node_id)
    if not node_id:
        node_id = "element"

    # Get bounding box
    bbox = node.get("absoluteBoundingBox", {})
    abs_x = bbox.get("x", 0)
    abs_y = bbox.get("y", 0)
    width = int(bbox.get("width", 0) * scale)
    height = int(bbox.get("height", 0) * scale)
    x = int((abs_x - parent_x) * scale)
    y = int((abs_y - parent_y) * scale)

    # Ensure non-negative
    x = max(0, x)
    y = max(0, y)

    ind = "    " * indent
    lines = []

    # Get fills and strokes
    fills = node.get("fills", [])
    strokes = node.get("strokes", [])
    corner_radius = node.get("cornerRadius", 0)
    if isinstance(corner_radius, dict):
        corner_radius = corner_radius.get("topLeft", 0)
    corner_radius = int(corner_radius * scale)

    # Get primary fill color
    fill_color = None
    fill_opacity = 1.0
    for fill in fills:
        if fill.get("visible", True) and fill.get("type") == "SOLID":
            fill_color = _figma_color_to_hex(fill.get("color"))
            fill_opacity = fill.get("opacity", 1.0)
            break

    # Get stroke info
    stroke_color = None
    stroke_width = 0
    for stroke in strokes:
        if stroke.get("visible", True) and stroke.get("type") == "SOLID":
            stroke_color = _figma_color_to_hex(stroke.get("color"))
            stroke_width = int(node.get("strokeWeight", 1) * scale)
            break

    # Handle different node types
    if node_type == "TEXT":
        # Text node -> Label
        text = node.get("characters", "")
        style = node.get("style", {})

        # Check if it's a Material Symbol icon (single character in PUA range)
        is_icon = False
        if len(text) == 1 and ord(text) >= 0xE000:
            is_icon = True
            text = f"&#x{ord(text):04X};"

        font_attrs = _figma_font_to_egui(style)
        h_align = style.get("textAlignHorizontal", "LEFT")
        v_align = style.get("textAlignVertical", "CENTER")
        align_type = _figma_align_to_egui(h_align, v_align)

        # Build Label element
        attrs = [f'id="{node_id}"', f'x="{x}"', f'y="{y}"', f'width="{width}"', f'height="{height}"']

        if is_icon:
            attrs.append(f'text="{text}"')
        else:
            attrs.append(f'text="{_escape_xml_text(text)}"')

        for k, v in font_attrs.items():
            attrs.append(f'{k}="{v}"')

        if fill_color:
            attrs.append(f'color="EGUI_COLOR_HEX(0x{fill_color})"')
        else:
            attrs.append('color="EGUI_COLOR_WHITE"')
        attrs.append(f'alpha="{_figma_opacity_to_alpha(fill_opacity)}"')
        attrs.append(f'align_type="{align_type}"')

        lines.append(f'{ind}<Label {" ".join(attrs)} />')

    elif node_type in ("FRAME", "GROUP", "COMPONENT", "INSTANCE", "SECTION"):
        # Container node -> LinearLayout, GridLayout, Group, or Card
        children = node.get("children", [])
        layout_mode = node.get("layoutMode")  # "VERTICAL", "HORIZONTAL", or None
        layout_wrap = node.get("layoutWrap")   # "WRAP" or None
        item_spacing = int(node.get("itemSpacing", 0) * scale)
        child_margin = max(1, item_spacing // 2) if item_spacing > 0 else 0

        if layout_mode and layout_wrap == "WRAP":
            # Auto-layout with wrap -> GridLayout
            col_count = max(1, int(width // (children[0].get("absoluteBoundingBox", {}).get("width", width) * scale))) if children else 2
            primary = node.get("primaryAxisAlignItems", "MIN")
            counter = node.get("counterAxisAlignItems", "MIN")
            align_type = _figma_autolayout_align_to_egui(primary, counter, "vertical")
            attrs = [f'id="{node_id}"', f'x="{x}"', f'y="{y}"', f'width="{width}"', f'height="{height}"',
                     f'col_count="{col_count}"', f'align_type="{align_type}"']
            lines.append(f'{ind}<GridLayout {" ".join(attrs)}>')
            if fill_color:
                bg_attrs = ['type="solid"', f'color="EGUI_COLOR_HEX(0x{fill_color})"', f'alpha="{_figma_opacity_to_alpha(fill_opacity)}"']
                lines.append(f'{ind}    <Background {" ".join(bg_attrs)} />')
            for child in children:
                child_xml = _figma_node_to_xml(child, abs_x, abs_y, scale, indent + 1)
                if child_xml:
                    if child_margin > 0:
                        child_xml = child_xml.replace("/>", f' margin="{child_margin}" />', 1) if "/>" in child_xml else child_xml
                    lines.append(child_xml)
            lines.append(f'{ind}</GridLayout>')

        elif layout_mode in ("VERTICAL", "HORIZONTAL"):
            # Auto-layout -> LinearLayout
            orientation = "vertical" if layout_mode == "VERTICAL" else "horizontal"
            primary = node.get("primaryAxisAlignItems", "MIN")
            counter = node.get("counterAxisAlignItems", "MIN")
            align_type = _figma_autolayout_align_to_egui(primary, counter, orientation)
            attrs = [f'id="{node_id}"', f'x="{x}"', f'y="{y}"', f'width="{width}"', f'height="{height}"',
                     f'orientation="{orientation}"', f'align_type="{align_type}"']
            # Extract auto-layout padding
            pad_l = int(node.get("paddingLeft", 0) * scale)
            pad_r = int(node.get("paddingRight", 0) * scale)
            pad_t = int(node.get("paddingTop", 0) * scale)
            pad_b = int(node.get("paddingBottom", 0) * scale)
            if pad_l:
                attrs.append(f'padding_left="{pad_l}"')
            if pad_r:
                attrs.append(f'padding_right="{pad_r}"')
            if pad_t:
                attrs.append(f'padding_top="{pad_t}"')
            if pad_b:
                attrs.append(f'padding_bottom="{pad_b}"')
            lines.append(f'{ind}<LinearLayout {" ".join(attrs)}>')
            if fill_color:
                bg_type = "round_rectangle" if corner_radius > 0 else "solid"
                bg_attrs = [f'type="{bg_type}"', f'color="EGUI_COLOR_HEX(0x{fill_color})"', f'alpha="{_figma_opacity_to_alpha(fill_opacity)}"']
                if corner_radius > 0:
                    bg_attrs.append(f'radius="{corner_radius}"')
                lines.append(f'{ind}    <Background {" ".join(bg_attrs)} />')
            for child in children:
                child_xml = _figma_node_to_xml(child, abs_x, abs_y, scale, indent + 1)
                if child_xml:
                    if child_margin > 0:
                        child_xml = child_xml.replace("/>", f' margin="{child_margin}" />', 1) if "/>" in child_xml else child_xml
                    lines.append(child_xml)
            lines.append(f'{ind}</LinearLayout>')

        elif corner_radius > 0 or (fill_color and fill_color != "000000"):
            attrs = [f'id="{node_id}"', f'x="{x}"', f'y="{y}"', f'width="{width}"', f'height="{height}"']
            if corner_radius > 0:
                attrs.append(f'corner_radius="{corner_radius}"')
            lines.append(f'{ind}<Card {" ".join(attrs)}>')

            # Add Background
            bg_attrs = ['type="round_rectangle"']
            if fill_color:
                bg_attrs.append(f'color="EGUI_COLOR_HEX(0x{fill_color})"')
            else:
                bg_attrs.append('color="EGUI_COLOR_HEX(0x1A2130)"')
            bg_attrs.append(f'alpha="{_figma_opacity_to_alpha(fill_opacity)}"')
            if corner_radius > 0:
                bg_attrs.append(f'radius="{corner_radius}"')
            if stroke_width > 0 and stroke_color:
                bg_attrs.append(f'stroke_width="{stroke_width}"')
                bg_attrs.append(f'stroke_color="EGUI_COLOR_HEX(0x{stroke_color})"')
                bg_attrs.append('stroke_alpha="EGUI_ALPHA_100"')
            lines.append(f'{ind}    <Background {" ".join(bg_attrs)} />')

            # Process children
            for child in children:
                child_xml = _figma_node_to_xml(child, abs_x, abs_y, scale, indent + 1)
                if child_xml:
                    lines.append(child_xml)

            lines.append(f'{ind}</Card>')
        else:
            attrs = [f'id="{node_id}"', f'x="{x}"', f'y="{y}"', f'width="{width}"', f'height="{height}"']
            lines.append(f'{ind}<Group {" ".join(attrs)}>')

            # Add Background if has fill
            if fill_color:
                bg_attrs = ['type="solid"', f'color="EGUI_COLOR_HEX(0x{fill_color})"']
                bg_attrs.append(f'alpha="{_figma_opacity_to_alpha(fill_opacity)}"')
                lines.append(f'{ind}    <Background {" ".join(bg_attrs)} />')

            # Process children
            for child in children:
                child_xml = _figma_node_to_xml(child, abs_x, abs_y, scale, indent + 1)
                if child_xml:
                    lines.append(child_xml)

            lines.append(f'{ind}</Group>')

    elif node_type == "RECTANGLE":
        # Rectangle without children -> could be a background element, skip if inside a frame
        # Or render as a Card if standalone
        if fill_color:
            attrs = [f'id="{node_id}"', f'x="{x}"', f'y="{y}"', f'width="{width}"', f'height="{height}"']
            if corner_radius > 0:
                attrs.append(f'corner_radius="{corner_radius}"')
            lines.append(f'{ind}<Card {" ".join(attrs)}>')
            bg_attrs = ['type="round_rectangle"', f'color="EGUI_COLOR_HEX(0x{fill_color})"']
            bg_attrs.append(f'alpha="{_figma_opacity_to_alpha(fill_opacity)}"')
            if corner_radius > 0:
                bg_attrs.append(f'radius="{corner_radius}"')
            lines.append(f'{ind}    <Background {" ".join(bg_attrs)} />')
            lines.append(f'{ind}</Card>')

    elif node_type == "ELLIPSE":
        # Ellipse -> could be CircularProgressBar or decorative
        # Check if it looks like a progress indicator (has stroke, no fill)
        if stroke_color and not fill_color and width == height:
            attrs = [f'id="{node_id}"', f'x="{x}"', f'y="{y}"', f'width="{width}"', f'height="{height}"']
            attrs.append('value="50"')
            attrs.append(f'stroke_width="{stroke_width}"')
            attrs.append(f'progress_color="EGUI_COLOR_HEX(0x{stroke_color})"')
            attrs.append('bk_color="EGUI_COLOR_HEX(0x1E293B)"')
            lines.append(f'{ind}<CircularProgressBar {" ".join(attrs)} />')

    elif node_type in ("VECTOR", "BOOLEAN_OPERATION", "LINE", "STAR", "POLYGON"):
        # Vector shapes -> export as image (handled separately)
        # For now, add a placeholder comment
        lines.append(f'{ind}<!-- Vector: {name} (export as PNG) -->')

    # Skip other node types (SLICE, etc.)

    return "\n".join(lines) if lines else ""


def _figma_export_images(file_key, node_ids, token, output_dir, scale=1):
    """Export Figma nodes as PNG images.

    Args:
        file_key: Figma file key
        node_ids: List of node IDs to export
        token: Figma personal access token
        output_dir: Directory to save PNGs
        scale: Export scale (1, 2, 3, 4)

    Returns dict of {node_id: filename}.
    """
    import urllib.request
    import urllib.error

    if not node_ids:
        return {}

    ids_param = ",".join(node_ids)
    url = f"https://api.figma.com/v1/images/{file_key}?ids={ids_param}&format=png&scale={scale}"

    req = urllib.request.Request(url)
    req.add_header("X-Figma-Token", token)

    try:
        with urllib.request.urlopen(req, timeout=60) as resp:
            data = json.loads(resp.read().decode("utf-8"))
    except urllib.error.HTTPError as e:
        print(f"  WARNING: Failed to export images: {e}")
        return {}

    images = data.get("images", {})
    results = {}

    os.makedirs(output_dir, exist_ok=True)

    for node_id, image_url in images.items():
        if not image_url:
            continue
        # Sanitize filename
        safe_id = node_id.replace(":", "_").replace("-", "_")
        filename = f"figma_{safe_id}.png"
        filepath = os.path.join(output_dir, filename)

        try:
            with urllib.request.urlopen(image_url, timeout=30) as img_resp:
                with open(filepath, "wb") as f:
                    f.write(img_resp.read())
            results[node_id] = filename
            print(f"  Exported: {filename}")
        except Exception as e:
            print(f"  WARNING: Failed to download {node_id}: {e}")

    return results


def cmd_figma2xml(args):
    """Convert Figma design to EGUI XML layout via REST API."""
    # Get token from args or environment
    token = args.token or os.environ.get("FIGMA_TOKEN")
    if not token:
        print("ERROR: Figma token required. Use --token or set FIGMA_TOKEN env var.")
        print("  Get token at: https://www.figma.com/developers/api#access-tokens")
        sys.exit(1)

    # Parse URL or use file-key + node-id
    if args.url:
        try:
            file_key, node_id = _parse_figma_url(args.url)
        except ValueError as e:
            print(f"ERROR: {e}")
            sys.exit(1)
    elif args.file_key:
        file_key = args.file_key
        node_id = args.node_id
    else:
        print("ERROR: Must specify --url or --file-key")
        sys.exit(1)

    print(f"Figma file: {file_key}")
    if node_id:
        print(f"Node ID: {node_id}")

    # Fetch node tree
    print("Fetching design from Figma API...")
    try:
        if node_id:
            data = _fetch_figma_nodes(file_key, [node_id], token)
            # Extract the specific node
            nodes = data.get("nodes", {})
            if node_id not in nodes:
                print(f"ERROR: Node {node_id} not found in response")
                sys.exit(1)
            root_node = nodes[node_id].get("document")
        else:
            data = _fetch_figma_nodes(file_key, None, token)
            # Use first page's first frame
            doc = data.get("document", {})
            pages = doc.get("children", [])
            if not pages:
                print("ERROR: No pages found in Figma file")
                sys.exit(1)
            frames = pages[0].get("children", [])
            if not frames:
                print("ERROR: No frames found in first page")
                sys.exit(1)
            root_node = frames[0]
    except ValueError as e:
        print(f"ERROR: {e}")
        sys.exit(1)

    if not root_node:
        print("ERROR: Could not find root node")
        sys.exit(1)

    print(f"Root node: {root_node.get('name', 'unnamed')} ({root_node.get('type', 'unknown')})")

    # Get frame dimensions
    bbox = root_node.get("absoluteBoundingBox", {})
    frame_w = int(bbox.get("width", 320))
    frame_h = int(bbox.get("height", 480))
    print(f"Frame size: {frame_w}x{frame_h}")

    # Parse target size
    if args.target:
        try:
            target_w, target_h = map(int, args.target.lower().split("x"))
        except ValueError:
            print(f"ERROR: Invalid target format: {args.target} (use WxH, e.g., 320x480)")
            sys.exit(1)
    else:
        target_w, target_h = frame_w, frame_h

    # Calculate scale
    scale_x = target_w / frame_w if frame_w > 0 else 1.0
    scale_y = target_h / frame_h if frame_h > 0 else 1.0
    scale = min(scale_x, scale_y)  # Maintain aspect ratio
    if abs(scale - 1.0) > 0.01:
        print(f"Scaling: {scale:.2f}x (target: {target_w}x{target_h})")

    # Setup app directory
    root = _find_egui_root()
    app_name = args.app
    app_dir = os.path.join(root, "example", app_name)

    # Run scaffold if app doesn't exist
    if not os.path.exists(app_dir):
        print(f"\nCreating app scaffold: {app_name}")
        scaffold_args = argparse.Namespace(
            app=app_name, width=target_w, height=target_h,
            color_depth=16, force=False
        )
        cmd_scaffold(scaffold_args)

    # Convert node tree to XML
    print("\nConverting to EGUI XML...")
    parent_x = bbox.get("x", 0)
    parent_y = bbox.get("y", 0)

    # Build XML document
    xml_lines = ['<?xml version="1.0" encoding="utf-8"?>', '<Page>']

    # Create root group
    xml_lines.append(f'    <Group id="root" x="0" y="0" width="{target_w}" height="{target_h}">')

    # Check for background fill on root
    fills = root_node.get("fills", [])
    for fill in fills:
        if fill.get("visible", True) and fill.get("type") == "SOLID":
            bg_color = _figma_color_to_hex(fill.get("color"))
            if bg_color:
                xml_lines.append(f'        <Background type="solid" color="EGUI_COLOR_HEX(0x{bg_color})" alpha="EGUI_ALPHA_100" />')
            break

    # Process children
    for child in root_node.get("children", []):
        child_xml = _figma_node_to_xml(child, parent_x, parent_y, scale, indent=2)
        if child_xml:
            xml_lines.append(child_xml)

    xml_lines.append('    </Group>')
    xml_lines.append('</Page>')

    xml_content = "\n".join(xml_lines)

    # Write XML file
    page_name = args.page or "main_page"
    layout_dir = os.path.join(app_dir, ".eguiproject", "layout")
    os.makedirs(layout_dir, exist_ok=True)
    xml_path = os.path.join(layout_dir, f"{page_name}.xml")

    with open(xml_path, "w", encoding="utf-8", newline="\n") as f:
        f.write(xml_content)
    print(f"  Written: {xml_path}")

    # Collect vector nodes for image export
    vector_nodes = []
    def _collect_vectors(node):
        if node.get("type") in ("VECTOR", "BOOLEAN_OPERATION"):
            node_id = node.get("id")
            if node_id:
                vector_nodes.append(node_id)
        for child in node.get("children", []):
            _collect_vectors(child)
    _collect_vectors(root_node)

    # Export vector images if any
    if vector_nodes and not args.no_export:
        print(f"\nExporting {len(vector_nodes)} vector node(s) as PNG...")
        images_dir = os.path.join(app_dir, ".eguiproject", "resources", "images")
        _figma_export_images(file_key, vector_nodes, token, images_dir, scale=2)

    print(f"\nFigma conversion complete: {app_name}")
    print(f"\nNext steps:")
    print(f"  1. Review/edit XML: {xml_path}")
    print(f"  2. Generate code:   python scripts/html2egui_helper.py generate-code --app {app_name}")
    print(f"  3. Gen resources:   python scripts/html2egui_helper.py gen-resource --app {app_name}")
    print(f"  4. Build & verify:  python scripts/html2egui_helper.py verify --app {app_name}")


# ── Sub-command: figma-mcp ────────────────────────────────────────

def cmd_figma_mcp(args):
    """Convert Figma MCP JSON data to EGUI XML layout (no REST API needed)."""
    import json as _json

    # Load JSON from file or stdin
    if args.input:
        with open(args.input, encoding="utf-8") as f:
            data = _json.load(f)
    else:
        data = _json.load(sys.stdin)

    # Extract root node(s) - support raw node, API-style, and multi-frame
    frames = []
    if "document" in data:
        doc = data["document"]
        # Check if document has pages with multiple frames
        pages = doc.get("children", [])
        if pages and not doc.get("absoluteBoundingBox"):
            for page in pages:
                for child in page.get("children", []):
                    if child.get("type") == "FRAME":
                        frames.append(child)
        if not frames:
            frames = [doc]
    elif "absoluteBoundingBox" in data:
        frames = [data]
    elif "nodes" in data:
        for key in data["nodes"]:
            node = data["nodes"][key].get("document", data["nodes"][key])
            if node:
                frames.append(node)
    else:
        print("ERROR: Cannot find root node in JSON")
        sys.exit(1)

    # If --pages specified, use those names; otherwise auto-generate
    if args.pages:
        page_names = [p.strip() for p in args.pages.split(",")]
    elif len(frames) == 1:
        page_names = [args.page or "main_page"]
    else:
        page_names = []
        for f in frames:
            name = f.get("name", "page").lower().replace(" ", "_").replace("-", "_")
            name = re.sub(r'[^a-z0-9_]', '', name) or "page"
            page_names.append(name)

    # Process only the requested subset
    if len(frames) > len(page_names):
        frames = frames[:len(page_names)]
    elif len(page_names) > len(frames):
        page_names = page_names[:len(frames)]

    print(f"Found {len(frames)} frame(s): {', '.join(page_names)}")
    root_node = frames[0]

    # Get dimensions from first frame for scaffold
    bbox = root_node.get("absoluteBoundingBox", {})
    frame_w = int(bbox.get("width", 320))
    frame_h = int(bbox.get("height", 480))

    if args.target:
        target_w, target_h = map(int, args.target.lower().split("x"))
    else:
        target_w, target_h = frame_w, frame_h

    # Setup app directory
    root = _find_egui_root()
    app_name = args.app
    app_dir = os.path.join(root, "example", app_name)

    if not os.path.exists(app_dir):
        print(f"Creating app scaffold: {app_name}")
        scaffold_args = argparse.Namespace(
            app=app_name, width=target_w, height=target_h,
            color_depth=16, force=False
        )
        cmd_scaffold(scaffold_args)

    layout_dir = os.path.join(app_dir, ".eguiproject", "layout")
    os.makedirs(layout_dir, exist_ok=True)

    # Process each frame as a page
    for frame, page_name in zip(frames, page_names):
        print(f"\nConverting frame '{frame.get('name', page_name)}' -> {page_name}.xml")
        fb = frame.get("absoluteBoundingBox", {})
        fw = int(fb.get("width", 320))
        fh = int(fb.get("height", 480))
        sx = target_w / fw if fw > 0 else 1.0
        sy = target_h / fh if fh > 0 else 1.0
        scale = min(sx, sy)
        px, py = fb.get("x", 0), fb.get("y", 0)

        xml_lines = ['<?xml version="1.0" encoding="utf-8"?>', '<Page>']
        xml_lines.append(f'    <Group id="root" x="0" y="0" width="{target_w}" height="{target_h}">')

        for fill in frame.get("fills", []):
            if fill.get("visible", True) and fill.get("type") == "SOLID":
                bg_color = _figma_color_to_hex(fill.get("color"))
                if bg_color:
                    xml_lines.append(f'        <Background type="solid" color="EGUI_COLOR_HEX(0x{bg_color})" alpha="EGUI_ALPHA_100" />')
                break

        for child in frame.get("children", []):
            child_xml = _figma_node_to_xml(child, px, py, scale, indent=2)
            if child_xml:
                xml_lines.append(child_xml)

        xml_lines.append('    </Group>')
        xml_lines.append('</Page>')

        xml_path = os.path.join(layout_dir, f"{page_name}.xml")
        with open(xml_path, "w", encoding="utf-8", newline="\n") as f:
            f.write("\n".join(xml_lines))
        print(f"  Written: {xml_path}")

    print(f"\nNext steps:")
    print(f"  1. Review/edit XML: {xml_path}")
    print(f"  2. Generate code:   python scripts/html2egui_helper.py generate-code --app {app_name}")
    print(f"  3. Gen resources:   python scripts/html2egui_helper.py gen-resource --app {app_name}")
    print(f"  4. Build & verify:  python scripts/html2egui_helper.py verify --app {app_name}")


# ── Sub-command: figmamake-extract ────────────────────────────────

# Lucide icon name (PascalCase) -> Material Symbols name (snake_case)
_LUCIDE_TO_MATERIAL = {
    # Navigation
    "ArrowLeft": "arrow_back", "ArrowRight": "arrow_forward",
    "ArrowUp": "arrow_upward", "ArrowDown": "arrow_downward",
    "ChevronLeft": "chevron_left", "ChevronRight": "chevron_right",
    "ChevronDown": "expand_more", "ChevronUp": "expand_less",
    "MoveLeft": "arrow_back", "MoveRight": "arrow_forward",
    # Energy / Battery
    "Battery": "battery_full", "BatteryCharging": "battery_charging_full",
    "BatteryLow": "battery_low", "BatteryMedium": "battery_half",
    "BatteryFull": "battery_full", "BatteryWarning": "battery_alert",
    "Zap": "bolt", "ZapOff": "flash_off",
    "Plug": "power", "PlugZap": "electrical_services",
    # Temperature / Weather
    "ThermometerSun": "thermostat", "Thermometer": "thermostat",
    "Sun": "light_mode", "Moon": "dark_mode",
    "Cloud": "cloud", "CloudRain": "rainy", "CloudSun": "partly_cloudy_day",
    "Snowflake": "ac_unit", "Wind": "air", "Droplets": "water_drop",
    # Settings / System
    "Settings": "settings", "SlidersHorizontal": "tune",
    "Wifi": "wifi", "WifiOff": "wifi_off",
    "Bluetooth": "bluetooth", "BluetoothOff": "bluetooth_disabled",
    "Volume2": "volume_up", "Volume1": "volume_down",
    "VolumeX": "volume_off", "Volume": "volume_mute",
    "Info": "info", "HelpCircle": "help",
    "AlertTriangle": "warning", "AlertCircle": "error",
    "AlertOctagon": "report", "ShieldAlert": "gpp_maybe",
    # General actions
    "X": "close", "Search": "search", "Check": "check",
    "Plus": "add", "Minus": "remove", "PlusCircle": "add_circle",
    "Home": "home", "User": "person", "Users": "group",
    "Mail": "mail", "Phone": "phone", "MessageSquare": "chat",
    "Camera": "camera", "Image": "image",
    "Edit": "edit", "Edit2": "edit", "Edit3": "edit",
    "Trash": "delete", "Trash2": "delete",
    "Download": "download", "Upload": "upload",
    "Eye": "visibility", "EyeOff": "visibility_off",
    "Lock": "lock", "Unlock": "lock_open",
    "Clock": "schedule", "Timer": "timer",
    "Calendar": "calendar_today", "CalendarDays": "calendar_month",
    "Star": "star", "Heart": "favorite",
    "Bell": "notifications", "BellOff": "notifications_off",
    "Power": "power_settings_new", "PowerOff": "power_off",
    "MapPin": "location_on", "Map": "map", "Navigation": "navigation",
    "RefreshCw": "refresh", "RefreshCcw": "refresh",
    "RotateCcw": "undo", "RotateCw": "redo",
    "Copy": "content_copy", "Clipboard": "assignment",
    "FileText": "description", "File": "insert_drive_file",
    "Folder": "folder", "FolderOpen": "folder_open",
    "GripVertical": "drag_indicator", "Menu": "menu",
    "MoreHorizontal": "more_horiz", "MoreVertical": "more_vert",
    "Circle": "circle", "Square": "square",
    "ToggleLeft": "toggle_off", "ToggleRight": "toggle_on",
    "Activity": "monitoring", "BarChart": "bar_chart",
    "PieChart": "pie_chart", "TrendingUp": "trending_up",
    "TrendingDown": "trending_down", "Gauge": "speed",
    "Cpu": "memory", "HardDrive": "storage",
    "Monitor": "desktop_windows", "Smartphone": "smartphone",
    "Laptop": "laptop", "Tablet": "tablet",
    "Signal": "signal_cellular_alt", "Radio": "radio",
    "Save": "save", "Share": "share", "ExternalLink": "open_in_new",
    "Link": "link", "Unlink": "link_off",
    "Filter": "filter_list", "SortAsc": "arrow_upward",
    "List": "list", "LayoutGrid": "grid_view",
    "Maximize": "fullscreen", "Minimize": "fullscreen_exit",
    "LogIn": "login", "LogOut": "logout",
}


def _lucide_name_to_material(lucide_name):
    """Map a Lucide icon name to Material Symbols name.

    Falls back to PascalCase -> snake_case heuristic.
    Returns (material_name, was_mapped).
    """
    if lucide_name in _LUCIDE_TO_MATERIAL:
        return _LUCIDE_TO_MATERIAL[lucide_name], True
    # Fallback: PascalCase to snake_case
    snake = re.sub(r'(?<!^)(?=[A-Z])', '_', lucide_name).lower()
    return snake, False


def _extract_lucide_imports(tsx_content):
    """Extract Lucide React icon imports from a TSX file.

    Handles: import { Battery, Zap as ZapIcon } from "lucide-react";
    Returns dict: {local_name: lucide_name}
    """
    imports = {}
    for m in re.finditer(r'import\s*\{([^}]+)\}\s*from\s*["\']lucide-react["\']', tsx_content):
        items = m.group(1).split(",")
        for item in items:
            item = item.strip()
            if not item:
                continue
            # Handle "Name as Alias"
            as_match = re.match(r'(\w+)\s+as\s+(\w+)', item)
            if as_match:
                imports[as_match.group(2)] = as_match.group(1)
            else:
                imports[item] = item
    return imports


def _extract_jsx_return(tsx_content, component_name=None):
    """Extract the JSX block from a component's return statement.

    If *component_name* is given, locate the exported component first so that
    sub-component return blocks are skipped.
    """
    search_in = tsx_content
    offset = 0

    if component_name:
        # Try to locate the exported component function body
        patterns = [
            rf'export\s+(?:const|function)\s+{re.escape(component_name)}\b[^{{]*\{{',
            rf'(?:const|function)\s+{re.escape(component_name)}\b[^{{]*\{{',
        ]
        for pat in patterns:
            m = re.search(pat, tsx_content)
            if m:
                # Find the function body by brace-balancing from the opening {
                brace_start = m.end() - 1  # position of '{'
                depth = 1
                j = brace_start + 1
                in_str = None
                while j < len(tsx_content) and depth > 0:
                    ch = tsx_content[j]
                    if in_str:
                        if ch == '\\' and j + 1 < len(tsx_content):
                            j += 2
                            continue
                        if ch == in_str:
                            in_str = None
                    else:
                        if ch in ('"', "'", '`'):
                            in_str = ch
                        elif ch == '{':
                            depth += 1
                        elif ch == '}':
                            depth -= 1
                    j += 1
                search_in = tsx_content[brace_start:j]
                offset = brace_start
                break

    # Find the return statement inside the component function
    idx = search_in.find("return (")
    if idx == -1:
        # Try return <...> without parens
        m = re.search(r'\breturn\s+(<)', search_in)
        if not m:
            return ""
        start = offset + m.start(1)
        # Find end by matching angle brackets
        depth = 0
        i = start
        while i < len(tsx_content):
            if tsx_content[i] == '<':
                depth += 1
            elif tsx_content[i] == '>':
                depth -= 1
                if depth == 0:
                    return tsx_content[start:i + 1]
            i += 1
        return tsx_content[start:]

    # Balance parentheses from "return ("
    start = offset + idx + len("return (")
    depth = 1
    i = start
    in_string = None
    in_template = False
    while i < len(tsx_content) and depth > 0:
        ch = tsx_content[i]
        if in_string:
            if ch == '\\':
                i += 2
                continue
            if ch == in_string:
                in_string = None
        elif in_template:
            if ch == '\\':
                i += 2
                continue
            if ch == '`':
                in_template = False
        else:
            if ch in ('"', "'"):
                in_string = ch
            elif ch == '`':
                in_template = True
            elif ch == '(':
                depth += 1
            elif ch == ')':
                depth -= 1
        i += 1
    return tsx_content[start:i - 1].strip()


def _extract_jsx_comments(jsx_text):
    """Extract JSX comments {/* text */} and return (cleaned_jsx, comments_list)."""
    comments = []
    def _collect(m):
        comments.append(m.group(1).strip())
        return ""
    cleaned = re.sub(r'\{/\*\s*(.+?)\s*\*/\}', _collect, jsx_text, flags=re.DOTALL)
    return cleaned, comments


def _jsx_to_pseudo_html(jsx_text, lucide_imports):
    """Convert JSX template to pseudo-HTML parseable by HTMLParser.

    Transforms:
    - className -> class
    - Lucide icon components -> <span class="lucide-icon ..." data-icon="name">
    - Third-party components -> <div data-component="Name">
    - Template literal classNames -> static classes
    - Dynamic expressions -> placeholder text
    - Event handlers -> removed
    - style={{ ... }} -> removed
    """
    html = jsx_text

    # 0. Extract navigate() targets from onClick before stripping events
    #    onClick={() => navigate('/path')} -> data-navigate="/path"
    html = re.sub(
        r"""\bonClick=\{[^}]*navigate\(\s*['"]([^'"]+)['"]\s*\)[^}]*\}""",
        r'data-navigate="\1"',
        html,
    )

    # 1. Strip remaining event handlers: onClick={...}, onChange={...}, onXxx={...}
    html = re.sub(r'\bon[A-Z]\w*=\{[^}]*\}', '', html)
    # Multi-line event handlers with arrow functions
    html = re.sub(r'\bon[A-Z]\w*=\{[^}]*?\{[^}]*\}[^}]*\}', '', html, flags=re.DOTALL)

    # 2. Strip style={{ ... }} attributes
    html = re.sub(r'\bstyle=\{\{[^}]*\}\}', '', html, flags=re.DOTALL)

    # 3. Handle template literal className:
    #    className={`static ${cond ? 'a' : 'b'}`}
    #    -> class="static a"
    def _resolve_template_classname(m):
        template = m.group(1)
        # Remove ${...} expressions, keeping first branch of ternary
        def _resolve_expr(em):
            expr = em.group(1)
            # Ternary: cond ? 'val1' : 'val2' -> val1
            tern = re.search(r"\?\s*'([^']*)'", expr)
            if tern:
                return tern.group(1)
            tern = re.search(r'\?\s*"([^"]*)"', expr)
            if tern:
                return tern.group(1)
            return ""
        resolved = re.sub(r'\$\{([^}]+)\}', _resolve_expr, template)
        resolved = resolved.strip()
        return f'class="{resolved}"'
    html = re.sub(r'className=\{`([^`]*)`\}', _resolve_template_classname, html, flags=re.DOTALL)

    # 4. className="..." -> class="..."
    html = html.replace('className=', 'class=')

    # 5. Convert Lucide icon components to spans
    #    <Battery className="w-5 h-5 text-cyan-400" /> -> <span class="lucide-icon ..." data-icon="battery_full"></span>
    for local_name, lucide_name in lucide_imports.items():
        material_name, _ = _lucide_name_to_material(lucide_name)
        # Self-closing: <Battery className="..." />
        pattern = rf'<{local_name}\s+class="([^"]*)"(?:\s*/\s*>|/>)'
        replacement = f'<span class="lucide-icon \\1" data-icon="{material_name}" data-lucide="{lucide_name}"></span>'
        html = re.sub(pattern, replacement, html)
        # Without class: <Battery />
        pattern = rf'<{local_name}\s*/>'
        replacement = f'<span class="lucide-icon" data-icon="{material_name}" data-lucide="{lucide_name}"></span>'
        html = re.sub(pattern, replacement, html)

    # 6. Convert third-party React components (PascalCase tags) to divs
    #    <ResponsiveContainer width="100%" height={50}> -> <div data-component="ResponsiveContainer">
    #    Must skip already-handled Lucide icons (now <span>)
    # Self-closing FIRST (before opening tags, to avoid <Comp .../> matching as open tag)
    html = re.sub(r'<([A-Z][A-Za-z0-9]+)(?:\s+[^>]*)?\s*/>', lambda m: f'<div data-component="{m.group(1)}"></div>', html)
    # Opening tags (use negative lookbehind to skip self-closing already handled)
    def _replace_component_open(m):
        name = m.group(1)
        attrs_str = m.group(2) or ""
        # Extract class if present
        cls_match = re.search(r'class="([^"]*)"', attrs_str)
        cls = cls_match.group(1) if cls_match else ""
        return f'<div data-component="{name}" class="{cls}">'
    html = re.sub(r'<([A-Z][A-Za-z0-9]+)((?:\s+[^>]*)?)(?<!/)>', _replace_component_open, html)
    # Closing tags: </Component> -> </div>
    html = re.sub(r'</([A-Z][A-Za-z0-9]+)>', '</div>', html)

    # 7. Handle .map() expressions: {arr.map(item => (<div>...</div>))}
    #    Extract the template JSX inside
    def _replace_map(m):
        inner = m.group(1)
        # Find the JSX part: everything after => (  or => <
        arrow_idx = inner.find("=>")
        if arrow_idx < 0:
            return ""
        jsx_part = inner[arrow_idx + 2:].strip()
        # Strip wrapping parens
        if jsx_part.startswith("("):
            jsx_part = jsx_part[1:]
            if jsx_part.endswith(")"):
                jsx_part = jsx_part[:-1]
        return jsx_part.strip()
    # Match {expr.map(... => (...))} - use brace balancing
    map_pattern = re.compile(r'\{[^}]*\.map\(', re.DOTALL)
    pos = 0
    result_parts = []
    while pos < len(html):
        mm = map_pattern.search(html, pos)
        if not mm:
            result_parts.append(html[pos:])
            break
        result_parts.append(html[pos:mm.start()])
        # Balance braces from mm.start()
        brace_depth = 0
        i = mm.start()
        while i < len(html):
            if html[i] == '{':
                brace_depth += 1
            elif html[i] == '}':
                brace_depth -= 1
                if brace_depth == 0:
                    map_block = html[mm.start() + 1:i]  # content inside { }
                    replaced = _replace_map(re.match(r'(.*)', map_block, re.DOTALL))
                    result_parts.append(replaced)
                    pos = i + 1
                    break
            i += 1
        else:
            result_parts.append(html[mm.start():])
            break
    html = "".join(result_parts)

    # 8. Handle conditional expressions: {cond && <Element .../>}
    #    Keep the element part
    def _replace_conditional(m):
        inner = m.group(1)
        # Find the JSX part after &&
        amp_idx = inner.find("&&")
        if amp_idx < 0:
            return ""
        jsx_part = inner[amp_idx + 2:].strip()
        # Strip wrapping parens
        if jsx_part.startswith("("):
            jsx_part = jsx_part[1:]
            if jsx_part.endswith(")"):
                jsx_part = jsx_part[:-1]
        return jsx_part.strip()
    html = re.sub(r'\{([^}]*&&[^}]*)\}', _replace_conditional, html)

    # 9. Replace remaining {expression} with placeholder text
    html = re.sub(r'\{([^{}]+)\}', lambda m: m.group(1).split(".")[-1].strip('"\'() ') or "__VAR__", html)

    # 10. Strip key="..." and other React-only attributes
    html = re.sub(r'\bkey=\{[^}]*\}', '', html)
    html = re.sub(r'\bref=\{[^}]*\}', '', html)

    # 11. Convert <></> (fragments) to <div></div>
    html = html.replace("<>", "<div>").replace("</>", "</div>")

    # 12. Remove SVG definition blocks (<defs>...</defs>) — not layout-relevant
    html = re.sub(r'<defs>.*?</defs>', '', html, flags=re.DOTALL)

    # 13. Clean up extra whitespace in tags
    html = re.sub(r'<(\w+)\s{2,}', r'<\1 ', html)
    html = re.sub(r'\s+/>', ' />', html)
    html = re.sub(r'\s+>', '>', html)

    return html


def _discover_figmamake_pages(project_dir):
    """Discover page components from a Figma Make project.

    Parses routes.ts (or routes.tsx) to find page components and their routes.
    Also reads Root component for screen dimensions.

    Returns dict: {
        "root_file": str,
        "screen": {"width": int, "height": int},
        "root_bg_color": str or None,
        "pages": [{"name": str, "path": str, "file": str}, ...]
    }
    """
    # Find routes file
    routes_candidates = [
        os.path.join(project_dir, "src", "app", "routes.ts"),
        os.path.join(project_dir, "src", "app", "routes.tsx"),
        os.path.join(project_dir, "src", "routes.ts"),
        os.path.join(project_dir, "src", "routes.tsx"),
    ]
    routes_file = None
    for candidate in routes_candidates:
        if os.path.isfile(candidate):
            routes_file = candidate
            break

    if not routes_file:
        print("WARNING: No routes.ts found. Scanning for component files.", file=sys.stderr)
        # Fallback: scan for TSX files in components directory
        comp_dir = os.path.join(project_dir, "src", "app", "components")
        if not os.path.isdir(comp_dir):
            comp_dir = os.path.join(project_dir, "src", "components")
        pages = []
        if os.path.isdir(comp_dir):
            for fname in sorted(os.listdir(comp_dir)):
                if fname.endswith(".tsx") and not fname.startswith("_") and fname != "Root.tsx":
                    name = fname.replace(".tsx", "")
                    pages.append({"name": name, "path": "/" + name.lower(), "file": os.path.join(comp_dir, fname)})
        return {"root_file": None, "screen": {"width": 320, "height": 480}, "root_bg_color": None, "pages": pages}

    routes_dir = os.path.dirname(routes_file)
    with open(routes_file, "r", encoding="utf-8") as f:
        routes_content = f.read()

    # Extract imports: default and named
    import_map = {}  # ComponentName -> file path
    # Default imports: import Root from "./components/Root"
    for m in re.finditer(r'import\s+(\w+)\s+from\s+["\']([^"\']+)["\']', routes_content):
        comp_name = m.group(1)
        rel_path = m.group(2)
        abs_path = os.path.normpath(os.path.join(routes_dir, rel_path))
        for ext in (".tsx", ".ts", ".jsx", ".js", ""):
            candidate = abs_path + ext
            if os.path.isfile(candidate):
                import_map[comp_name] = candidate
                break
        if comp_name not in import_map:
            import_map[comp_name] = abs_path + ".tsx"
    # Named imports: import { Dashboard } from "./components/Dashboard"
    for m in re.finditer(r'import\s+\{\s*([\w\s,]+)\s*\}\s+from\s+["\']([^"\']+)["\']', routes_content):
        names = [n.strip() for n in m.group(1).split(",") if n.strip()]
        rel_path = m.group(2)
        abs_path = os.path.normpath(os.path.join(routes_dir, rel_path))
        resolved = None
        for ext in (".tsx", ".ts", ".jsx", ".js", ""):
            candidate = abs_path + ext
            if os.path.isfile(candidate):
                resolved = candidate
                break
        if not resolved:
            resolved = abs_path + ".tsx"
        for comp_name in names:
            if comp_name not in import_map:
                import_map[comp_name] = resolved

    # Find Root component (contains <Outlet/>)
    root_file = None
    screen = {"width": 320, "height": 480}
    root_bg_color = None
    for comp_name, comp_file in import_map.items():
        if comp_name.lower() == "root" or (os.path.isfile(comp_file) and "Outlet" in open(comp_file, "r", encoding="utf-8").read()):
            root_file = comp_file
            if os.path.isfile(comp_file):
                root_content = open(comp_file, "r", encoding="utf-8").read()
                w_match = re.search(r'w-\[(\d+)px\]', root_content)
                h_match = re.search(r'h-\[(\d+)px\]', root_content)
                if w_match:
                    screen["width"] = int(w_match.group(1))
                if h_match:
                    screen["height"] = int(h_match.group(1))
                bg_match = re.search(r'bg-\[#([0-9a-fA-F]{6})\]', root_content)
                if bg_match:
                    root_bg_color = bg_match.group(1).upper()
            break

    # Extract route definitions
    pages = []
    # Pattern: { index: true, Component: Home }
    index_match = re.search(r'\{\s*index\s*:\s*true\s*,\s*Component\s*:\s*(\w+)', routes_content)
    if index_match:
        comp_name = index_match.group(1)
        if comp_name in import_map:
            pages.append({"name": comp_name, "path": "/", "file": import_map[comp_name]})

    # Find the root component name to exclude from pages
    root_comp_name = None
    if root_file:
        for cn, cf in import_map.items():
            if os.path.normpath(cf) == os.path.normpath(root_file):
                root_comp_name = cn
                break

    # Pattern: { path: "battery", Component: BatteryDetail }
    for m in re.finditer(r'\{\s*path\s*:\s*["\']([^"\']+)["\']\s*,\s*Component\s*:\s*(\w+)', routes_content):
        route_path = "/" + m.group(1)
        comp_name = m.group(2)
        if comp_name == root_comp_name:
            continue  # Skip root layout component
        if comp_name in import_map and comp_name not in [p["name"] for p in pages]:
            pages.append({"name": comp_name, "path": route_path, "file": import_map[comp_name]})

    return {"root_file": root_file, "screen": screen, "root_bg_color": root_bg_color, "pages": pages}


def _extract_figmamake_text_chars(tsx_content):
    """Extract unique non-ASCII characters from TSX file for font generation."""
    # Find text content: string literals and JSX text
    chars = set()
    # JSX text content: >text<  (between tags)
    for m in re.finditer(r'>([^<{]+)<', tsx_content):
        for ch in m.group(1):
            if ord(ch) > 127:
                chars.add(ch)
    # String literals: "text" or 'text'
    for m in re.finditer(r'["\']([^"\']+)["\']', tsx_content):
        for ch in m.group(1):
            if ord(ch) > 127:
                chars.add(ch)
    return chars


def cmd_figmamake_extract(args):
    """Extract layout from a Figma Make JSX/TSX project."""
    project_dir = args.input
    if not os.path.isdir(project_dir):
        print(f"ERROR: Directory not found: {project_dir}", file=sys.stderr)
        sys.exit(1)

    # Discover project structure
    discovery = _discover_figmamake_pages(project_dir)

    if not discovery["pages"]:
        print("ERROR: No page components found.", file=sys.stderr)
        sys.exit(1)

    # --list-pages mode
    if args.list_pages:
        print("Discovered pages:")
        for p in discovery["pages"]:
            print(f"  {p['name']:20s}  route: {p['path']:15s}  file: {p['file']}")
        print(f"\nScreen: {discovery['screen']['width']}x{discovery['screen']['height']}")
        if discovery["root_bg_color"]:
            print(f"Root background: #{discovery['root_bg_color']}")
        return

    # Filter pages if --page specified
    selected_pages = discovery["pages"]
    if args.page:
        selected_pages = [p for p in discovery["pages"] if p["name"] == args.page]
        if not selected_pages:
            names = ", ".join(p["name"] for p in discovery["pages"])
            print(f"ERROR: Page '{args.page}' not found. Available: {names}", file=sys.stderr)
            sys.exit(1)

    # Process each page
    all_colors = {}
    all_icons = []
    all_text_chars = set()
    pages_result = []
    seen_icon_materials = set()

    for page_info in selected_pages:
        page_file = page_info["file"]
        if not os.path.isfile(page_file):
            print(f"WARNING: File not found: {page_file}", file=sys.stderr)
            continue

        with open(page_file, "r", encoding="utf-8") as f:
            tsx_content = f.read()

        # Extract Lucide imports
        lucide_imports = _extract_lucide_imports(tsx_content)

        # Collect icon info from usage in JSX
        for local_name, lucide_name in lucide_imports.items():
            material_name, was_mapped = _lucide_name_to_material(lucide_name)
            if material_name in seen_icon_materials:
                continue
            seen_icon_materials.add(material_name)
            # Find usage to extract color and size
            usage_match = re.search(rf'<{local_name}\s+class(?:Name)?="([^"]*)"', tsx_content)
            icon_info = {
                "lucide": lucide_name,
                "material": material_name,
                "mapped": was_mapped,
            }
            if usage_match:
                icon_cls = usage_match.group(1)
                color_info = _extract_tw_color(icon_cls)
                if color_info:
                    icon_info["color"] = color_info.get("name") if isinstance(color_info, dict) else color_info
                # Size from w-N h-N
                size_match = re.search(r'w-(\d+)\s+h-\d+', icon_cls)
                if size_match:
                    icon_info["size_px"] = _tw_to_px(size_match.group(1))
                size_match2 = re.search(r'w-\[(\d+)px\]', icon_cls)
                if size_match2:
                    icon_info["size_px"] = int(size_match2.group(1))
            all_icons.append(icon_info)

        # Extract JSX template
        jsx_text = _extract_jsx_return(tsx_content)
        if not jsx_text:
            print(f"WARNING: No JSX return found in {page_file}", file=sys.stderr)
            continue

        # Extract comments before preprocessing
        jsx_text, comments = _extract_jsx_comments(jsx_text)

        # Preprocess to pseudo-HTML
        pseudo_html = _jsx_to_pseudo_html(jsx_text, lucide_imports)

        # Parse with HTML parser
        parser = _LayoutHTMLParser()
        try:
            parser.feed(pseudo_html)
        except Exception as e:
            print(f"WARNING: HTML parse error in {page_file}: {e}", file=sys.stderr)
            continue

        # Find the root element (the outermost div returned by the component)
        root_node = parser._root
        if not root_node:
            # Stack may have remaining items due to SVG void elements (<stop/>, <br/> etc.)
            # that HTMLParser treats as open tags. Use bottom of stack as root.
            if parser._stack:
                root_node = parser._stack[0]
            else:
                continue

        # For Figma Make, the root element IS the page container (no need to search for main)
        # Mark it as main if it has w-full h-full pattern
        main_node = root_node

        # Process sections (direct children of root)
        sections = []
        comment_idx = 0
        for i, child in enumerate(main_node.get("children", [])):
            if child.get("tag") in ("style", "script", "link"):
                continue
            comment = ""
            if comment_idx < len(comments):
                comment = comments[comment_idx]
                comment_idx += 1
            section = _node_to_section(child, i, prev_comment=comment)
            sections.append(section)

        # Collect colors used in this page
        _collect_colors_from_sections(sections, all_colors)

        # Collect text characters
        all_text_chars.update(_extract_figmamake_text_chars(tsx_content))

        pages_result.append({
            "name": page_info["name"],
            "file": os.path.relpath(page_info["file"], project_dir).replace("\\", "/"),
            "route": page_info["path"],
            "sections": sections,
        })

    # Build navigation map from navigate_to fields
    navigation = []
    for page in pages_result:
        route = page.get("route", "")
        _collect_navigation(page.get("sections", []), route, navigation)

    # Build result
    result = {
        "source": "figma-make",
        "screen": discovery["screen"],
        "colors": all_colors,
        "icons": all_icons,
        "pages": pages_result,
        "navigation": navigation,
        "text_chars": "".join(sorted(all_text_chars)),
    }
    if discovery["root_bg_color"]:
        result["root_bg_color"] = discovery["root_bg_color"]

    # For backward compatibility: if single page, also emit top-level sections
    if len(pages_result) == 1:
        result["sections"] = pages_result[0]["sections"]

    output_json = json.dumps(result, indent=2, ensure_ascii=False)
    if args.output:
        with open(args.output, "w", encoding="utf-8", newline="\n") as f:
            f.write(output_json)
        print(f"Figma Make layout analysis written to: {args.output}", file=sys.stderr)
    else:
        sys.stdout.buffer.write(output_json.encode("utf-8"))
        sys.stdout.buffer.write(b"\n")


def _collect_colors_from_sections(sections, colors_dict):
    """Recursively collect Tailwind color names used in sections."""
    for section in sections:
        cls = section.get("classes", "")
        # Collect text colors
        for m in re.finditer(r'text-((?:[a-z]+-)?[a-z]+-\d{2,3})', cls):
            name = m.group(1)
            if name in _TAILWIND_COLORS:
                colors_dict[name] = _TAILWIND_COLORS[name]
        # Collect bg colors
        for m in re.finditer(r'bg-((?:[a-z]+-)?[a-z]+-\d{2,3})', cls):
            name = m.group(1)
            if name in _TAILWIND_COLORS:
                colors_dict[name] = _TAILWIND_COLORS[name]
        # Collect border colors
        for m in re.finditer(r'border-((?:[a-z]+-)?[a-z]+-\d{2,3})', cls):
            name = m.group(1)
            if name in _TAILWIND_COLORS:
                colors_dict[name] = _TAILWIND_COLORS[name]
        # Inline hex
        for m in re.finditer(r'(?:bg|text|border)-\[#([0-9a-fA-F]{6})\]', cls):
            hex_val = m.group(1).upper()
            colors_dict[f"#{hex_val}"] = hex_val
        # Recurse into children
        for child in section.get("children", []):
            if isinstance(child, dict):
                _collect_colors_from_sections([child], colors_dict)


def _collect_navigation(sections, from_route, navigation):
    """Recursively collect navigate_to entries from sections/children."""
    for section in sections:
        nav_to = section.get("navigate_to")
        if nav_to:
            navigation.append({
                "from": from_route,
                "to": nav_to,
                "trigger": section.get("id", section.get("type", "unknown")),
            })
        for child in section.get("children", []):
            if isinstance(child, dict):
                _collect_navigation([child], from_route, navigation)


# ── CLI ───────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(
        description="HTML to EmbeddedGUI conversion helper",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Sub-commands:
  scaffold        Create UI Designer project structure (.egui + XML layout)
  export-icons    Extract Material Symbols from HTML and render as PNG
  export-svgs     Extract inline SVGs from HTML and convert to PNG (via CairoSVG)
  extract-text    Extract unique characters from HTML for font generation
  gen-resource    Generate C resource files from PNG/TTF sources
  extract-layout  Parse HTML and output structured JSON layout analysis
  generate-code   Generate C code from UI Designer XML project
  verify          Build + runtime check in one step
  figma2xml       Convert Figma design to EGUI XML layout via REST API
  figmamake-extract  Extract layout from Figma Make JSX/TSX project

See .claude/skills/html-to-egui.md for the full conversion workflow.
""")

    sub = parser.add_subparsers(dest="command", help="Sub-command")

    # scaffold
    p_scaffold = sub.add_parser("scaffold", help="Create UI Designer project scaffold")
    p_scaffold.add_argument("--app", "-n", required=True, help="App name (e.g., HelloSmartHome)")
    p_scaffold.add_argument("--width", "-W", type=int, default=320, help="Screen width (default: 320)")
    p_scaffold.add_argument("--height", "-H", type=int, default=480, help="Screen height (default: 480)")
    p_scaffold.add_argument("--color-depth", type=int, default=16, choices=[16, 32], help="Color depth (default: 16)")
    p_scaffold.add_argument("--force", action="store_true", help="Overwrite template files (layout XML preserved)")
    p_scaffold.add_argument("--pages", default=None, help="Comma-separated page names (default: main_page)")

    # export-icons
    p_icons = sub.add_parser("export-icons", help="Export Material Symbols icons from HTML")
    p_icons.add_argument("--input", "-i", required=True, help="Input HTML file path")
    p_icons.add_argument("--output", "-o", default=None, help="Output directory for PNG files (default: auto from --app)")
    p_icons.add_argument("--app", "-n", default=None, help="App name (auto-sets output to .eguiproject/resources/images/)")
    p_icons.add_argument("--size", "-s", type=int, default=24, help="Icon size in pixels (default: 24)")
    p_icons.add_argument("--color", "-c", default="FFFFFF", help="Icon color hex (default: FFFFFF = white)")
    p_icons.add_argument("--font", "-f", default=None, help="Path to MaterialSymbolsOutlined TTF file")
    p_icons.add_argument("--suffix", default="", help="Suffix for icon filenames (e.g., _16 for 16px variants)")
    p_icons.add_argument("--auto-color", action="store_true", help="Auto-detect per-icon colors from HTML Tailwind classes")
    p_icons.add_argument("--filled", action="store_true", help="Force all icons to use filled (solid) style (FILL=1)")
    p_icons.add_argument("--image-format", default="alpha", help="Image format for resource config (alpha, rgb565, rgb32, gray8; default: alpha)")

    # export-svgs
    p_svgs = sub.add_parser("export-svgs", help="Extract inline SVGs from HTML and convert to PNG")
    p_svgs.add_argument("--input", "-i", required=True, help="Input HTML file path")
    p_svgs.add_argument("--output", "-o", default=None, help="Output directory for PNG files (default: auto from --app)")
    p_svgs.add_argument("--app", "-n", default=None, help="App name (auto-sets output to .eguiproject/resources/images/)")
    p_svgs.add_argument("--size", "-s", type=int, default=64, help="Output PNG size in pixels (default: 64)")
    p_svgs.add_argument("--prefix", default="svg_", help="Filename prefix (default: svg_)")
    p_svgs.add_argument("--image-format", default="rgb565", help="Image format for resource config (default: rgb565)")

    # extract-text
    p_text = sub.add_parser("extract-text", help="Extract unique characters from HTML for font generation")
    p_text.add_argument("--input", "-i", required=True, help="Input HTML file path")
    p_text.add_argument("--output", "-o", default=None, help="Output .txt file path (default: auto from --app or stdout)")
    p_text.add_argument("--app", "-n", default=None, help="App name (auto-saves to resource/src/supported_text.txt)")

    # gen-resource
    p_resource = sub.add_parser("gen-resource", help="Generate C resource files")
    p_resource.add_argument("--app", "-n", required=True, help="App name")

    # extract-layout
    p_layout = sub.add_parser("extract-layout", help="Parse HTML and output structured JSON layout")
    p_layout.add_argument("--input", "-i", required=True, help="Input HTML file path")
    p_layout.add_argument("--output", "-o", default=None, help="Output JSON file (default: stdout)")

    # generate-code
    p_gencode = sub.add_parser("generate-code", help="Generate C code from UI Designer XML project")
    p_gencode.add_argument("--app", "-n", required=True, help="App name")

    # verify
    p_verify = sub.add_parser("verify", help="Build + runtime check in one step")
    p_verify.add_argument("--app", "-n", required=True, help="App name")
    p_verify.add_argument("--bits64", action="store_true", default=True, help="Use 64-bit build (default: true)")
    p_verify.add_argument("--no-clean", action="store_true", help="Skip clean step")
    p_verify.add_argument("--timeout", type=int, default=None, help="Runtime check timeout in seconds")
    p_verify.add_argument("--compare-design", default=None, help="Design screenshot path for visual comparison")

    # figma2xml
    p_figma = sub.add_parser("figma2xml", help="Convert Figma design to EGUI XML layout via REST API")
    p_figma.add_argument("--url", "-u", default=None, help="Figma design URL (extracts file_key + node_id)")
    p_figma.add_argument("--file-key", "-f", default=None, help="Figma file key (alternative to --url)")
    p_figma.add_argument("--node-id", default=None, help="Figma node ID (use with --file-key)")
    p_figma.add_argument("--token", "-t", default=None, help="Figma personal access token (or set FIGMA_TOKEN env var)")
    p_figma.add_argument("--app", "-n", required=True, help="App name")
    p_figma.add_argument("--target", default=None, help="Target resolution WxH (e.g., 320x480; default: use frame size)")
    p_figma.add_argument("--page", default="main_page", help="Page name (default: main_page)")
    p_figma.add_argument("--no-export", action="store_true", help="Skip exporting vector nodes as PNG")

    # figma-mcp
    p_fmcp = sub.add_parser("figma-mcp", help="Convert Figma MCP JSON to EGUI XML layout (no REST API)")
    p_fmcp.add_argument("--input", "-i", default=None, help="Input JSON file (default: stdin)")
    p_fmcp.add_argument("--app", "-n", required=True, help="App name")
    p_fmcp.add_argument("--target", default=None, help="Target resolution WxH (e.g., 320x480)")
    p_fmcp.add_argument("--page", default="main_page", help="Page name (default: main_page)")
    p_fmcp.add_argument("--pages", default=None, help="Comma-separated page names for multi-frame (e.g., main_page,settings)")

    # figmamake-extract
    p_fmextract = sub.add_parser("figmamake-extract", help="Extract layout from Figma Make JSX/TSX project")
    p_fmextract.add_argument("--input", "-i", required=True, help="Figma Make project directory")
    p_fmextract.add_argument("--page", "-p", default=None, help="Page component name to extract (default: all)")
    p_fmextract.add_argument("--list-pages", action="store_true", help="List available pages and exit")
    p_fmextract.add_argument("--output", "-o", default=None, help="Output JSON file (default: stdout)")

    args = parser.parse_args()

    if args.command == "scaffold":
        cmd_scaffold(args)
    elif args.command == "export-icons":
        cmd_export_icons(args)
    elif args.command == "export-svgs":
        cmd_export_svgs(args)
    elif args.command == "extract-text":
        cmd_extract_text(args)
    elif args.command == "gen-resource":
        cmd_gen_resource(args)
    elif args.command == "extract-layout":
        cmd_extract_layout(args)
    elif args.command == "generate-code":
        cmd_generate_code(args)
    elif args.command == "verify":
        cmd_verify(args)
    elif args.command == "figma2xml":
        cmd_figma2xml(args)
    elif args.command == "figma-mcp":
        cmd_figma_mcp(args)
    elif args.command == "figmamake-extract":
        cmd_figmamake_extract(args)
    else:
        parser.print_help()
        sys.exit(1)


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Stage 2b: Extract framer-motion animations from TSX and map to EGUI animation types.

Parses motion.* components and animate/initial/transition props from Figma Make TSX files.
Outputs animation descriptions that can be merged into XML layout generation.

Usage:
    python scripts/figmamake/figmamake_anim_extractor.py \
        --project-dir /path/to/figmamake/project \
        --output animations.json
"""

import argparse
import json
import os
import re
import sys
from pathlib import Path

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPTS_DIR = os.path.dirname(SCRIPT_DIR)
sys.path.insert(0, SCRIPT_DIR)
sys.path.insert(0, SCRIPTS_DIR)

from html2egui_helper import _discover_figmamake_pages


# framer-motion property -> EGUI animation type mapping
_MOTION_TO_EGUI = {
    "opacity": {
        "type": "alpha",
        "param_map": lambda from_v, to_v: {
            "from_alpha": f"EGUI_ALPHA_{int(float(from_v) * 100)}" if float(from_v) <= 1 else str(from_v),
            "to_alpha": f"EGUI_ALPHA_{int(float(to_v) * 100)}" if float(to_v) <= 1 else str(to_v),
        },
    },
    "x": {
        "type": "translate",
        "param_map": lambda from_v, to_v: {
            "from_x": str(int(float(from_v))),
            "to_x": str(int(float(to_v))),
            "from_y": "0",
            "to_y": "0",
        },
    },
    "y": {
        "type": "translate",
        "param_map": lambda from_v, to_v: {
            "from_x": "0",
            "to_x": "0",
            "from_y": str(int(float(from_v))),
            "to_y": str(int(float(to_v))),
        },
    },
    "scale": {
        "type": "scale",
        "param_map": lambda from_v, to_v: {
            "from_scale": f"EGUI_FLOAT_VALUE({from_v})",
            "to_scale": f"EGUI_FLOAT_VALUE({to_v})",
        },
    },
    "width": {
        "type": "resize",
        "param_map": lambda from_v, to_v: {
            "from_width_ratio": _width_to_ratio(from_v),
            "to_width_ratio": _width_to_ratio(to_v),
            "from_height_ratio": "EGUI_FLOAT_VALUE(1.0)",
            "to_height_ratio": "EGUI_FLOAT_VALUE(1.0)",
            "mode": "EGUI_ANIMATION_RESIZE_MODE_LEFT",
        },
    },
    "height": {
        "type": "resize",
        "param_map": lambda from_v, to_v: {
            "from_width_ratio": "EGUI_FLOAT_VALUE(1.0)",
            "to_width_ratio": "EGUI_FLOAT_VALUE(1.0)",
            "from_height_ratio": _width_to_ratio(from_v),
            "to_height_ratio": _width_to_ratio(to_v),
            "mode": "EGUI_ANIMATION_RESIZE_MODE_LEFT",
        },
    },
}

# Transition type -> EGUI interpolator
_TRANSITION_TO_INTERPOLATOR = {
    "spring": "overshoot",
    "tween": "decelerate",
    "inertia": "decelerate",
}

# Easing name -> EGUI interpolator
_EASING_TO_INTERPOLATOR = {
    "easeIn": "accelerate",
    "easeOut": "decelerate",
    "easeInOut": "accelerate_decelerate",
    "linear": "linear",
    "anticipate": "anticipate",
    "backIn": "anticipate",
    "backOut": "overshoot",
    "backInOut": "anticipate_overshoot",
    "circIn": "accelerate",
    "circOut": "decelerate",
}


def _width_to_ratio(val: str) -> str:
    """Convert width/height value to EGUI float ratio."""
    val = str(val).strip().strip('"').strip("'")
    if val.endswith("%"):
        pct = float(val.rstrip("%")) / 100.0
        return f"EGUI_FLOAT_VALUE({pct})"
    if val == "0" or val == "0px":
        return "EGUI_FLOAT_VALUE(0.0)"
    # Pixel value — will be converted to ratio relative to original size
    # at code generation time. For now, store as-is.
    return f"EGUI_FLOAT_VALUE(1.0)"  # Default to full size


def _parse_motion_props(tsx_content: str) -> list:
    """Extract motion.* component animations from TSX content.

    Finds patterns like:
        <motion.div initial={{opacity: 0, y: 10}} animate={{opacity: 1, y: 0}}
                    transition={{duration: 0.8, delay: 0.2}}>
    """
    animations = []

    # Pattern: <motion.TAG ... initial={{...}} animate={{...}} transition={{...}}>
    motion_pattern = re.compile(
        r'<motion\.(\w+)\s+([^>]*?)(?:/>|>)',
        re.DOTALL
    )

    for match in motion_pattern.finditer(tsx_content):
        tag = match.group(1)
        attrs_str = match.group(2)

        # Extract initial props
        initial = _extract_object_prop(attrs_str, "initial")
        # Extract animate props
        animate = _extract_object_prop(attrs_str, "animate")
        # Extract transition props
        transition = _extract_object_prop(attrs_str, "transition")

        if not animate:
            continue

        # Extract className/id for target identification
        cls_match = re.search(r'class(?:Name)?="([^"]*)"', attrs_str)
        target_class = cls_match.group(1) if cls_match else ""

        # Generate a target ID from class or position
        target_id = _class_to_id(target_class) or f"motion_{tag}_{match.start()}"

        # Map each animated property
        for prop_name, to_val in animate.items():
            from_val = initial.get(prop_name, "0") if initial else "0"

            mapping = _MOTION_TO_EGUI.get(prop_name)
            if mapping:
                anim_desc = {
                    "target_id": target_id,
                    "target_class": target_class,
                    "type": mapping["type"],
                    "params": mapping["param_map"](str(from_val), str(to_val)),
                    "duration": int(float(transition.get("duration", 0.5)) * 1000),
                    "delay": int(float(transition.get("delay", 0)) * 1000),
                    "interpolator": _resolve_interpolator(transition),
                    "status": "supported",
                }
                animations.append(anim_desc)
            else:
                # Unknown property — mark as needs extension
                animations.append({
                    "target_id": target_id,
                    "target_class": target_class,
                    "type": "unknown",
                    "property": prop_name,
                    "from": str(from_val),
                    "to": str(to_val),
                    "status": "NEEDS_EXTENSION",
                    "reason": f"No EGUI mapping for motion property '{prop_name}'",
                })

    return animations


def _extract_object_prop(attrs_str: str, prop_name: str) -> dict | None:
    """Extract a JSX object prop like initial={{opacity: 0, y: 10}}."""
    # Match prop_name={{ ... }}
    pattern = re.compile(rf'{prop_name}=\{{\{{([^}}]*)\}}\}}', re.DOTALL)
    m = pattern.search(attrs_str)
    if not m:
        return None

    obj_str = m.group(1)
    result = {}

    # Parse key: value pairs
    for pair in re.finditer(r'(\w+)\s*:\s*([^,}]+)', obj_str):
        key = pair.group(1)
        val = pair.group(2).strip().strip('"').strip("'")
        result[key] = val

    return result


def _resolve_interpolator(transition: dict) -> str:
    """Map framer-motion transition to EGUI interpolator name."""
    if not transition:
        return "decelerate"

    # Check transition type
    trans_type = transition.get("type", "tween")
    if trans_type in _TRANSITION_TO_INTERPOLATOR:
        return _TRANSITION_TO_INTERPOLATOR[trans_type]

    # Check easing
    ease = transition.get("ease", "")
    if ease in _EASING_TO_INTERPOLATOR:
        return _EASING_TO_INTERPOLATOR[ease]

    return "decelerate"


def _class_to_id(cls_str: str) -> str:
    """Generate a widget ID from Tailwind classes."""
    if not cls_str:
        return ""
    # Use first meaningful class as ID basis
    parts = cls_str.split()
    for part in parts:
        if part.startswith("bg-") or part.startswith("text-") or part.startswith("w-"):
            continue
        if len(part) > 3:
            return re.sub(r'[^a-zA-Z0-9]', '_', part)[:20]
    return ""


def _extract_stagger_animations(tsx_content: str) -> list:
    """Extract stagger delay patterns from .map() with index-based delays.

    Finds patterns like:
        items.map((item, index) => (
            <motion.div transition={{delay: index * 0.1}} ...>
        ))
    """
    staggers = []

    # Look for .map with index parameter and motion components
    map_pattern = re.compile(
        r'\.map\(\s*\(\s*\w+\s*,\s*(\w+)\s*\)\s*=>\s*\(',
        re.DOTALL
    )

    for m in map_pattern.finditer(tsx_content):
        index_var = m.group(1)
        # Find motion components after this .map
        map_end = _find_balanced_paren(tsx_content, m.end() - 1)
        if map_end < 0:
            continue
        map_body = tsx_content[m.end():map_end]

        # Look for delay: index * N pattern
        delay_match = re.search(
            rf'delay\s*:\s*{index_var}\s*\*\s*([\d.]+)',
            map_body
        )
        if delay_match:
            stagger_delay_s = float(delay_match.group(1))
            staggers.append({
                "stagger_delay_ms": int(stagger_delay_s * 1000),
                "index_var": index_var,
                "context": map_body[:100],
            })

    return staggers


def _find_balanced_paren(text: str, start: int) -> int:
    """Find matching closing parenthesis."""
    depth = 0
    i = start
    while i < len(text):
        if text[i] == '(':
            depth += 1
        elif text[i] == ')':
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return -1


class AnimExtractor:
    """Extract and map framer-motion animations from Figma Make TSX files."""

    def extract_all(self, project_dir: str) -> dict:
        """Extract animations from all pages in a Figma Make project."""
        discovery = _discover_figmamake_pages(project_dir)

        pages_anims = []
        all_extensions = []

        for page_info in discovery["pages"]:
            page_file = page_info["file"]
            if not os.path.isfile(page_file):
                continue

            with open(page_file, "r", encoding="utf-8") as f:
                tsx_content = f.read()

            page_name = re.sub(r'(?<!^)(?=[A-Z])', '_', page_info["name"]).lower()
            anims = _parse_motion_props(tsx_content)
            staggers = _extract_stagger_animations(tsx_content)

            # Separate supported vs needs-extension
            supported = [a for a in anims if a["status"] == "supported"]
            needs_ext = [a for a in anims if a["status"] == "NEEDS_EXTENSION"]
            all_extensions.extend(needs_ext)

            pages_anims.append({
                "page": page_name,
                "component": page_info["name"],
                "animations": supported,
                "staggers": staggers,
                "extensions_needed": needs_ext,
            })

        return {
            "pages": pages_anims,
            "extensions_needed": all_extensions,
            "has_extensions_needed": len(all_extensions) > 0,
        }


def generate_extension_proposals(extensions: list, output_dir: str):
    """Generate extension proposal markdown files for unsupported animations."""
    os.makedirs(output_dir, exist_ok=True)

    # Group by property type
    by_type = {}
    for ext in extensions:
        prop = ext.get("property", "unknown")
        if prop not in by_type:
            by_type[prop] = []
        by_type[prop].append(ext)

    for prop_type, items in by_type.items():
        proposal_path = os.path.join(output_dir, f"{prop_type}.md")
        with open(proposal_path, "w", encoding="utf-8", newline="\n") as f:
            f.write(f"# Extension Proposal: egui_animation_{prop_type}\n\n")
            f.write(f"## Need\n\n")
            f.write(f"The following Figma Make components use `{prop_type}` animation:\n\n")
            for item in items:
                f.write(f"- Target: `{item['target_id']}` ({item.get('from', '?')} -> {item.get('to', '?')})\n")
            f.write(f"\n## API Design\n\n")
            f.write(f"```c\n")
            f.write(f"typedef struct {{\n")
            f.write(f"    // TODO: define params for {prop_type} animation\n")
            f.write(f"}} egui_animation_{prop_type}_params_t;\n")
            f.write(f"```\n\n")
            f.write(f"## Files to Change\n\n")
            f.write(f"- New: `src/anim/egui_animation_{prop_type}.h`\n")
            f.write(f"- New: `src/anim/egui_animation_{prop_type}.c`\n")
            f.write(f"- Modify: `src/egui.h` (add include)\n")
            f.write(f"- Modify: `scripts/ui_designer/model/widget_model.py` (register type)\n")

        print(f"  Extension proposal: {proposal_path}", file=sys.stderr)


def main():
    parser = argparse.ArgumentParser(description="Extract animations from Figma Make TSX")
    parser.add_argument("--project-dir", required=True, help="Figma Make project directory")
    parser.add_argument("--output", default=None, help="Output JSON file path")
    parser.add_argument("--proposals-dir", default=None, help="Directory for extension proposals")
    args = parser.parse_args()

    extractor = AnimExtractor()
    result = extractor.extract_all(args.project_dir)

    # Generate extension proposals if needed
    if result["has_extensions_needed"] and args.proposals_dir:
        generate_extension_proposals(result["extensions_needed"], args.proposals_dir)

    output_json = json.dumps(result, indent=2, ensure_ascii=False)
    if args.output:
        Path(args.output).parent.mkdir(parents=True, exist_ok=True)
        with open(args.output, "w", encoding="utf-8", newline="\n") as f:
            f.write(output_json)
        print(f"Animation descriptions written to: {args.output}", file=sys.stderr)
    else:
        print(output_json)

    if result["has_extensions_needed"]:
        print(f"\nWARNING: {len(result['extensions_needed'])} animations need framework extensions.",
              file=sys.stderr)
        sys.exit(2)  # Non-zero exit to signal pipeline should pause


if __name__ == "__main__":
    main()

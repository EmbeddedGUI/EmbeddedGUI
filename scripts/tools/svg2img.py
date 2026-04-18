#!/usr/bin/python
# -*- coding: utf-8 -*-

import os
import sys
import tempfile
from pathlib import Path

from PIL import Image


SCRIPT_DIR = Path(__file__).resolve().parent
SCRIPTS_ROOT = SCRIPT_DIR.parent
if str(SCRIPTS_ROOT) not in sys.path:
    sys.path.insert(0, str(SCRIPTS_ROOT))

import resvg_tool


class SvgError(ValueError):
    pass


def describe_svg_rasterizer_unavailable():
    return resvg_tool.describe_resvg_missing()


def render_svg_to_pil(svg_path, dim):
    if dim is None or len(dim) != 2:
        raise SvgError(f"SVG resource '{os.path.basename(svg_path)}' requires a dim value like \"80,80\".")

    width_px = int(dim[0])
    height_px = int(dim[1])
    if width_px <= 0 or height_px <= 0:
        raise SvgError(f"SVG resource '{os.path.basename(svg_path)}' requires a positive dim value.")

    if not os.path.exists(svg_path):
        raise FileNotFoundError(f"Cannot open image file {svg_path}")

    if resvg_tool.find_resvg_binary() is None:
        raise SvgError(describe_svg_rasterizer_unavailable())

    try:
        with tempfile.TemporaryDirectory(prefix="svg2img_") as temp_dir_name:
            temp_dir = Path(temp_dir_name)
            output_png_path = temp_dir / "render.png"
            resvg_tool.render_svg_to_png(Path(svg_path), output_png_path, width_px, height_px)
            with Image.open(output_png_path) as image:
                return image.convert("RGBA")
    except Exception as exc:
        raise SvgError(f"Failed to rasterize SVG resource '{os.path.basename(svg_path)}': {exc}") from exc

#!/usr/bin/python
# -*- coding: utf-8 -*-

import io
import os
import sys
from pathlib import Path

from PIL import Image


SCRIPT_DIR = Path(__file__).resolve().parent
SCRIPTS_ROOT = SCRIPT_DIR.parent
if str(SCRIPTS_ROOT) not in sys.path:
    sys.path.insert(0, str(SCRIPTS_ROOT))

import cairo_runtime

cairo_runtime.prepare_cairo_runtime()

try:
    import cairosvg
except (ImportError, OSError) as exc:
    cairosvg = None
    CAIROSVG_IMPORT_ERROR = exc
else:
    CAIROSVG_IMPORT_ERROR = None


class SvgError(ValueError):
    pass


def describe_cairosvg_unavailable():
    if CAIROSVG_IMPORT_ERROR is None:
        return "CairoSVG is required to rasterize SVG resources. Install it with `pip install cairosvg`."

    detail = str(CAIROSVG_IMPORT_ERROR).strip()
    if detail:
        return (
            "CairoSVG is required to rasterize SVG resources, but its native Cairo runtime is unavailable: "
            f"{detail}"
        )
    return "CairoSVG is required to rasterize SVG resources, but its native Cairo runtime is unavailable."


def render_svg_to_pil(svg_path, dim):
    if dim is None or len(dim) != 2:
        raise SvgError(f"SVG resource '{os.path.basename(svg_path)}' requires a dim value like \"80,80\".")

    width_px = int(dim[0])
    height_px = int(dim[1])
    if width_px <= 0 or height_px <= 0:
        raise SvgError(f"SVG resource '{os.path.basename(svg_path)}' requires a positive dim value.")

    if cairosvg is None:
        raise SvgError(describe_cairosvg_unavailable())

    if not os.path.exists(svg_path):
        raise FileNotFoundError(f"Cannot open image file {svg_path}")

    try:
        png_bytes = cairosvg.svg2png(url=os.path.abspath(svg_path), output_width=width_px, output_height=height_px)
    except Exception as exc:
        raise SvgError(f"Failed to rasterize SVG resource '{os.path.basename(svg_path)}': {exc}") from exc

    with Image.open(io.BytesIO(png_bytes)) as image:
        return image.convert("RGBA")

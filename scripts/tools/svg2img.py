#!/usr/bin/python
# -*- coding: utf-8 -*-

import io
import os

from PIL import Image

try:
    import cairosvg
except ImportError:
    cairosvg = None


class SvgError(ValueError):
    pass


def render_svg_to_pil(svg_path, dim):
    if dim is None or len(dim) != 2:
        raise SvgError(f"SVG resource '{os.path.basename(svg_path)}' requires a dim value like \"80,80\".")

    width_px = int(dim[0])
    height_px = int(dim[1])
    if width_px <= 0 or height_px <= 0:
        raise SvgError(f"SVG resource '{os.path.basename(svg_path)}' requires a positive dim value.")

    if cairosvg is None:
        raise SvgError("CairoSVG is required to rasterize SVG resources. Install it with `pip install cairosvg`.")

    if not os.path.exists(svg_path):
        raise FileNotFoundError(f"Cannot open image file {svg_path}")

    try:
        png_bytes = cairosvg.svg2png(url=os.path.abspath(svg_path), output_width=width_px, output_height=height_px)
    except Exception as exc:
        raise SvgError(f"Failed to rasterize SVG resource '{os.path.basename(svg_path)}': {exc}") from exc

    with Image.open(io.BytesIO(png_bytes)) as image:
        return image.convert("RGBA")

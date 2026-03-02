"""Python-based preview renderer using Pillow.

Renders a page's widget tree to a PIL Image for fast preview without
compiling C code. Supports basic shapes, text, and widget approximations.
"""

from PIL import Image, ImageDraw, ImageFont

from ..model.widget_model import COLOR_RGB
from ..model.widget_registry import WidgetRegistry
from .layout_engine import compute_page_layout


# Default background color (dark grey)
_BG_COLOR = (42, 42, 42)

# Fallback font (Pillow default)
_DEFAULT_FONT = ImageFont.load_default()


def _resolve_color(color_str):
    """Convert EGUI color string to RGB tuple."""
    if not color_str:
        return (200, 200, 200)
    rgb = COLOR_RGB.get(color_str)
    if rgb:
        return rgb
    # Try EGUI_COLOR_HEX(0xRRGGBB)
    import re
    m = re.match(r'EGUI_COLOR_HEX\(\s*0x([0-9A-Fa-f]{6})\s*\)', color_str)
    if m:
        h = m.group(1)
        return (int(h[:2], 16), int(h[2:4], 16), int(h[4:6], 16))
    return (200, 200, 200)


def _resolve_alpha(alpha_str):
    """Convert EGUI alpha string to 0-255 value."""
    if not alpha_str:
        return 255
    # EGUI_ALPHA_XX -> XX%
    import re
    m = re.match(r'EGUI_ALPHA_(\d+)', alpha_str)
    if m:
        pct = int(m.group(1))
        return int(pct * 255 / 100)
    return 255


def _draw_background(draw, widget, x, y, w, h):
    """Draw widget background if present."""
    bg = widget.background
    if bg is None or bg.bg_type == "none":
        return

    color = _resolve_color(bg.color)
    alpha = _resolve_alpha(bg.alpha)
    fill = color + (alpha,)

    if bg.bg_type == "rectangle":
        draw.rectangle([x, y, x + w - 1, y + h - 1], fill=fill)
    elif bg.bg_type in ("round_rectangle", "round_rectangle_corners"):
        radius = getattr(bg, "radius", 5)
        draw.rounded_rectangle([x, y, x + w - 1, y + h - 1], radius=radius, fill=fill)
    elif bg.bg_type == "circle":
        draw.ellipse([x, y, x + w - 1, y + h - 1], fill=fill)

    # Stroke
    sw = getattr(bg, "stroke_width", 0)
    if sw > 0:
        sc = _resolve_color(bg.stroke_color)
        if bg.bg_type == "circle":
            draw.ellipse([x, y, x + w - 1, y + h - 1], outline=sc, width=sw)
        elif bg.bg_type in ("round_rectangle", "round_rectangle_corners"):
            radius = getattr(bg, "radius", 5)
            draw.rounded_rectangle([x, y, x + w - 1, y + h - 1], radius=radius, outline=sc, width=sw)
        else:
            draw.rectangle([x, y, x + w - 1, y + h - 1], outline=sc, width=sw)


def _render_widget(draw, widget, font):
    """Render a single widget onto the draw context."""
    x = widget.display_x
    y = widget.display_y
    w = widget.width
    h = widget.height
    wtype = widget.widget_type
    props = widget.properties

    # Draw background
    _draw_background(draw, widget, x, y, w, h)

    if wtype == "label":
        text = props.get("text", widget.name)
        color = _resolve_color(props.get("font_color", "EGUI_COLOR_WHITE"))
        draw.text((x + 2, y + 2), str(text), fill=color, font=font)

    elif wtype == "button":
        # Draw button-like rectangle
        if widget.background is None:
            draw.rounded_rectangle([x, y, x + w - 1, y + h - 1], radius=4,
                                   fill=(80, 80, 80, 200), outline=(120, 120, 120))
        text = props.get("text", widget.name)
        color = _resolve_color(props.get("font_color", "EGUI_COLOR_WHITE"))
        bbox = font.getbbox(str(text))
        tw = bbox[2] - bbox[0]
        th = bbox[3] - bbox[1]
        tx = x + (w - tw) // 2
        ty = y + (h - th) // 2
        draw.text((tx, ty), str(text), fill=color, font=font)

    elif wtype == "image":
        # Image placeholder
        draw.rectangle([x, y, x + w - 1, y + h - 1], fill=(60, 60, 80, 150))
        draw.line([(x, y), (x + w - 1, y + h - 1)], fill=(100, 100, 120), width=1)
        draw.line([(x + w - 1, y), (x, y + h - 1)], fill=(100, 100, 120), width=1)
        draw.text((x + 2, y + 2), "IMG", fill=(150, 150, 170), font=font)

    elif wtype == "progress_bar":
        # Background track
        draw.rounded_rectangle([x, y, x + w - 1, y + h - 1], radius=3,
                               fill=(50, 50, 50, 200))
        # Fill bar
        value = int(props.get("value", 50))
        fill_w = max(1, int(w * value / 100))
        color = _resolve_color(props.get("color", "EGUI_COLOR_GREEN"))
        draw.rounded_rectangle([x, y, x + fill_w - 1, y + h - 1], radius=3, fill=color)

    elif wtype == "switch":
        # Track
        track_color = (80, 160, 80) if props.get("checked") else (100, 100, 100)
        draw.rounded_rectangle([x, y, x + w - 1, y + h - 1], radius=h // 2, fill=track_color)
        # Thumb
        thumb_r = h // 2 - 2
        if props.get("checked"):
            cx = x + w - thumb_r - 4
        else:
            cx = x + thumb_r + 4
        cy = y + h // 2
        draw.ellipse([cx - thumb_r, cy - thumb_r, cx + thumb_r, cy + thumb_r], fill=(220, 220, 220))

    elif wtype == "slider":
        # Track line
        track_y = y + h // 2
        draw.line([(x + 4, track_y), (x + w - 4, track_y)], fill=(100, 100, 100), width=3)
        # Thumb position
        value = int(props.get("value", 50))
        thumb_x = x + 4 + int((w - 8) * value / 100)
        draw.ellipse([thumb_x - 6, track_y - 6, thumb_x + 6, track_y + 6], fill=(100, 150, 255))

    elif wtype == "circular_progress_bar":
        # Simple circle approximation
        draw.ellipse([x + 2, y + 2, x + w - 3, y + h - 3], outline=(100, 100, 100), width=3)
        value = int(props.get("value", 50))
        color = _resolve_color(props.get("color", "EGUI_COLOR_GREEN"))
        draw.arc([x + 2, y + 2, x + w - 3, y + h - 3],
                 start=-90, end=-90 + int(360 * value / 100), fill=color, width=4)

    elif wtype == "checkbox":
        # Box
        box_size = min(w, h, 20)
        bx = x + 2
        by = y + (h - box_size) // 2
        draw.rectangle([bx, by, bx + box_size, by + box_size], outline=(150, 150, 150), width=2)
        if props.get("checked"):
            draw.line([(bx + 3, by + box_size // 2), (bx + box_size // 2, by + box_size - 3)],
                      fill=(100, 200, 100), width=2)
            draw.line([(bx + box_size // 2, by + box_size - 3), (bx + box_size - 3, by + 3)],
                      fill=(100, 200, 100), width=2)


def render_page(page, screen_width=240, screen_height=320):
    """Render a Page to a PIL RGBA Image.

    Args:
        page: Page object with root_widget
        screen_width: Canvas width in pixels
        screen_height: Canvas height in pixels

    Returns:
        PIL.Image.Image in RGBA mode
    """
    if page.root_widget:
        compute_page_layout(page)

    img = Image.new("RGBA", (screen_width, screen_height), _BG_COLOR + (255,))
    draw = ImageDraw.Draw(img, "RGBA")

    if page.root_widget is None:
        return img

    def _collect(widget, out):
        out.append(widget)
        for child in widget.children:
            _collect(child, out)

    widgets = []
    _collect(page.root_widget, widgets)

    for w in widgets:
        _render_widget(draw, w, _DEFAULT_FONT)

    return img


def render_page_to_bytes(page, screen_width=240, screen_height=320, fmt="PNG"):
    """Render a page and return raw image bytes."""
    import io
    img = render_page(page, screen_width, screen_height)
    buf = io.BytesIO()
    img.save(buf, format=fmt)
    return buf.getvalue()

"""Widget model definitions for EmbeddedGUI Designer."""

import os
import re
import xml.etree.ElementTree as ET

# XML tag to widget_type mapping (legacy, kept empty for backward compat)
TAG_TO_TYPE = {}
TYPE_TO_TAG = {}

# Widget type descriptors are now defined in custom_widgets/*.py plugin files.
# This dict is kept empty for backward compatibility with imports.
WIDGET_TYPES = {}

# Color constants
COLORS = [
    "EGUI_COLOR_BLACK",
    "EGUI_COLOR_WHITE",
    "EGUI_COLOR_RED",
    "EGUI_COLOR_GREEN",
    "EGUI_COLOR_BLUE",
    "EGUI_COLOR_YELLOW",
    "EGUI_COLOR_CYAN",
    "EGUI_COLOR_MAGENTA",
    "EGUI_COLOR_ORANGE",
    "EGUI_COLOR_DARK_GREY",
    "EGUI_COLOR_LIGHT_GREY",
    "EGUI_COLOR_NAVY",
    "EGUI_COLOR_DARK_GREEN",
    "EGUI_COLOR_DARK_CYAN",
    "EGUI_COLOR_MAROON",
    "EGUI_COLOR_PURPLE",
    "EGUI_COLOR_OLIVE",
]

# Color to RGB mapping for preview
COLOR_RGB = {
    "EGUI_COLOR_BLACK": (0, 0, 0),
    "EGUI_COLOR_WHITE": (255, 255, 255),
    "EGUI_COLOR_RED": (255, 0, 0),
    "EGUI_COLOR_GREEN": (0, 255, 0),
    "EGUI_COLOR_BLUE": (0, 0, 255),
    "EGUI_COLOR_YELLOW": (255, 255, 0),
    "EGUI_COLOR_CYAN": (0, 255, 255),
    "EGUI_COLOR_MAGENTA": (255, 0, 255),
    "EGUI_COLOR_ORANGE": (255, 128, 0),
    "EGUI_COLOR_DARK_GREY": (128, 128, 128),
    "EGUI_COLOR_LIGHT_GREY": (192, 192, 192),
    "EGUI_COLOR_NAVY": (0, 0, 128),
    "EGUI_COLOR_DARK_GREEN": (0, 128, 0),
    "EGUI_COLOR_DARK_CYAN": (0, 128, 128),
    "EGUI_COLOR_MAROON": (128, 0, 0),
    "EGUI_COLOR_PURPLE": (128, 0, 128),
    "EGUI_COLOR_OLIVE": (128, 128, 0),
}

# Alpha constants
ALPHAS = [
    "EGUI_ALPHA_0",
    "EGUI_ALPHA_10",
    "EGUI_ALPHA_20",
    "EGUI_ALPHA_30",
    "EGUI_ALPHA_40",
    "EGUI_ALPHA_50",
    "EGUI_ALPHA_60",
    "EGUI_ALPHA_70",
    "EGUI_ALPHA_80",
    "EGUI_ALPHA_90",
    "EGUI_ALPHA_100",
]

# Font constants
FONTS = [
    "EGUI_CONFIG_FONT_DEFAULT",
    "&egui_res_font_montserrat_8_4",
    "&egui_res_font_montserrat_10_4",
    "&egui_res_font_montserrat_12_4",
    "&egui_res_font_montserrat_14_4",
    "&egui_res_font_montserrat_16_4",
    "&egui_res_font_montserrat_18_4",
    "&egui_res_font_montserrat_20_4",
    "&egui_res_font_montserrat_22_4",
    "&egui_res_font_montserrat_24_4",
    "&egui_res_font_montserrat_26_4",
    "&egui_res_font_montserrat_28_4",
    "&egui_res_font_montserrat_30_4",
    "&egui_res_font_montserrat_32_4",
    "&egui_res_font_montserrat_34_4",
    "&egui_res_font_montserrat_36_4",
    "&egui_res_font_montserrat_38_4",
    "&egui_res_font_montserrat_42_4",
    "&egui_res_font_montserrat_44_4",
    "&egui_res_font_montserrat_46_4",
    "&egui_res_font_montserrat_48_4",
]

# Alignment constants
ALIGNS = [
    "EGUI_ALIGN_CENTER",
    "EGUI_ALIGN_LEFT",
    "EGUI_ALIGN_RIGHT",
    "EGUI_ALIGN_TOP",
    "EGUI_ALIGN_BOTTOM",
    "EGUI_ALIGN_HCENTER",
    "EGUI_ALIGN_VCENTER",
]

# Background types
BG_TYPES = [
    "none",
    "solid",
    "round_rectangle",
    "round_rectangle_corners",
    "circle",
    "gradient",
]

# Gradient direction choices
GRADIENT_DIRECTIONS = [
    "vertical",
    "horizontal",
]

# Image format choices
IMAGE_FORMATS = ["rgb565", "rgb32", "gray8"]

# Image alpha choices
IMAGE_ALPHAS = ["0", "1", "2", "4", "8"]

# Image external choices
IMAGE_EXTERNALS = ["0", "1"]

# Font pixel size choices
FONT_PIXELSIZES = [str(i) for i in range(4, 49)]

# Font bit size choices
FONT_BITSIZES = ["1", "2", "4", "8"]

# Font external choices
FONT_EXTERNALS = ["0", "1"]


# ── C name / expression helpers ───────────────────────────────────

def format_file_name(file_name):
    """Sanitize filename to C-safe identifier (mirrors app_resource_generate.py)."""
    file_name = file_name.replace('-', '_')
    file_name = file_name.replace('.', '_')
    file_name = file_name.replace('(', '_')
    file_name = file_name.replace(')', '_')
    file_name = file_name.replace(',', '_')
    file_name = file_name.replace(' ', '_')
    file_name = file_name.replace('//', '_')
    file_name = file_name.replace('/', '_')
    file_name = file_name.replace('\\', '_')
    file_name = file_name.replace('__', '_')
    file_name = file_name.replace('__', '_')
    file_name = file_name.replace('__', '_')
    file_name = file_name.lower()
    return file_name


def _file_stem(filename):
    """Get filename without extension: 'star.png' -> 'star'."""
    return os.path.splitext(filename)[0] if filename else ""


def derive_image_c_expr(widget):
    """Derive C expression from widget's image properties.

    Returns string like ``&egui_res_image_star_rgb565_4`` or ``NULL``.
    """
    image_file = widget.properties.get("image_file", "")
    if not image_file:
        return "NULL"
    name = format_file_name(_file_stem(image_file))
    fmt = widget.properties.get("image_format", "rgb565")
    alpha = widget.properties.get("image_alpha", "4")
    return f"&egui_res_image_{name}_{fmt}_{alpha}"


def derive_font_c_expr(widget):
    """Derive C expression from widget's font properties.

    When font_file is set, returns ``&egui_res_font_{name}_{pixelsize}_{bitsize}``.
    Otherwise returns the font_builtin value (e.g., ``EGUI_CONFIG_FONT_DEFAULT``).
    """
    font_file = widget.properties.get("font_file", "")
    if not font_file:
        return widget.properties.get("font_builtin", "EGUI_CONFIG_FONT_DEFAULT")
    name = format_file_name(_file_stem(font_file))
    pixelsize = widget.properties.get("font_pixelsize", "16")
    fontbitsize = widget.properties.get("font_fontbitsize", "4")
    return f"&egui_res_font_{name}_{pixelsize}_{fontbitsize}"


# ── Legacy C expression parsers (for backward-compat migration) ───

_RE_IMAGE_EXPR = re.compile(r'&egui_res_image_(.+)_(rgb565|rgb32|gray8)_(\d+)')
_RE_FONT_EXPR = re.compile(r'&egui_res_font_(.+)_(\d+)_(\d+)')


def parse_legacy_image_expr(expr):
    """Parse legacy ``&egui_res_image_{name}_{format}_{alpha}`` into components.

    Returns dict with keys (name, format, alpha) or None.
    """
    m = _RE_IMAGE_EXPR.match(expr)
    if m:
        return {"name": m.group(1), "format": m.group(2), "alpha": m.group(3)}
    return None


def parse_legacy_font_expr(expr):
    """Parse legacy ``&egui_res_font_{name}_{pixelsize}_{fontbitsize}`` into components.

    Returns dict with keys (name, pixelsize, fontbitsize) or None.
    """
    m = _RE_FONT_EXPR.match(expr)
    if m:
        return {"name": m.group(1), "pixelsize": m.group(2), "fontbitsize": m.group(3)}
    return None


def _guess_filename_from_c_name(c_name, extensions, src_dir=None):
    """Try to guess the original filename from a C-sanitized name.

    Looks in src_dir for files whose sanitized name matches c_name.
    Falls back to c_name + first extension.
    """
    if src_dir and os.path.isdir(src_dir):
        for fname in os.listdir(src_dir):
            stem = os.path.splitext(fname)[0]
            ext = os.path.splitext(fname)[1].lower()
            if ext in extensions and format_file_name(stem) == c_name:
                return fname
    # Fallback: assume name + first extension
    return c_name + extensions[0]


class BackgroundModel:
    """Model for a widget background."""

    def __init__(self):
        self.bg_type = "none"
        self.color = "EGUI_COLOR_WHITE"
        self.alpha = "EGUI_ALPHA_100"
        self.radius = 0
        self.radius_left_top = 0
        self.radius_left_bottom = 0
        self.radius_right_top = 0
        self.radius_right_bottom = 0
        self.stroke_width = 0
        self.stroke_color = "EGUI_COLOR_BLACK"
        self.stroke_alpha = "EGUI_ALPHA_100"
        # Gradient properties (only used when bg_type == "gradient")
        self.start_color = "EGUI_COLOR_BLACK"
        self.end_color = "EGUI_COLOR_WHITE"
        self.direction = "vertical"
        # Pressed state
        self.pressed_color = "EGUI_COLOR_DARK_GREY"
        self.pressed_alpha = "EGUI_ALPHA_100"
        self.has_pressed = False
        # Disabled state
        self.has_disabled = False
        self.disabled_color = "EGUI_COLOR_LIGHT_GREY"
        self.disabled_alpha = "EGUI_ALPHA_100"

    def to_dict(self):
        d = {
            "bg_type": self.bg_type,
            "color": self.color,
            "alpha": self.alpha,
            "radius": self.radius,
            "radius_left_top": self.radius_left_top,
            "radius_left_bottom": self.radius_left_bottom,
            "radius_right_top": self.radius_right_top,
            "radius_right_bottom": self.radius_right_bottom,
            "stroke_width": self.stroke_width,
            "stroke_color": self.stroke_color,
            "stroke_alpha": self.stroke_alpha,
            "has_pressed": self.has_pressed,
            "pressed_color": self.pressed_color,
            "pressed_alpha": self.pressed_alpha,
            "has_disabled": self.has_disabled,
            "disabled_color": self.disabled_color,
            "disabled_alpha": self.disabled_alpha,
            "start_color": self.start_color,
            "end_color": self.end_color,
            "direction": self.direction,
        }
        return d

    @classmethod
    def from_dict(cls, d):
        bg = cls()
        for k, v in d.items():
            if hasattr(bg, k):
                setattr(bg, k, v)
        return bg

    def to_xml_element(self):
        """Serialize background to XML element."""
        elem = ET.Element("Background")
        elem.set("type", self.bg_type)
        if self.bg_type == "gradient":
            elem.set("direction", self.direction)
            elem.set("start_color", self.start_color)
            elem.set("end_color", self.end_color)
            elem.set("alpha", self.alpha)
        else:
            elem.set("color", self.color)
            elem.set("alpha", self.alpha)
            if self.bg_type in ("round_rectangle", "circle"):
                elem.set("radius", str(self.radius))
            if self.bg_type == "round_rectangle_corners":
                elem.set("radius_left_top", str(self.radius_left_top))
                elem.set("radius_left_bottom", str(self.radius_left_bottom))
                elem.set("radius_right_top", str(self.radius_right_top))
                elem.set("radius_right_bottom", str(self.radius_right_bottom))
            if self.stroke_width > 0:
                elem.set("stroke_width", str(self.stroke_width))
                elem.set("stroke_color", self.stroke_color)
                elem.set("stroke_alpha", self.stroke_alpha)
        if self.has_pressed:
            elem.set("has_pressed", "true")
            elem.set("pressed_color", self.pressed_color)
            elem.set("pressed_alpha", self.pressed_alpha)
        if self.has_disabled:
            elem.set("has_disabled", "true")
            elem.set("disabled_color", self.disabled_color)
            elem.set("disabled_alpha", self.disabled_alpha)
        return elem

    @classmethod
    def from_xml_element(cls, elem):
        """Deserialize background from XML element."""
        bg = cls()
        bg.bg_type = elem.get("type", "none")
        bg.color = elem.get("color", "EGUI_COLOR_WHITE")
        bg.alpha = elem.get("alpha", "EGUI_ALPHA_100")
        bg.radius = int(elem.get("radius", "0"))
        bg.radius_left_top = int(elem.get("radius_left_top", "0"))
        bg.radius_left_bottom = int(elem.get("radius_left_bottom", "0"))
        bg.radius_right_top = int(elem.get("radius_right_top", "0"))
        bg.radius_right_bottom = int(elem.get("radius_right_bottom", "0"))
        bg.stroke_width = int(elem.get("stroke_width", "0"))
        bg.stroke_color = elem.get("stroke_color", "EGUI_COLOR_BLACK")
        bg.stroke_alpha = elem.get("stroke_alpha", "EGUI_ALPHA_100")
        bg.has_pressed = elem.get("has_pressed", "false").lower() == "true"
        bg.pressed_color = elem.get("pressed_color", "EGUI_COLOR_DARK_GREY")
        bg.pressed_alpha = elem.get("pressed_alpha", "EGUI_ALPHA_100")
        bg.has_disabled = elem.get("has_disabled", "false").lower() == "true"
        bg.disabled_color = elem.get("disabled_color", "EGUI_COLOR_LIGHT_GREY")
        bg.disabled_alpha = elem.get("disabled_alpha", "EGUI_ALPHA_100")
        # Gradient properties
        bg.start_color = elem.get("start_color", "EGUI_COLOR_BLACK")
        bg.end_color = elem.get("end_color", "EGUI_COLOR_WHITE")
        bg.direction = elem.get("direction", "vertical")
        return bg


class ShadowModel:
    """Model for a widget shadow/glow effect."""

    def __init__(self):
        self.width = 8
        self.ofs_x = 0
        self.ofs_y = 4
        self.color = "EGUI_COLOR_BLACK"
        self.opa = "EGUI_ALPHA_30"
        self.corner_radius = 0

    def to_dict(self):
        return {
            "width": self.width,
            "ofs_x": self.ofs_x,
            "ofs_y": self.ofs_y,
            "color": self.color,
            "opa": self.opa,
            "corner_radius": self.corner_radius,
        }

    @classmethod
    def from_dict(cls, d):
        s = cls()
        for k, v in d.items():
            if hasattr(s, k):
                setattr(s, k, v)
        return s

    def to_xml_element(self):
        """Serialize shadow to XML element."""
        elem = ET.Element("Shadow")
        elem.set("width", str(self.width))
        elem.set("ofs_x", str(self.ofs_x))
        elem.set("ofs_y", str(self.ofs_y))
        elem.set("color", self.color)
        elem.set("opa", self.opa)
        if self.corner_radius > 0:
            elem.set("corner_radius", str(self.corner_radius))
        return elem

    @classmethod
    def from_xml_element(cls, elem):
        """Deserialize shadow from XML element."""
        s = cls()
        s.width = int(elem.get("width", "8"))
        s.ofs_x = int(elem.get("ofs_x", "0"))
        s.ofs_y = int(elem.get("ofs_y", "4"))
        s.color = elem.get("color", "EGUI_COLOR_BLACK")
        s.opa = elem.get("opa", "EGUI_ALPHA_30")
        s.corner_radius = int(elem.get("corner_radius", "0"))
        return s


# ── Animation types and interpolators ──────────────────────────────

ANIMATION_TYPES = {
    "alpha": {
        "c_type": "egui_animation_alpha_t",
        "init_func": "egui_animation_alpha_init",
        "params_macro": "EGUI_ANIMATION_ALPHA_PARAMS_INIT",
        "params_set_func": "egui_animation_alpha_params_set",
        "params": ["from_alpha", "to_alpha"],
    },
    "translate": {
        "c_type": "egui_animation_translate_t",
        "init_func": "egui_animation_translate_init",
        "params_macro": "EGUI_ANIMATION_TRANSLATE_PARAMS_INIT",
        "params_set_func": "egui_animation_translate_params_set",
        "params": ["from_x", "to_x", "from_y", "to_y"],
    },
    "scale": {
        "c_type": "egui_animation_scale_size_t",
        "init_func": "egui_animation_scale_size_init",
        "params_macro": "EGUI_ANIMATION_SCALE_SIZE_PARAMS_INIT",
        "params_set_func": "egui_animation_scale_size_params_set",
        "params": ["from_scale", "to_scale"],
    },
    "resize": {
        "c_type": "egui_animation_resize_t",
        "init_func": "egui_animation_resize_init",
        "params_macro": "EGUI_ANIMATION_RESIZE_PARAMS_INIT",
        "params_set_func": "egui_animation_resize_params_set",
        "params": ["from_width_ratio", "to_width_ratio", "from_height_ratio", "to_height_ratio", "mode"],
    },
    "color": {
        "c_type": "egui_animation_color_t",
        "init_func": "egui_animation_color_init",
        "params_macro": "EGUI_ANIMATION_COLOR_PARAMS_INIT",
        "params_set_func": "egui_animation_color_params_set",
        "params": ["from_color", "to_color"],
    },
}

INTERPOLATOR_TYPES = {
    "linear": "egui_interpolator_linear",
    "accelerate": "egui_interpolator_accelerate",
    "decelerate": "egui_interpolator_decelerate",
    "accelerate_decelerate": "egui_interpolator_accelerate_decelerate",
    "anticipate": "egui_interpolator_anticipate",
    "overshoot": "egui_interpolator_overshoot",
    "anticipate_overshoot": "egui_interpolator_anticipate_overshoot",
    "bounce": "egui_interpolator_bounce",
    "cycle": "egui_interpolator_cycle",
}


class AnimationModel:
    """Model for a widget animation."""

    def __init__(self):
        self.anim_type = "alpha"
        self.duration = 500
        self.interpolator = "linear"
        self.repeat_count = 0
        self.repeat_mode = "restart"
        self.auto_start = True
        # Type-specific params (stored as strings for C literal output)
        self.params = {}

    def to_dict(self):
        return {
            "anim_type": self.anim_type,
            "duration": self.duration,
            "interpolator": self.interpolator,
            "repeat_count": self.repeat_count,
            "repeat_mode": self.repeat_mode,
            "auto_start": self.auto_start,
            "params": dict(self.params),
        }

    @classmethod
    def from_dict(cls, d):
        a = cls()
        a.anim_type = d.get("anim_type", "alpha")
        a.duration = d.get("duration", 500)
        a.interpolator = d.get("interpolator", "linear")
        a.repeat_count = d.get("repeat_count", 0)
        a.repeat_mode = d.get("repeat_mode", "restart")
        a.auto_start = d.get("auto_start", True)
        a.params = d.get("params", {})
        return a

    def to_xml_element(self):
        elem = ET.Element("Animation")
        elem.set("type", self.anim_type)
        elem.set("duration", str(self.duration))
        elem.set("interpolator", self.interpolator)
        if self.repeat_count != 0:
            elem.set("repeat_count", str(self.repeat_count))
        if self.repeat_mode != "restart":
            elem.set("repeat_mode", self.repeat_mode)
        if not self.auto_start:
            elem.set("auto_start", "false")
        for k, v in self.params.items():
            elem.set(k, str(v))
        return elem

    @classmethod
    def from_xml_element(cls, elem):
        a = cls()
        a.anim_type = elem.get("type", "alpha")
        a.duration = int(elem.get("duration", "500"))
        a.interpolator = elem.get("interpolator", "linear")
        a.repeat_count = int(elem.get("repeat_count", "0"))
        a.repeat_mode = elem.get("repeat_mode", "restart")
        a.auto_start = elem.get("auto_start", "true").lower() in ("true", "1")
        # Collect type-specific params
        anim_info = ANIMATION_TYPES.get(a.anim_type, {})
        for p in anim_info.get("params", []):
            if p in elem.attrib:
                a.params[p] = elem.get(p)
        return a


class WidgetModel:
    """Model for a single widget in the UI tree."""

    _counter = {}

    @staticmethod
    def _get_type_info(widget_type):
        """Get type info from WidgetRegistry."""
        from .widget_registry import WidgetRegistry
        return WidgetRegistry.instance().get(widget_type)

    def __init__(self, widget_type, name=None, x=0, y=0, width=100, height=40):
        self.widget_type = widget_type
        if name is None:
            name = self._auto_name(widget_type)
        self.name = name
        self.x = x
        self.y = y
        self.width = width
        self.height = height
        self.margin = 0
        self.margin_left = 0
        self.margin_right = 0
        self.margin_top = 0
        self.margin_bottom = 0
        self.on_click = ""
        self.events = {}  # event_name -> callback_function_name
        self.properties = {}
        self.background = None
        self.shadow = None
        self.animations = []
        self.children = []
        self.parent = None
        self.designer_locked = False
        self.designer_hidden = False

        # Computed display coordinates (set by layout_engine, not serialized)
        self.display_x = x
        self.display_y = y

        # Set default properties
        type_info = self._get_type_info(widget_type)
        for prop_name, prop_info in type_info.get("properties", {}).items():
            self.properties[prop_name] = prop_info["default"]

    @classmethod
    def _auto_name(cls, widget_type):
        if widget_type not in cls._counter:
            cls._counter[widget_type] = 0
        cls._counter[widget_type] += 1
        return f"{widget_type}_{cls._counter[widget_type]}"

    @classmethod
    def reset_counter(cls):
        cls._counter = {}

    @property
    def is_container(self):
        type_info = self._get_type_info(self.widget_type)
        return type_info.get("is_container", False)

    def add_child(self, child):
        child.parent = self
        self.children.append(child)

    def remove_child(self, child):
        child.parent = None
        self.children.remove(child)

    def to_dict(self):
        d = {
            "name": self.name,
            "type": self.widget_type,
            "x": self.x,
            "y": self.y,
            "width": self.width,
            "height": self.height,
            "margin": self.margin,
            "on_click": self.on_click,
            "events": dict(self.events),
            "properties": dict(self.properties),
            "background": self.background.to_dict() if self.background else None,
            "shadow": self.shadow.to_dict() if self.shadow else None,
            "animations": [a.to_dict() for a in self.animations],
            "designer_locked": self.designer_locked,
            "designer_hidden": self.designer_hidden,
            "children": [c.to_dict() for c in self.children],
        }
        return d

    @classmethod
    def from_dict(cls, d):
        w = cls(
            widget_type=d["type"],
            name=d["name"],
            x=d.get("x", 0),
            y=d.get("y", 0),
            width=d.get("width", 100),
            height=d.get("height", 40),
        )
        w.margin = d.get("margin", 0)
        w.on_click = d.get("on_click", "")
        w.events = d.get("events", {})
        w.properties = d.get("properties", {})
        bg_data = d.get("background")
        if bg_data:
            w.background = BackgroundModel.from_dict(bg_data)
        shadow_data = d.get("shadow")
        if shadow_data:
            w.shadow = ShadowModel.from_dict(shadow_data)
        for anim_data in d.get("animations", []):
            w.animations.append(AnimationModel.from_dict(anim_data))
        w.designer_locked = bool(d.get("designer_locked", False))
        w.designer_hidden = bool(d.get("designer_hidden", False))
        for child_data in d.get("children", []):
            child = cls.from_dict(child_data)
            w.add_child(child)
        return w

    def to_xml_element(self):
        """Serialize widget to XML element."""
        from .widget_registry import WidgetRegistry
        reg = WidgetRegistry.instance()
        tag = reg.type_to_tag(self.widget_type)
        elem = ET.Element(tag)
        elem.set("id", self.name)
        elem.set("x", str(self.x))
        elem.set("y", str(self.y))
        elem.set("width", str(self.width))
        elem.set("height", str(self.height))
        if self.margin != 0:
            elem.set("margin", str(self.margin))
        if self.margin_left != 0:
            elem.set("margin_left", str(self.margin_left))
        if self.margin_right != 0:
            elem.set("margin_right", str(self.margin_right))
        if self.margin_top != 0:
            elem.set("margin_top", str(self.margin_top))
        if self.margin_bottom != 0:
            elem.set("margin_bottom", str(self.margin_bottom))
        if self.on_click:
            elem.set("onClick", self.on_click)
        if self.designer_locked:
            elem.set("designer_locked", "true")
        if self.designer_hidden:
            elem.set("designer_hidden", "true")
        # Event callbacks
        for event_name, func_name in self.events.items():
            if func_name:
                elem.set(event_name, func_name)
        # Type-specific properties
        type_info = self._get_type_info(self.widget_type)
        for prop_name, prop_value in self.properties.items():
            default = type_info.get("properties", {}).get(prop_name, {}).get("default")
            if prop_value != default:
                if isinstance(prop_value, bool):
                    elem.set(prop_name, "true" if prop_value else "false")
                else:
                    elem.set(prop_name, str(prop_value))
        # Background
        if self.background and self.background.bg_type != "none":
            elem.append(self.background.to_xml_element())
        # Shadow
        if self.shadow:
            elem.append(self.shadow.to_xml_element())
        # Animations
        for anim in self.animations:
            elem.append(anim.to_xml_element())
        # Children
        for child in self.children:
            elem.append(child.to_xml_element())
        return elem

    @classmethod
    def from_xml_element(cls, elem, src_dir=None):
        """Deserialize widget from XML element.

        Args:
            elem: XML element to parse.
            src_dir: Optional resource/src/ directory path for legacy migration
                     (used to guess filenames from C names).
        """
        widget_type = TAG_TO_TYPE.get(elem.tag, None)
        if widget_type is None:
            from .widget_registry import WidgetRegistry
            widget_type = WidgetRegistry.instance().tag_to_type(elem.tag)
        w = cls(
            widget_type=widget_type,
            name=elem.get("id", None),
            x=int(elem.get("x", "0")),
            y=int(elem.get("y", "0")),
            width=int(elem.get("width", "100")),
            height=int(elem.get("height", "40")),
        )
        w.margin = int(elem.get("margin", "0"))
        w.margin_left = int(elem.get("margin_left", "0"))
        w.margin_right = int(elem.get("margin_right", "0"))
        w.margin_top = int(elem.get("margin_top", "0"))
        w.margin_bottom = int(elem.get("margin_bottom", "0"))
        w.on_click = elem.get("onClick", "")
        w.designer_locked = elem.get("designer_locked", "false").lower() in ("true", "1", "yes")
        w.designer_hidden = elem.get("designer_hidden", "false").lower() in ("true", "1", "yes")
        # Parse event callbacks from attributes
        type_info = cls._get_type_info(widget_type)
        for event_name in type_info.get("events", {}):
            if event_name in elem.attrib:
                w.events[event_name] = elem.get(event_name)
        # Parse type-specific properties from attributes
        type_info = cls._get_type_info(widget_type)
        for prop_name, prop_info in type_info.get("properties", {}).items():
            if prop_name in elem.attrib:
                val = elem.get(prop_name)
                ptype = prop_info.get("type")
                if ptype == "int":
                    val = int(val)
                elif ptype == "bool":
                    val = val.lower() in ("true", "1", "yes")
                w.properties[prop_name] = val

        # ── Legacy migration: old "image" attribute -> new structured properties ──
        if widget_type == "image" and "image" in elem.attrib and "image_file" not in elem.attrib:
            legacy_expr = elem.get("image", "")
            if legacy_expr and legacy_expr != "NULL":
                parsed = parse_legacy_image_expr(legacy_expr)
                if parsed:
                    filename = _guess_filename_from_c_name(
                        parsed["name"],
                        [".png", ".bmp", ".jpg", ".jpeg"],
                        src_dir,
                    )
                    w.properties["image_file"] = filename
                    w.properties["image_format"] = parsed["format"]
                    w.properties["image_alpha"] = parsed["alpha"]

        # ── Legacy migration: old "font" attribute -> new structured properties ──
        if widget_type in ("label", "button") and "font" in elem.attrib and "font_file" not in elem.attrib:
            legacy_expr = elem.get("font", "")
            if legacy_expr and not legacy_expr.startswith("EGUI_CONFIG"):
                parsed = parse_legacy_font_expr(legacy_expr)
                if parsed:
                    filename = _guess_filename_from_c_name(
                        parsed["name"],
                        [".ttf", ".otf"],
                        src_dir,
                    )
                    # Only set font_file for non-builtin fonts (not montserrat)
                    if "montserrat" not in parsed["name"]:
                        w.properties["font_file"] = filename
                        w.properties["font_pixelsize"] = parsed["pixelsize"]
                        w.properties["font_fontbitsize"] = parsed["fontbitsize"]
                    else:
                        # Built-in montserrat font: keep as font_builtin
                        w.properties["font_builtin"] = legacy_expr
            elif legacy_expr:
                # EGUI_CONFIG_FONT_DEFAULT or similar
                w.properties["font_builtin"] = legacy_expr

        # Parse children, background, shadow, and animations
        for child_elem in elem:
            if child_elem.tag == "Background":
                w.background = BackgroundModel.from_xml_element(child_elem)
            elif child_elem.tag == "Shadow":
                w.shadow = ShadowModel.from_xml_element(child_elem)
            elif child_elem.tag == "Animation":
                w.animations.append(AnimationModel.from_xml_element(child_elem))
            else:
                child = cls.from_xml_element(child_elem, src_dir=src_dir)
                w.add_child(child)
        return w

    def get_all_widgets_flat(self):
        """Return flat list of self and all descendants (depth-first)."""
        result = [self]
        for child in self.children:
            result.extend(child.get_all_widgets_flat())
        return result

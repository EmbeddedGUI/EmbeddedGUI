"""Helpers for widget animation editing in UI Designer."""

from __future__ import annotations

from .widget_model import ALPHAS, ANIMATION_TYPES, AnimationModel, COLORS, INTERPOLATOR_TYPES


ANIMATION_REPEAT_MODES = ["restart", "reverse"]

ANIMATION_RESIZE_MODES = [
    "EGUI_ANIMATION_RESIZE_MODE_LEFT",
    "EGUI_ANIMATION_RESIZE_MODE_CENTER",
    "EGUI_ANIMATION_RESIZE_MODE_RIGHT",
]

DEFAULT_ANIMATION_PARAMS = {
    "alpha": {
        "from_alpha": "EGUI_ALPHA_100",
        "to_alpha": "EGUI_ALPHA_20",
    },
    "translate": {
        "from_x": "0",
        "to_x": "0",
        "from_y": "0",
        "to_y": "32",
    },
    "scale": {
        "from_scale": "EGUI_FLOAT_VALUE(1.0f)",
        "to_scale": "EGUI_FLOAT_VALUE(1.15f)",
    },
    "resize": {
        "from_width_ratio": "EGUI_FLOAT_VALUE(1.0f)",
        "to_width_ratio": "EGUI_FLOAT_VALUE(1.2f)",
        "from_height_ratio": "EGUI_FLOAT_VALUE(1.0f)",
        "to_height_ratio": "EGUI_FLOAT_VALUE(1.2f)",
        "mode": "EGUI_ANIMATION_RESIZE_MODE_CENTER",
    },
    "color": {
        "from_color": "EGUI_COLOR_WHITE",
        "to_color": "EGUI_COLOR_ORANGE",
    },
}


def animation_param_defaults(anim_type):
    return dict(DEFAULT_ANIMATION_PARAMS.get(anim_type, {}))


def _bool_value(value):
    if isinstance(value, str):
        return value.strip().lower() in ("1", "true", "yes", "on")
    return bool(value)


def _int_value(value, fallback=0, minimum=0):
    try:
        value = int(value)
    except (TypeError, ValueError):
        value = fallback
    return max(minimum, value)


def create_default_animation(anim_type="alpha"):
    animation = AnimationModel()
    normalized_type = anim_type if anim_type in ANIMATION_TYPES else "alpha"
    animation.anim_type = normalized_type
    animation.duration = 500
    animation.interpolator = "linear"
    animation.repeat_count = 0
    animation.repeat_mode = "restart"
    animation.auto_start = True
    animation.params = animation_param_defaults(normalized_type)
    return animation


def normalize_animation(animation):
    if isinstance(animation, AnimationModel):
        data = animation.to_dict()
    else:
        data = dict(animation or {})

    anim_type = str(data.get("anim_type", "alpha") or "").strip()
    if anim_type not in ANIMATION_TYPES:
        anim_type = "alpha"

    result = create_default_animation(anim_type)
    result.duration = _int_value(data.get("duration", 500), fallback=500, minimum=0)

    interpolator = str(data.get("interpolator", "linear") or "").strip()
    result.interpolator = interpolator if interpolator in INTERPOLATOR_TYPES else "linear"

    result.repeat_count = _int_value(data.get("repeat_count", 0), fallback=0, minimum=0)

    repeat_mode = str(data.get("repeat_mode", "restart") or "").strip()
    result.repeat_mode = repeat_mode if repeat_mode in ANIMATION_REPEAT_MODES else "restart"

    result.auto_start = _bool_value(data.get("auto_start", True))

    raw_params = data.get("params", {}) or {}
    normalized_params = {}
    for key, default_value in animation_param_defaults(result.anim_type).items():
        value = raw_params.get(key, default_value)
        text = str(value).strip() if value not in (None, "") else str(default_value)
        normalized_params[key] = text or str(default_value)
    result.params = normalized_params
    return result


def clone_animation(animation):
    return normalize_animation(animation)


def normalize_widget_animations(animations):
    return [normalize_animation(animation) for animation in (animations or [])]


def animation_type_names():
    return list(ANIMATION_TYPES.keys())


def animation_interpolator_names():
    return list(INTERPOLATOR_TYPES.keys())


def animation_param_choices(param_name):
    if param_name in ("from_alpha", "to_alpha"):
        return list(ALPHAS)
    if param_name in ("from_color", "to_color"):
        return list(COLORS)
    if param_name == "mode":
        return list(ANIMATION_RESIZE_MODES)
    return []

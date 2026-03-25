"""C code generator for EmbeddedGUI Designer — MFC-style multi-file output.

Architecture (per page):
    {page_name}.h          — struct definition (generated, auto page fields + USER CODE regions)
    {page_name}_layout.c   — pure layout code (100% generated, always overwritten)
    {page_name}.c          — user implementation (skeleton, created once, NEVER overwritten)

Framework files (fully generated, always overwritten):
    uicode.h               — page enum, exported functions
    uicode.c               — page switching backend (EasyPage or Activity mode)
    app_egui_config.h      — screen config (only if not present)

The {page_name}_layout.c exports ``egui_{page}_layout_init()`` which is
called by the user's ``egui_{page}_on_open()`` in {page_name}.c.

This achieves zero overlap between generated and user code:
  - Layout changes only touch *_layout.c (fully generated)
  - User logic lives entirely in {page_name}.c (never overwritten)
  - The .h bridges both via struct definition + function declarations

Reference patterns taken from example/HelloEasyPage.
"""

import re

from ..model.widget_model import (
    WidgetModel, BackgroundModel,
    AnimationModel, ANIMATION_TYPES, INTERPOLATOR_TYPES,
    derive_image_c_expr, derive_font_c_expr,
)
from ..model.page_fields import page_field_declaration, page_field_default_assignment, valid_page_fields
from ..model.page_timers import valid_page_timers
from ..model.widget_registry import WidgetRegistry
from ..model.string_resource import parse_string_ref


def _get_type_info(widget_type):
    """Get type info from registry."""
    return WidgetRegistry.instance().get(widget_type)


# ── Helpers ────────────────────────────────────────────────────────

def _simple_init_func(widget_type):
    """Get the simple init function (without params) for a widget type."""
    info = _get_type_info(widget_type)
    func = info.get("init_func", "")
    return func.replace("_with_params", "")


def _bg_macro_name(bg_type):
    """Map bg_type to the EGUI background macro suffix."""
    mapping = {
        "solid": "SOLID",
        "round_rectangle": "ROUND_RECTANGLE",
        "round_rectangle_corners": "ROUND_RECTANGLE_CORNERS",
        "circle": "CIRCLE",
    }
    return mapping.get(bg_type, "SOLID")


def _gradient_dir_macro(direction):
    """Map gradient direction string to C macro."""
    mapping = {
        "vertical": "EGUI_BACKGROUND_GRADIENT_DIR_VERTICAL",
        "horizontal": "EGUI_BACKGROUND_GRADIENT_DIR_HORIZONTAL",
    }
    return mapping.get(direction, "EGUI_BACKGROUND_GRADIENT_DIR_VERTICAL")


def _gen_bg_param_init(param_name, bg, bg_type_override=None):
    """Generate a EGUI_BACKGROUND_COLOR_PARAM_INIT_xxx(...) line."""
    bt = bg_type_override or bg.bg_type

    # Gradient background uses a different macro
    if bt == "gradient":
        direction = _gradient_dir_macro(bg.direction)
        return (
            f"EGUI_BACKGROUND_GRADIENT_PARAM_INIT({param_name}, "
            f"{direction}, {bg.start_color}, {bg.end_color}, {bg.alpha});"
        )
    macro = _bg_macro_name(bt)

    if bg.stroke_width > 0:
        macro += "_STROKE"

    args = [param_name]

    if bt == "solid":
        args += [bg.color, bg.alpha]
    elif bt == "round_rectangle":
        args += [bg.color, bg.alpha, str(bg.radius)]
    elif bt == "round_rectangle_corners":
        args += [
            bg.color, bg.alpha,
            str(bg.radius_left_top), str(bg.radius_left_bottom),
            str(bg.radius_right_top), str(bg.radius_right_bottom),
        ]
    elif bt == "circle":
        args += [bg.color, bg.alpha, str(bg.radius)]

    if bg.stroke_width > 0:
        args += [str(bg.stroke_width), bg.stroke_color, bg.stroke_alpha]

    return f"EGUI_BACKGROUND_COLOR_PARAM_INIT_{macro}({', '.join(args)});"


def _upper_guard(name):
    """Convert page name to header guard: main_page -> _MAIN_PAGE_H_"""
    return f"_{name.upper()}_H_"


def _timer_helper_names(name):
    prefix = f"egui_{name}"
    return {
        "init": f"{prefix}_timers_init",
        "start_auto": f"{prefix}_timers_start_auto",
        "stop": f"{prefix}_timers_stop",
    }


def _format_callback_signature(signature, func_name):
    if not signature or not func_name:
        return ""
    try:
        return signature.format(func_name=func_name)
    except Exception:
        return signature.replace("{func_name}", func_name)


def _extract_parameter_names(signature_line):
    if "(" not in signature_line or ")" not in signature_line:
        return []
    params = signature_line.split("(", 1)[1].rsplit(")", 1)[0].strip()
    if not params or params == "void":
        return []

    names = []
    for raw_param in params.split(","):
        param = raw_param.strip()
        if not param or param == "void":
            continue
        match = re.search(r"([A-Za-z_]\w*)\s*(?:\[[^\]]*\])?$", param)
        if match:
            names.append(match.group(1))
    return names


def collect_page_callback_stubs(page):
    """Collect callback skeleton metadata for a page."""
    callbacks = []
    seen = set()

    if page is None:
        return callbacks

    for widget in page.get_all_widgets():
        if widget.on_click:
            key = ("view", widget.on_click)
            if key not in seen:
                seen.add(key)
                callbacks.append(
                    {
                        "kind": "view",
                        "name": widget.on_click,
                        "signature": "void {func_name}(egui_view_t *self)",
                    }
                )

        type_info = _get_type_info(widget.widget_type)
        events_def = type_info.get("events", {})
        for event_name, func_name in sorted(widget.events.items()):
            if not func_name:
                continue
            event_info = events_def.get(event_name)
            if not event_info:
                continue
            key = ("view", func_name)
            if key in seen:
                continue
            seen.add(key)
            callbacks.append(
                {
                    "kind": "view",
                    "name": func_name,
                    "signature": event_info.get("signature", ""),
                }
            )

    for timer in valid_page_timers(page, getattr(page, "timers", [])):
        callback_name = timer.get("callback", "")
        if not callback_name:
            continue
        key = ("timer", callback_name)
        if key in seen:
            continue
        seen.add(key)
        callbacks.append(
            {
                "kind": "timer",
                "name": callback_name,
                "signature": "void {func_name}(egui_timer_t *timer)",
            }
        )

    return callbacks


def render_page_callback_stub(page, callback_name, signature, kind="view"):
    """Render a callback stub suitable for a page user source file."""
    signature_line = _format_callback_signature(signature, callback_name)
    if not signature_line:
        return ""

    lines = [signature_line, "{"]
    if kind == "timer":
        struct_type = f"egui_{page.name}_t"
        lines.append(f"    {struct_type} *local = ({struct_type} *)timer->user_data;")
        lines.append("    EGUI_UNUSED(local);")
        lines.append("    // TODO: Handle timer tick here")
    else:
        for param_name in _extract_parameter_names(signature_line):
            lines.append(f"    EGUI_UNUSED({param_name});")
        lines.append("    // TODO: Handle callback here")
    lines.append("}")
    return "\n".join(lines)


# ── Per-widget init code (data-driven) ───────────────────────────

def _emit_property_code(widget, prop_name, prop_def, cg, cast, indent):
    """Emit a single C line from a code_gen descriptor.

    Returns a string (one C statement) or None if nothing to emit.
    """
    kind = cg["kind"]

    # Kinds that carry no runtime setter (used only for params / struct init)
    if kind in ("param_only", "param"):
        return None

    func = cg["func"]
    value = widget.properties.get(prop_name, prop_def.get("default"))

    if kind == "setter":
        # Skip default values by default (explicit skip_default: False to override)
        skip = cg.get("skip_default", True)
        if skip and value == prop_def.get("default"):
            return None
        skip_values = cg.get("skip_values", [])
        if value in skip_values:
            return None
        # Value mapping
        value_map = cg.get("value_map")
        if value_map and value in value_map:
            c_value = value_map[value]
        elif cg.get("bool_to_int"):
            c_value = "1" if value else "0"
        else:
            c_value = str(value)
        return f"{indent}{func}({cast}, {c_value});"

    elif kind == "text_setter":
        text = str(value or "")
        if not text:
            return None
        str_key = parse_string_ref(text)
        if str_key is not None:
            enum_name = f"EGUI_STR_{str_key.upper()}"
            return f'{indent}{func}({cast}, egui_i18n_get({enum_name}));'
        return f'{indent}{func}({cast}, "{text}");'

    elif kind == "multi_setter":
        # Check skip conditions
        if cg.get("skip_default") and value == prop_def.get("default"):
            return None
        # Collect values from multiple properties referenced in args template
        args_tpl = cg["args"]
        # Normalize list form to comma-separated string (e.g. ["{year}", "{month}"])
        if isinstance(args_tpl, list):
            args_tpl = ", ".join(args_tpl)
        props = widget.properties
        # Replace {prop_name} placeholders with actual values
        import re as _re
        def _repl(m):
            key = m.group(1)
            return str(props.get(key, prop_def.get("default", "")))
        c_args = _re.sub(r'\{(\w+)\}', _repl, args_tpl)
        return f"{indent}{func}({cast}, {c_args});"

    elif kind == "derived_setter":
        derive_type = cg["derive"]
        cast_prefix = cg.get("cast", "")
        if derive_type == "font":
            expr = derive_font_c_expr(widget)
        elif derive_type == "image":
            expr = derive_image_c_expr(widget)
        else:
            return None
        if not expr or expr == "NULL":
            return None
        return f"{indent}{func}({cast}, {cast_prefix}{expr});"

    elif kind == "image_setter":
        expr = derive_image_c_expr(widget)
        if not expr or expr == "NULL":
            return None
        cast_prefix = cg.get("cast", "")
        return f"{indent}{func}({cast}, {cast_prefix}{expr});"

    return None


def _gen_widget_init_lines(widget, indent="    "):
    """Generate explicit API calls to init a single widget.

    Uses ``local->{name}`` as the widget reference (inside on_open).
    Returns a list of lines (without leading newlines).

    Property-specific code is driven by the ``code_gen`` descriptors
    in WIDGET_TYPES, so new widget types need no changes here.
    """
    lines = []
    wt = widget.widget_type
    ref = f"local->{widget.name}"
    cast = f"(egui_view_t *)&{ref}"

    # Widget name comment for readability
    lines.append(f"{indent}// {widget.name} ({wt})")

    # Init
    init_func = _simple_init_func(wt)
    if init_func:
        lines.append(f"{indent}{init_func}({cast});")

    # Position & Size
    lines.append(f"{indent}egui_view_set_position({cast}, {widget.x}, {widget.y});")
    lines.append(f"{indent}egui_view_set_size({cast}, {widget.width}, {widget.height});")

    # Margin
    has_individual = (widget.margin_left or widget.margin_right or
                      widget.margin_top or widget.margin_bottom)
    if has_individual:
        lines.append(
            f"{indent}egui_view_set_margin({cast}, "
            f"{widget.margin_left}, {widget.margin_right}, "
            f"{widget.margin_top}, {widget.margin_bottom});"
        )
    elif widget.margin != 0:
        lines.append(f"{indent}egui_view_set_margin_all({cast}, {widget.margin});")

    # ── Data-driven property setters ──
    type_info = _get_type_info(wt)
    props_def = type_info.get("properties", {})
    emitted_groups = set()

    for prop_name, prop_def in props_def.items():
        cg = prop_def.get("code_gen")
        if cg is None:
            continue

        # Skip if this property belongs to a group already emitted
        group = cg.get("group")
        if group:
            if group in emitted_groups:
                continue
            emitted_groups.add(group)

        line = _emit_property_code(widget, prop_name, prop_def, cg, cast, indent)
        if line:
            lines.append(line)

    # on_click listener
    if widget.on_click:
        lines.append(f"{indent}egui_view_set_on_click_listener({cast}, {widget.on_click});")

    # Event listeners (data-driven from registry)
    type_info = _get_type_info(wt)
    events_def = type_info.get("events", {})
    for event_name, func_name in widget.events.items():
        if not func_name:
            continue
        event_info = events_def.get(event_name)
        if not event_info:
            continue
        setter = event_info["setter"]
        lines.append(f"{indent}{setter}({cast}, {func_name});")

    return lines


# ── Background generation (file-scope declarations) ──────────────

def _gen_bg_declarations(all_widgets, page_name):
    """Generate background declaration lines at file scope.

    Returns (decl_lines, init_lines).
      decl_lines: file-scope static declarations
      init_lines: code to run inside on_open (init_with_params + set_background)
    """
    decl_lines = []
    init_lines = []

    bg_widgets = [w for w in all_widgets if w.background and w.background.bg_type != "none"]
    if not bg_widgets:
        return decl_lines, init_lines

    decl_lines.append("// Background declarations")

    for w in bg_widgets:
        bg = w.background
        bg_name = f"bg_{page_name}_{w.name}"
        is_gradient = (bg.bg_type == "gradient")

        # Static object — different type for gradient
        if is_gradient:
            decl_lines.append(f"static egui_background_gradient_t {bg_name};")
        else:
            decl_lines.append(f"static egui_background_color_t {bg_name};")

        # Normal param
        normal_param = f"{bg_name}_param_normal"
        decl_lines.append(_gen_bg_param_init(normal_param, bg))

        # Pressed param
        pressed_ref = "NULL"
        if bg.has_pressed:
            pressed_bg = BackgroundModel()
            pressed_bg.bg_type = bg.bg_type
            pressed_bg.color = bg.pressed_color
            pressed_bg.alpha = bg.pressed_alpha
            pressed_bg.radius = bg.radius
            pressed_bg.radius_left_top = bg.radius_left_top
            pressed_bg.radius_left_bottom = bg.radius_left_bottom
            pressed_bg.radius_right_top = bg.radius_right_top
            pressed_bg.radius_right_bottom = bg.radius_right_bottom
            pressed_bg.stroke_width = bg.stroke_width
            pressed_bg.stroke_color = bg.stroke_color
            pressed_bg.stroke_alpha = bg.stroke_alpha
            pressed_param = f"{bg_name}_param_pressed"
            decl_lines.append(_gen_bg_param_init(pressed_param, pressed_bg))
            pressed_ref = f"&{pressed_param}"

        # Disabled param
        disabled_ref = "NULL"
        if bg.has_disabled:
            disabled_bg = BackgroundModel()
            disabled_bg.bg_type = "solid"
            disabled_bg.color = bg.disabled_color
            disabled_bg.alpha = bg.disabled_alpha
            disabled_bg.stroke_width = 0
            disabled_param = f"{bg_name}_param_disabled"
            decl_lines.append(_gen_bg_param_init(disabled_param, disabled_bg, "solid"))
            disabled_ref = f"&{disabled_param}"

        # Combined params
        decl_lines.append(
            f"EGUI_BACKGROUND_PARAM_INIT({bg_name}_params, "
            f"&{normal_param}, {pressed_ref}, {disabled_ref});"
        )
        decl_lines.append("")

        # Init code for on_open
        if is_gradient:
            init_lines.append(
                f"    egui_background_gradient_init_with_params("
                f"(egui_background_t *)&{bg_name}, &{bg_name}_params);"
            )
        else:
            init_lines.append(
                f"    egui_background_color_init_with_params("
                f"(egui_background_t *)&{bg_name}, &{bg_name}_params);"
            )
        init_lines.append(
            f"    egui_view_set_background("
            f"(egui_view_t *)&local->{w.name}, (egui_background_t *)&{bg_name});"
        )

        # Card widgets have their own bg_color/border drawn in on_draw(),
        # which covers the view background. Sync them from the Background.
        if w.widget_type == "card":
            init_lines.append(
                f"    egui_view_card_set_bg_color("
                f"(egui_view_t *)&local->{w.name}, {bg.color}, {bg.alpha});"
            )
            if bg.stroke_width > 0:
                init_lines.append(
                    f"    egui_view_card_set_border("
                    f"(egui_view_t *)&local->{w.name}, {bg.stroke_width}, {bg.stroke_color});"
                )
            else:
                init_lines.append(
                    f"    egui_view_card_set_border("
                    f"(egui_view_t *)&local->{w.name}, 0, {bg.color});"
                )

    return decl_lines, init_lines


# ── Shadow generation (file-scope declarations) ──────────────────

def _gen_shadow_declarations(all_widgets, page_name):
    """Generate shadow declaration lines at file scope.

    Returns (decl_lines, init_lines).
      decl_lines: file-scope static shadow param declarations
      init_lines: code to run inside on_open (egui_view_set_shadow calls)
    """
    decl_lines = []
    init_lines = []

    shadow_widgets = [w for w in all_widgets if w.shadow is not None]
    if not shadow_widgets:
        return decl_lines, init_lines

    decl_lines.append("// Shadow declarations")

    for w in shadow_widgets:
        s = w.shadow
        shadow_name = f"shadow_{page_name}_{w.name}"

        if s.corner_radius > 0:
            decl_lines.append(
                f"EGUI_SHADOW_PARAM_INIT_ROUND({shadow_name}, "
                f"{s.width}, {s.ofs_x}, {s.ofs_y}, "
                f"{s.color}, {s.opa}, {s.corner_radius});"
            )
        else:
            decl_lines.append(
                f"EGUI_SHADOW_PARAM_INIT({shadow_name}, "
                f"{s.width}, {s.ofs_x}, {s.ofs_y}, "
                f"{s.color}, {s.opa});"
            )

        init_lines.append(
            f"    egui_view_set_shadow("
            f"(egui_view_t *)&local->{w.name}, &{shadow_name});"
        )

    decl_lines.append("")
    return decl_lines, init_lines


# ── Animation generation (file-scope declarations) ────────────────

def _gen_anim_declarations(all_widgets, page_name):
    """Generate animation declaration and init lines.

    Returns (decl_lines, init_lines).
    """
    decl_lines = []
    init_lines = []

    anim_widgets = [w for w in all_widgets if w.animations]
    if not anim_widgets:
        return decl_lines, init_lines

    decl_lines.append("// Animation declarations")
    for w in anim_widgets:
        for i, anim in enumerate(w.animations):
            ainfo = ANIMATION_TYPES.get(anim.anim_type)
            if not ainfo:
                continue
            suffix = f"_{i}" if len(w.animations) > 1 else ""
            anim_name = f"anim_{w.name}_{anim.anim_type}{suffix}"
            member = f"local->{anim_name}"
            anim_cast = f"EGUI_ANIM_OF(&{member})"
            view_cast = f"(egui_view_t *)&local->{w.name}"

            # Params macro
            param_vals = ", ".join(
                str(anim.params.get(p, "0")) for p in ainfo["params"]
            )
            decl_lines.append(
                f"{ainfo['params_macro']}({anim_name}_params, {param_vals});"
            )

            # Interpolator instance
            interp_key = anim.interpolator
            interp_base = INTERPOLATOR_TYPES.get(interp_key, "egui_interpolator_linear")
            decl_lines.append(
                f"static {interp_base}_t {anim_name}_interpolator;"
            )
            decl_lines.append("")

            # Init lines (inside layout_init)
            init_lines.append(
                f"    {ainfo['init_func']}({anim_cast});"
            )
            init_lines.append(
                f"    {ainfo['params_set_func']}(&{member}, &{anim_name}_params);"
            )
            init_lines.append(
                f"    egui_animation_duration_set({anim_cast}, {anim.duration});"
            )
            # Interpolator init + set
            init_lines.append(
                f"    {interp_base}_init((egui_interpolator_t *)&{anim_name}_interpolator);"
            )
            init_lines.append(
                f"    egui_animation_interpolator_set({anim_cast}, "
                f"(egui_interpolator_t *)&{anim_name}_interpolator);"
            )
            # Repeat settings
            if anim.repeat_count != 0:
                init_lines.append(
                    f"    egui_animation_repeat_count_set({anim_cast}, {anim.repeat_count});"
                )
            if anim.repeat_mode == "reverse":
                init_lines.append(
                    f"    egui_animation_repeat_mode_set({anim_cast}, "
                    f"EGUI_ANIMATION_REPEAT_MODE_REVERSE);"
                )
            # Target view
            init_lines.append(
                f"    egui_animation_target_view_set({anim_cast}, {view_cast});"
            )
            # Auto start
            if anim.auto_start:
                init_lines.append(
                    f"    egui_animation_start({anim_cast});"
                )
            init_lines.append("")

    return decl_lines, init_lines


# ── Page Header (.h) ─────────────────────────────────────────────

def generate_page_header(page, project):
    """Generate the {page_name}.h file content.

    This file IS regenerated on every save, but preserves a USER CODE
    region for manual struct members after the Designer-generated page fields.

    Declares both:
      - ``egui_{page}_layout_init()``  (implemented in *_layout.c, generated)
      - ``egui_{page}_init()``          (implemented in *.c, user-owned)
    """
    name = page.name
    guard = _upper_guard(name)
    struct_name = f"egui_{name}"
    struct_type = f"egui_{name}_t"

    lines = []
    lines.append(f"#ifndef {guard}")
    lines.append(f"#define {guard}")
    lines.append("")
    lines.append("// ===== Auto-generated by EmbeddedGUI Designer =====")
    lines.append("// This file is regenerated when the layout changes.")
    lines.append("// Only the USER CODE regions are preserved.")
    lines.append("")
    lines.append('#include "egui.h"')
    lines.append("")
    lines.append("// USER CODE BEGIN includes")
    lines.append("// USER CODE END includes")
    lines.append("")
    lines.append("/* Set up for C function definitions, even when using C++ */")
    lines.append("#ifdef __cplusplus")
    lines.append('extern "C" {')
    lines.append("#endif")
    lines.append("")

    # Typedef + struct
    lines.append(f"typedef struct {struct_name} {struct_type};")
    lines.append(f"struct {struct_name}")
    lines.append("{")
    lines.append("    egui_page_base_t base;")
    lines.append("")

    # Widget members (all widgets in this page) — generated
    all_widgets = page.get_all_widgets()
    if all_widgets:
        lines.append("    // UI widgets (auto-generated, do not edit)")
        for w in all_widgets:
            type_info = _get_type_info(w.widget_type)
            c_type = type_info.get("c_type", "egui_view_t")
            lines.append(f"    {c_type} {w.name};")
        lines.append("")

    # Animation struct members
    anim_widgets = [w for w in all_widgets if w.animations]
    if anim_widgets:
        lines.append("    // Animations (auto-generated, do not edit)")
        for w in anim_widgets:
            for i, anim in enumerate(w.animations):
                ainfo = ANIMATION_TYPES.get(anim.anim_type, {})
                c_type = ainfo.get("c_type", "egui_animation_t")
                suffix = f"_{i}" if len(w.animations) > 1 else ""
                lines.append(
                    f"    {c_type} anim_{w.name}_{anim.anim_type}{suffix};"
                )
        lines.append("")

    generated_timers = valid_page_timers(page, getattr(page, "timers", []))
    if generated_timers:
        lines.append("    // Timers (auto-generated from Designer metadata)")
        for timer in generated_timers:
            lines.append(f"    egui_timer_t {timer['name']};")
        lines.append("")

    generated_page_fields = [field for field in valid_page_fields(page, page.user_fields) if page_field_declaration(field)]
    if generated_page_fields:
        lines.append("    // Page fields (auto-generated from Designer metadata)")
        for field in generated_page_fields:
            lines.append(f"    {page_field_declaration(field)}")
        lines.append("")

    # User-defined struct members
    lines.append("    // USER CODE BEGIN user_fields")
    lines.append("    // USER CODE END user_fields")
    lines.append("};")
    lines.append("")

    # Function declarations
    lines.append(f"// Layout init (auto-generated in {name}_layout.c)")
    lines.append(f"void {struct_name}_layout_init(egui_page_base_t *self);")
    lines.append("")
    lines.append(f"// Page init (user-implemented in {name}.c)")
    lines.append(f"void {struct_name}_init(egui_page_base_t *self);")
    lines.append("")
    if generated_timers:
        helper_names = _timer_helper_names(name)
        lines.append(f"// Timer helpers (auto-generated in {name}_layout.c)")
        lines.append(f"void {helper_names['init']}(egui_page_base_t *self);")
        lines.append(f"void {helper_names['start_auto']}(egui_page_base_t *self);")
        lines.append(f"void {helper_names['stop']}(egui_page_base_t *self);")
        lines.append("")
    lines.append("// USER CODE BEGIN declarations")
    lines.append("// USER CODE END declarations")
    lines.append("")
    lines.append("/* Ends C function definitions when using C++ */")
    lines.append("#ifdef __cplusplus")
    lines.append("}")
    lines.append("#endif")
    lines.append("")
    lines.append(f"#endif /* {guard} */")
    lines.append("")

    return "\n".join(lines)


# ── Page Layout Source (*_layout.c) — 100% generated ─────────────

def generate_page_layout_source(page, project):
    """Generate the {page_name}_layout.c file content.

    This file is 100% auto-generated and overwritten on every save.
    It contains NO user code regions. All widget initialization,
    hierarchy building, background setup, and layout logic lives here.

    Exports: ``void egui_{page}_layout_init(egui_page_base_t *self)``
    """
    name = page.name
    prefix = f"egui_{name}"
    struct_type = f"egui_{name}_t"
    all_widgets = page.get_all_widgets()

    lines = []
    lines.append("// ===== Auto-generated by EmbeddedGUI Designer =====")
    lines.append("// DO NOT EDIT — this file is fully regenerated on every save.")
    lines.append(f"// User logic belongs in {name}.c (never overwritten).")
    lines.append("")
    lines.append('#include "egui.h"')
    lines.append("#include <stdlib.h>")
    lines.append("")
    lines.append('#include "uicode.h"')
    lines.append(f'#include "{name}.h"')

    # Include i18n header if any widget uses @string/ references
    _has_string_refs = False
    for w in all_widgets:
        text = w.properties.get("text", "")
        if parse_string_ref(text) is not None:
            _has_string_refs = True
            break
    if _has_string_refs:
        lines.append('#include "egui_strings.h"')

    lines.append("")

    # Forward declarations for onClick callbacks (defined in user .c file)
    callback_funcs = set()
    for w in all_widgets:
        if w.on_click:
            callback_funcs.add(w.on_click)
    if callback_funcs:
        lines.append("// Forward declarations for onClick callbacks")
        for func_name in sorted(callback_funcs):
            lines.append(f"extern void {func_name}(egui_view_t *self);")
        lines.append("")

    # Forward declarations for event callbacks (defined in user .c file)
    event_decls = []  # (signature_line, sort_key)
    for w in all_widgets:
        type_info = _get_type_info(w.widget_type)
        events_def = type_info.get("events", {})
        for event_name, func_name in w.events.items():
            if not func_name:
                continue
            event_info = events_def.get(event_name)
            if not event_info:
                continue
            sig = event_info["signature"].format(func_name=func_name)
            decl = f"extern {sig};"
            event_decls.append((decl, func_name))
    if event_decls:
        lines.append("// Forward declarations for event callbacks")
        seen = set()
        for decl, key in sorted(event_decls, key=lambda x: x[1]):
            if decl not in seen:
                seen.add(decl)
                lines.append(decl)
        lines.append("")

    # Background file-scope declarations
    bg_decl_lines, bg_init_lines = _gen_bg_declarations(all_widgets, name)
    if bg_decl_lines:
        lines.extend(bg_decl_lines)
        lines.append("")

    # Shadow file-scope declarations
    shadow_decl_lines, shadow_init_lines = _gen_shadow_declarations(all_widgets, name)
    if shadow_decl_lines:
        lines.extend(shadow_decl_lines)
        lines.append("")

    # Animation file-scope declarations (params + interpolators)
    anim_decl_lines, anim_init_lines = _gen_anim_declarations(all_widgets, name)
    if anim_decl_lines:
        lines.extend(anim_decl_lines)
        lines.append("")

    generated_timers = valid_page_timers(page, getattr(page, "timers", []))
    if generated_timers:
        callback_names = sorted({timer["callback"] for timer in generated_timers if timer.get("callback")})
        if callback_names:
            lines.append("// Forward declarations for timer callbacks")
            for callback_name in callback_names:
                lines.append(f"extern void {callback_name}(egui_timer_t *timer);")
            lines.append("")

        helper_names = _timer_helper_names(name)
        auto_start_timers = [timer for timer in generated_timers if timer.get("auto_start")]

        lines.append(f"void {helper_names['init']}(egui_page_base_t *self)")
        lines.append("{")
        lines.append(f"    {struct_type} *local = ({struct_type} *)self;")
        for timer in generated_timers:
            lines.append(
                f"    egui_timer_init_timer(&local->{timer['name']}, (void *)local, {timer['callback']});"
            )
        lines.append("}")
        lines.append("")

        lines.append(f"void {helper_names['start_auto']}(egui_page_base_t *self)")
        lines.append("{")
        lines.append(f"    {struct_type} *local = ({struct_type} *)self;")
        if auto_start_timers:
            for timer in auto_start_timers:
                lines.append(
                    f"    egui_timer_start_timer(&local->{timer['name']}, {timer['delay_ms']}, {timer['period_ms']});"
                )
        else:
            lines.append("    EGUI_UNUSED(local);")
        lines.append("}")
        lines.append("")

        lines.append(f"void {helper_names['stop']}(egui_page_base_t *self)")
        lines.append("{")
        lines.append(f"    {struct_type} *local = ({struct_type} *)self;")
        for timer in generated_timers:
            lines.append(f"    egui_timer_stop_timer(&local->{timer['name']});")
        lines.append("}")
        lines.append("")

    # ── layout_init function ──
    lines.append(f"void {prefix}_layout_init(egui_page_base_t *self)")
    lines.append("{")
    lines.append(f"    {struct_type} *local = ({struct_type} *)self;")
    lines.append("")

    field_init_lines = []
    for field in valid_page_fields(page, page.user_fields):
        assignment = page_field_default_assignment(field)
        if assignment:
            field_init_lines.append(f"    {assignment}")
    if field_init_lines:
        lines.append("    // Initialize page fields")
        lines.extend(field_init_lines)
        lines.append("")

    if all_widgets:
        # Init each widget
        lines.append("    // Init views")
        for w in all_widgets:
            lines.extend(_gen_widget_init_lines(w))
            lines.append("")

        # Set backgrounds
        if bg_init_lines:
            lines.append("    // Set backgrounds")
            lines.extend(bg_init_lines)
            lines.append("")

        # Set shadows
        if shadow_init_lines:
            lines.append("    // Set shadows")
            lines.extend(shadow_init_lines)
            lines.append("")

        # Build hierarchy (add children)
        hierarchy_lines = []
        for w in all_widgets:
            if not w.children:
                continue
            type_info = _get_type_info(w.widget_type)
            add_func = type_info.get("add_child_func")
            if add_func:
                for child in w.children:
                    hierarchy_lines.append(
                        f"    {add_func}("
                        f"(egui_view_t *)&local->{w.name}, "
                        f"(egui_view_t *)&local->{child.name});"
                    )
        if hierarchy_lines:
            lines.append("    // Build hierarchy")
            lines.extend(hierarchy_lines)
            lines.append("")

        # Layout (for containers with layout_func)
        layout_lines = []
        for w in all_widgets:
            if not w.children:
                continue
            type_info = _get_type_info(w.widget_type)
            layout_func = type_info.get("layout_func")
            if layout_func:
                # Skip auto-layout if any child has explicit (non-zero) position
                has_explicit_pos = any(
                    c.x != 0 or c.y != 0 for c in w.children
                )
                if has_explicit_pos:
                    continue
                args_tpl = type_info.get("layout_func_args")
                if args_tpl:
                    # Expand {prop} placeholders from widget properties
                    import re as _re
                    def _repl_layout(m, _w=w):
                        key = m.group(1)
                        if key == "orientation_value":
                            ori = _w.properties.get("orientation", "vertical")
                            return "1" if ori == "horizontal" else "0"
                        return str(_w.properties.get(key, ""))
                    extra = _re.sub(r'\{(\w+)\}', _repl_layout, args_tpl)
                    layout_lines.append(
                        f"    {layout_func}((egui_view_t *)&local->{w.name}, {extra});"
                    )
                else:
                    layout_lines.append(
                        f"    {layout_func}((egui_view_t *)&local->{w.name});"
                    )
        if layout_lines:
            lines.append("    // Re-layout children")
            lines.extend(layout_lines)
            lines.append("")

        # Init animations
        if anim_init_lines:
            lines.append("    // Init animations")
            lines.extend(anim_init_lines)
            lines.append("")

        # Add root widget(s) to page
        if page.root_widget:
            root_name = page.root_widget.name
            lines.append("    // Add to page root")
            lines.append(
                f"    egui_page_base_add_view(self, (egui_view_t *)&local->{root_name});"
            )
            lines.append("")

            # If root widget has a background, apply it to self->root_view
            root_bg = page.root_widget.background
            if root_bg and root_bg.bg_type != "none":
                bg_name = f"bg_{name}_{root_name}"
                lines.append("    // Set page background")
                lines.append(
                    f"    egui_view_set_background("
                    f"(egui_view_t *)&self->root_view, "
                    f"(egui_background_t *)&{bg_name});"
                )
                lines.append("")

    lines.append("}")
    lines.append("")

    return "\n".join(lines)


# ── Page User Source (.c) — created once, never overwritten ──────

def generate_page_user_source(page, project):
    """Generate the {page_name}.c user implementation skeleton.

    This file is only created when it does not yet exist. Once created,
    the designer NEVER overwrites it — it belongs to the user.

    Contains:
      - on_open   (calls layout_init, then user's custom logic)
      - on_close  (calls super, then user cleanup)
      - on_key_pressed (user key handling)
      - vtable
      - init      (calls super, sets vtable, then user init)
    """
    name = page.name
    prefix = f"egui_{name}"
    struct_type = f"egui_{name}_t"
    helper_names = _timer_helper_names(name)
    generated_timers = valid_page_timers(page, getattr(page, "timers", []))
    generated_callbacks = collect_page_callback_stubs(page)
    has_timers = bool(generated_timers)
    has_auto_start_timers = any(timer.get("auto_start") for timer in generated_timers)

    lines = []
    lines.append(f"// {name}.c — User implementation for {name}")
    lines.append("// This file is YOUR code. The designer preserves USER CODE regions on regeneration.")
    lines.append(f"// Layout/widget init is in {name}_layout.c (auto-generated).")
    lines.append("")
    lines.append('#include "egui.h"')
    lines.append("#include <stdlib.h>")
    lines.append("")
    lines.append('#include "uicode.h"')
    lines.append(f'#include "{name}.h"')
    lines.append("")
    lines.append("// USER CODE BEGIN includes")
    lines.append("// USER CODE END includes")
    lines.append("")
    lines.append("// USER CODE BEGIN variables")
    lines.append("// USER CODE END variables")
    lines.append("")
    lines.append("// USER CODE BEGIN callbacks")
    if generated_callbacks:
        for callback in generated_callbacks:
            stub = render_page_callback_stub(
                page,
                callback["name"],
                callback["signature"],
                kind=callback.get("kind", "view"),
            )
            if stub:
                lines.append(stub)
                lines.append("")
    lines.append("// USER CODE END callbacks")
    lines.append("")

    # Placeholder sections for user code
    lines.append("// ── Your includes ──")
    lines.append("")
    lines.append("// ── Your static variables ──")
    lines.append("")
    lines.append("// ── Your callback functions ──")
    lines.append("")

    # ── on_open ──
    lines.append(f"static void {prefix}_on_open(egui_page_base_t *self)")
    lines.append("{")
    lines.append(f"    {struct_type} *local = ({struct_type} *)self;")
    lines.append("    EGUI_UNUSED(local);")
    lines.append("    // Call super on_open")
    lines.append("    egui_page_base_on_open(self);")
    lines.append("")
    lines.append("    // Auto-generated layout initialization")
    lines.append(f"    {prefix}_layout_init(self);")
    if has_timers and has_auto_start_timers:
        lines.append(f"    {helper_names['start_auto']}(self);")
    lines.append("")
    lines.append("    // USER CODE BEGIN on_open")
    lines.append("    // USER CODE END on_open")
    lines.append("    // TODO: Add your post-init logic here")
    lines.append("    // e.g. register click listeners, start timers, set dynamic text")
    lines.append("}")
    lines.append("")

    # ── on_close ──
    lines.append(f"static void {prefix}_on_close(egui_page_base_t *self)")
    lines.append("{")
    lines.append(f"    {struct_type} *local = ({struct_type} *)self;")
    lines.append("    EGUI_UNUSED(local);")
    if has_timers:
        lines.append("    // Auto-generated timer cleanup")
        lines.append(f"    {helper_names['stop']}(self);")
        lines.append("")
    lines.append("    // Call super on_close")
    lines.append("    egui_page_base_on_close(self);")
    lines.append("")
    lines.append("    // USER CODE BEGIN on_close")
    lines.append("    // USER CODE END on_close")
    lines.append("    // TODO: Add your cleanup logic here")
    lines.append("}")
    lines.append("")

    # ── on_key_pressed ──
    lines.append(f"static void {prefix}_on_key_pressed(egui_page_base_t *self, uint16_t keycode)")
    lines.append("{")
    lines.append(f"    {struct_type} *local = ({struct_type} *)self;")
    lines.append("    EGUI_UNUSED(local);")
    lines.append("")
    lines.append("    // USER CODE BEGIN on_key_pressed")
    lines.append("    // USER CODE END on_key_pressed")
    lines.append("    // TODO: Handle key events here")
    lines.append("}")
    lines.append("")

    # ── vtable ──
    lines.append(f"static const egui_page_base_api_t EGUI_VIEW_API_TABLE_NAME({struct_type}) = {{")
    lines.append(f"    .on_open = {prefix}_on_open,")
    lines.append(f"    .on_close = {prefix}_on_close,")
    lines.append(f"    .on_key_pressed = {prefix}_on_key_pressed,")
    lines.append("};")
    lines.append("")

    # ── init ──
    lines.append(f"void {prefix}_init(egui_page_base_t *self)")
    lines.append("{")
    lines.append(f"    {struct_type} *local = ({struct_type} *)self;")
    lines.append("    EGUI_UNUSED(local);")
    lines.append("    // Call super init")
    lines.append("    egui_page_base_init(self);")
    lines.append("    // Set vtable")
    lines.append(f"    self->api = &EGUI_VIEW_API_TABLE_NAME({struct_type});")
    if has_timers:
        lines.append("    // Auto-generated timer initialization")
        lines.append(f"    {helper_names['init']}(self);")
    lines.append(f'    egui_page_base_set_name(self, "{name}");')
    lines.append("")
    lines.append("    // USER CODE BEGIN init")
    lines.append("    // USER CODE END init")
    lines.append("    // TODO: Add your custom init logic here")
    lines.append("}")
    lines.append("")

    return "\n".join(lines)


# ── helpers ───────────────────────────────────────────────────────

def _project_uses_resources(project):
    """Return True if any widget references a generated resource (image or font)."""
    for page in project.pages:
        for w in page.get_all_widgets():
            # New structured resource properties
            if w.properties.get("image_file", ""):
                return True
            if w.properties.get("font_file", ""):
                return True
            # Legacy: check for C expression strings
            for val in w.properties.values():
                if isinstance(val, str) and val.startswith("&egui_res_"):
                    return True
    return False


# ── uicode.h ─────────────────────────────────────────────────────

def generate_uicode_header(project):
    """Generate uicode.h with page enum and exported functions."""
    lines = []
    lines.append("#ifndef _UICODE_H_")
    lines.append("#define _UICODE_H_")
    lines.append("")
    lines.append('#include "egui.h"')
    lines.append("")
    if _project_uses_resources(project):
        lines.append('#include "app_egui_resource_generate.h"')
        lines.append("")
    lines.append("/* Set up for C function definitions, even when using C++ */")
    lines.append("#ifdef __cplusplus")
    lines.append('extern "C" {')
    lines.append("#endif")
    lines.append("")

    # Page index enum
    lines.append("// Page indices")
    lines.append("enum {")
    for i, page in enumerate(project.pages):
        enum_name = f"PAGE_{page.name.upper()}"
        lines.append(f"    {enum_name} = {i},")
    lines.append(f"    PAGE_COUNT = {len(project.pages)},")
    lines.append("};")
    lines.append("")

    # Exported functions
    lines.append("void uicode_switch_page(int page_index);")
    lines.append("int uicode_start_next_page(void);")
    lines.append("int uicode_start_prev_page(void);")
    lines.append("void uicode_create_ui(void);")
    lines.append("")
    lines.append("/* Ends C function definitions when using C++ */")
    lines.append("#ifdef __cplusplus")
    lines.append("}")
    lines.append("#endif")
    lines.append("")
    lines.append("#endif /* _UICODE_H_ */")
    lines.append("")

    return "\n".join(lines)


# ── uicode.c (EasyPage mode) ─────────────────────────────────────

def _gen_uicode_easy_page(project):
    """Generate uicode.c for EasyPage mode (union + switch page management)."""
    pages = project.pages
    has_i18n = project.string_catalog.has_strings
    lines = []
    lines.append("// ===== Auto-generated by EmbeddedGUI Designer =====")
    lines.append("")
    lines.append('#include "egui.h"')
    lines.append("#include <stdlib.h>")
    lines.append('#include "uicode.h"')
    if has_i18n:
        lines.append('#include "egui_strings.h"')
    lines.append("")

    # Include each page header
    for page in pages:
        lines.append(f'#include "{page.name}.h"')
    lines.append("")

    # Toast
    lines.append("static egui_toast_std_t toast;")
    lines.append("static egui_page_base_t *current_page = NULL;")
    lines.append("")

    # Page union
    lines.append("union page_array {")
    for page in pages:
        lines.append(f"    egui_{page.name}_t {page.name};")
    lines.append("};")
    lines.append("")
    lines.append("static union page_array g_page_array;")
    lines.append("static int current_index = 0;")
    lines.append("static char toast_str[50];")
    lines.append("")

    # Determine startup page index
    startup_index = 0
    for i, page in enumerate(pages):
        if page.name == project.startup_page:
            startup_index = i
            break

    # uicode_switch_page
    lines.append("void uicode_switch_page(int page_index)")
    lines.append("{")
    lines.append("    current_index = page_index;")
    lines.append("")
    lines.append('    egui_api_sprintf(toast_str, "Start page %d", page_index);')
    lines.append("    egui_core_toast_show_info(toast_str);")
    lines.append("")
    lines.append("    if (current_page)")
    lines.append("    {")
    lines.append("        egui_page_base_close(current_page);")
    lines.append("    }")
    lines.append("")
    lines.append("    switch (page_index)")
    lines.append("    {")
    for i, page in enumerate(pages):
        lines.append(f"    case {i}:")
        lines.append(
            f"        egui_{page.name}_init("
            f"(egui_page_base_t *)&g_page_array.{page.name});"
        )
        lines.append(
            f"        current_page = "
            f"(egui_page_base_t *)&g_page_array.{page.name};"
        )
        lines.append("        break;")
    lines.append("    default:")
    lines.append("        break;")
    lines.append("    }")
    lines.append("")
    lines.append("    egui_page_base_open(current_page);")
    lines.append("}")
    lines.append("")

    # uicode_start_next_page
    lines.append("int uicode_start_next_page(void)")
    lines.append("{")
    lines.append("    int page_index = current_index + 1;")
    lines.append("    if (page_index >= PAGE_COUNT)")
    lines.append("    {")
    lines.append('        egui_core_toast_show_info("No more next page");')
    lines.append("        return -1;")
    lines.append("    }")
    lines.append("")
    lines.append("    uicode_switch_page(page_index);")
    lines.append("    return 0;")
    lines.append("}")
    lines.append("")

    # uicode_start_prev_page
    lines.append("int uicode_start_prev_page(void)")
    lines.append("{")
    lines.append("    int page_index = current_index - 1;")
    lines.append("    if (page_index < 0)")
    lines.append("    {")
    lines.append('        egui_core_toast_show_info("No more previous page");')
    lines.append("        return -1;")
    lines.append("    }")
    lines.append("")
    lines.append("    uicode_switch_page(page_index);")
    lines.append("    return 0;")
    lines.append("}")
    lines.append("")

    # Key event handler
    lines.append("void egui_port_hanlde_key_event(int key, int event)")
    lines.append("{")
    lines.append("    if (event == 0)")
    lines.append("    {")
    lines.append("        return;")
    lines.append("    }")
    lines.append("")
    lines.append("    if (current_page)")
    lines.append("    {")
    lines.append("        egui_page_base_key_pressed(current_page, key);")
    lines.append("    }")
    lines.append("}")
    lines.append("")

    # uicode_init_ui
    lines.append("void uicode_init_ui(void)")
    lines.append("{")
    if has_i18n:
        lines.append("    // Initialize i18n string tables")
        lines.append("    egui_strings_init();")
        lines.append("")
    lines.append("    // Start with startup page")
    lines.append(f"    current_index = {startup_index};")
    lines.append(f"    uicode_switch_page({startup_index});")
    lines.append("")
    lines.append("    // Init toast")
    lines.append("    egui_toast_std_init((egui_toast_t *)&toast);")
    lines.append("    egui_core_toast_set((egui_toast_t *)&toast);")
    lines.append("}")
    lines.append("")

    # uicode_create_ui
    lines.append("void uicode_create_ui(void)")
    lines.append("{")
    lines.append("    uicode_init_ui();")
    lines.append("}")
    lines.append("")

    # Recording test actions — auto-cycle through all pages
    if len(pages) > 1:
        lines.append("#if EGUI_CONFIG_RECORDING_TEST")
        lines.append(
            "// Recording actions: visit every page for visual verification"
        )
        lines.append(
            "bool egui_port_get_recording_action"
            "(int action_index, egui_sim_action_t *p_action)"
        )
        lines.append("{")
        # Guard: this callback is called every frame; only switch page on
        # the first call for each action_index to avoid repeated side effects.
        lines.append("    static int last_action = -1;")
        lines.append("    int first_call = (action_index != last_action);")
        lines.append("    last_action = action_index;")
        lines.append("")
        lines.append("    switch (action_index)")
        lines.append("    {")
        case_idx = 0
        lines.append(f"    case {case_idx}:")
        lines.append("        // Capture startup page")
        lines.append("        if (first_call)")
        lines.append("            recording_request_snapshot();")
        lines.append("        EGUI_SIM_SET_WAIT(p_action, 200);")
        lines.append("        return true;")
        for i, page in enumerate(pages):
            if i == startup_index:
                continue  # startup page is already shown
            case_idx += 1
            lines.append(f"    case {case_idx}:")
            lines.append(
                f"        // Switch to {page.name} (PAGE_{page.name.upper()})"
            )
            lines.append("        if (first_call)")
            lines.append("        {")
            lines.append(f"            uicode_switch_page({i});")
            lines.append("            recording_request_snapshot();")
            lines.append("        }")
            lines.append("        EGUI_SIM_SET_WAIT(p_action, 200);")
            lines.append("        return true;")
        # Return to startup page at the end
        case_idx += 1
        lines.append(f"    case {case_idx}:")
        lines.append("        // Return to startup page")
        lines.append("        if (first_call)")
        lines.append("        {")
        lines.append(
            f"            uicode_switch_page({startup_index});"
        )
        lines.append("            recording_request_snapshot();")
        lines.append("        }")
        lines.append("        EGUI_SIM_SET_WAIT(p_action, 200);")
        lines.append("        return true;")
        lines.append("    default:")
        lines.append("        return false;")
        lines.append("    }")
        lines.append("}")
        lines.append("#endif")
        lines.append("")

    return "\n".join(lines)


# ── uicode.c (Activity mode) ─────────────────────────────────────

def _gen_uicode_activity(project):
    """Generate uicode.c for Activity mode.

    Each page struct is wrapped inside an activity adapter.
    The activity lifecycle delegates to page_base lifecycle.
    """
    pages = project.pages
    has_i18n = project.string_catalog.has_strings
    lines = []
    lines.append("// ===== Auto-generated by EmbeddedGUI Designer =====")
    lines.append("// Activity mode — pages wrapped in activity adapters")
    lines.append("")
    lines.append('#include "egui.h"')
    lines.append("#include <stdlib.h>")
    lines.append('#include "uicode.h"')
    if has_i18n:
        lines.append('#include "egui_strings.h"')
    lines.append("")

    for page in pages:
        lines.append(f'#include "{page.name}.h"')
    lines.append("")

    # Define activity adapter for each page
    lines.append("// ── Activity adapters wrapping page_base pages ──")
    lines.append("")
    for page in pages:
        adapter = f"{page.name}_activity"
        page_type = f"egui_{page.name}_t"
        lines.append(f"typedef struct {{")
        lines.append(f"    egui_activity_t base;")
        lines.append(f"    {page_type} page_data;")
        lines.append(f"}} {adapter}_t;")
        lines.append("")

        # on_create
        lines.append(f"static void {adapter}_on_create(egui_activity_t *self)")
        lines.append("{")
        lines.append(f"    egui_activity_on_create(self);")
        lines.append(f"    {adapter}_t *w = ({adapter}_t *)self;")
        lines.append(f"    egui_{page.name}_init((egui_page_base_t *)&w->page_data);")
        lines.append(f"    egui_page_base_open((egui_page_base_t *)&w->page_data);")
        lines.append("}")
        lines.append("")

        # on_destroy
        lines.append(f"static void {adapter}_on_destroy(egui_activity_t *self)")
        lines.append("{")
        lines.append(f"    {adapter}_t *w = ({adapter}_t *)self;")
        lines.append(f"    egui_page_base_close((egui_page_base_t *)&w->page_data);")
        lines.append(f"    egui_activity_on_destroy(self);")
        lines.append("}")
        lines.append("")

        # vtable
        lines.append(f"static const egui_activity_api_t {adapter}_api = {{")
        lines.append(f"    .on_create = {adapter}_on_create,")
        lines.append(f"    .on_destroy = {adapter}_on_destroy,")
        lines.append(f"    .on_start = egui_activity_on_start,")
        lines.append(f"    .on_stop = egui_activity_on_stop,")
        lines.append(f"    .on_resume = egui_activity_on_resume,")
        lines.append(f"    .on_pause = egui_activity_on_pause,")
        lines.append("};")
        lines.append("")

        # init
        lines.append(f"static void {adapter}_init(egui_activity_t *self)")
        lines.append("{")
        lines.append(f"    egui_activity_init(self);")
        lines.append(f"    self->api = &{adapter}_api;")
        lines.append("}")
        lines.append("")

    # Union
    lines.append("// ── Page/Activity union ──")
    lines.append("union page_array {")
    for page in pages:
        lines.append(f"    {page.name}_activity_t {page.name};")
    lines.append("};")
    lines.append("")
    lines.append("static union page_array g_page_array;")
    lines.append("static int current_index = 0;")
    lines.append("static egui_toast_std_t toast;")
    lines.append("static char toast_str[50];")
    lines.append("")

    startup_index = 0
    for i, page in enumerate(pages):
        if page.name == project.startup_page:
            startup_index = i
            break

    # uicode_switch_page
    lines.append("void uicode_switch_page(int page_index)")
    lines.append("{")
    lines.append("    current_index = page_index;")
    lines.append('    egui_api_sprintf(toast_str, "Start page %d", page_index);')
    lines.append("    egui_core_toast_show_info(toast_str);")
    lines.append("")
    lines.append("    egui_core_activity_back();")
    lines.append("")
    lines.append("    switch (page_index)")
    lines.append("    {")
    for i, page in enumerate(pages):
        adapter = f"{page.name}_activity"
        lines.append(f"    case {i}:")
        lines.append(
            f"        {adapter}_init("
            f"(egui_activity_t *)&g_page_array.{page.name});"
        )
        lines.append(
            f"        egui_core_activity_start("
            f"(egui_activity_t *)&g_page_array.{page.name});"
        )
        lines.append("        break;")
    lines.append("    default:")
    lines.append("        break;")
    lines.append("    }")
    lines.append("}")
    lines.append("")

    # next / prev
    lines.append("int uicode_start_next_page(void)")
    lines.append("{")
    lines.append("    int page_index = current_index + 1;")
    lines.append("    if (page_index >= PAGE_COUNT)")
    lines.append("    {")
    lines.append('        egui_core_toast_show_info("No more next page");')
    lines.append("        return -1;")
    lines.append("    }")
    lines.append("    uicode_switch_page(page_index);")
    lines.append("    return 0;")
    lines.append("}")
    lines.append("")
    lines.append("int uicode_start_prev_page(void)")
    lines.append("{")
    lines.append("    int page_index = current_index - 1;")
    lines.append("    if (page_index < 0)")
    lines.append("    {")
    lines.append('        egui_core_toast_show_info("No more previous page");')
    lines.append("        return -1;")
    lines.append("    }")
    lines.append("    uicode_switch_page(page_index);")
    lines.append("    return 0;")
    lines.append("}")
    lines.append("")

    # uicode_init_ui / create_ui
    lines.append("void uicode_init_ui(void)")
    lines.append("{")
    if has_i18n:
        lines.append("    // Initialize i18n string tables")
        lines.append("    egui_strings_init();")
        lines.append("")
    lines.append(f"    current_index = {startup_index};")
    lines.append(f"    uicode_switch_page({startup_index});")
    lines.append("")
    lines.append("    egui_toast_std_init((egui_toast_t *)&toast);")
    lines.append("    egui_core_toast_set((egui_toast_t *)&toast);")
    lines.append("}")
    lines.append("")
    lines.append("void uicode_create_ui(void)")
    lines.append("{")
    lines.append("    uicode_init_ui();")
    lines.append("}")
    lines.append("")

    return "\n".join(lines)


def generate_uicode_source(project):
    """Generate uicode.c based on project page_mode."""
    if project.page_mode == "activity":
        return _gen_uicode_activity(project)
    return _gen_uicode_easy_page(project)


# ── app_egui_config.h ─────────────────────────────────────────────

def generate_app_config(project):
    """Generate app_egui_config.h with screen dimensions."""
    w = project.screen_width
    h = project.screen_height
    # PFB size: 1/8 of screen dimensions
    pfb_w = w // 8
    pfb_h = h // 8

    # Determine max circle radius needed from circular progress bars
    max_radius = 20  # default
    for page in project.pages:
        for widget in page.get_all_widgets():
            if widget.widget_type == "circular_progress_bar":
                r = min(widget.width, widget.height) // 2
                if r > max_radius:
                    max_radius = r

    # Recording test macro is injected via USER_CFLAGS by code_runtime_check.py
    # (not baked into config — keeps config portable)
    recording_line = ""

    return (
        "#ifndef _APP_EGUI_CONFIG_H_\n"
        "#define _APP_EGUI_CONFIG_H_\n"
        "\n"
        "#ifdef __cplusplus\n"
        'extern "C" {\n'
        "#endif\n"
        "\n"
        f"#define EGUI_CONFIG_SCEEN_WIDTH  {w}\n"
        f"#define EGUI_CONFIG_SCEEN_HEIGHT {h}\n"
        f"#define EGUI_CONFIG_PFB_WIDTH    {pfb_w}\n"
        f"#define EGUI_CONFIG_PFB_HEIGHT   {pfb_h}\n"
        "\n"
        f"#define EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE {max_radius}\n"
        "\n"
        f"{recording_line}"
        "#ifdef __cplusplus\n"
        "}\n"
        "#endif\n"
        "\n"
        "#endif /* _APP_EGUI_CONFIG_H_ */\n"
    )


# ── Public API ────────────────────────────────────────────────────

# File categories for the generation system:
#   GENERATED_ALWAYS  — fully generated, overwritten every time
#   GENERATED_PRESERVED — generated but USER CODE regions preserved
#   USER_OWNED — created once as skeleton, NEVER overwritten

GENERATED_ALWAYS = "generated_always"
GENERATED_PRESERVED = "generated_preserved"
USER_OWNED = "user_owned"


def generate_all_files(project):
    """Generate all C files for the project.

    Returns:
        dict[str, tuple[str, str]]: filename -> (content, category)
            category is one of GENERATED_ALWAYS, GENERATED_PRESERVED, USER_OWNED
    """
    files = {}

    for page in project.pages:
        files[f"{page.name}.h"] = (
            generate_page_header(page, project), GENERATED_PRESERVED
        )
        files[f"{page.name}_layout.c"] = (
            generate_page_layout_source(page, project), GENERATED_ALWAYS
        )
        files[f"{page.name}.c"] = (
            generate_page_user_source(page, project), USER_OWNED
        )

    files["uicode.h"] = (generate_uicode_header(project), GENERATED_ALWAYS)
    files["uicode.c"] = (generate_uicode_source(project), GENERATED_ALWAYS)
    files["app_egui_config.h"] = (generate_app_config(project), GENERATED_ALWAYS)

    # i18n string resources
    if project.string_catalog.has_strings:
        from .string_resource_generator import generate_string_files
        string_files = generate_string_files(project.string_catalog)
        files.update(string_files)

    return files


def generate_all_files_preserved(project, output_dir, backup=True):
    """Generate all C files with proper ownership semantics.

    This is the preferred API for production code generation. It:

    1. Generates fresh file contents from the current project model.
    2. For GENERATED_PRESERVED files (.h): extracts ``USER CODE``
       blocks from existing files and re-injects them.
    3. For GENERATED_ALWAYS files (*_layout.c, uicode.*, etc.):
       always overwrites.
    4. For USER_OWNED files (.c user implementation): only creates
       the skeleton if the file does not exist. Never overwrites.
    5. Optionally backs up overwritten files.

    Migration: if an old-style {page}.c exists (with "Auto-generated"
    header and USER CODE markers), user code is extracted and migrated
    into the new skeleton format.

    Args:
        project:    The Project model.
        output_dir: Directory where generated files will be written.
        backup:     If True, create backups before overwriting.

    Returns:
        dict[str, str]: filename -> final content for files that should
        be written.  User-owned files that already exist are excluded.
    """
    import os
    from .user_code_preserver import (
        preserve_user_code,
        extract_user_code,
        read_existing_file,
        backup_file,
        cleanup_old_backups,
        embed_source_hash,
        should_skip_generation,
        compute_source_hash,
    )

    all_files = generate_all_files(project)
    result = {}

    backup_root = os.path.join(output_dir, ".eguiproject", "backup")

    for filename, (content, category) in all_files.items():
        filepath = os.path.join(output_dir, filename)

        if category == USER_OWNED:
            # User-owned file: only create if it doesn't exist
            if os.path.isfile(filepath):
                # Check if this is an old-style auto-generated file
                # that needs migration to the new user-owned format
                existing = read_existing_file(filepath)
                if existing and "Auto-generated by EmbeddedGUI Designer" in existing:
                    migrated = _migrate_old_page_source(existing, content)
                    if migrated:
                        if backup:
                            backup_file(filepath, backup_root)
                        result[filename] = migrated
                    continue
                if existing and _looks_like_designer_user_source(existing):
                    source_hash = compute_source_hash(content)
                    if should_skip_generation(filepath, source_hash):
                        continue
                    content_with_hash = embed_source_hash(content, source_hash)
                    migrated = _migrate_designer_user_source(existing, content_with_hash, filename)
                    if migrated and migrated != existing:
                        if backup:
                            backup_file(filepath, backup_root)
                        result[filename] = migrated
                continue
            else:
                source_hash = compute_source_hash(content)
                result[filename] = embed_source_hash(content, source_hash)

        elif category == GENERATED_PRESERVED:
            # Generated but preserves USER CODE regions
            source_hash = compute_source_hash(content)
            content_with_hash = embed_source_hash(content, source_hash)
            final_content = preserve_user_code(filepath, content_with_hash)
            if backup and os.path.isfile(filepath):
                backup_file(filepath, backup_root)
            result[filename] = final_content

        else:  # GENERATED_ALWAYS
            source_hash = compute_source_hash(content)
            if should_skip_generation(filepath, source_hash):
                continue  # Nothing changed, skip
            content_with_hash = embed_source_hash(content, source_hash)
            if backup and os.path.isfile(filepath):
                backup_file(filepath, backup_root)
            result[filename] = content_with_hash

    # Cleanup old backups (keep last 20)
    if backup:
        cleanup_old_backups(backup_root, keep=20)

    return result


def _migrate_old_page_source(old_content, new_skeleton):
    """Migrate an old-style {page}.c (with USER CODE markers) to the new
    user-owned skeleton format.

    Extracts user code from the old file's USER CODE regions and injects
    them into the appropriate places in the new skeleton.

    Returns the migrated content, or None if migration is not needed.
    """
    from .user_code_preserver import extract_user_code, inject_user_code

    old_blocks = extract_user_code(old_content)
    if not old_blocks:
        return new_skeleton  # No user code to preserve, just replace

    mapped_blocks = {
        "includes": old_blocks.get("includes", ""),
        "variables": old_blocks.get("variables", ""),
        "callbacks": old_blocks.get("callbacks", ""),
        "on_open": old_blocks.get("on_open", ""),
        "on_close": old_blocks.get("on_close", ""),
        "on_key_pressed": old_blocks.get("key_handler", ""),
        "init": old_blocks.get("init", ""),
    }
    return inject_user_code(new_skeleton, mapped_blocks)

    # Map old USER CODE tags to locations in the new skeleton
    # Old format had: includes, variables, callbacks, on_open, on_close,
    #                 key_handler, init
    # New format is a plain .c file — inject as sections

    lines = new_skeleton.split("\n")
    result_lines = []
    for line in lines:
        result_lines.append(line)

        # After "// ── Your includes ──", insert old includes
        if line.strip() == "// ── Your includes ──":
            code = old_blocks.get("includes", "").strip()
            if code:
                result_lines.append(code)

        # After "// ── Your static variables ──", insert old variables
        elif line.strip() == "// ── Your static variables ──":
            code = old_blocks.get("variables", "").strip()
            if code:
                result_lines.append(code)

        # After "// ── Your callback functions ──", insert old callbacks
        elif line.strip() == "// ── Your callback functions ──":
            code = old_blocks.get("callbacks", "").strip()
            if code:
                result_lines.append(code)

        # After "// TODO: Add your post-init logic here" in on_open
        elif "TODO: Add your post-init logic here" in line:
            code = old_blocks.get("on_open", "").strip()
            if code:
                # Replace the TODO line with actual code
                result_lines[-1] = code

        # After "// TODO: Add your cleanup logic here" in on_close
        elif "TODO: Add your cleanup logic here" in line:
            code = old_blocks.get("on_close", "").strip()
            if code:
                result_lines[-1] = code

        # After "// TODO: Handle key events here"
        elif "TODO: Handle key events here" in line:
            code = old_blocks.get("key_handler", "").strip()
            if code:
                result_lines[-1] = code

        # After "// TODO: Add your custom init logic here"
        elif "TODO: Add your custom init logic here" in line:
            code = old_blocks.get("init", "").strip()
            if code:
                result_lines[-1] = code

    return "\n".join(result_lines)


def _looks_like_designer_user_source(content):
    return (
        "Layout/widget init is in" in content
        and "EGUI_VIEW_API_TABLE_NAME" in content
        and "_on_open(egui_page_base_t *self)" in content
    )


def _extract_function_body(content, signature):
    start = content.find(signature)
    if start < 0:
        return ""

    brace_start = content.find("{", start)
    if brace_start < 0:
        return ""

    depth = 0
    index = brace_start
    while index < len(content):
        char = content[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return content[brace_start + 1:index]
        index += 1
    return ""


def _clean_preserved_block(text, skip_lines):
    if not text:
        return ""

    normalized_skip = {line.strip() for line in skip_lines if line.strip()}
    kept_lines = []
    for line in text.splitlines():
        if line.strip() in normalized_skip:
            continue
        kept_lines.append(line.rstrip())

    while kept_lines and not kept_lines[0].strip():
        kept_lines.pop(0)
    while kept_lines and not kept_lines[-1].strip():
        kept_lines.pop()

    if not kept_lines:
        return ""
    return "\n".join(kept_lines) + "\n"


def _extract_designer_user_blocks(existing_content, page_name):
    prefix = f"egui_{page_name}"
    struct_type = f"egui_{page_name}_t"
    blocks = {}

    include_anchor = f'#include "{page_name}.h"'
    on_open_signature = f"static void {prefix}_on_open"
    include_index = existing_content.find(include_anchor)
    on_open_index = existing_content.find(on_open_signature)
    if include_index >= 0 and on_open_index > include_index:
        preamble = existing_content[include_index + len(include_anchor):on_open_index]
        blocks["includes"] = _clean_preserved_block(preamble, set())

    on_open_body = _extract_function_body(existing_content, on_open_signature)
    blocks["on_open"] = _clean_preserved_block(
        on_open_body,
        {
            f"{struct_type} *local = ({struct_type} *)self;",
            "EGUI_UNUSED(local);",
            "// Call super on_open",
            "egui_page_base_on_open(self);",
            "// Auto-generated layout initialization",
            f"{prefix}_layout_init(self);",
            f"{prefix}_timers_start_auto(self);",
            "// USER CODE BEGIN on_open",
            "// USER CODE END on_open",
            "// TODO: Add your post-init logic here",
            "// e.g. register click listeners, start timers, set dynamic text",
        },
    )

    on_close_body = _extract_function_body(existing_content, f"static void {prefix}_on_close")
    blocks["on_close"] = _clean_preserved_block(
        on_close_body,
        {
            f"{struct_type} *local = ({struct_type} *)self;",
            "EGUI_UNUSED(local);",
            "// Auto-generated timer cleanup",
            f"{prefix}_timers_stop(self);",
            "// Call super on_close",
            "egui_page_base_on_close(self);",
            "// USER CODE BEGIN on_close",
            "// USER CODE END on_close",
            "// TODO: Add your cleanup logic here",
        },
    )

    on_key_body = _extract_function_body(existing_content, f"static void {prefix}_on_key_pressed")
    blocks["on_key_pressed"] = _clean_preserved_block(
        on_key_body,
        {
            f"{struct_type} *local = ({struct_type} *)self;",
            "EGUI_UNUSED(local);",
            "// USER CODE BEGIN on_key_pressed",
            "// USER CODE END on_key_pressed",
            "// TODO: Handle key events here",
        },
    )

    init_body = _extract_function_body(existing_content, f"void {prefix}_init")
    blocks["init"] = _clean_preserved_block(
        init_body,
        {
            f"{struct_type} *local = ({struct_type} *)self;",
            "EGUI_UNUSED(local);",
            "// Call super init",
            "egui_page_base_init(self);",
            "// Set vtable",
            f"self->api = &EGUI_VIEW_API_TABLE_NAME({struct_type});",
            "// Auto-generated timer initialization",
            f"{prefix}_timers_init(self);",
            f'egui_page_base_set_name(self, "{page_name}");',
            "// USER CODE BEGIN init",
            "// USER CODE END init",
            "// TODO: Add your custom init logic here",
        },
    )

    return {tag: body for tag, body in blocks.items() if body}


def _migrate_designer_user_source(existing_content, new_skeleton, filename):
    from .user_code_preserver import extract_user_code, inject_user_code
    import os

    user_blocks = extract_user_code(existing_content)
    if not user_blocks:
        page_name = os.path.splitext(os.path.basename(filename))[0]
        user_blocks = _extract_designer_user_blocks(existing_content, page_name)
    return inject_user_code(new_skeleton, user_blocks)


# ── Legacy single-file generator (backward compatibility) ────────

def generate_uicode(project):
    """Generate uicode.c content.

    For multi-page projects (MFC mode), returns the uicode.c only.
    Full project generation should use generate_all_files() instead.
    """
    if project.pages:
        return generate_uicode_source(project)

    # Fallback for empty projects
    return (
        '#include "egui.h"\n'
        '#include "uicode.h"\n'
        "\n"
        "void uicode_init_ui(void) {}\n"
        "void uicode_create_ui(void) { uicode_init_ui(); }\n"
    )

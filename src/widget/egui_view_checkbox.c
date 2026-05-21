#include <stdio.h>
#include <assert.h>

#include "egui_view_checkbox.h"
#if EGUI_CONFIG_FUNCTION_EVENT_LITE
#include "core/egui_event.h"
#endif
#include "core/egui_core.h"
#include "egui_view_icon_font.h"
#include "egui_view_circle_dirty.h"
#include "resource/egui_resource.h"
#include "style/egui_theme.h"

#if EGUI_CONFIG_FUNCTION_WIDGET_ENHANCED_DRAW
#include "canvas/egui_canvas_gradient.h"
#include "shadow/egui_shadow.h"
#endif

/**
 * @file egui_view_checkbox.c
 * @brief Checkbox widget with square indicator, optional label, and mark-style variants.
 *
 * The checkbox keeps interaction intentionally simple: a completed click flips
 * one boolean flag, and redraw tries to invalidate only the square indicator
 * when the label text is unchanged.
 */

/** Return the effective label font, falling back to the default UI font. */
static const egui_font_t *egui_view_checkbox_get_text_font(const egui_view_checkbox_t *local)
{
    if (local->font != NULL)
    {
        return local->font;
    }

    return (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
}

extern const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_checkbox_t);

/** Calculate the smallest dirty region that still fully covers the indicator box. */
static uint8_t egui_view_checkbox_get_indicator_dirty_region(egui_view_t *self, egui_view_checkbox_t *local, egui_region_t *dirty_region)
{
    egui_region_t region;
    egui_dim_t box_size;
    egui_dim_t box_x;
    egui_dim_t box_y;

    if (dirty_region == NULL)
    {
        return 0;
    }

    egui_region_init_empty(dirty_region);
    if (self->region_screen.size.width <= 0 || self->region_screen.size.height <= 0)
    {
        return 0;
    }

    egui_view_get_work_region(self, &region);
    box_size = region.size.height;
    if (box_size > region.size.width)
    {
        box_size = region.size.width;
    }
    if (box_size <= 0)
    {
        return 0;
    }

    box_x = region.location.x + (region.size.width - box_size) / 2;
    box_y = region.location.y + (region.size.height - box_size) / 2;
    if (EGUI_VIEW_TEXT_VALID(local->text))
    {
        box_x = region.location.x + 1;
    }

    // State changes only repaint the square indicator, not the entire label row.
    egui_view_circle_dirty_add_rect_region(dirty_region, box_x, box_y, box_size, box_size, EGUI_VIEW_CIRCLE_DIRTY_AA_PAD + 1);
    return egui_region_is_empty(dirty_region) ? 0 : 1;
}

/** Replace the optional label text and redraw the whole widget row. */
void egui_view_checkbox_set_text(egui_view_t *self, const char *text)
{
    EGUI_LOCAL_INIT(egui_view_checkbox_t);
    local->text = text;
    egui_view_invalidate(self);
}

const char *egui_view_checkbox_get_text(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_checkbox_t);
    return local->text;
}

/** Override the label font used when drawing optional checkbox text. */
void egui_view_checkbox_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_checkbox_t);
    local->font = font;
    egui_view_invalidate(self);
}

const egui_font_t *egui_view_checkbox_get_font(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_checkbox_t);
    return local->font;
}

/** Set the text color used for the optional label while enabled. */
void egui_view_checkbox_set_text_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_checkbox_t);
    local->text_color = color;
    egui_view_invalidate(self);
}

egui_color_t egui_view_checkbox_get_text_color(egui_view_t *self)
{
    if (self == NULL)
    {
        egui_color_t zero;
        zero.full = 0;
        return zero;
    }
    EGUI_LOCAL_INIT(egui_view_checkbox_t);
    return local->text_color;
}

egui_color_t egui_view_checkbox_get_box_color(egui_view_t *self)
{
    egui_color_t zero;
    zero.full = 0;
    if (self == NULL)
    {
        return zero;
    }
    EGUI_LOCAL_INIT(egui_view_checkbox_t);
    return local->box_color;
}

egui_color_t egui_view_checkbox_get_check_color(egui_view_t *self)
{
    egui_color_t zero;
    zero.full = 0;
    if (self == NULL)
    {
        return zero;
    }
    EGUI_LOCAL_INIT(egui_view_checkbox_t);
    return local->check_color;
}

egui_color_t egui_view_checkbox_get_box_fill_color(egui_view_t *self)
{
    egui_color_t zero;
    zero.full = 0;
    if (self == NULL)
    {
        return zero;
    }
    EGUI_LOCAL_INIT(egui_view_checkbox_t);
    return local->box_fill_color;
}

egui_alpha_t egui_view_checkbox_get_alpha(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_checkbox_t);
    return local->alpha;
}

egui_dim_t egui_view_checkbox_get_text_gap(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_checkbox_t);
    return local->text_gap;
}

/** Switch between vector check-mark rendering and icon-font rendering. */
void egui_view_checkbox_set_mark_style(egui_view_t *self, egui_view_checkbox_mark_style_t style)
{
    EGUI_LOCAL_INIT(egui_view_checkbox_t);
    local->mark_style = (uint8_t)style;
    egui_view_invalidate(self);
}

egui_view_checkbox_mark_style_t egui_view_checkbox_get_mark_style(egui_view_t *self)
{
    if (self == NULL)
    {
        return EGUI_VIEW_CHECKBOX_MARK_STYLE_VECTOR;
    }
    EGUI_LOCAL_INIT(egui_view_checkbox_t);
    return (egui_view_checkbox_mark_style_t)local->mark_style;
}

/** Replace the icon glyph used when the checked mark style is `ICON`. */
void egui_view_checkbox_set_mark_icon(egui_view_t *self, const char *icon)
{
    EGUI_LOCAL_INIT(egui_view_checkbox_t);

    if (local->mark_icon == icon)
    {
        return;
    }

    local->mark_icon = icon;
    egui_view_invalidate(self);
}

const char *egui_view_checkbox_get_mark_icon(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_checkbox_t);
    return local->mark_icon;
}

/** Override the icon font used for icon-style checked marks. */
void egui_view_checkbox_set_icon_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_checkbox_t);

    if (local->icon_font == font)
    {
        return;
    }

    local->icon_font = font;
    egui_view_invalidate(self);
}

const egui_font_t *egui_view_checkbox_get_icon_font(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_checkbox_t);
    return local->icon_font;
}

/** Set the gap between the indicator box and the optional label text. */
void egui_view_checkbox_set_icon_text_gap(egui_view_t *self, egui_dim_t gap)
{
    EGUI_LOCAL_INIT(egui_view_checkbox_t);

    if (gap < 0)
    {
        gap = 0;
    }

    if (local->text_gap == gap)
    {
        return;
    }

    local->text_gap = gap;
    egui_view_invalidate(self);
}

egui_dim_t egui_view_checkbox_get_icon_text_gap(egui_view_t *self)
{
    return egui_view_checkbox_get_text_gap(self);
}

/** Register the callback fired after checked-state changes. */
void egui_view_checkbox_set_on_checked_listener(egui_view_t *self, egui_view_on_checked_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_checkbox_t);
    local->on_checked_changed = listener;
}

egui_view_on_checked_listener_t egui_view_checkbox_get_on_checked_listener(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_checkbox_t);
    return local->on_checked_changed;
}

/** Toggle the stored checked state, notify listeners, and invalidate the indicator region. */
void egui_view_checkbox_set_checked(egui_view_t *self, uint8_t is_checked)
{
    EGUI_LOCAL_INIT(egui_view_checkbox_t);
    egui_region_t dirty_region;
    if (is_checked != local->is_checked)
    {
        local->is_checked = is_checked;
        if (local->on_checked_changed)
        {
            local->on_checked_changed(self, is_checked);
        }
#if EGUI_CONFIG_FUNCTION_EVENT_LITE
        egui_view_send_event(self, EGUI_EVENT_VALUE_CHANGED, &local->is_checked);
#endif

        // The label does not change here, so try to invalidate only the indicator footprint.
        if (egui_view_checkbox_get_indicator_dirty_region(self, local, &dirty_region))
        {
            egui_view_invalidate_region(self, &dirty_region);
        }
        else
        {
            egui_view_invalidate(self);
        }
    }
}

uint8_t egui_view_checkbox_get_checked(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_checkbox_t);
    return local->is_checked;
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH || EGUI_CONFIG_FUNCTION_SUPPORT_KEY
/** Flip the stored boolean when the normal click pipeline completes. */
static int egui_view_checkbox_perform_click(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_checkbox_t);

    // Checkbox click behavior is symmetric: every completed click flips the stored boolean.
    egui_view_checkbox_set_checked(self, !local->is_checked);
    return 1;
}
#endif

/** Draw the square indicator using either vector or icon-style checked marks. */
static void egui_view_checkbox_draw_indicator(egui_view_t *self, uint8_t use_icon_mark)
{
    egui_canvas_t *canvas = egui_view_get_canvas(self);
    EGUI_LOCAL_INIT(egui_view_checkbox_t);

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    // The checkbox is a square. Use the height as the box size.
    egui_dim_t box_size = region.size.height;
    if (box_size > region.size.width)
        box_size = region.size.width;

    egui_dim_t box_x = region.location.x + (region.size.width - box_size) / 2;
    egui_dim_t box_y = region.location.y + (region.size.height - box_size) / 2;

    if (EGUI_VIEW_TEXT_VALID(local->text))
    {
        box_x = region.location.x + 1;
    }

    // Ask the active theme for checkbox colors, including the dedicated CHECKED state.
    const egui_theme_t *theme = egui_theme_get(egui_view_get_core(self));
    const egui_widget_style_desc_t *desc = theme ? theme->checkbox : NULL;
    egui_state_t cb_state;
    if (!egui_view_get_enable(self))
        cb_state = EGUI_STATE_DISABLED;
    else if (local->is_checked)
        cb_state = EGUI_STATE_CHECKED;
    else
        cb_state = EGUI_STATE_NORMAL;
    const egui_style_t *style = desc ? egui_style_get(desc, EGUI_PART_MAIN, cb_state) : NULL;

    egui_color_t box_color = style ? style->border_color : local->box_color;
    egui_color_t fill_color = style ? style->bg_color : local->box_fill_color;
    egui_color_t check_color = (style && (style->flags & EGUI_STYLE_PROP_TEXT_COLOR)) ? style->text_color : local->check_color;

    if (!egui_view_get_enable(self) && !style)
    {
        box_color = EGUI_THEME_DISABLED;
        fill_color = EGUI_THEME_DISABLED;
        check_color = EGUI_THEME_SURFACE;
    }

    if (local->is_checked)
    {
        // A checked checkbox draws a filled square first, then overlays the mark.
#if EGUI_CONFIG_FUNCTION_WIDGET_ENHANCED_DRAW
        {
            egui_color_t grad_top =
                    (style && (style->flags & EGUI_STYLE_PROP_BG_COLOR)) ? egui_rgb_mix(fill_color, EGUI_COLOR_WHITE, 30) : EGUI_THEME_PRIMARY_LIGHT;
            egui_gradient_stop_t cb_stops[2] = {
                    {.position = 0, .color = grad_top},
                    {.position = 255, .color = fill_color},
            };
            egui_gradient_t cb_grad = {
                    .type = EGUI_GRADIENT_TYPE_LINEAR_VERTICAL,
                    .stop_count = 2,
                    .alpha = local->alpha,
                    .stops = cb_stops,
            };
            egui_dim_t cb_radius = box_size / 6;
            egui_canvas_draw_round_rectangle_fill_gradient(canvas, box_x, box_y, box_size, box_size, cb_radius, &cb_grad);
        }
#else
        egui_canvas_draw_round_rectangle_fill(canvas, box_x, box_y, box_size, box_size, box_size / 6, fill_color, local->alpha);
#endif

        if (use_icon_mark)
        {
            egui_region_t icon_region = {{box_x, box_y}, {box_size, box_size}};
            const egui_font_t *icon_font = EGUI_VIEW_ICON_FONT_RESOLVE(local->icon_font, box_size, 18, 22);
            if (icon_font != NULL && EGUI_VIEW_ICON_TEXT_VALID(local->mark_icon))
            {
                egui_canvas_draw_text_in_rect(canvas, icon_font, local->mark_icon, &icon_region, EGUI_ALIGN_CENTER, check_color, local->alpha);
            }
        }
        else
        {
            // The vector mark is just a two-segment polyline shaped like a check.
            egui_dim_t x1 = box_x + box_size / 4;
            egui_dim_t y1 = box_y + box_size / 2;
            egui_dim_t x2 = box_x + box_size * 5 / 12;
            egui_dim_t y2 = box_y + box_size * 7 / 10;
            egui_dim_t x3 = box_x + box_size * 3 / 4;
            egui_dim_t y3 = box_y + box_size * 3 / 10;
            egui_dim_t stroke = EGUI_MAX(box_size / 8, 1);

#if EGUI_CONFIG_FUNCTION_WIDGET_ENHANCED_DRAW
            {
                egui_dim_t check_pts[] = {x1, y1, x2, y2, x3, y3};
                egui_canvas_draw_polyline_round_cap_hq(canvas, check_pts, 3, stroke, check_color, local->alpha);
                // Round joint at the V junction to fill the acute-angle gap
                egui_dim_t joint_r = stroke >> 1;
                if (joint_r > 0)
                {
                    egui_canvas_draw_circle_fill_hq(canvas, x2, y2, joint_r, check_color, local->alpha);
                }
            }
#else
            egui_canvas_draw_line(canvas, x1, y1, x2, y2, stroke, check_color, local->alpha);
            egui_canvas_draw_line(canvas, x2, y2, x3, y3, stroke, check_color, local->alpha);
#endif
        }
    }
    else
    {
        // An unchecked checkbox keeps a visible frame by drawing border and inner surface separately.
        egui_color_t surface_color = (style && (style->flags & EGUI_STYLE_PROP_BG_COLOR)) ? style->bg_color : EGUI_THEME_SURFACE;
        egui_dim_t stroke = EGUI_MAX(EGUI_THEME_STROKE_WIDTH, 1);
        egui_dim_t radius = box_size / 6;
        egui_dim_t inner_size = box_size - stroke * 2;
        egui_dim_t inner_radius = radius - stroke;

        if (inner_radius < 0)
        {
            inner_radius = 0;
        }

        egui_canvas_draw_round_rectangle_fill(canvas, box_x, box_y, box_size, box_size, radius, box_color, local->alpha);
        if (inner_size > 0)
        {
            egui_canvas_draw_round_rectangle_fill(canvas, box_x + stroke, box_y + stroke, inner_size, inner_size, inner_radius, surface_color, local->alpha);
        }
    }
}

/** Draw the indicator first, then lay out the optional label from its right edge. */
void egui_view_checkbox_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_checkbox_t);
    egui_canvas_t *canvas = egui_view_get_canvas(self);
    egui_region_t region;
    egui_dim_t box_size;
    egui_dim_t box_x;

    egui_view_checkbox_draw_indicator(self, local->mark_style == EGUI_VIEW_CHECKBOX_MARK_STYLE_ICON);

    if (EGUI_VIEW_TEXT_VALID(local->text))
    {
        // The label is laid out from the indicator's right edge instead of centering the whole pair.
        const egui_font_t *font = egui_view_checkbox_get_text_font(local);
        egui_color_t text_color = egui_view_get_enable(self) ? local->text_color : EGUI_THEME_DISABLED;

        egui_view_get_work_region(self, &region);
        box_size = region.size.height;
        if (box_size > region.size.width)
        {
            box_size = region.size.width;
        }
        box_x = region.location.x + 1;
        egui_region_t text_region;
        text_region.location.x = box_x + box_size + local->text_gap;
        text_region.location.y = region.location.y;
        text_region.size.width = region.location.x + region.size.width - text_region.location.x;
        text_region.size.height = region.size.height;
        if (text_region.size.width > 0 && font != NULL)
        {
            egui_canvas_draw_text_in_rect(canvas, font, local->text, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, text_color, local->alpha);
        }
    }
}

/** API table for the checkbox widget. */
const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_checkbox_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_checkbox_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH || EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .perform_click = egui_view_checkbox_perform_click,
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

/** Initialize the checkbox with clickable toggle behavior and stock theme defaults. */
void egui_view_checkbox_init(egui_view_t *self, egui_core_t *core)
{
    EGUI_INIT_LOCAL(egui_view_checkbox_t);
    // Initialize the base view before swapping in checkbox-specific callbacks.
    egui_view_init(self, core);
    // Install the checkbox API so draw and click behavior come from this widget.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_checkbox_t);

    // Defaults match a standard form checkbox with optional label support.
    local->is_checked = false;
    local->alpha = EGUI_ALPHA_100;
    local->box_color = EGUI_THEME_BORDER;
    local->check_color = EGUI_THEME_TEXT;
    local->box_fill_color = EGUI_THEME_PRIMARY;
    local->text = NULL;
    local->font = NULL;
    local->text_color = EGUI_THEME_TEXT;
    local->text_gap = 6;
    local->mark_style = EGUI_VIEW_CHECKBOX_MARK_STYLE_VECTOR;
    local->mark_icon = EGUI_ICON_MS_CHECK_MARK;
    local->icon_font = NULL;
    local->on_checked_changed = NULL;
    self->is_clickable = true;
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    self->is_focusable = true;
#endif

#if EGUI_CONFIG_FUNCTION_WIDGET_ENHANCED_DRAW
    {
        // The shadow is attached once at init and reused by every draw pass.
        static const egui_shadow_t checkbox_shadow = {
                .width = EGUI_THEME_SHADOW_WIDTH_SM,
                .ofs_x = 0,
                .ofs_y = EGUI_THEME_SHADOW_OFS_Y_SM,
                .spread = 0,
                .opa = 60,
                .color = EGUI_COLOR_BLACK,
                .corner_radius = EGUI_THEME_RADIUS_SM,
        };
        egui_view_set_shadow(self, &checkbox_shadow);
    }
#endif

    egui_view_set_view_name(self, "egui_view_checkbox");
}

/** Apply geometry, initial checked state, and optional label from one parameter block. */
void egui_view_checkbox_apply_params(egui_view_t *self, const egui_view_checkbox_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_checkbox_t);

    self->region = params->region;

    local->is_checked = params->is_checked;
    local->text = params->text;

    egui_view_invalidate(self);
}

/** Convenience helper that initializes the checkbox before applying params. */
void egui_view_checkbox_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_checkbox_params_t *params)
{
    egui_view_checkbox_init(self, core);
    egui_view_checkbox_apply_params(self, params);
}

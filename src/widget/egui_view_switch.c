#include <stdio.h>
#include <assert.h>

#include "egui_view_switch.h"
#if EGUI_CONFIG_FUNCTION_EVENT_LITE
#include "core/egui_event.h"
#endif
#include "core/egui_core.h"
#include "egui_view_icon_font.h"
#include "resource/egui_resource.h"
#include "style/egui_theme.h"

#if EGUI_CONFIG_FUNCTION_WIDGET_ENHANCED_DRAW
#include "canvas/egui_canvas_gradient.h"
#include "shadow/egui_shadow.h"
#endif

/**
 * @file egui_view_switch.c
 * @brief Toggle switch widget with theme-aware track and thumb rendering.
 *
 * It demonstrates a common egui pattern:
 * - one
 * helper resolves geometry and colors from state,
 * - drawing reuses that prepared data,
 * - `perform_click` flips the checked state through the public
 * setter.
 */

extern const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_switch_t);

/**
 * @brief Resolve draw geometry plus track/thumb colors for the current state.
 */
static uint8_t egui_view_switch_prepare_draw(egui_view_t *self, egui_region_t *region, egui_dim_t *thumb_x, egui_dim_t *thumb_y, egui_dim_t *thumb_radius,
                                             egui_color_t *track_color, egui_color_t *thumb_color)
{
    EGUI_LOCAL_INIT(egui_view_switch_t);

    egui_dim_t height;
    egui_dim_t track_radius;
    egui_state_t state;
    const egui_theme_t *theme;
    const egui_widget_style_desc_t *desc;
    const egui_style_t *track_style;
    const egui_style_t *thumb_style;

    egui_view_get_work_region(self, region);

    if (region->size.width <= 0 || region->size.height <= 0)
    {
        return 0;
    }

    height = region->size.height;
    track_radius = height / 2;
    if (track_radius > region->size.width / 2)
    {
        track_radius = region->size.width / 2;
    }
    *thumb_radius = track_radius - 3;

    if (*thumb_radius < 1)
    {
        *thumb_radius = 1;
    }

    if (!egui_view_get_enable(self))
        state = EGUI_STATE_DISABLED;
    else if (local->is_checked)
        state = EGUI_STATE_CHECKED;
    else
        state = EGUI_STATE_NORMAL;

    theme = egui_theme_get(egui_view_get_core(self));
    desc = theme ? theme->switch_ctrl : NULL;
    track_style = desc ? egui_style_get(desc, EGUI_PART_MAIN, state) : NULL;
    thumb_style = desc ? egui_style_get(desc, EGUI_PART_INDICATOR, state) : NULL;

    if (track_style)
    {
        *track_color = track_style->bg_color;
    }
    else
    {
        *track_color = local->is_checked ? local->bk_color_on : local->bk_color_off;
        if (!egui_view_get_enable(self))
        {
            *track_color = EGUI_THEME_DISABLED;
        }
    }

    if (thumb_style)
    {
        *thumb_color = thumb_style->bg_color;
    }
    else
    {
        *thumb_color = local->is_checked ? local->switch_color_on : local->switch_color_off;
        if (!egui_view_get_enable(self))
        {
            *thumb_color = EGUI_THEME_SURFACE;
        }
    }

    *thumb_y = region->location.y + ((height - 1) >> 1);
    if (local->is_checked)
    {
        *thumb_x = region->location.x + region->size.width - track_radius;
    }
    else
    {
        *thumb_x = region->location.x + track_radius;
    }

    return 1;
}

/**
 * @brief Draw the rounded track and movable thumb circle.
 */
static void egui_view_switch_draw_track_and_thumb(egui_view_t *self, const egui_region_t *region, egui_dim_t thumb_x, egui_dim_t thumb_y,
                                                  egui_dim_t thumb_radius, egui_color_t track_color, egui_color_t thumb_color)
{
    egui_canvas_t *canvas = egui_view_get_canvas(self);
    EGUI_LOCAL_INIT(egui_view_switch_t);
    egui_dim_t track_radius = region->size.height / 2;

    if (track_radius > region->size.width / 2)
    {
        track_radius = region->size.width / 2;
    }

#if EGUI_CONFIG_FUNCTION_WIDGET_ENHANCED_DRAW
    {
        egui_color_t track_top = egui_rgb_mix(track_color, EGUI_COLOR_WHITE, 40);
        egui_color_t track_bottom = egui_rgb_mix(track_color, EGUI_COLOR_BLACK, 40);
        egui_gradient_stop_t track_stops[2] = {
                {.position = 0, .color = track_top},
                {.position = 255, .color = track_bottom},
        };
        egui_gradient_t track_grad = {
                .type = EGUI_GRADIENT_TYPE_LINEAR_VERTICAL,
                .stop_count = 2,
                .alpha = local->alpha,
                .stops = track_stops,
        };
        egui_canvas_draw_round_rectangle_fill_gradient(canvas, region->location.x, region->location.y, region->size.width, region->size.height, track_radius,
                                                       &track_grad);
        egui_canvas_draw_circle_fill_hq(canvas, thumb_x, thumb_y, thumb_radius, thumb_color, local->alpha);
    }
#else
    egui_canvas_draw_round_rectangle_fill(canvas, region->location.x, region->location.y, region->size.width, region->size.height, track_radius, track_color,
                                          local->alpha);
    egui_canvas_draw_circle_fill_basic(canvas, thumb_x, thumb_y, thumb_radius, thumb_color, local->alpha);
    egui_canvas_draw_circle_basic(canvas, thumb_x, thumb_y, thumb_radius, 1, EGUI_THEME_BORDER, local->alpha);
#endif
}

/**
 * @brief Draw the switch body and optional state icon inside the thumb.
 */
void egui_view_switch_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_switch_t);
    egui_canvas_t *canvas = egui_view_get_canvas(self);
    egui_region_t region;
    egui_dim_t thumb_x;
    egui_dim_t thumb_y;
    egui_dim_t thumb_radius;
    egui_color_t track_color;
    egui_color_t thumb_color;
    const char *icon_text;
    const egui_font_t *icon_font;
    egui_region_t icon_region;
    egui_color_t icon_color;

    if (!egui_view_switch_prepare_draw(self, &region, &thumb_x, &thumb_y, &thumb_radius, &track_color, &thumb_color))
    {
        return;
    }

    egui_view_switch_draw_track_and_thumb(self, &region, thumb_x, thumb_y, thumb_radius, track_color, thumb_color);

    icon_text = local->is_checked ? local->icon_on : local->icon_off;
    if (!EGUI_VIEW_ICON_TEXT_VALID(icon_text))
    {
        return;
    }

    icon_font = EGUI_VIEW_ICON_FONT_RESOLVE(local->icon_font, thumb_radius * 2, 18, 22);
    if (icon_font == NULL)
    {
        return;
    }

    icon_region.location.x = thumb_x - thumb_radius;
    icon_region.location.y = thumb_y - thumb_radius;
    icon_region.size.width = thumb_radius * 2;
    icon_region.size.height = thumb_radius * 2;
    icon_color = egui_view_get_enable(self) ? track_color : EGUI_THEME_DISABLED;
    egui_canvas_draw_text_in_rect(canvas, icon_font, icon_text, &icon_region, EGUI_ALIGN_CENTER, icon_color, local->alpha);
}

/**
 * @brief Register the callback fired after checked-state changes.
 */
void egui_view_switch_set_on_checked_listener(egui_view_t *self, egui_view_on_checked_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_switch_t);
    local->on_checked_changed = listener;
}

egui_view_on_checked_listener_t egui_view_switch_get_on_checked_listener(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_switch_t);
    return local->on_checked_changed;
}

/**
 * @brief Update the checked state, notify listeners, and redraw on real changes.
 */
void egui_view_switch_set_checked(egui_view_t *self, uint8_t is_checked)
{
    EGUI_LOCAL_INIT(egui_view_switch_t);
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

        egui_view_invalidate(self);
    }
}

uint8_t egui_view_switch_get_checked(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_switch_t);
    return local->is_checked;
}

egui_color_t egui_view_switch_get_bk_color_on(egui_view_t *self)
{
    egui_color_t zero;
    zero.full = 0;
    if (self == NULL)
    {
        return zero;
    }
    EGUI_LOCAL_INIT(egui_view_switch_t);
    return local->bk_color_on;
}

egui_color_t egui_view_switch_get_bk_color_off(egui_view_t *self)
{
    egui_color_t zero;
    zero.full = 0;
    if (self == NULL)
    {
        return zero;
    }
    EGUI_LOCAL_INIT(egui_view_switch_t);
    return local->bk_color_off;
}

egui_color_t egui_view_switch_get_switch_color_on(egui_view_t *self)
{
    egui_color_t zero;
    zero.full = 0;
    if (self == NULL)
    {
        return zero;
    }
    EGUI_LOCAL_INIT(egui_view_switch_t);
    return local->switch_color_on;
}

egui_color_t egui_view_switch_get_switch_color_off(egui_view_t *self)
{
    egui_color_t zero;
    zero.full = 0;
    if (self == NULL)
    {
        return zero;
    }
    EGUI_LOCAL_INIT(egui_view_switch_t);
    return local->switch_color_off;
}

egui_alpha_t egui_view_switch_get_alpha(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_switch_t);
    return local->alpha;
}

/**
 * @brief Replace the optional on/off icon glyph pointers.
 */
void egui_view_switch_set_state_icons(egui_view_t *self, const char *icon_on, const char *icon_off)
{
    EGUI_LOCAL_INIT(egui_view_switch_t);
    if (local->icon_on == icon_on && local->icon_off == icon_off)
    {
        return;
    }

    local->icon_on = icon_on;
    local->icon_off = icon_off;
    egui_view_invalidate(self);
}

const char *egui_view_switch_get_icon_on(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_switch_t);
    return local->icon_on;
}

const char *egui_view_switch_get_icon_off(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_switch_t);
    return local->icon_off;
}

/**
 * @brief Override the icon font used by optional state glyphs.
 */
void egui_view_switch_set_icon_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_switch_t);
    if (local->icon_font == font)
    {
        return;
    }

    local->icon_font = font;
    egui_view_invalidate(self);
}

const egui_font_t *egui_view_switch_get_icon_font(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_switch_t);
    return local->icon_font;
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH || EGUI_CONFIG_FUNCTION_SUPPORT_KEY
/**
 * @brief Toggle the checked state when the normal click pipeline completes.
 */
static int egui_view_switch_perform_click(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_switch_t);

    egui_view_switch_set_checked(self, !local->is_checked);
    return 1;
}
#endif

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_switch_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_switch_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH || EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .perform_click = egui_view_switch_perform_click,
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

/**
 * @brief Initialize the switch with clickable toggle behavior and theme defaults.
 */
void egui_view_switch_init(egui_view_t *self, egui_core_t *core)
{
    EGUI_INIT_LOCAL(egui_view_switch_t);
    // call super init.
    egui_view_init(self, core);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_switch_t);

    // init local data.
    local->is_checked = false;
    local->bk_color_on = EGUI_THEME_PRIMARY;
    local->bk_color_off = EGUI_THEME_TRACK_OFF;
    local->switch_color_on = EGUI_THEME_THUMB;
    local->switch_color_off = EGUI_THEME_THUMB;
    local->icon_on = NULL;
    local->icon_off = NULL;
    local->icon_font = NULL;
    local->alpha = EGUI_ALPHA_100;
    self->is_clickable = true;
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    self->is_focusable = true;
#endif

#if EGUI_CONFIG_FUNCTION_WIDGET_ENHANCED_DRAW
    /* No default shadow - let theme control shadows */
#endif

    egui_view_set_view_name(self, "egui_view_switch");
}

/**
 * @brief Apply geometry and initial checked state from one parameter block.
 */
void egui_view_switch_apply_params(egui_view_t *self, const egui_view_switch_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_switch_t);

    self->region = params->region;

    local->is_checked = params->is_checked;

    egui_view_invalidate(self);
}

/**
 * @brief Convenience initializer that chains switch init and params.
 */
void egui_view_switch_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_switch_params_t *params)
{
    egui_view_switch_init(self, core);
    egui_view_switch_apply_params(self, params);
}

#include <stdio.h>
#include <assert.h>

#include "egui_view_switch.h"
#include "resource/egui_resource.h"
#include "style/egui_theme.h"

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
#include "core/egui_canvas_gradient.h"
#include "shadow/egui_shadow.h"
#endif

static const egui_font_t *egui_view_switch_get_icon_font(egui_view_switch_t *local, egui_dim_t area_size)
{
    if (local->icon_font != NULL)
    {
        return local->icon_font;
    }

    if (area_size <= 18)
    {
        return EGUI_FONT_ICON_MS_16;
    }
    if (area_size <= 22)
    {
        return EGUI_FONT_ICON_MS_20;
    }
    return EGUI_FONT_ICON_MS_24;
}

void egui_view_switch_set_on_checked_listener(egui_view_t *self, egui_view_on_checked_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_switch_t);
    local->on_checked_changed = listener;
}

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

        egui_view_invalidate(self);
    }
}

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

static void egui_view_switch_on_click(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_switch_t);

    egui_view_switch_set_checked(self, !local->is_checked);
}

void egui_view_switch_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_switch_t);

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    if (region.size.width <= 0 || region.size.height <= 0)
    {
        return;
    }

    egui_dim_t height = region.size.height;
    egui_dim_t track_radius = height / 2;
    if (track_radius > region.size.width / 2)
    {
        track_radius = region.size.width / 2;
    }
    egui_dim_t padding = 3; // visual gap between thumb and track edge
    egui_dim_t thumb_radius = track_radius - padding;

    // Ensure positive radius
    if (thumb_radius < 1)
        thumb_radius = 1;

    /* Determine state for style lookup */
    egui_state_t state;
    if (!egui_view_get_enable(self))
        state = EGUI_STATE_DISABLED;
    else if (local->is_checked)
        state = EGUI_STATE_CHECKED;
    else
        state = EGUI_STATE_NORMAL;

    const egui_widget_style_desc_t *desc = egui_current_theme ? egui_current_theme->switch_ctrl : NULL;
    const egui_style_t *track_style = desc ? egui_style_get(desc, EGUI_PART_MAIN, state) : NULL;
    const egui_style_t *thumb_style = desc ? egui_style_get(desc, EGUI_PART_INDICATOR, state) : NULL;

    egui_color_t track_color;
    egui_color_t thumb_color;

    if (track_style)
    {
        track_color = track_style->bg_color;
    }
    else
    {
        track_color = local->is_checked ? local->bk_color_on : local->bk_color_off;
        if (!egui_view_get_enable(self))
        {
            track_color = EGUI_THEME_DISABLED;
        }
    }

    if (thumb_style)
    {
        thumb_color = thumb_style->bg_color;
    }
    else
    {
        thumb_color = local->is_checked ? local->switch_color_on : local->switch_color_off;
        if (!egui_view_get_enable(self))
        {
            thumb_color = EGUI_THEME_SURFACE;
        }
    }

    // Draw track
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
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
        egui_canvas_draw_round_rectangle_fill_gradient(region.location.x, region.location.y, region.size.width, region.size.height, track_radius, &track_grad);
    }
#else
    egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, track_radius, track_color, local->alpha);
#endif

    // Draw thumb
    egui_dim_t thumb_y = region.location.y + ((height - 1) >> 1); // center Y
    egui_dim_t thumb_x;

    if (local->is_checked)
    {
        thumb_x = region.location.x + region.size.width - track_radius;
    }
    else
    {
        thumb_x = region.location.x + track_radius;
    }

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
    {
        egui_canvas_draw_circle_fill_hq(thumb_x, thumb_y, thumb_radius, thumb_color, local->alpha);
    }
#else
    egui_canvas_draw_circle_fill(thumb_x, thumb_y, thumb_radius, thumb_color, local->alpha);
    egui_canvas_draw_circle(thumb_x, thumb_y, thumb_radius, 1, EGUI_THEME_BORDER, local->alpha);
#endif

    {
        const char *icon_text = local->is_checked ? local->icon_on : local->icon_off;
        if (icon_text != NULL)
        {
            egui_region_t icon_region = {
                    {thumb_x - thumb_radius, thumb_y - thumb_radius},
                    {thumb_radius * 2, thumb_radius * 2},
            };
            egui_color_t icon_color = egui_view_get_enable(self) ? track_color : EGUI_THEME_DISABLED;
            egui_canvas_draw_text_in_rect(egui_view_switch_get_icon_font(local, thumb_radius * 2), icon_text, &icon_region, EGUI_ALIGN_CENTER, icon_color,
                                          local->alpha);
        }
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_switch_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_switch_on_draw, // changed
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_switch_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_switch_t);
    // call super init.
    egui_view_init(self);
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
    egui_view_set_on_click_listener(self, egui_view_switch_on_click);

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
    /* No default shadow - let theme control shadows */
#endif

    egui_view_set_view_name(self, "egui_view_switch");
}

void egui_view_switch_apply_params(egui_view_t *self, const egui_view_switch_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_switch_t);

    self->region = params->region;

    local->is_checked = params->is_checked;

    egui_view_invalidate(self);
}

void egui_view_switch_init_with_params(egui_view_t *self, const egui_view_switch_params_t *params)
{
    egui_view_switch_init(self);
    egui_view_switch_apply_params(self, params);
}

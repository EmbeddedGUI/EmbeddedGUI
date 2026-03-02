#include <stdio.h>
#include <assert.h>

#include "egui_view_activity_ring.h"

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
#include "core/egui_canvas_gradient.h"
#endif

void egui_view_activity_ring_set_value(egui_view_t *self, uint8_t ring_index, uint8_t value)
{
    EGUI_LOCAL_INIT(egui_view_activity_ring_t);
    if (ring_index >= EGUI_VIEW_ACTIVITY_RING_MAX_RINGS)
    {
        return;
    }
    if (value > 100)
    {
        value = 100;
    }
    if (value != local->values[ring_index])
    {
        local->values[ring_index] = value;
        egui_view_invalidate(self);
    }
}

uint8_t egui_view_activity_ring_get_value(egui_view_t *self, uint8_t ring_index)
{
    EGUI_LOCAL_INIT(egui_view_activity_ring_t);
    if (ring_index >= EGUI_VIEW_ACTIVITY_RING_MAX_RINGS)
    {
        return 0;
    }
    return local->values[ring_index];
}

void egui_view_activity_ring_set_ring_count(egui_view_t *self, uint8_t count)
{
    EGUI_LOCAL_INIT(egui_view_activity_ring_t);
    if (count > EGUI_VIEW_ACTIVITY_RING_MAX_RINGS)
    {
        count = EGUI_VIEW_ACTIVITY_RING_MAX_RINGS;
    }
    if (count != local->ring_count)
    {
        local->ring_count = count;
        egui_view_invalidate(self);
    }
}

void egui_view_activity_ring_set_ring_color(egui_view_t *self, uint8_t ring_index, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_activity_ring_t);
    if (ring_index >= EGUI_VIEW_ACTIVITY_RING_MAX_RINGS)
    {
        return;
    }
    local->ring_colors[ring_index] = color;
    egui_view_invalidate(self);
}

void egui_view_activity_ring_set_ring_bg_color(egui_view_t *self, uint8_t ring_index, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_activity_ring_t);
    if (ring_index >= EGUI_VIEW_ACTIVITY_RING_MAX_RINGS)
    {
        return;
    }
    local->ring_bg_colors[ring_index] = color;
    egui_view_invalidate(self);
}

void egui_view_activity_ring_set_stroke_width(egui_view_t *self, egui_dim_t stroke_width)
{
    EGUI_LOCAL_INIT(egui_view_activity_ring_t);
    if (stroke_width != local->stroke_width)
    {
        local->stroke_width = stroke_width;
        egui_view_invalidate(self);
    }
}

void egui_view_activity_ring_set_ring_gap(egui_view_t *self, egui_dim_t ring_gap)
{
    EGUI_LOCAL_INIT(egui_view_activity_ring_t);
    if (ring_gap != local->ring_gap)
    {
        local->ring_gap = ring_gap;
        egui_view_invalidate(self);
    }
}

void egui_view_activity_ring_set_start_angle(egui_view_t *self, int16_t start_angle)
{
    EGUI_LOCAL_INIT(egui_view_activity_ring_t);
    if (start_angle != local->start_angle)
    {
        local->start_angle = start_angle;
        egui_view_invalidate(self);
    }
}

void egui_view_activity_ring_set_show_round_cap(egui_view_t *self, uint8_t show)
{
    EGUI_LOCAL_INIT(egui_view_activity_ring_t);
    if (show != local->show_round_cap)
    {
        local->show_round_cap = show;
        egui_view_invalidate(self);
    }
}

void egui_view_activity_ring_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_activity_ring_t);

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    egui_dim_t w = region.size.width;
    egui_dim_t h = region.size.height;
    egui_dim_t center_x = region.location.x + w / 2;
    egui_dim_t center_y = region.location.y + h / 2;
    egui_dim_t outer_radius = EGUI_MIN(w, h) / 2 - local->stroke_width / 2 - 1;

    if (outer_radius <= 0)
    {
        return;
    }

    uint8_t i;
    for (i = 0; i < local->ring_count; i++)
    {
        egui_dim_t cur_radius = outer_radius - i * (local->stroke_width + local->ring_gap);
        if (cur_radius <= 0)
        {
            break;
        }

        // Background arc (full 360)
        egui_canvas_draw_arc(center_x, center_y, cur_radius, 0, 360, local->stroke_width, local->ring_bg_colors[i], EGUI_ALPHA_100);

        // Progress arc
        if (local->values[i] > 0)
        {
            int16_t end_angle = local->start_angle + (int16_t)((uint32_t)local->values[i] * 360 / 100);
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
            {
                egui_color_t color_light = egui_rgb_mix(local->ring_colors[i], EGUI_COLOR_WHITE, 80);
                egui_gradient_stop_t stops[2] = {
                        {.position = 0, .color = color_light},
                        {.position = 255, .color = local->ring_colors[i]},
                };
                egui_gradient_t grad = {
                        .type = EGUI_GRADIENT_TYPE_RADIAL,
                        .stop_count = 2,
                        .alpha = EGUI_ALPHA_100,
                        .stops = stops,
                };
                egui_dim_t inner_r = cur_radius - local->stroke_width;
                if (inner_r < 0)
                    inner_r = 0;
                egui_canvas_draw_arc_ring_fill_gradient_round_cap(center_x, center_y, cur_radius, inner_r, local->start_angle, end_angle, &grad,
                                                                  local->show_round_cap ? EGUI_ARC_CAP_BOTH : EGUI_ARC_CAP_NONE);
            }
#else
            {
#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_CIRCLE_HQ
                if (local->show_round_cap)
                {
                    egui_canvas_draw_arc_round_cap_hq(center_x, center_y, cur_radius, local->start_angle, end_angle, local->stroke_width, local->ring_colors[i],
                                                      EGUI_ALPHA_100);
                }
                else
#endif
                {
                    egui_canvas_draw_arc(center_x, center_y, cur_radius, local->start_angle, end_angle, local->stroke_width, local->ring_colors[i],
                                         EGUI_ALPHA_100);
                }
            }
#endif
        }
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_activity_ring_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_activity_ring_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_activity_ring_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_activity_ring_t);
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_activity_ring_t);

    // init local data.
    local->ring_count = 3;
    local->stroke_width = 8;
    local->ring_gap = 4;
    local->start_angle = -90;
    local->show_round_cap = 1;

    local->values[0] = 0;
    local->values[1] = 0;
    local->values[2] = 0;

    local->ring_colors[0] = EGUI_THEME_DANGER;
    local->ring_colors[1] = EGUI_THEME_SUCCESS;
    local->ring_colors[2] = EGUI_THEME_PRIMARY;

    local->ring_bg_colors[0] = EGUI_THEME_TRACK_BG;
    local->ring_bg_colors[1] = EGUI_THEME_TRACK_BG;
    local->ring_bg_colors[2] = EGUI_THEME_TRACK_BG;

    egui_view_set_view_name(self, "egui_view_activity_ring");
}

void egui_view_activity_ring_apply_params(egui_view_t *self, const egui_view_activity_ring_params_t *params)
{
    self->region = params->region;
    egui_view_invalidate(self);
}

void egui_view_activity_ring_init_with_params(egui_view_t *self, const egui_view_activity_ring_params_t *params)
{
    egui_view_activity_ring_init(self);
    egui_view_activity_ring_apply_params(self, params);
}

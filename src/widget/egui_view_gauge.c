#include <stdio.h>
#include <assert.h>

#include "egui_view_gauge.h"
#include "utils/egui_fixmath.h"

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
#include "core/egui_canvas_gradient.h"
#endif

void egui_view_gauge_set_value(egui_view_t *self, uint8_t value)
{
    EGUI_LOCAL_INIT(egui_view_gauge_t);
    if (value > 100)
    {
        value = 100;
    }
    if (value != local->value)
    {
        local->value = value;
        egui_view_invalidate(self);
    }
}

void egui_view_gauge_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_gauge_t);

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    egui_dim_t w = region.size.width;
    egui_dim_t h = region.size.height;
    egui_dim_t center_x = region.location.x + w / 2;
    egui_dim_t center_y = region.location.y + h / 2;
    egui_dim_t radius = EGUI_MIN(w, h) / 2 - local->stroke_width / 2 - 1;

    if (radius <= 0)
    {
        return;
    }

    // Background arc (full sweep)
    int16_t bg_start = local->start_angle;
    int16_t bg_end = local->start_angle + local->sweep_angle;
    egui_canvas_draw_arc(center_x, center_y, radius, bg_start, bg_end, local->stroke_width, local->bk_color, EGUI_ALPHA_100);

    // Progress arc
    int16_t progress_end = local->start_angle + (int16_t)((int32_t)local->sweep_angle * local->value / 100);
    if (local->value > 0)
    {
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
        {
            egui_color_t color_light = egui_rgb_mix(local->progress_color, EGUI_COLOR_WHITE, 80);
            egui_gradient_stop_t stops[2] = {
                    {.position = 0, .color = color_light},
                    {.position = 255, .color = local->progress_color},
            };
            egui_gradient_t grad = {
                    .type = EGUI_GRADIENT_TYPE_RADIAL,
                    .stop_count = 2,
                    .alpha = EGUI_ALPHA_100,
                    .stops = stops,
            };
            egui_canvas_draw_arc_ring_fill_gradient(center_x, center_y, radius, radius - local->stroke_width, bg_start, progress_end, &grad);
        }
#else
        egui_canvas_draw_arc(center_x, center_y, radius, bg_start, progress_end, local->stroke_width, local->progress_color, EGUI_ALPHA_100);
#endif
    }

    // Needle - analog clock style hand
    egui_dim_t needle_len = radius - local->stroke_width / 2;
    egui_dim_t center_dot_r = EGUI_MAX(radius / 10, 3);
    if (needle_len > 0)
    {
        // angle_deg = start_angle + value * sweep_angle / 100
        int16_t needle_deg = local->start_angle + (int16_t)((int32_t)local->sweep_angle * local->value / 100);
        // Convert degrees to fixed-point radians: rad = deg * PI / 180
        egui_float_t angle_rad = EGUI_FLOAT_DIV(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(needle_deg), EGUI_FLOAT_PI), EGUI_FLOAT_VALUE_INT(180));

        egui_float_t cos_val = EGUI_FLOAT_COS(angle_rad);
        egui_float_t sin_val = EGUI_FLOAT_SIN(angle_rad);

        egui_dim_t tip_x = center_x + (egui_dim_t)EGUI_FLOAT_INT_PART(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(needle_len), cos_val));
        egui_dim_t tip_y = center_y + (egui_dim_t)EGUI_FLOAT_INT_PART(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(needle_len), sin_val));
        egui_dim_t hand_w = EGUI_MAX(local->stroke_width, 2);
#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_LINE_HQ
        egui_canvas_draw_line_round_cap_hq(center_x, center_y, tip_x, tip_y, hand_w, local->needle_color, EGUI_ALPHA_100);
#else
        egui_canvas_draw_line(center_x, center_y, tip_x, tip_y, hand_w, local->needle_color, EGUI_ALPHA_100);
#endif
    }

    // Center dot decoration
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
    {
        egui_color_t color_light = egui_rgb_mix(local->needle_color, EGUI_COLOR_WHITE, 80);
        egui_gradient_stop_t stops[2] = {
                {.position = 0, .color = color_light},
                {.position = 255, .color = local->needle_color},
        };
        egui_gradient_t grad = {
                .type = EGUI_GRADIENT_TYPE_RADIAL,
                .stop_count = 2,
                .alpha = EGUI_ALPHA_100,
                .stops = stops,
        };
        egui_canvas_draw_circle_fill_gradient(center_x, center_y, center_dot_r, &grad);
    }
#else
    egui_canvas_draw_circle_fill(center_x, center_y, center_dot_r, local->needle_color, EGUI_ALPHA_100);
#endif
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_gauge_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_gauge_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_gauge_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_gauge_t);
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_gauge_t);

    // init local data.
    local->value = 0;
    local->stroke_width = EGUI_THEME_TRACK_THICKNESS;
    local->start_angle = 150; // ~7 o'clock (0=3 o'clock, clockwise)
    local->sweep_angle = 240; // sweep 240 degrees, gap at bottom
    local->bk_color = EGUI_THEME_TRACK_BG;
    local->progress_color = EGUI_THEME_PRIMARY;
    local->needle_color = EGUI_THEME_DANGER;

    egui_view_set_view_name(self, "egui_view_gauge");
}

void egui_view_gauge_apply_params(egui_view_t *self, const egui_view_gauge_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_gauge_t);

    self->region = params->region;

    local->value = params->value;

    egui_view_invalidate(self);
}

void egui_view_gauge_init_with_params(egui_view_t *self, const egui_view_gauge_params_t *params)
{
    egui_view_gauge_init(self);
    egui_view_gauge_apply_params(self, params);
}

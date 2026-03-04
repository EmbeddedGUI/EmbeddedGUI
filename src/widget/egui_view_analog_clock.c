#include <stdio.h>
#include <assert.h>

#include "egui_view_analog_clock.h"
#include "core/egui_canvas_gradient.h"
#include "utils/egui_fixmath.h"

void egui_view_analog_clock_set_time(egui_view_t *self, uint8_t h, uint8_t m, uint8_t s)
{
    EGUI_LOCAL_INIT(egui_view_analog_clock_t);
    h = h % 12;
    if (m > 59)
    {
        m = 59;
    }
    if (s > 59)
    {
        s = 59;
    }
    if (local->hour != h || local->minute != m || local->second != s)
    {
        local->hour = h;
        local->minute = m;
        local->second = s;
        egui_view_invalidate(self);
    }
}

void egui_view_analog_clock_show_second(egui_view_t *self, uint8_t show)
{
    EGUI_LOCAL_INIT(egui_view_analog_clock_t);
    if (local->show_second != show)
    {
        local->show_second = show;
        egui_view_invalidate(self);
    }
}

void egui_view_analog_clock_show_ticks(egui_view_t *self, uint8_t show)
{
    EGUI_LOCAL_INIT(egui_view_analog_clock_t);
    if (local->show_ticks != show)
    {
        local->show_ticks = show;
        egui_view_invalidate(self);
    }
}

static void egui_view_analog_clock_draw_hand(egui_dim_t cx, egui_dim_t cy, int16_t angle_deg, egui_dim_t length, egui_dim_t width, egui_color_t color,
                                             uint8_t use_round_cap)
{
    egui_float_t angle_rad = EGUI_FLOAT_DIV(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(angle_deg), EGUI_FLOAT_PI), EGUI_FLOAT_VALUE_INT(180));
    egui_float_t cos_val = EGUI_FLOAT_COS(angle_rad);
    egui_float_t sin_val = EGUI_FLOAT_SIN(angle_rad);

    egui_dim_t end_x = cx + (egui_dim_t)EGUI_FLOAT_INT_PART(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(length), cos_val));
    egui_dim_t end_y = cy + (egui_dim_t)EGUI_FLOAT_INT_PART(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(length), sin_val));

    if (use_round_cap)
    {
#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_LINE_HQ
        egui_canvas_draw_line_round_cap_hq(cx, cy, end_x, end_y, width, color, EGUI_ALPHA_100);
#else
        egui_canvas_draw_line(cx, cy, end_x, end_y, width, color, EGUI_ALPHA_100);
#endif
    }
    else
    {
        egui_canvas_draw_line(cx, cy, end_x, end_y, width, color, EGUI_ALPHA_100);
    }
}

void egui_view_analog_clock_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_analog_clock_t);

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    egui_dim_t w = region.size.width;
    egui_dim_t h = region.size.height;
    egui_dim_t cx = region.location.x + w / 2;
    egui_dim_t cy = region.location.y + h / 2;
    egui_dim_t radius = EGUI_MIN(w, h) / 2 - 2;

    if (radius <= 0)
    {
        return;
    }

    // Outer circle
    egui_canvas_draw_circle(cx, cy, radius, 2, local->dial_color, EGUI_ALPHA_100);

    // Tick marks
    if (local->show_ticks)
    {
        int i;
        for (i = 0; i < 60; i++)
        {
            int16_t angle_deg = i * 6 - 90;
            egui_float_t angle_rad = EGUI_FLOAT_DIV(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(angle_deg), EGUI_FLOAT_PI), EGUI_FLOAT_VALUE_INT(180));
            egui_float_t cos_val = EGUI_FLOAT_COS(angle_rad);
            egui_float_t sin_val = EGUI_FLOAT_SIN(angle_rad);

            egui_dim_t inner_r, outer_r, tick_w;
            if (i % 5 == 0)
            {
                inner_r = radius - 10;
                outer_r = radius - 4;
                tick_w = 2;
            }
            else
            {
                inner_r = radius - 6;
                outer_r = radius - 4;
                tick_w = 1;
            }

            egui_dim_t x1 = cx + (egui_dim_t)EGUI_FLOAT_INT_PART(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(inner_r), cos_val));
            egui_dim_t y1 = cy + (egui_dim_t)EGUI_FLOAT_INT_PART(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(inner_r), sin_val));
            egui_dim_t x2 = cx + (egui_dim_t)EGUI_FLOAT_INT_PART(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(outer_r), cos_val));
            egui_dim_t y2 = cy + (egui_dim_t)EGUI_FLOAT_INT_PART(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(outer_r), sin_val));

            egui_canvas_draw_line(x1, y1, x2, y2, tick_w, local->tick_color, EGUI_ALPHA_100);
        }
    }

    // Hour hand: angle = (hour%12)*30 + minute/2 - 90, length = radius*50/100
    {
        int16_t hour_angle = (int16_t)(local->hour % 12) * 30 + (int16_t)local->minute / 2 - 90;
        egui_dim_t hour_len = radius * 50 / 100;
        egui_view_analog_clock_draw_hand(cx, cy, hour_angle, hour_len, local->hour_hand_width, local->hour_color, 1);
    }

    // Minute hand: angle = minute*6 - 90, length = radius*70/100
    {
        int16_t min_angle = (int16_t)local->minute * 6 - 90;
        egui_dim_t min_len = radius * 70 / 100;
        egui_view_analog_clock_draw_hand(cx, cy, min_angle, min_len, local->minute_hand_width, local->minute_color, 1);
    }

    // Second hand (if show_second): angle = second*6 - 90, length = radius*80/100
    if (local->show_second)
    {
        int16_t sec_angle = (int16_t)local->second * 6 - 90;
        egui_dim_t sec_len = radius * 80 / 100;
        egui_view_analog_clock_draw_hand(cx, cy, sec_angle, sec_len, local->second_hand_width, local->second_color, 0);
    }

    // Center dot
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
    {
        egui_dim_t dot_r = EGUI_MAX(radius / 10, 3);
        egui_color_t color_light = egui_rgb_mix(local->hour_color, EGUI_COLOR_WHITE, 80);
        egui_gradient_stop_t stops[2] = {
                {.position = 0, .color = color_light},
                {.position = 255, .color = local->hour_color},
        };
        egui_gradient_t grad = {
                .type = EGUI_GRADIENT_TYPE_RADIAL,
                .stop_count = 2,
                .alpha = EGUI_ALPHA_100,
                .stops = stops,
        };
        egui_canvas_draw_circle_fill_gradient(cx, cy, dot_r, &grad);
    }
#else
    egui_canvas_draw_circle_fill(cx, cy, EGUI_MAX(radius / 10, 3), local->hour_color, EGUI_ALPHA_100);
#endif
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_analog_clock_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_analog_clock_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_analog_clock_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_analog_clock_t);
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_analog_clock_t);

    // init local data.
    local->hour = 0;
    local->minute = 0;
    local->second = 0;
    local->show_second = 1;
    local->show_ticks = 1;
    local->hour_hand_width = EGUI_THEME_STROKE_WIDTH * 2;
    local->minute_hand_width = EGUI_THEME_STROKE_WIDTH;
    local->second_hand_width = 1;
    local->dial_color = EGUI_THEME_TEXT;
    local->hour_color = EGUI_THEME_TEXT;
    local->minute_color = EGUI_THEME_TEXT;
    local->second_color = EGUI_THEME_DANGER;
    local->tick_color = EGUI_THEME_TRACK_BG;

    egui_view_set_view_name(self, "egui_view_analog_clock");
}

void egui_view_analog_clock_apply_params(egui_view_t *self, const egui_view_analog_clock_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_analog_clock_t);

    self->region = params->region;

    local->hour = params->hour % 12;
    local->minute = (params->minute > 59) ? 59 : params->minute;
    local->second = (params->second > 59) ? 59 : params->second;

    egui_view_invalidate(self);
}

void egui_view_analog_clock_init_with_params(egui_view_t *self, const egui_view_analog_clock_params_t *params)
{
    egui_view_analog_clock_init(self);
    egui_view_analog_clock_apply_params(self, params);
}

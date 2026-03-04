#include <assert.h>

#include "egui_view_compass.h"
#include "utils/egui_sprintf.h"
#include "core/egui_canvas_gradient.h"
#include "font/egui_font.h"
#include "font/egui_font_std.h"
#include "resource/egui_resource.h"
#include "utils/egui_fixmath.h"

void egui_view_compass_set_heading(egui_view_t *self, int16_t heading)
{
    EGUI_LOCAL_INIT(egui_view_compass_t);
    // Normalize to 0-359
    heading = heading % 360;
    if (heading < 0)
    {
        heading += 360;
    }
    if (heading != local->heading)
    {
        local->heading = heading;
        egui_view_invalidate(self);
    }
}

void egui_view_compass_set_show_degree(egui_view_t *self, uint8_t show)
{
    EGUI_LOCAL_INIT(egui_view_compass_t);
    if (show != local->show_degree)
    {
        local->show_degree = show;
        egui_view_invalidate(self);
    }
}

void egui_view_compass_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_compass_t);

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    egui_dim_t w = region.size.width;
    egui_dim_t h = region.size.height;
    egui_dim_t cx = region.location.x + w / 2;
    egui_dim_t cy = region.location.y + h / 2;
    egui_dim_t radius = EGUI_MIN(w, h) / 2 - local->stroke_width / 2 - 1;

    if (radius <= 0)
    {
        return;
    }

    // Outer circle
    egui_canvas_draw_circle(cx, cy, radius, local->stroke_width, local->dial_color, EGUI_ALPHA_100);

    // Rotating tick marks (all angles offset by -heading)
    int i;
    for (i = 0; i < 36; i++)
    {
        // angle_deg = i * 10 - heading - 90 (rotate by heading, -90 for canvas coords)
        int16_t angle_deg = i * 10 - local->heading - 90;
        egui_float_t angle_rad = EGUI_FLOAT_DIV(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(angle_deg), EGUI_FLOAT_PI), EGUI_FLOAT_VALUE_INT(180));

        egui_float_t cos_v = EGUI_FLOAT_COS(angle_rad);
        egui_float_t sin_v = EGUI_FLOAT_SIN(angle_rad);

        egui_dim_t inner_r;
        egui_dim_t outer_r;
        egui_dim_t line_w;
        egui_color_t tick_color;

        if (i % 9 == 0)
        {
            // N/E/S/W (every 90 degrees) - major tick
            inner_r = radius - 10;
            outer_r = radius - 4;
            line_w = 2;
            tick_color = (i == 0) ? local->north_color : local->dial_color;
        }
        else if (i % 3 == 0)
        {
            // every 30 degrees
            inner_r = radius - 7;
            outer_r = radius - 4;
            line_w = 2;
            tick_color = local->dial_color;
        }
        else
        {
            // every 10 degrees
            inner_r = radius - 6;
            outer_r = radius - 4;
            line_w = 1;
            tick_color = local->dial_color;
        }

        egui_dim_t x1 = cx + (egui_dim_t)EGUI_FLOAT_INT_PART(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(inner_r), cos_v));
        egui_dim_t y1 = cy + (egui_dim_t)EGUI_FLOAT_INT_PART(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(inner_r), sin_v));
        egui_dim_t x2 = cx + (egui_dim_t)EGUI_FLOAT_INT_PART(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(outer_r), cos_v));
        egui_dim_t y2 = cy + (egui_dim_t)EGUI_FLOAT_INT_PART(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(outer_r), sin_v));

        egui_canvas_draw_line(x1, y1, x2, y2, line_w, tick_color, EGUI_ALPHA_100);
    }

    // Fixed needle pointing up (north indicator)
    egui_dim_t needle_len = radius * 60 / 100;

    // Upper triangle (red/north): vertices at (cx, cy-needle_len), (cx-4, cy), (cx+4, cy)
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
    {
        egui_color_t color_light = egui_rgb_mix(local->north_color, EGUI_COLOR_WHITE, 80);
        egui_gradient_stop_t stops[2] = {
                {.position = 0, .color = color_light},
                {.position = 255, .color = local->north_color},
        };
        egui_gradient_t grad = {
                .type = EGUI_GRADIENT_TYPE_LINEAR_VERTICAL,
                .stop_count = 2,
                .alpha = EGUI_ALPHA_100,
                .stops = stops,
        };
        egui_canvas_draw_triangle_fill_gradient(cx, cy - needle_len, cx - 4, cy, cx + 4, cy, &grad);
    }
#else
    egui_canvas_draw_triangle_fill(cx, cy - needle_len, cx - 4, cy, cx + 4, cy, local->north_color, EGUI_ALPHA_100);
#endif
    // Lower triangle (white/south): vertices at (cx, cy+needle_len), (cx-4, cy), (cx+4, cy)
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
    {
        egui_color_t color_light = egui_rgb_mix(local->needle_color, EGUI_COLOR_WHITE, 80);
        egui_gradient_stop_t stops[2] = {
                {.position = 0, .color = local->needle_color},
                {.position = 255, .color = color_light},
        };
        egui_gradient_t grad = {
                .type = EGUI_GRADIENT_TYPE_LINEAR_VERTICAL,
                .stop_count = 2,
                .alpha = EGUI_ALPHA_100,
                .stops = stops,
        };
        egui_canvas_draw_triangle_fill_gradient(cx, cy + needle_len, cx - 4, cy, cx + 4, cy, &grad);
    }
#else
    egui_canvas_draw_triangle_fill(cx, cy + needle_len, cx - 4, cy, cx + 4, cy, local->needle_color, EGUI_ALPHA_100);
#endif

    // Center degree text (if show_degree)
    if (local->show_degree && local->font != NULL)
    {
        char buf[8];
        egui_sprintf_int(buf, sizeof(buf), local->heading);
        egui_region_t degree_region;
        degree_region.location.x = region.location.x;
        degree_region.location.y = region.location.y + h * 2 / 3;
        degree_region.size.width = w;
        degree_region.size.height = h / 3;
        egui_canvas_draw_text_in_rect(local->font, buf, &degree_region, EGUI_ALIGN_CENTER, local->text_color, EGUI_ALPHA_100);
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_compass_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_compass_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_compass_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_compass_t);
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_compass_t);

    // init local data.
    local->heading = 0;
    local->stroke_width = EGUI_THEME_STROKE_WIDTH;
    local->dial_color = EGUI_THEME_TEXT;
    local->north_color = EGUI_THEME_DANGER;
    local->needle_color = EGUI_THEME_TRACK_BG;
    local->text_color = EGUI_COLOR_WHITE;
    local->show_degree = 1;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;

    egui_view_set_view_name(self, "egui_view_compass");
}

void egui_view_compass_apply_params(egui_view_t *self, const egui_view_compass_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_compass_t);

    self->region = params->region;

    local->heading = params->heading;

    egui_view_invalidate(self);
}

void egui_view_compass_init_with_params(egui_view_t *self, const egui_view_compass_params_t *params)
{
    egui_view_compass_init(self);
    egui_view_compass_apply_params(self, params);
}

#include <stdio.h>
#include <assert.h>

#include "egui_view_spinner.h"

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
#include "core/egui_canvas_gradient.h"
#endif

static void egui_view_spinner_timer_callback(egui_timer_t *timer)
{
    egui_view_spinner_t *local = (egui_view_spinner_t *)timer->user_data;
    local->rotation_angle = (local->rotation_angle + 10) % 360;
    egui_view_invalidate((egui_view_t *)local);
}

void egui_view_spinner_start(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_spinner_t);
    if (!local->is_spinning)
    {
        local->is_spinning = 1;
        egui_timer_start_timer(&local->spin_timer, 50, 50);
    }
}

void egui_view_spinner_stop(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_spinner_t);
    if (local->is_spinning)
    {
        local->is_spinning = 0;
        egui_timer_stop_timer(&local->spin_timer);
    }
}

void egui_view_spinner_set_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_spinner_t);
    local->color = color;
    egui_view_invalidate(self);
}

void egui_view_spinner_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_spinner_t);

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

    int16_t start = local->rotation_angle;
    int16_t end = local->rotation_angle + local->arc_length;

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
    {
        egui_color_t color_light = egui_rgb_mix(local->color, EGUI_COLOR_WHITE, 80);
        egui_gradient_stop_t stops[2] = {
                {.position = 0, .color = color_light},
                {.position = 255, .color = local->color},
        };
        egui_gradient_t grad = {
                .type = EGUI_GRADIENT_TYPE_RADIAL,
                .stop_count = 2,
                .alpha = EGUI_ALPHA_100,
                .stops = stops,
        };
        egui_canvas_draw_arc_ring_fill_gradient(center_x, center_y, radius, radius - local->stroke_width, start, end, &grad);
    }
#else
    egui_canvas_draw_arc(center_x, center_y, radius, start, end, local->stroke_width, local->color, EGUI_ALPHA_100);
#endif
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_spinner_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_spinner_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_spinner_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_spinner_t);
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_spinner_t);

    // init local data.
    local->arc_length = 60;
    local->rotation_angle = 0;
    local->stroke_width = EGUI_THEME_STROKE_WIDTH * 2;
    local->color = EGUI_THEME_PRIMARY;
    local->is_spinning = 0;

    egui_timer_init_timer(&local->spin_timer, local, egui_view_spinner_timer_callback);

    egui_view_set_view_name(self, "egui_view_spinner");
}

void egui_view_spinner_apply_params(egui_view_t *self, const egui_view_spinner_params_t *params)
{
    self->region = params->region;
    egui_view_invalidate(self);
}

void egui_view_spinner_init_with_params(egui_view_t *self, const egui_view_spinner_params_t *params)
{
    egui_view_spinner_init(self);
    egui_view_spinner_apply_params(self, params);
}

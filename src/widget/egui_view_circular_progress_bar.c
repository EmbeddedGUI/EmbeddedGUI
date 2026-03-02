#include <stdio.h>
#include <assert.h>

#include "egui_view_circular_progress_bar.h"

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
#include "core/egui_canvas_gradient.h"
#endif

void egui_view_circular_progress_bar_set_on_progress_listener(egui_view_t *self, egui_view_on_progress_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_circular_progress_bar_t);
    local->on_progress_changed = listener;
}

void egui_view_circular_progress_bar_set_process(egui_view_t *self, uint8_t process)
{
    EGUI_LOCAL_INIT(egui_view_circular_progress_bar_t);
    if (process > 100)
    {
        process = 100;
    }
    if (process != local->process)
    {
        local->process = process;
        if (local->on_progress_changed)
        {
            local->on_progress_changed(self, process);
        }

        egui_view_invalidate(self);
    }
}

void egui_view_circular_progress_bar_set_stroke_width(egui_view_t *self, egui_dim_t stroke_width)
{
    EGUI_LOCAL_INIT(egui_view_circular_progress_bar_t);
    if (stroke_width != local->stroke_width)
    {
        local->stroke_width = stroke_width;
        egui_view_invalidate(self);
    }
}

void egui_view_circular_progress_bar_set_progress_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_circular_progress_bar_t);
    local->progress_color = color;
    egui_view_invalidate(self);
}

void egui_view_circular_progress_bar_set_bk_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_circular_progress_bar_t);
    local->bk_color = color;
    egui_view_invalidate(self);
}

void egui_view_circular_progress_bar_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_circular_progress_bar_t);

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    // Calculate center and radius from the widget region
    egui_dim_t center_x = region.location.x + region.size.width / 2;
    egui_dim_t center_y = region.location.y + region.size.height / 2;
    egui_dim_t radius = EGUI_MIN(region.size.width, region.size.height) / 2 - local->stroke_width / 2 - 1;

    if (radius <= 0)
    {
        return;
    }

    // Draw background arc (full 360 degrees)
    egui_canvas_draw_arc(center_x, center_y, radius, 0, 360, local->stroke_width, local->bk_color, EGUI_ALPHA_100);

    // Draw progress arc
    if (local->process > 0)
    {
        int16_t end_angle = local->start_angle + (int16_t)((uint32_t)local->process * 360 / 100);
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
            egui_canvas_draw_arc_ring_fill_gradient(center_x, center_y, radius, radius - local->stroke_width, local->start_angle, end_angle, &grad);
        }
#else
        egui_canvas_draw_arc(center_x, center_y, radius, local->start_angle, end_angle, local->stroke_width, local->progress_color, EGUI_ALPHA_100);
#endif
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_circular_progress_bar_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_circular_progress_bar_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_circular_progress_bar_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_circular_progress_bar_t);
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_circular_progress_bar_t);

    // init local data.
    local->on_progress_changed = NULL;
    local->process = 0;
    local->stroke_width = 4;
    local->start_angle = -90;
    local->bk_color = EGUI_COLOR_DARK_GREY;
    local->progress_color = EGUI_COLOR_GREEN;

    egui_view_set_view_name(self, "egui_view_circular_progress_bar");
}

void egui_view_circular_progress_bar_apply_params(egui_view_t *self, const egui_view_circular_progress_bar_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_circular_progress_bar_t);

    self->region = params->region;

    local->process = params->process;

    egui_view_invalidate(self);
}

void egui_view_circular_progress_bar_init_with_params(egui_view_t *self, const egui_view_circular_progress_bar_params_t *params)
{
    egui_view_circular_progress_bar_init(self);
    egui_view_circular_progress_bar_apply_params(self, params);
}

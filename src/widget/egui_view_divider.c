#include <stdio.h>
#include <assert.h>

#include "egui_view_divider.h"
#include "core/egui_canvas_gradient.h"

void egui_view_divider_set_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_divider_t);
    if (local->color.full == color.full)
    {
        return;
    }
    local->color = color;
    egui_view_invalidate(self);
}

void egui_view_divider_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_divider_t);

    egui_region_t region;
    egui_view_get_work_region(self, &region);

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
    {
        egui_color_t color_light = egui_rgb_mix(local->color, EGUI_COLOR_WHITE, 80);
        egui_gradient_stop_t stops[2] = {
                {.position = 0, .color = color_light},
                {.position = 255, .color = local->color},
        };
        egui_gradient_t grad = {
                .type = EGUI_GRADIENT_TYPE_LINEAR_VERTICAL,
                .stop_count = 2,
                .alpha = local->alpha,
                .stops = stops,
        };
        egui_canvas_draw_rectangle_fill_gradient(region.location.x, region.location.y, region.size.width, region.size.height, &grad);
    }
#else
    egui_canvas_draw_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, local->color, local->alpha);
#endif
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_divider_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_divider_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_divider_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_divider_t);
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_divider_t);

    // init local data.
    local->alpha = EGUI_ALPHA_100;
    local->color = EGUI_THEME_BORDER;

    egui_view_set_view_name(self, "egui_view_divider");
}

void egui_view_divider_apply_params(egui_view_t *self, const egui_view_divider_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_divider_t);

    self->region = params->region;

    local->color = params->color;

    egui_view_invalidate(self);
}

void egui_view_divider_init_with_params(egui_view_t *self, const egui_view_divider_params_t *params)
{
    egui_view_divider_init(self);
    egui_view_divider_apply_params(self, params);
}

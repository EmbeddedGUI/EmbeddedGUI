#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_view_chart_scatter.h"
#include "core/egui_canvas_gradient.h"
#include "font/egui_font_std.h"
#include "resource/egui_resource.h"

// ============== Scatter Drawing (virtual) ==============

static void egui_view_chart_scatter_draw_data(egui_view_t *self, const egui_region_t *plot_area)
{
    EGUI_LOCAL_INIT(egui_view_chart_scatter_t);
    egui_chart_axis_base_t *ab = &local->axis_base.ab;

    for (uint8_t s = 0; s < ab->series_count; s++)
    {
        const egui_chart_series_t *series = &ab->series[s];
        uint8_t r = local->point_radius > 0 ? local->point_radius : 3;

        for (uint8_t i = 0; i < series->point_count; i++)
        {
            egui_dim_t px = egui_chart_map_x(ab, series->points[i].x, plot_area->location.x, plot_area->size.width);
            egui_dim_t py = egui_chart_map_y(ab, series->points[i].y, plot_area->location.y, plot_area->size.height);
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
            {
                egui_color_t color_light = egui_rgb_mix(series->color, EGUI_COLOR_WHITE, 80);
                egui_gradient_stop_t stops[2] = {
                        {.position = 0, .color = color_light},
                        {.position = 255, .color = series->color},
                };
                egui_gradient_t grad = {
                        .type = EGUI_GRADIENT_TYPE_RADIAL,
                        .stop_count = 2,
                        .alpha = EGUI_ALPHA_100,
                        .stops = stops,
                };
                egui_canvas_draw_circle_fill_gradient(px, py, r, &grad);
            }
#else
            egui_canvas_draw_circle_fill(px, py, r, series->color, EGUI_ALPHA_100);
#endif
        }
    }
}

// ============== API Table ==============

const egui_view_chart_axis_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_chart_scatter_t) = {
        .base =
                {
                        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
                        .on_touch_event = egui_view_chart_axis_on_touch_event,
#else
                        .on_touch_event = egui_view_on_touch_event,
#endif
                        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
                        .compute_scroll = egui_view_compute_scroll,
                        .calculate_layout = egui_view_calculate_layout,
                        .request_layout = egui_view_request_layout,
                        .draw = egui_view_draw,
                        .on_attach_to_window = egui_view_on_attach_to_window,
                        .on_draw = egui_view_chart_axis_on_draw,
                        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
                        .dispatch_key_event = egui_view_dispatch_key_event,
                        .on_key_event = egui_view_on_key_event,
#endif
                },
        .draw_data = egui_view_chart_scatter_draw_data,
};

// ============== Init / Params ==============

void egui_view_chart_scatter_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_chart_scatter_t);
    // call super init (initializes axis base with defaults)
    egui_view_chart_axis_init(self);
    // update api
    self->api = (const egui_view_api_t *)&EGUI_VIEW_API_TABLE_NAME(egui_view_chart_scatter_t);

    // scatter chart defaults
    local->point_radius = 3;
    local->axis_base.clip_margin = local->point_radius; // FIX Bug #4

    egui_view_set_view_name(self, "egui_view_chart_scatter");
}

void egui_view_chart_scatter_apply_params(egui_view_t *self, const egui_view_chart_scatter_params_t *params)
{
    self->region = params->region;
    egui_view_invalidate(self);
}

void egui_view_chart_scatter_init_with_params(egui_view_t *self, const egui_view_chart_scatter_params_t *params)
{
    egui_view_chart_scatter_init(self);
    egui_view_chart_scatter_apply_params(self, params);
}

// ============== Scatter-Specific Setter ==============

void egui_view_chart_scatter_set_point_radius(egui_view_t *self, uint8_t radius)
{
    EGUI_LOCAL_INIT(egui_view_chart_scatter_t);
    local->point_radius = radius;
    local->axis_base.clip_margin = radius;
    egui_view_invalidate(self);
}

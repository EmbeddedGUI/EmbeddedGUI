#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_view_chart_scatter.h"
#include "core/egui_canvas_gradient.h"
#include "font/egui_font_std.h"
#include "resource/egui_resource.h"

// ============== Scatter Drawing (virtual) ==============

__EGUI_STATIC_INLINE__ egui_dim_t egui_view_chart_scatter_map_x_fast(int16_t data_x, egui_dim_t plot_x, int32_t plot_w_span, int16_t view_x_min,
                                                                     int32_t range_x)
{
    if (range_x <= 0 || plot_w_span <= 0)
    {
        return plot_x;
    }

    return plot_x + (egui_dim_t)(((int32_t)data_x - (int32_t)view_x_min) * plot_w_span / range_x);
}

__EGUI_STATIC_INLINE__ egui_dim_t egui_view_chart_scatter_map_y_fast(int16_t data_y, egui_dim_t plot_y, int32_t plot_h_span, int16_t view_y_min,
                                                                     int32_t range_y)
{
    if (range_y <= 0 || plot_h_span <= 0)
    {
        return plot_y;
    }

    return plot_y + plot_h_span - (egui_dim_t)(((int32_t)data_y - (int32_t)view_y_min) * plot_h_span / range_y);
}

static int egui_view_chart_scatter_get_visible_data_range(const egui_region_t *plot_area, egui_dim_t work_x1, egui_dim_t work_y1, egui_dim_t work_x2,
                                                          egui_dim_t work_y2, int16_t view_x_min, int16_t view_x_max, int16_t view_y_min, int16_t view_y_max,
                                                          int16_t *out_x_min, int16_t *out_x_max, int16_t *out_y_min, int16_t *out_y_max)
{
    int32_t range_x;
    int32_t range_y;
    int32_t rel_x_min;
    int32_t rel_x_max;
    int32_t rel_y_min;
    int32_t rel_y_max;
    int32_t inv_y_min;
    int32_t inv_y_max;
    int32_t data_x_min;
    int32_t data_x_max;
    int32_t data_y_min;
    int32_t data_y_max;

    if (plot_area == NULL || out_x_min == NULL || out_x_max == NULL || out_y_min == NULL || out_y_max == NULL || plot_area->size.width <= 1 ||
        plot_area->size.height <= 1)
    {
        return 0;
    }

    range_x = (int32_t)view_x_max - (int32_t)view_x_min;
    range_y = (int32_t)view_y_max - (int32_t)view_y_min;
    if (range_x <= 0 || range_y <= 0)
    {
        return 0;
    }

    rel_x_min = EGUI_MAX((int32_t)work_x1 - (int32_t)plot_area->location.x, 0);
    rel_x_max = EGUI_MIN((int32_t)work_x2 - (int32_t)plot_area->location.x, (int32_t)plot_area->size.width - 1);
    rel_y_min = EGUI_MAX((int32_t)work_y1 - (int32_t)plot_area->location.y, 0);
    rel_y_max = EGUI_MIN((int32_t)work_y2 - (int32_t)plot_area->location.y, (int32_t)plot_area->size.height - 1);
    if (rel_x_min > rel_x_max || rel_y_min > rel_y_max)
    {
        return 0;
    }

    data_x_min = (int32_t)view_x_min + rel_x_min * range_x / ((int32_t)plot_area->size.width - 1) - 1;
    data_x_max = (int32_t)view_x_min + rel_x_max * range_x / ((int32_t)plot_area->size.width - 1) + 1;

    inv_y_min = ((int32_t)plot_area->size.height - 1) - rel_y_max;
    inv_y_max = ((int32_t)plot_area->size.height - 1) - rel_y_min;
    data_y_min = (int32_t)view_y_min + inv_y_min * range_y / ((int32_t)plot_area->size.height - 1) - 1;
    data_y_max = (int32_t)view_y_min + inv_y_max * range_y / ((int32_t)plot_area->size.height - 1) + 1;

    if (data_x_min < view_x_min)
    {
        data_x_min = view_x_min;
    }
    if (data_x_max > view_x_max)
    {
        data_x_max = view_x_max;
    }
    if (data_y_min < view_y_min)
    {
        data_y_min = view_y_min;
    }
    if (data_y_max > view_y_max)
    {
        data_y_max = view_y_max;
    }

    *out_x_min = (int16_t)data_x_min;
    *out_x_max = (int16_t)data_x_max;
    *out_y_min = (int16_t)data_y_min;
    *out_y_max = (int16_t)data_y_max;
    return 1;
}

static void egui_view_chart_scatter_draw_data(egui_view_t *self, const egui_region_t *plot_area)
{
    EGUI_LOCAL_INIT(egui_view_chart_scatter_t);
    egui_chart_axis_base_t *ab = &local->axis_base.ab;
    egui_region_t *work = egui_canvas_get_base_view_work_region();
    egui_dim_t work_x1;
    egui_dim_t work_y1;
    egui_dim_t work_x2;
    egui_dim_t work_y2;
    egui_dim_t expanded_work_x1;
    egui_dim_t expanded_work_y1;
    egui_dim_t expanded_work_x2;
    egui_dim_t expanded_work_y2;
    egui_dim_t plot_x;
    egui_dim_t plot_y;
    int32_t plot_w_span;
    int32_t plot_h_span;
    int16_t view_x_min;
    int16_t view_x_max;
    int16_t view_y_min;
    int16_t view_y_max;
    int32_t range_x;
    int32_t range_y;
    int16_t data_x_min = 0;
    int16_t data_x_max = 0;
    int16_t data_y_min = 0;
    int16_t data_y_max = 0;
    int has_visible_data_range;
    uint8_t point_radius;

    if (work == NULL || egui_region_is_empty(work))
    {
        return;
    }

    work_x1 = work->location.x;
    work_y1 = work->location.y;
    work_x2 = work->location.x + work->size.width;
    work_y2 = work->location.y + work->size.height;
    point_radius = local->point_radius > 0 ? local->point_radius : 3;
    expanded_work_x1 = work_x1 - point_radius;
    expanded_work_y1 = work_y1 - point_radius;
    expanded_work_x2 = work_x2 + point_radius;
    expanded_work_y2 = work_y2 + point_radius;
    plot_x = plot_area->location.x;
    plot_y = plot_area->location.y;
    plot_w_span = (int32_t)plot_area->size.width - 1;
    plot_h_span = (int32_t)plot_area->size.height - 1;
    egui_chart_get_view_x(ab, &view_x_min, &view_x_max);
    egui_chart_get_view_y(ab, &view_y_min, &view_y_max);
    range_x = (int32_t)view_x_max - (int32_t)view_x_min;
    range_y = (int32_t)view_y_max - (int32_t)view_y_min;

    has_visible_data_range =
            egui_view_chart_scatter_get_visible_data_range(plot_area, expanded_work_x1, expanded_work_y1, expanded_work_x2, expanded_work_y2, view_x_min,
                                                           view_x_max, view_y_min, view_y_max, &data_x_min, &data_x_max, &data_y_min, &data_y_max);
    if (!has_visible_data_range)
    {
        return;
    }

    for (uint8_t s = 0; s < ab->series_count; s++)
    {
        const egui_chart_series_t *series = &ab->series[s];
        uint8_t r = point_radius;
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
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
#endif

        for (uint8_t i = 0; i < series->point_count; i++)
        {
            if (series->points[i].x < data_x_min || series->points[i].x > data_x_max || series->points[i].y < data_y_min || series->points[i].y > data_y_max)
            {
                continue;
            }

            egui_dim_t px = egui_view_chart_scatter_map_x_fast(series->points[i].x, plot_x, plot_w_span, view_x_min, range_x);

            if (px + r < work_x1 || px - r > work_x2)
            {
                continue;
            }

            egui_dim_t py = egui_view_chart_scatter_map_y_fast(series->points[i].y, plot_y, plot_h_span, view_y_min, range_y);

            if (py + r < work_y1 || py - r > work_y2)
            {
                continue;
            }
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
            egui_canvas_draw_circle_fill_gradient(px, py, r, &grad);
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

#include "egui_view_chart_line.h"
#include "core/egui_canvas_gradient.h"

// ============== Line Drawing ==============

static void egui_view_chart_line_draw_data(egui_view_t *self, const egui_region_t *plot_area)
{
    EGUI_LOCAL_INIT(egui_view_chart_line_t);
    egui_chart_axis_base_t *ab = &local->axis_base.ab;
    egui_region_t *work = egui_canvas_get_base_view_work_region();
    egui_dim_t clip_expand = local->point_radius > local->line_width ? local->point_radius : local->line_width;
    egui_dim_t work_x1;
    egui_dim_t work_y1;
    egui_dim_t work_x2;
    egui_dim_t work_y2;

    if (work == NULL || egui_region_is_empty(work))
    {
        return;
    }

    work_x1 = work->location.x - clip_expand;
    work_y1 = work->location.y - clip_expand;
    work_x2 = work->location.x + work->size.width + clip_expand;
    work_y2 = work->location.y + work->size.height + clip_expand;

    for (uint8_t s = 0; s < ab->series_count; s++)
    {
        const egui_chart_series_t *series = &ab->series[s];
        if (series->point_count < 1)
        {
            continue;
        }

        // Handle single-point series: draw marker only
        if (series->point_count == 1)
        {
            if (local->point_radius > 0)
            {
                egui_dim_t px = egui_chart_map_x(ab, series->points[0].x, plot_area->location.x, plot_area->size.width);
                egui_dim_t py = egui_chart_map_y(ab, series->points[0].y, plot_area->location.y, plot_area->size.height);
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
                    egui_canvas_draw_circle_fill_gradient(px, py, local->point_radius, &grad);
                }
#else
                egui_canvas_draw_circle_fill(px, py, local->point_radius, series->color, EGUI_ALPHA_100);
#endif
            }
            continue;
        }

// Build a bounded coordinate array to avoid large stack usage with long series.
#define CHART_LINE_MAX_POLYLINE_POINTS 64
        uint8_t pt_count = series->point_count;
        if (pt_count > CHART_LINE_MAX_POLYLINE_POINTS)
        {
            pt_count = CHART_LINE_MAX_POLYLINE_POINTS;
        }
        egui_dim_t coords[CHART_LINE_MAX_POLYLINE_POINTS * 2];
        for (uint8_t i = 0; i < pt_count; i++)
        {
            coords[i * 2] = egui_chart_map_x(ab, series->points[i].x, plot_area->location.x, plot_area->size.width);
            coords[i * 2 + 1] = egui_chart_map_y(ab, series->points[i].y, plot_area->location.y, plot_area->size.height);
        }
        egui_canvas_draw_polyline(coords, pt_count, local->line_width, series->color, EGUI_ALPHA_100);

        // If there are more points than the buffer can hold, draw the tail incrementally.
        {
            egui_dim_t prev_x = coords[(pt_count - 1) * 2];
            egui_dim_t prev_y = coords[(pt_count - 1) * 2 + 1];
            for (uint8_t i = pt_count; i < series->point_count; i++)
            {
                egui_dim_t x2 = egui_chart_map_x(ab, series->points[i].x, plot_area->location.x, plot_area->size.width);
                egui_dim_t y2 = egui_chart_map_y(ab, series->points[i].y, plot_area->location.y, plot_area->size.height);
                egui_canvas_draw_line(prev_x, prev_y, x2, y2, local->line_width, series->color, EGUI_ALPHA_100);
                prev_x = x2;
                prev_y = y2;
            }
        }

        // Draw data point markers
        if (local->point_radius > 0)
        {
            for (uint8_t i = 0; i < series->point_count; i++)
            {
                egui_dim_t px = egui_chart_map_x(ab, series->points[i].x, plot_area->location.x, plot_area->size.width);
                egui_dim_t py = egui_chart_map_y(ab, series->points[i].y, plot_area->location.y, plot_area->size.height);

                if (px + local->point_radius < work_x1 || px - local->point_radius > work_x2 || py + local->point_radius < work_y1 ||
                    py - local->point_radius > work_y2)
                {
                    continue;
                }
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
                    egui_canvas_draw_circle_fill_gradient(px, py, local->point_radius, &grad);
                }
#else
                egui_canvas_draw_circle_fill(px, py, local->point_radius, series->color, EGUI_ALPHA_100);
#endif
            }
        }
    }
}

// ============== API Table ==============

const egui_view_chart_axis_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_chart_line_t) = {
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
        .draw_data = egui_view_chart_line_draw_data,
};

// ============== Init / Params ==============

void egui_view_chart_line_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_chart_line_t);
    // call base class init
    egui_view_chart_axis_init(self);
    // update api
    self->api = (const egui_view_api_t *)&EGUI_VIEW_API_TABLE_NAME(egui_view_chart_line_t);

    // line chart defaults
    local->line_width = 2;
    local->point_radius = 3;
    local->axis_base.clip_margin = local->point_radius + local->line_width;

    egui_view_set_view_name(self, "egui_view_chart_line");
}

void egui_view_chart_line_apply_params(egui_view_t *self, const egui_view_chart_line_params_t *params)
{
    self->region = params->region;
    egui_view_invalidate(self);
}

void egui_view_chart_line_init_with_params(egui_view_t *self, const egui_view_chart_line_params_t *params)
{
    egui_view_chart_line_init(self);
    egui_view_chart_line_apply_params(self, params);
}

// ============== Setters ==============

void egui_view_chart_line_set_line_width(egui_view_t *self, uint8_t width)
{
    EGUI_LOCAL_INIT(egui_view_chart_line_t);
    local->line_width = width;
    local->axis_base.clip_margin = local->point_radius + local->line_width;
    egui_view_invalidate(self);
}

void egui_view_chart_line_set_point_radius(egui_view_t *self, uint8_t radius)
{
    EGUI_LOCAL_INIT(egui_view_chart_line_t);
    local->point_radius = radius;
    local->axis_base.clip_margin = local->point_radius + local->line_width;
    egui_view_invalidate(self);
}

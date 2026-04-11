#include "egui_view_chart_line.h"
#include "core/egui_canvas_gradient.h"

// ============== Line Drawing ==============

static int egui_view_chart_line_series_is_monotonic_increasing(const egui_chart_series_t *series)
{
    if (series == NULL || series->point_count < 2)
    {
        return 1;
    }

    for (uint8_t i = 1; i < series->point_count; i++)
    {
        if (series->points[i].x < series->points[i - 1].x)
        {
            return 0;
        }
    }

    return 1;
}

static uint8_t egui_view_chart_line_series_lower_bound_x(const egui_chart_series_t *series, int16_t target_x)
{
    uint8_t left = 0;
    uint8_t right;

    if (series == NULL)
    {
        return 0;
    }

    right = series->point_count;
    while (left < right)
    {
        uint8_t mid = (uint8_t)(left + (uint8_t)((right - left) >> 1));
        if (series->points[mid].x < target_x)
        {
            left = (uint8_t)(mid + 1);
        }
        else
        {
            right = mid;
        }
    }

    return left;
}

static uint8_t egui_view_chart_line_series_upper_bound_x(const egui_chart_series_t *series, int16_t target_x)
{
    uint8_t left = 0;
    uint8_t right;

    if (series == NULL)
    {
        return 0;
    }

    right = series->point_count;
    while (left < right)
    {
        uint8_t mid = (uint8_t)(left + (uint8_t)((right - left) >> 1));
        if (series->points[mid].x <= target_x)
        {
            left = (uint8_t)(mid + 1);
        }
        else
        {
            right = mid;
        }
    }

    return left;
}

__EGUI_STATIC_INLINE__ egui_dim_t egui_view_chart_line_map_x_fast(int16_t data_x, egui_dim_t plot_x, int32_t plot_w_span, int16_t view_x_min, int32_t range_x)
{
    if (range_x <= 0 || plot_w_span <= 0)
    {
        return plot_x;
    }

    return plot_x + (egui_dim_t)(((int32_t)data_x - (int32_t)view_x_min) * plot_w_span / range_x);
}

__EGUI_STATIC_INLINE__ egui_dim_t egui_view_chart_line_map_y_fast(int16_t data_y, egui_dim_t plot_y, int32_t plot_h_span, int16_t view_y_min, int32_t range_y)
{
    if (range_y <= 0 || plot_h_span <= 0)
    {
        return plot_y;
    }

    return plot_y + plot_h_span - (egui_dim_t)(((int32_t)data_y - (int32_t)view_y_min) * plot_h_span / range_y);
}

static int egui_view_chart_line_get_visible_index_range(const egui_chart_series_t *series, const egui_region_t *plot_area, egui_dim_t work_x1,
                                                        egui_dim_t work_x2, int16_t view_x_min, int16_t view_x_max, uint8_t *draw_start, uint8_t *draw_end,
                                                        uint8_t *marker_start, uint8_t *marker_end)
{
    int32_t range;
    int32_t rel_min_px;
    int32_t rel_max_px;
    int32_t data_min_x;
    int32_t data_max_x;
    uint8_t first_idx;
    uint8_t last_exclusive;

    if (series == NULL || plot_area == NULL || draw_start == NULL || draw_end == NULL || marker_start == NULL || marker_end == NULL || series->point_count == 0)
    {
        return 0;
    }

    if (series->point_count == 1 || plot_area->size.width <= 1)
    {
        *draw_start = 0;
        *draw_end = (uint8_t)(series->point_count - 1);
        *marker_start = 0;
        *marker_end = (uint8_t)(series->point_count - 1);
        return 1;
    }

    range = (int32_t)view_x_max - (int32_t)view_x_min;
    if (range <= 0)
    {
        *draw_start = 0;
        *draw_end = (uint8_t)(series->point_count - 1);
        *marker_start = 0;
        *marker_end = (uint8_t)(series->point_count - 1);
        return 1;
    }

    rel_min_px = (int32_t)work_x1 - (int32_t)plot_area->location.x;
    rel_max_px = (int32_t)work_x2 - (int32_t)plot_area->location.x;
    data_min_x = (int32_t)view_x_min + rel_min_px * range / (int32_t)(plot_area->size.width - 1) - 1;
    data_max_x = (int32_t)view_x_min + rel_max_px * range / (int32_t)(plot_area->size.width - 1) + 1;

    if (data_min_x < INT16_MIN)
    {
        data_min_x = INT16_MIN;
    }
    if (data_max_x > INT16_MAX)
    {
        data_max_x = INT16_MAX;
    }

    first_idx = egui_view_chart_line_series_lower_bound_x(series, (int16_t)data_min_x);
    last_exclusive = egui_view_chart_line_series_upper_bound_x(series, (int16_t)data_max_x);

    if (first_idx >= series->point_count || last_exclusive == 0)
    {
        return 0;
    }

    *draw_start = (first_idx > 0) ? (uint8_t)(first_idx - 1) : 0;
    *draw_end = (last_exclusive < series->point_count) ? last_exclusive : (uint8_t)(series->point_count - 1);

    if (first_idx < last_exclusive)
    {
        *marker_start = first_idx;
        *marker_end = (uint8_t)(last_exclusive - 1);
    }
    else
    {
        *marker_start = 1;
        *marker_end = 0;
    }

    return (*draw_start <= *draw_end) ? 1 : 0;
}

static int egui_view_chart_line_get_visible_data_y_range(const egui_region_t *plot_area, egui_dim_t work_y1, egui_dim_t work_y2, int16_t view_y_min,
                                                         int16_t view_y_max, int16_t *out_min, int16_t *out_max)
{
    int32_t range_y;
    int32_t rel_y_min;
    int32_t rel_y_max;
    int32_t inv_y_min;
    int32_t inv_y_max;
    int32_t data_y_min;
    int32_t data_y_max;

    if (plot_area == NULL || out_min == NULL || out_max == NULL || plot_area->size.height <= 1)
    {
        return 0;
    }

    range_y = (int32_t)view_y_max - (int32_t)view_y_min;
    if (range_y <= 0)
    {
        return 0;
    }

    rel_y_min = EGUI_MAX((int32_t)work_y1 - (int32_t)plot_area->location.y, 0);
    rel_y_max = EGUI_MIN((int32_t)work_y2 - (int32_t)plot_area->location.y, (int32_t)plot_area->size.height - 1);
    if (rel_y_min > rel_y_max)
    {
        return 0;
    }

    inv_y_min = ((int32_t)plot_area->size.height - 1) - rel_y_max;
    inv_y_max = ((int32_t)plot_area->size.height - 1) - rel_y_min;
    data_y_min = (int32_t)view_y_min + inv_y_min * range_y / ((int32_t)plot_area->size.height - 1) - 1;
    data_y_max = (int32_t)view_y_min + inv_y_max * range_y / ((int32_t)plot_area->size.height - 1) + 1;

    if (data_y_min < view_y_min)
    {
        data_y_min = view_y_min;
    }
    if (data_y_max > view_y_max)
    {
        data_y_max = view_y_max;
    }

    *out_min = (int16_t)data_y_min;
    *out_max = (int16_t)data_y_max;
    return 1;
}

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
    int16_t visible_data_y_min = 0;
    int16_t visible_data_y_max = 0;
    int has_visible_data_y_range;

    if (work == NULL || egui_region_is_empty(work))
    {
        return;
    }

    work_x1 = work->location.x - clip_expand;
    work_y1 = work->location.y - clip_expand;
    work_x2 = work->location.x + work->size.width + clip_expand;
    work_y2 = work->location.y + work->size.height + clip_expand;
    plot_x = plot_area->location.x;
    plot_y = plot_area->location.y;
    plot_w_span = (int32_t)plot_area->size.width - 1;
    plot_h_span = (int32_t)plot_area->size.height - 1;
    egui_chart_get_view_x(ab, &view_x_min, &view_x_max);
    egui_chart_get_view_y(ab, &view_y_min, &view_y_max);
    range_x = (int32_t)view_x_max - (int32_t)view_x_min;
    range_y = (int32_t)view_y_max - (int32_t)view_y_min;
    has_visible_data_y_range =
            egui_view_chart_line_get_visible_data_y_range(plot_area, work_y1, work_y2, view_y_min, view_y_max, &visible_data_y_min, &visible_data_y_max);

    for (uint8_t s = 0; s < ab->series_count; s++)
    {
        const egui_chart_series_t *series = &ab->series[s];
        int monotonic_increasing = egui_view_chart_line_series_is_monotonic_increasing(series);
        uint8_t draw_start = 0;
        uint8_t draw_end = 0;
        uint8_t marker_start = 0;
        uint8_t marker_end = 0;
        uint8_t has_visible_range = 0;
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
        if (series->point_count < 1)
        {
            continue;
        }

        if (monotonic_increasing)
        {
            has_visible_range = (uint8_t)egui_view_chart_line_get_visible_index_range(series, plot_area, work_x1, work_x2, view_x_min, view_x_max, &draw_start,
                                                                                      &draw_end, &marker_start, &marker_end);
            if (!has_visible_range)
            {
                continue;
            }
        }

        // Handle single-point series: draw marker only
        if (series->point_count == 1)
        {
            if (local->point_radius > 0)
            {
                egui_dim_t px = egui_view_chart_line_map_x_fast(series->points[0].x, plot_x, plot_w_span, view_x_min, range_x);
                egui_dim_t py = egui_view_chart_line_map_y_fast(series->points[0].y, plot_y, plot_h_span, view_y_min, range_y);
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
                egui_canvas_draw_circle_fill_gradient(px, py, local->point_radius, &grad);
#else
                egui_canvas_draw_circle_fill(px, py, local->point_radius, series->color, EGUI_ALPHA_100);
#endif
            }
            continue;
        }

// Build a bounded coordinate array to avoid large stack usage with long series.
#define CHART_LINE_MAX_POLYLINE_POINTS 64
        uint8_t point_start = monotonic_increasing ? draw_start : 0;
        uint8_t point_end = monotonic_increasing ? draw_end : (uint8_t)(series->point_count - 1);
        uint8_t draw_point_count = (point_end >= point_start) ? (uint8_t)(point_end - point_start + 1) : 0;
        uint8_t pt_count = draw_point_count;

        if (draw_point_count >= 2)
        {
            if (pt_count > CHART_LINE_MAX_POLYLINE_POINTS)
            {
                pt_count = CHART_LINE_MAX_POLYLINE_POINTS;
            }
            egui_dim_t coords[CHART_LINE_MAX_POLYLINE_POINTS * 2];
            for (uint8_t i = 0; i < pt_count; i++)
            {
                const egui_chart_point_t *point = &series->points[point_start + i];
                coords[i * 2] = egui_view_chart_line_map_x_fast(point->x, plot_x, plot_w_span, view_x_min, range_x);
                coords[i * 2 + 1] = egui_view_chart_line_map_y_fast(point->y, plot_y, plot_h_span, view_y_min, range_y);
            }
            egui_canvas_draw_polyline(coords, pt_count, local->line_width, series->color, EGUI_ALPHA_100);

            // If there are more points than the buffer can hold, draw the tail incrementally.
            {
                egui_dim_t prev_x = coords[(pt_count - 1) * 2];
                egui_dim_t prev_y = coords[(pt_count - 1) * 2 + 1];
                for (uint8_t i = pt_count; i < draw_point_count; i++)
                {
                    const egui_chart_point_t *point = &series->points[point_start + i];
                    egui_dim_t x2 = egui_view_chart_line_map_x_fast(point->x, plot_x, plot_w_span, view_x_min, range_x);
                    egui_dim_t y2 = egui_view_chart_line_map_y_fast(point->y, plot_y, plot_h_span, view_y_min, range_y);
                    egui_canvas_draw_line(prev_x, prev_y, x2, y2, local->line_width, series->color, EGUI_ALPHA_100);
                    prev_x = x2;
                    prev_y = y2;
                }
            }
        }

        // Draw data point markers
        if (local->point_radius > 0)
        {
            uint8_t marker_loop_start = 0;
            uint8_t marker_loop_end = (uint8_t)(series->point_count - 1);

            if (monotonic_increasing && has_visible_range)
            {
                if (marker_start > marker_end)
                {
                    continue;
                }
                marker_loop_start = marker_start;
                marker_loop_end = marker_end;
            }

            for (uint16_t i = marker_loop_start; i <= marker_loop_end; i++)
            {
                if (has_visible_data_y_range &&
                    (series->points[i].y < visible_data_y_min || series->points[i].y > visible_data_y_max))
                {
                    continue;
                }

                egui_dim_t px = egui_view_chart_line_map_x_fast(series->points[i].x, plot_x, plot_w_span, view_x_min, range_x);

                if (monotonic_increasing)
                {
                    if (px + local->point_radius < work_x1)
                    {
                        continue;
                    }
                    if (px - local->point_radius > work_x2)
                    {
                        break;
                    }
                }

                egui_dim_t py = egui_view_chart_line_map_y_fast(series->points[i].y, plot_y, plot_h_span, view_y_min, range_y);

                if (px + local->point_radius < work_x1 || px - local->point_radius > work_x2 || py + local->point_radius < work_y1 ||
                    py - local->point_radius > work_y2)
                {
                    continue;
                }
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
                egui_canvas_draw_circle_fill_gradient(px, py, local->point_radius, &grad);
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

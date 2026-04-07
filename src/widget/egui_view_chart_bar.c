#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_view_chart_bar.h"
#include "core/egui_canvas_gradient.h"
#include "font/egui_font_std.h"
#include "resource/egui_resource.h"

// ============== Bar Drawing (virtual) ==============

static void egui_view_chart_bar_draw_data(egui_view_t *self, const egui_region_t *plot_area)
{
    EGUI_LOCAL_INIT(egui_view_chart_bar_t);
    egui_chart_axis_base_t *ab = &local->axis_base.ab;

    if (ab->series_count == 0 || ab->series[0].point_count == 0)
    {
        return;
    }

    uint8_t n_points = ab->series[0].point_count;
    uint8_t n_series = ab->series_count;

    // Calculate bar dimensions (Bug #5 fix: clamp effective gap)
    egui_dim_t group_width = plot_area->size.width / n_points;
    egui_dim_t max_gap = (group_width > (n_series + 1)) ? (group_width / (n_series + 1) - 1) : 0;
    uint8_t effective_gap = local->bar_gap;
    if (effective_gap > (uint8_t)max_gap)
    {
        effective_gap = (uint8_t)max_gap;
    }
    egui_dim_t total_gap = (egui_dim_t)effective_gap * (n_series + 1);
    egui_dim_t bar_width = (group_width - total_gap) / n_series;
    if (bar_width < 1)
    {
        bar_width = 1;
    }

    // Baseline Y (where value == 0)
    egui_dim_t baseline_y = egui_chart_map_y(ab, 0, plot_area->location.y, plot_area->size.height);
    if (baseline_y < plot_area->location.y)
    {
        baseline_y = plot_area->location.y;
    }
    if (baseline_y > plot_area->location.y + plot_area->size.height)
    {
        baseline_y = plot_area->location.y + plot_area->size.height;
    }

    for (uint8_t i = 0; i < n_points; i++)
    {
        egui_dim_t group_x = plot_area->location.x + group_width * i;

        for (uint8_t s = 0; s < n_series; s++)
        {
            if (i >= ab->series[s].point_count)
            {
                continue;
            }

            egui_dim_t bar_x = group_x + effective_gap + s * (bar_width + effective_gap);
            egui_dim_t val_y = egui_chart_map_y(ab, ab->series[s].points[i].y, plot_area->location.y, plot_area->size.height);

            egui_dim_t bar_top, bar_h;
            if (val_y < baseline_y)
            {
                bar_top = val_y;
                bar_h = baseline_y - val_y;
            }
            else
            {
                bar_top = baseline_y;
                bar_h = val_y - baseline_y;
            }
            // Bug #6 fix: skip zero-value bars instead of forcing height=1
            if (bar_h < 1)
            {
                continue;
            }

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
            {
                egui_color_t color_light = egui_rgb_mix(ab->series[s].color, EGUI_COLOR_WHITE, 80);
                egui_gradient_stop_t stops[2] = {
                        {.position = 0, .color = color_light},
                        {.position = 255, .color = ab->series[s].color},
                };
                egui_gradient_t grad = {
                        .type = EGUI_GRADIENT_TYPE_LINEAR_VERTICAL,
                        .stop_count = 2,
                        .alpha = EGUI_ALPHA_100,
                        .stops = stops,
                };
                egui_canvas_draw_rectangle_fill_gradient(bar_x, bar_top, bar_width, bar_h, &grad);
            }
#else
            egui_canvas_draw_rectangle_fill(bar_x, bar_top, bar_width, bar_h, ab->series[s].color, EGUI_ALPHA_100);
#endif
        }
    }
}

// ============== API Table ==============

const egui_view_chart_axis_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_chart_bar_t) = {
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
        .draw_data = egui_view_chart_bar_draw_data,
};

// ============== Init / Params ==============

void egui_view_chart_bar_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_chart_bar_t);
    egui_view_chart_axis_init(self);
    self->api = (const egui_view_api_t *)&EGUI_VIEW_API_TABLE_NAME(egui_view_chart_bar_t);

    // bar chart defaults
    local->bar_gap = 2;
    local->axis_base.clip_margin = local->bar_gap + 2;
    egui_chart_axis_base_set_axis_x_categorical(&local->axis_base.ab, 1);

    egui_view_set_view_name(self, "egui_view_chart_bar");
}

void egui_view_chart_bar_apply_params(egui_view_t *self, const egui_view_chart_bar_params_t *params)
{
    egui_view_set_position(self, params->region.location.x, params->region.location.y);
    egui_view_set_size(self, params->region.size.width, params->region.size.height);
}

void egui_view_chart_bar_init_with_params(egui_view_t *self, const egui_view_chart_bar_params_t *params)
{
    egui_view_chart_bar_init(self);
    egui_view_chart_bar_apply_params(self, params);
}

// ============== Setters ==============

void egui_view_chart_bar_set_bar_gap(egui_view_t *self, uint8_t gap)
{
    EGUI_LOCAL_INIT(egui_view_chart_bar_t);
    local->bar_gap = gap;
    local->axis_base.clip_margin = gap + 2;
    egui_view_invalidate(self);
}

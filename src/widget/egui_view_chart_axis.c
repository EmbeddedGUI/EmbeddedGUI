#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>

#include "egui_view_chart_axis.h"
#include "font/egui_font_std.h"
#include "resource/egui_resource.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
#include "egui_view_group.h"
#endif

// ============== Zoom / Touch Handling ==============

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH

int egui_view_chart_axis_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_chart_axis_t);

    return egui_chart_axis_on_touch_event(self, &local->ab, event);
}

#endif // EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH

// ============== Unified on_draw ==============

void egui_view_chart_axis_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_chart_axis_t);

    egui_region_t region;
    egui_view_get_work_region(self, &region);
    if (region.size.width < 10 || region.size.height < 10)
    {
        return;
    }

    // 1. Draw background
    egui_canvas_draw_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, local->ab.bg_color, EGUI_ALPHA_100);

    // 2. Calculate plot area (in view-local coordinates)
    egui_region_t plot_area;
    egui_chart_calc_plot_area(&local->ab, &region, &plot_area);

    // 3. Draw axes (in view-local coordinate system)
    egui_chart_draw_axes(&local->ab, &region, &plot_area);

    // 4. Set clip region for data drawing
    //    Inset by 1px from axes so data never overlaps axis lines.
    //    egui_canvas_calc_work_region changes the coordinate system to clip-local.
    egui_region_t clip;
    clip.location.x = plot_area.location.x + 1 + self->region_screen.location.x;
    clip.location.y = plot_area.location.y + 1 + self->region_screen.location.y;
    clip.size.width = plot_area.size.width - 2;
    clip.size.height = plot_area.size.height - 2;
    egui_canvas_calc_work_region(&clip);

    // 5. Call draw_data virtual function (with clip-local coordinates)
    if (!egui_region_is_empty(egui_canvas_get_base_view_work_region()))
    {
        const egui_view_chart_axis_api_t *api = (const egui_view_chart_axis_api_t *)self->api;
        if (api->draw_data)
        {
            // Convert plot_area to clip-local coordinates
            egui_region_t clip_plot_area;
            clip_plot_area.location.x = 0;
            clip_plot_area.location.y = 0;
            clip_plot_area.size.width = plot_area.size.width - 2;
            clip_plot_area.size.height = plot_area.size.height - 2;
            api->draw_data(self, &clip_plot_area);
        }
    }

    // 6. Restore clip
    egui_canvas_calc_work_region(&self->region_screen);

    // 7. Draw legend
    if (local->ab.legend_pos != EGUI_CHART_LEGEND_NONE)
    {
        egui_chart_draw_legend_series(&local->ab, &region, &plot_area);
    }
}

// ============== API Table ==============

const egui_view_chart_axis_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_chart_axis_t) = {
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
        .draw_data = NULL,
};

// ============== Init ==============

void egui_view_chart_axis_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_chart_axis_t);

    // Initialize base view
    egui_view_init(self);
    self->api = (const egui_view_api_t *)&EGUI_VIEW_API_TABLE_NAME(egui_view_chart_axis_t);

    // Initialize axis base with defaults
    egui_chart_axis_base_init_defaults(&local->ab);

    // clip margin
    local->clip_margin = 0;

    egui_view_set_view_name(self, "egui_view_chart_axis");
}

// ============== Apply Params ==============

void egui_view_chart_axis_apply_params(egui_view_t *self, const egui_view_chart_axis_params_t *params)
{
    egui_view_set_position(self, params->region.location.x, params->region.location.y);
    egui_view_set_size(self, params->region.size.width, params->region.size.height);
}

// ============== Unified Setters ==============

void egui_view_chart_axis_set_series(egui_view_t *self, const egui_chart_series_t *series, uint8_t count)
{
    EGUI_LOCAL_INIT(egui_view_chart_axis_t);
    local->ab.series = series;
    local->ab.series_count = count;
    egui_view_invalidate(self);
}

void egui_view_chart_axis_set_axis_x(egui_view_t *self, int16_t min_val, int16_t max_val, int16_t tick_step)
{
    EGUI_LOCAL_INIT(egui_view_chart_axis_t);
    local->ab.axis_x.min_value = min_val;
    local->ab.axis_x.max_value = max_val;
    local->ab.axis_x.tick_step = tick_step;
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    local->ab.view_x_min = min_val;
    local->ab.view_x_max = max_val;
#endif
    egui_view_invalidate(self);
}

void egui_view_chart_axis_set_axis_y(egui_view_t *self, int16_t min_val, int16_t max_val, int16_t tick_step)
{
    EGUI_LOCAL_INIT(egui_view_chart_axis_t);
    local->ab.axis_y.min_value = min_val;
    local->ab.axis_y.max_value = max_val;
    local->ab.axis_y.tick_step = tick_step;
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    local->ab.view_y_min = min_val;
    local->ab.view_y_max = max_val;
#endif
    egui_view_invalidate(self);
}

void egui_view_chart_axis_set_axis_x_config(egui_view_t *self, const egui_chart_axis_config_t *config)
{
    EGUI_LOCAL_INIT(egui_view_chart_axis_t);
    local->ab.axis_x = *config;
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    local->ab.view_x_min = config->min_value;
    local->ab.view_x_max = config->max_value;
#endif
    egui_view_invalidate(self);
}

void egui_view_chart_axis_set_axis_y_config(egui_view_t *self, const egui_chart_axis_config_t *config)
{
    EGUI_LOCAL_INIT(egui_view_chart_axis_t);
    local->ab.axis_y = *config;
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    local->ab.view_y_min = config->min_value;
    local->ab.view_y_max = config->max_value;
#endif
    egui_view_invalidate(self);
}

void egui_view_chart_axis_set_legend_pos(egui_view_t *self, uint8_t pos)
{
    EGUI_LOCAL_INIT(egui_view_chart_axis_t);
    local->ab.legend_pos = pos;
    egui_view_invalidate(self);
}

void egui_view_chart_axis_set_colors(egui_view_t *self, egui_color_t bg, egui_color_t axis, egui_color_t grid, egui_color_t text)
{
    EGUI_LOCAL_INIT(egui_view_chart_axis_t);
    local->ab.bg_color = bg;
    local->ab.axis_color = axis;
    local->ab.grid_color = grid;
    local->ab.text_color = text;
    egui_view_invalidate(self);
}

void egui_view_chart_axis_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_chart_axis_t);
    local->ab.font = font;
    egui_view_invalidate(self);
}

void egui_view_chart_axis_set_clip_margin(egui_view_t *self, egui_dim_t margin)
{
    EGUI_LOCAL_INIT(egui_view_chart_axis_t);
    local->clip_margin = margin;
    egui_view_invalidate(self);
}

// ============== Unified Zoom API ==============

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH

void egui_view_chart_axis_set_zoom_enabled(egui_view_t *self, uint8_t enabled)
{
    EGUI_LOCAL_INIT(egui_view_chart_axis_t);
    local->ab.zoom_enabled = enabled;
    if (!enabled)
    {
        local->ab.view_x_min = local->ab.axis_x.min_value;
        local->ab.view_x_max = local->ab.axis_x.max_value;
        local->ab.view_y_min = local->ab.axis_y.min_value;
        local->ab.view_y_max = local->ab.axis_y.max_value;
    }
    egui_view_invalidate(self);
}

void egui_view_chart_axis_zoom_in(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_chart_axis_t);
    if (!local->ab.zoom_enabled)
    {
        return;
    }
    egui_chart_apply_zoom(&local->ab, 1);
    egui_view_invalidate(self);
}

void egui_view_chart_axis_zoom_out(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_chart_axis_t);
    if (!local->ab.zoom_enabled)
    {
        return;
    }
    egui_chart_apply_zoom(&local->ab, 0);
    egui_view_invalidate(self);
}

void egui_view_chart_axis_zoom_reset(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_chart_axis_t);
    local->ab.view_x_min = local->ab.axis_x.min_value;
    local->ab.view_x_max = local->ab.axis_x.max_value;
    local->ab.view_y_min = local->ab.axis_y.min_value;
    local->ab.view_y_max = local->ab.axis_y.max_value;
    egui_view_invalidate(self);
}

void egui_view_chart_axis_zoom_in_x(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_chart_axis_t);
    if (!local->ab.zoom_enabled)
    {
        return;
    }
    egui_chart_apply_zoom_axis(&local->ab, 1, 1, 0);
    egui_view_invalidate(self);
}

void egui_view_chart_axis_zoom_out_x(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_chart_axis_t);
    if (!local->ab.zoom_enabled)
    {
        return;
    }
    egui_chart_apply_zoom_axis(&local->ab, 0, 1, 0);
    egui_view_invalidate(self);
}

void egui_view_chart_axis_zoom_reset_x(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_chart_axis_t);
    local->ab.view_x_min = local->ab.axis_x.min_value;
    local->ab.view_x_max = local->ab.axis_x.max_value;
    egui_view_invalidate(self);
}

void egui_view_chart_axis_zoom_in_y(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_chart_axis_t);
    if (!local->ab.zoom_enabled)
    {
        return;
    }
    egui_chart_apply_zoom_axis(&local->ab, 1, 0, 1);
    egui_view_invalidate(self);
}

void egui_view_chart_axis_zoom_out_y(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_chart_axis_t);
    if (!local->ab.zoom_enabled)
    {
        return;
    }
    egui_chart_apply_zoom_axis(&local->ab, 0, 0, 1);
    egui_view_invalidate(self);
}

void egui_view_chart_axis_zoom_reset_y(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_chart_axis_t);
    local->ab.view_y_min = local->ab.axis_y.min_value;
    local->ab.view_y_max = local->ab.axis_y.max_value;
    egui_view_invalidate(self);
}

#endif // EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH

#include <stdio.h>
#include <assert.h>

#include "egui_view_line.h"

void egui_view_line_set_points(egui_view_t *self, const egui_view_line_point_t *points, uint8_t count)
{
    EGUI_LOCAL_INIT(egui_view_line_t);
    local->points = points;
    local->point_count = count;
    egui_view_invalidate(self);
}

void egui_view_line_set_line_width(egui_view_t *self, uint8_t width)
{
    EGUI_LOCAL_INIT(egui_view_line_t);
    if (local->line_width == width)
    {
        return;
    }
    local->line_width = width;
    egui_view_invalidate(self);
}

void egui_view_line_set_line_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_line_t);
    if (local->line_color.full == color.full)
    {
        return;
    }
    local->line_color = color;
    egui_view_invalidate(self);
}

void egui_view_line_set_use_round_cap(egui_view_t *self, uint8_t enable)
{
    EGUI_LOCAL_INIT(egui_view_line_t);
    if (local->use_round_cap == enable)
    {
        return;
    }
    local->use_round_cap = enable;
    egui_view_invalidate(self);
}

void egui_view_line_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_line_t);

    if (local->points == NULL || local->point_count < 2)
    {
        return;
    }

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    egui_dim_t ox = region.location.x;
    egui_dim_t oy = region.location.y;

    uint8_t i;
    for (i = 0; i < local->point_count - 1; i++)
    {
        egui_dim_t x1 = ox + local->points[i].x;
        egui_dim_t y1 = oy + local->points[i].y;
        egui_dim_t x2 = ox + local->points[i + 1].x;
        egui_dim_t y2 = oy + local->points[i + 1].y;

        if (local->use_round_cap)
        {
            egui_canvas_draw_line_round_cap_hq(x1, y1, x2, y2, local->line_width, local->line_color, EGUI_ALPHA_100);
        }
        else
        {
            egui_canvas_draw_line(x1, y1, x2, y2, local->line_width, local->line_color, EGUI_ALPHA_100);
        }
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_line_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_line_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_line_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_line_t);
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_line_t);

    // init local data.
    local->points = NULL;
    local->point_count = 0;
    local->line_width = EGUI_THEME_STROKE_WIDTH;
    local->use_round_cap = 0;
    local->line_color = EGUI_THEME_PRIMARY;

    egui_view_set_view_name(self, "egui_view_line");
}

void egui_view_line_apply_params(egui_view_t *self, const egui_view_line_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_line_t);

    self->region = params->region;

    local->line_width = params->line_width;
    local->line_color = params->line_color;

    egui_view_invalidate(self);
}

void egui_view_line_init_with_params(egui_view_t *self, const egui_view_line_params_t *params)
{
    egui_view_line_init(self);
    egui_view_line_apply_params(self, params);
}

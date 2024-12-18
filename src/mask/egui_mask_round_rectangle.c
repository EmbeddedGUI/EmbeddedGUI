#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_mask_round_rectangle.h"
#include "core/egui_common.h"
#include "core/egui_api.h"
#include "core/egui_canvas.h"

void egui_mask_round_rectangle_set_radius(egui_mask_t *self, egui_dim_t radius)
{
    egui_mask_round_rectangle_t *local = (egui_mask_round_rectangle_t *)self;
    local->radius = radius;
}

void egui_mask_round_rectangle_mask_point(egui_mask_t *self, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha)
{
    egui_mask_round_rectangle_t *local = (egui_mask_round_rectangle_t *)self;
    egui_dim_t radius = local->radius;
    egui_dim_t width = self->region.size.width;
    egui_dim_t height = self->region.size.height;
    egui_dim_t sel_x = self->region.location.x;
    egui_dim_t sel_y = self->region.location.y;

    if (egui_region_pt_in_rect(&self->region, x, y))
    {
        egui_region_t region;
        // check in the middle rectangle.
        egui_region_init(&region, sel_x + radius, sel_y, width - (radius << 1), height);
        if (egui_region_pt_in_rect(&region, x, y))
        {
            return;
        }

        // check in the left and right rectangles.
        egui_region_init(&region, sel_x, sel_y + radius, radius, height - (radius << 1));
        if (egui_region_pt_in_rect(&region, x, y))
        {
            return;
        }

        egui_region_init(&region, sel_x + width - radius, sel_y + radius, radius, height - (radius << 1));
        if (egui_region_pt_in_rect(&region, x, y))
        {
            return;
        }

        // check in the corners.
        if (egui_canvas_get_circle_left_top(sel_x + radius, sel_y + radius, radius, x, y, alpha))
        {
            return;
        }
        if (egui_canvas_get_circle_left_bottom(sel_x + radius, sel_y + height - radius - 1, radius, x, y, alpha))
        {
            return;
        }
        if (egui_canvas_get_circle_right_top(sel_x + width - radius - 1, sel_y + radius, radius, x, y, alpha))
        {
            return;
        }
        if (egui_canvas_get_circle_right_bottom(sel_x + width - radius - 1, sel_y + height - radius - 1, radius, x, y, alpha))
        {
            return;
        }
    }

    // clear value.
    color->full = 0;
    *alpha = 0;
}

// name must be _type##_api_table, it will be used by EGUI_VIEW_DEFINE to init api.
const egui_mask_api_t egui_mask_round_rectangle_t_api_table = {
        .mask_point = egui_mask_round_rectangle_mask_point,
};

void egui_mask_round_rectangle_init(egui_mask_t *self)
{
    egui_mask_round_rectangle_t *local = (egui_mask_round_rectangle_t *)self;
    // call super init.
    egui_mask_init(self);
    // update api.
    self->api = &egui_mask_round_rectangle_t_api_table;

    // init local data.
}

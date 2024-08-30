#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_mask_circle.h"
#include "core/egui_common.h"
#include "core/egui_api.h"
#include "core/egui_canvas.h"

void egui_mask_circle_mask_point(egui_mask_t *self, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha)
{
    egui_mask_circle_t *local = (egui_mask_circle_t *)self;

    if (egui_region_pt_in_rect(&self->region, x, y))
    {
        // Get the radio.
        egui_dim_t radius = EGUI_MIN(self->region.size.width, self->region.size.height);
        radius = (radius >> 1) - 1;

        // Get the center of the circle.
        egui_dim_t center_x = self->region.location.x + (self->region.size.width >> 1);
        egui_dim_t center_y = self->region.location.y + (self->region.size.height >> 1);

        if (egui_canvas_get_circle_left_top(center_x, center_y, radius, x, y, alpha))
        {
            return;
        }
        if (egui_canvas_get_circle_left_bottom(center_x, center_y, radius, x, y, alpha))
        {
            return;
        }
        if (egui_canvas_get_circle_right_top(center_x, center_y, radius, x, y, alpha))
        {
            return;
        }
        if (egui_canvas_get_circle_right_bottom(center_x, center_y, radius, x, y, alpha))
        {
            return;
        }

        egui_region_t region;
        egui_region_init(&region, center_x - radius, center_y, radius << 1, 1);
        if (egui_region_pt_in_rect(&region, x, y))
        {
            return;
        }

        egui_region_init(&region, center_x, center_y - radius, 1, radius << 1);
        if (egui_region_pt_in_rect(&region, x, y))
        {
            return;
        }
    }

    // clear value.
    color->full = 0;
    *alpha = 0;
}

// name must be _type##_api_table, it will be used by EGUI_VIEW_DEFINE to init api.
const egui_mask_api_t egui_mask_circle_t_api_table = {
        .mask_point = egui_mask_circle_mask_point,
};

void egui_mask_circle_init(egui_mask_t *self)
{
    egui_mask_circle_t *local = (egui_mask_circle_t *)self;
    // call super init.
    egui_mask_init(self);
    // update api.
    self->api = &egui_mask_circle_t_api_table;

    // init local data.
}

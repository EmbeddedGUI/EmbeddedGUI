#include <stdio.h>
#include <assert.h>

#include "egui_mask.h"
#include "egui_mask_round_rectangle.h"
#include "core/egui_api.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK

void egui_mask_set_position(egui_mask_t *self, egui_dim_t x, egui_dim_t y)
{
    self->region.location.x = x;
    self->region.location.y = y;
}

void egui_mask_set_size(egui_mask_t *self, egui_dim_t width, egui_dim_t height)
{
    self->region.size.width = width;
    self->region.size.height = height;

    if (self->api != NULL && self->api->kind == EGUI_MASK_KIND_ROUND_RECTANGLE)
    {
        egui_mask_round_rectangle_t *round_rect = (egui_mask_round_rectangle_t *)self;
        egui_dim_t max_radius = EGUI_MIN(width, height) >> 1;

        if (max_radius < 0)
        {
            max_radius = 0;
        }

        if (round_rect->radius > max_radius)
        {
            round_rect->radius = max_radius;
        }
        else if (round_rect->radius < 0)
        {
            round_rect->radius = 0;
        }
    }
}

void egui_mask_mask_point(egui_mask_t *self, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha)
{
    // implement is sub-class.
}

const egui_mask_api_t egui_mask_t_api_table = {
        .kind = EGUI_MASK_KIND_BASE,
        .mask_point = egui_mask_mask_point,
        .mask_get_row_range = NULL,
        .mask_get_row_visible_range = NULL,
        .mask_blend_row_color = NULL,
        .mask_get_row_overlay = NULL,
};

void egui_mask_init(egui_mask_t *self)
{
    // update api.
    self->api = &egui_mask_t_api_table;
}

#endif

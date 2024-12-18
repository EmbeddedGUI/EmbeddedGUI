#include <stdio.h>
#include <assert.h>

#include "egui_mask.h"
#include "core/egui_api.h"

void egui_mask_set_position(egui_mask_t *self, egui_dim_t x, egui_dim_t y)
{
    self->region.location.x = x;
    self->region.location.y = y;
}

void egui_mask_set_size(egui_mask_t *self, egui_dim_t width, egui_dim_t height)
{
    self->region.size.width = width;
    self->region.size.height = height;
}

void egui_mask_mask_point(egui_mask_t *self, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha)
{
    // implement is sub-class.
}

const egui_mask_api_t egui_mask_t_api_table = {
        .mask_point = egui_mask_mask_point,
};

void egui_mask_init(egui_mask_t *self)
{
    // update api.
    self->api = &egui_mask_t_api_table;
}

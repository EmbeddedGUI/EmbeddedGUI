#include <stdio.h>
#include <assert.h>

#include "egui_image.h"
#include "core/egui_api.h"

int egui_image_get_point(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha)
{
    // implement is sub-class.
    return 0;
}

int egui_image_get_point_resize(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t *color, egui_alpha_t *alpha)
{
    // implement is sub-class.
    return 0;
}

void egui_image_draw_image(const egui_image_t *self, egui_dim_t x, egui_dim_t y)
{
    // implement is sub-class.
}

void egui_image_draw_image_resize(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    // implement is sub-class.
}

const egui_image_api_t egui_image_t_api_table = {
        .get_point = egui_image_get_point,
        .get_point_resize = egui_image_get_point_resize,
        .draw_image = egui_image_draw_image,
        .draw_image_resize = egui_image_draw_image_resize,
};

void egui_image_init(egui_image_t *self, const void *res)
{
    self->res = res;

    // update api.
    self->api = &egui_image_t_api_table;
}

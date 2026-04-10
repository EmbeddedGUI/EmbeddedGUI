#include <stdio.h>
#include <assert.h>

#include "egui_image.h"
#include "core/egui_api.h"

static int egui_image_default_get_size(const egui_image_t *self, egui_dim_t *width, egui_dim_t *height)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(width);
    EGUI_UNUSED(height);
    return 0;
}

static int egui_image_default_get_point(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha)
{
    // implement is sub-class.
    return 0;
}

static int egui_image_default_get_point_resize(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t *color,
                                               egui_alpha_t *alpha)
{
    // implement is sub-class.
    return 0;
}

static void egui_image_default_draw_image(const egui_image_t *self, egui_dim_t x, egui_dim_t y)
{
    // implement is sub-class.
}

static void egui_image_default_draw_image_resize(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    // implement is sub-class.
}

static void egui_image_default_draw_image_color(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha)
{
    EGUI_UNUSED(color);
    EGUI_UNUSED(alpha);
    egui_image_default_draw_image(self, x, y);
}

static void egui_image_default_draw_image_resize_color(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height,
                                                       egui_color_t color, egui_alpha_t alpha)
{
    EGUI_UNUSED(color);
    EGUI_UNUSED(alpha);
    egui_image_default_draw_image_resize(self, x, y, width, height);
}

int egui_image_get_size(const egui_image_t *self, egui_dim_t *width, egui_dim_t *height)
{
    if (self == NULL || self->api == NULL || self->api->get_size == NULL)
    {
        return 0;
    }

    return self->api->get_size(self, width, height);
}

void egui_image_draw_image(const egui_image_t *self, egui_dim_t x, egui_dim_t y)
{
    if (self == NULL || self->api == NULL || self->api->draw_image == NULL)
    {
        return;
    }

    self->api->draw_image(self, x, y);
}

void egui_image_draw_image_resize(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    if (self == NULL || self->api == NULL || self->api->draw_image_resize == NULL)
    {
        return;
    }

    self->api->draw_image_resize(self, x, y, width, height);
}

void egui_image_draw_image_color(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha)
{
    if (self == NULL || self->api == NULL)
    {
        return;
    }
    if (self->api->draw_image_color != NULL)
    {
        self->api->draw_image_color(self, x, y, color, alpha);
        return;
    }

    egui_image_draw_image(self, x, y);
}

void egui_image_draw_image_resize_color(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t color,
                                        egui_alpha_t alpha)
{
    if (self == NULL || self->api == NULL)
    {
        return;
    }
    if (self->api->draw_image_resize_color != NULL)
    {
        self->api->draw_image_resize_color(self, x, y, width, height, color, alpha);
        return;
    }

    egui_image_draw_image_resize(self, x, y, width, height);
}

const egui_image_api_t egui_image_t_api_table = {
        .get_size = egui_image_default_get_size,
        .get_point = egui_image_default_get_point,
        .get_point_resize = egui_image_default_get_point_resize,
        .draw_image = egui_image_default_draw_image,
        .draw_image_resize = egui_image_default_draw_image_resize,
        .draw_image_color = egui_image_default_draw_image_color,
        .draw_image_resize_color = egui_image_default_draw_image_resize_color,
};

void egui_image_init(egui_image_t *self, const void *res)
{
    self->res = res;

    // update api.
    self->api = &egui_image_t_api_table;
}

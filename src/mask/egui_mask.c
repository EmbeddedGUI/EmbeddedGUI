#include <stdio.h>
#include <assert.h>

#include "egui_mask.h"
#include "egui_mask_round_rectangle.h"
#include "core/egui_api.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK

/**
 * @brief Move the mask region without changing its shape-specific parameters.
 *
 * Concrete masks interpret their geometry relative to `self->region`, so this
 * base helper is the common way to relocate an existing mask instance.
 */
void egui_mask_set_position(egui_mask_t *self, egui_dim_t x, egui_dim_t y)
{
    self->region.location.x = x;
    self->region.location.y = y;
}

/**
 * @brief Resize the base mask region and clamp derived shape data when needed.
 *
 * Most mask kinds only store the rectangle directly, but round rectangles also
 * keep a corner radius that must stay within the new bounds. Doing the clamp in
 * the base helper keeps every caller from having to repeat that safety logic.
 */
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

/**
 * @brief Default point-mask hook for the abstract base type.
 *
 * The base mask does not alter color or alpha by itself. Real behavior comes
 * from subclasses that replace this callback through their own API table.
 */
void egui_mask_mask_point(egui_mask_t *self, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(x);
    EGUI_UNUSED(y);
    EGUI_UNUSED(color);
    EGUI_UNUSED(alpha);
}

const egui_mask_api_t egui_mask_t_api_table = {
        .kind = EGUI_MASK_KIND_BASE,
        .mask_point = egui_mask_mask_point,
        .mask_get_row_range = NULL,
        .mask_get_row_visible_range = NULL,
        .mask_blend_row_color = NULL,
        .mask_get_row_overlay = NULL,
};

/**
 * @brief Initialize the abstract mask base object.
 *
 * Derived mask initializers usually call this first, then replace `self->api`
 * with their specialized table and populate any extra fields they own.
 */
void egui_mask_init(egui_mask_t *self)
{
    self->api = &egui_mask_t_api_table;
}

#endif

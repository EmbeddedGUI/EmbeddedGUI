#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_mask_image.h"
#include "core/egui_common.h"
#include "core/egui_api.h"
#include "core/egui_canvas.h"

void egui_mask_image_set_image(egui_mask_t *self, egui_image_t *img)
{
    egui_mask_image_t *local = (egui_mask_image_t *)self;
    local->img = img;
}

void egui_mask_image_mask_point(egui_mask_t *self, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha)
{
    egui_mask_image_t *local = (egui_mask_image_t *)self;
    if(local->img == NULL)
    {
        return;
    }
    egui_color_t orign_color;
    egui_alpha_t orign_alpha;
    if (local->img->api->get_point_resize(local->img, x, y, self->region.size.width, self->region.size.height, &orign_color, &orign_alpha))
    {
        *alpha = orign_alpha;
    }
    else
    {
        *alpha = 0;
    }
}

// name must be _type##_api_table, it will be used by EGUI_VIEW_DEFINE to init api.
const egui_mask_api_t egui_mask_image_t_api_table = {
        .mask_point = egui_mask_image_mask_point,
};

void egui_mask_image_init(egui_mask_t *self)
{
    egui_mask_image_t *local = (egui_mask_image_t *)self;
    // call super init.
    egui_mask_init(self);
    // update api.
    self->api = &egui_mask_image_t_api_table;

    // init local data.
}

#include <stdio.h>
#include <assert.h>

#include "egui_view_image.h"
#include "image/egui_image.h"

void egui_view_image_on_draw(egui_view_t *self)
{
    egui_view_image_t *local = (egui_view_image_t *)self;

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    if(local->image == NULL)
    {
        return;
    }
    if(local->image_type == EGUI_VIEW_IMAGE_TYPE_NORMAL)
    {
        egui_canvas_draw_image(local->image, region.location.x, region.location.y);
    }
    else
    {
        egui_canvas_draw_image_resize(local->image, region.location.x, region.location.y, region.size.width, region.size.height);
    }
}

void egui_view_image_set_image_type(egui_view_t *self, int image_type)
{
    egui_view_image_t *local = (egui_view_image_t *)self;
    local->image_type = image_type;
    egui_view_invalidate(self);
}

void egui_view_image_set_image(egui_view_t *self, egui_image_t *image)
{
    egui_view_image_t *local = (egui_view_image_t *)self;
    local->image = image;
    egui_view_invalidate(self);
}


void egui_view_image_std_normal_init(egui_view_t *self, egui_image_t *image, egui_dim_t x, egui_dim_t y)
{
    egui_dim_t width, height;

    egui_image_std_get_width_height(image, &width, &height);
    egui_view_image_init(self);
    egui_view_set_position(self, x, y);
    egui_view_set_size(self, width, height);
    egui_view_image_set_image(self, image);
}

EGUI_VIEW_API_DEFINE(egui_view_image_t, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, egui_view_image_on_draw, NULL);

void egui_view_image_init(egui_view_t *self)
{
    egui_view_image_t *local = (egui_view_image_t *)self;
    EGUI_UNUSED(local);
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_image_t);

    // init local data.
    egui_view_set_view_name(self, "egui_view_image");

    local->image_type = EGUI_VIEW_IMAGE_TYPE_NORMAL;
    local->image = NULL;
}

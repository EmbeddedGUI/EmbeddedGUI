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
    if(local->image_type == image_type)
    {
        return;
    }
    local->image_type = image_type;
    egui_view_invalidate(self);
}

void egui_view_image_set_image(egui_view_t *self, egui_image_t *image)
{
    egui_view_image_t *local = (egui_view_image_t *)self;
    if(local->image == image)
    {
        return;
    }
    local->image = image;
    egui_view_invalidate(self);
}


EGUI_VIEW_API_DEFINE(egui_view_image_t, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, egui_view_image_on_draw, NULL);

void egui_view_image_init(egui_view_t *self)
{
    egui_view_image_t *local = (egui_view_image_t *)self;
    EGUI_UNUSED(local);
#if EGUI_CONFIG_AC5		
	  EGUI_VIEW_API_INIT(egui_view_image_t, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, egui_view_image_on_draw, NULL);
#endif
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_image_t);

    // init local data.
    egui_view_set_view_name(self, "egui_view_image");

    local->image_type = EGUI_VIEW_IMAGE_TYPE_NORMAL;
    local->image = NULL;
}

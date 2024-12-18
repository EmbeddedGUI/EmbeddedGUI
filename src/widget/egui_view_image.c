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

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_image_t) = {
    .dispatch_touch_event = egui_view_dispatch_touch_event,
    .on_touch_event = egui_view_on_touch_event,
    .on_intercept_touch_event = egui_view_on_intercept_touch_event,
    .compute_scroll = egui_view_compute_scroll,
    .calculate_layout = egui_view_calculate_layout,
    .request_layout = egui_view_request_layout,
    .draw = egui_view_draw,
    .on_attach_to_window = egui_view_on_attach_to_window,
    .on_draw = egui_view_image_on_draw, // changed
    .on_detach_from_window = egui_view_on_detach_from_window,
};

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

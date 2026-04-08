#include <assert.h>

#include "egui_view_image.h"
#include "image/egui_image.h"

void egui_view_image_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_image_t);

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    if (local->image == NULL)
    {
        return;
    }
    if (local->image_color_alpha != 0)
    {
        // alpha-only color rendering path
        if (local->image_type == EGUI_VIEW_IMAGE_TYPE_NORMAL)
        {
            egui_canvas_draw_image_color(local->image, region.location.x, region.location.y, local->image_color, local->image_color_alpha);
        }
        else
        {
            egui_canvas_draw_image_resize_color(local->image, region.location.x, region.location.y, region.size.width, region.size.height, local->image_color,
                                                local->image_color_alpha);
        }
    }
    else if (local->image_type == EGUI_VIEW_IMAGE_TYPE_NORMAL)
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
    EGUI_LOCAL_INIT(egui_view_image_t);
    if (local->image_type == image_type)
    {
        return;
    }
    local->image_type = image_type;
    egui_view_invalidate(self);
}

void egui_view_image_set_image(egui_view_t *self, egui_image_t *image)
{
    EGUI_LOCAL_INIT(egui_view_image_t);
    if (local->image == image)
    {
        return;
    }
    local->image = image;
    egui_view_invalidate(self);
}

void egui_view_image_set_image_color(egui_view_t *self, egui_color_t color, egui_alpha_t alpha)
{
    EGUI_LOCAL_INIT(egui_view_image_t);
    if (local->image_color.full == color.full && local->image_color_alpha == alpha)
    {
        return;
    }
    local->image_color = color;
    local->image_color_alpha = alpha;
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
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_image_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_image_t);

    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_image_t);

    // init local data.
    egui_view_set_view_name(self, "egui_view_image");

    local->image_type = EGUI_VIEW_IMAGE_TYPE_NORMAL;
    local->image = NULL;
    local->image_color.full = 0;
    local->image_color_alpha = 0;
}

void egui_view_image_apply_params(egui_view_t *self, const egui_view_image_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_image_t);

    self->region = params->region;

    local->image = params->image;

    egui_view_invalidate(self);
}

void egui_view_image_init_with_params(egui_view_t *self, const egui_view_image_params_t *params)
{
    egui_view_image_init(self);
    egui_view_image_apply_params(self, params);
}

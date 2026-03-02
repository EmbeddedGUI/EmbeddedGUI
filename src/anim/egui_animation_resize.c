#include <stdio.h>
#include <assert.h>

#include "egui_animation_resize.h"
#include "widget/egui_view.h"
#include "core/egui_api.h"

void egui_animation_resize_on_start(egui_animation_t *self)
{
    EGUI_LOCAL_INIT(egui_animation_resize_t);
    if (local->params == NULL)
    {
        return;
    }

    egui_region_copy(&local->origin_region, &self->target_view->region);
}

void egui_animation_resize_on_update(egui_animation_t *self, egui_float_t fraction)
{
    EGUI_LOCAL_INIT(egui_animation_resize_t);

    // Calculate current width ratio
    egui_float_t w_delta = EGUI_FLOAT_MULT_LIMIT((local->params->to_width_ratio - local->params->from_width_ratio), fraction);
    egui_float_t w_ratio = local->params->from_width_ratio + w_delta;

    // Calculate current height ratio
    egui_float_t h_delta = EGUI_FLOAT_MULT_LIMIT((local->params->to_height_ratio - local->params->from_height_ratio), fraction);
    egui_float_t h_ratio = local->params->from_height_ratio + h_delta;

    egui_dim_t new_width = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(local->origin_region.size.width, w_ratio);
    egui_dim_t new_height = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(local->origin_region.size.height, h_ratio);

    egui_region_t region;
    egui_region_copy(&region, &local->origin_region);

    egui_dim_t delta_width = new_width - local->origin_region.size.width;
    egui_dim_t delta_height = new_height - local->origin_region.size.height;

    region.size.width = new_width;
    region.size.height = new_height;

    switch (local->params->mode)
    {
    case EGUI_ANIMATION_RESIZE_MODE_LEFT:
        // Left/top anchored: position stays, size changes
        break;
    case EGUI_ANIMATION_RESIZE_MODE_CENTER:
        // Center anchored: offset position by half the delta
        region.location.x -= delta_width / 2;
        region.location.y -= delta_height / 2;
        break;
    case EGUI_ANIMATION_RESIZE_MODE_RIGHT:
        // Right/bottom anchored: offset position by full delta
        region.location.x -= delta_width;
        region.location.y -= delta_height;
        break;
    default:
        break;
    }

    egui_view_layout(self->target_view, &region);
}

void egui_animation_resize_params_set(egui_animation_resize_t *self, const egui_animation_resize_params_t *params)
{
    self->params = params;
}

const egui_animation_api_t egui_animation_resize_t_api_table = {
        .on_start = egui_animation_resize_on_start,
        .update = egui_animation_update,
        .on_update = egui_animation_resize_on_update,
};

void egui_animation_resize_init(egui_animation_t *self)
{
    EGUI_LOCAL_INIT(egui_animation_resize_t);
    // call super init.
    egui_animation_init(self);
    // update api.
    self->api = &egui_animation_resize_t_api_table;

    // init local data.
}

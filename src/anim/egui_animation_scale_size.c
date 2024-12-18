#include <stdio.h>
#include <assert.h>

#include "egui_animation_scale_size.h"
#include "widget/egui_view.h"
#include "core/egui_api.h"

void egui_animation_scale_size_on_start(egui_animation_t *self)
{
    egui_animation_scale_size_t *local = (egui_animation_scale_size_t *)self;
    if (local->params == NULL)
    {
        return;
    }

    egui_region_copy(&local->origin_region, &self->target_view->region);
}

void egui_animation_scale_size_on_update(egui_animation_t *self, egui_float_t fraction)
{
    egui_animation_scale_size_t *local = (egui_animation_scale_size_t *)self;
    egui_float_t delta = EGUI_FLOAT_MULT_LIMIT((local->params->to_scale - local->params->from_scale), fraction);
    egui_float_t value = local->params->from_scale + delta;

    egui_region_t region;
    egui_region_copy(&region, &self->target_view->region);

    egui_dim_t width = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(local->origin_region.size.width, value);
    egui_dim_t height = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(local->origin_region.size.height, value);

    egui_dim_t delta_width = width - self->target_view->region.size.width;
    egui_dim_t delta_height = height - self->target_view->region.size.height;

    // round to even number.
    delta_width = delta_width & 0xfffffffe;
    delta_height = delta_height & 0xfffffffe;

    if (delta_width != 0)
    {
        region.location.x -= delta_width / 2;
        region.size.width += delta_width;
    }

    if (delta_height != 0)
    {
        region.location.y -= delta_height / 2;
        region.size.height += delta_height;
    }

    if (delta_width != 0 || delta_height != 0)
    {
        // need to update view's region.
        egui_view_layout(self->target_view, &region);
    }
}

void egui_animation_scale_size_params_set(egui_animation_scale_size_t *self, const egui_animation_scale_size_params_t *params)
{
    self->params = params;
}

const egui_animation_api_t egui_animation_scale_size_t_api_table = {
        .on_start = egui_animation_scale_size_on_start,
        .update = egui_animation_update,
        .on_update = egui_animation_scale_size_on_update,
};

void egui_animation_scale_size_init(egui_animation_t *self)
{
    egui_animation_scale_size_t *local = (egui_animation_scale_size_t *)self;
    // call super init.
    egui_animation_init(self);
    // update api.
    self->api = &egui_animation_scale_size_t_api_table;

    // init local data.
}

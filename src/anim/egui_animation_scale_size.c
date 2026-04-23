#include <stdio.h>
#include <assert.h>

#include "egui_animation_scale_size.h"
#include "widget/egui_view.h"
#include "core/egui_api.h"

/**
 * @file egui_animation_scale_size.c
 * @brief Uniform scale animation that grows or shrinks a target view around its center.
 */

/** Capture the original target region so each update scales from the same baseline bounds. */
void egui_animation_scale_size_on_start(egui_animation_t *self)
{
    EGUI_LOCAL_INIT(egui_animation_scale_size_t);
    if (local->params == NULL)
    {
        return;
    }

    egui_region_copy(&local->origin_region, &self->target_view->region);
}

/** Compatibility wrapper that forwards to the normal scale-size start hook. */
void egui_animation_scale_size_start(egui_animation_t *self)
{
    egui_animation_scale_size_on_start(self);
}

/** Interpolate the uniform scale, keep the view centered, and relayout only when the integer size changes. */
void egui_animation_scale_size_on_update(egui_animation_t *self, egui_float_t fraction)
{
    EGUI_LOCAL_INIT(egui_animation_scale_size_t);
    egui_float_t delta = EGUI_FLOAT_MULT_LIMIT((local->params->to_scale - local->params->from_scale), fraction);
    egui_float_t value = local->params->from_scale + delta;

    egui_region_t region;
    egui_region_copy(&region, &self->target_view->region);

    egui_dim_t width = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(local->origin_region.size.width, value);
    egui_dim_t height = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(local->origin_region.size.height, value);

    egui_dim_t delta_width = width - self->target_view->region.size.width;
    egui_dim_t delta_height = height - self->target_view->region.size.height;

    // Round to even deltas so splitting by two keeps the view centered on integer coordinates.
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
        egui_view_layout(self->target_view, &region);
    }
}

/** Bind the borrowed parameter block used by this scale-size animation instance. */
void egui_animation_scale_size_params_set(egui_animation_scale_size_t *self, const egui_animation_scale_size_params_t *params)
{
    self->params = params;
}

const egui_animation_api_t egui_animation_scale_size_t_api_table = {
        .on_start = egui_animation_scale_size_on_start,
        .update = egui_animation_update,
        .on_update = egui_animation_scale_size_on_update,
};

/** Initialize a scale-size animation and install the callbacks that resize the target symmetrically. */
void egui_animation_scale_size_init(egui_animation_t *self)
{
    egui_animation_init(self);
    self->api = &egui_animation_scale_size_t_api_table;
}

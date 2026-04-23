#include <stdio.h>
#include <assert.h>

#include "egui_animation_alpha.h"
#include "widget/egui_view.h"
#include "core/egui_api.h"

/**
 * @file egui_animation_alpha.c
 * @brief Alpha animation that linearly fades one target view between two opacity values.
 */

/** Start hook for alpha animation. The parameters are borrowed, so no extra setup is required beyond validation. */
void egui_animation_alpha_on_start(egui_animation_t *self)
{
    EGUI_LOCAL_INIT(egui_animation_alpha_t);
    if (local->params == NULL)
    {
        return;
    }
}

/** Interpolate one alpha value from the configured range and apply it to the target view. */
void egui_animation_alpha_on_update(egui_animation_t *self, egui_float_t fraction)
{
    EGUI_LOCAL_INIT(egui_animation_alpha_t);
    egui_alpha_t value = local->params->from_alpha + (egui_alpha_t)EGUI_FLOAT_MULT_LIMIT((local->params->to_alpha - local->params->from_alpha), fraction);

    egui_view_set_alpha(self->target_view, value);
}

/** Bind the borrowed parameter block used by this alpha animation instance. */
void egui_animation_alpha_params_set(egui_animation_alpha_t *self, const egui_animation_alpha_params_t *params)
{
    self->params = params;
}

const egui_animation_api_t egui_animation_alpha_t_api_table = {
        .on_start = egui_animation_alpha_on_start,
        .update = egui_animation_update,
        .on_update = egui_animation_alpha_on_update,
};

/** Initialize an alpha animation and replace the base callbacks with the fade-specific implementation. */
void egui_animation_alpha_init(egui_animation_t *self)
{
    egui_animation_init(self);
    self->api = &egui_animation_alpha_t_api_table;
}

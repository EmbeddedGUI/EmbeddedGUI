#include <stdio.h>
#include <assert.h>

#include "egui_animation_alpha.h"
#include "widget/egui_view.h"
#include "core/egui_api.h"

void egui_animation_alpha_on_start(egui_animation_t *self)
{
    egui_animation_alpha_t *local = (egui_animation_alpha_t *)self;
    if (local->params == NULL)
    {
        return;
    }
}

void egui_animation_alpha_on_update(egui_animation_t *self, egui_float_t fraction)
{
    egui_animation_alpha_t *local = (egui_animation_alpha_t *)self;
    egui_alpha_t value = local->params->from_alpha + (egui_alpha_t)EGUI_FLOAT_MULT_LIMIT((local->params->to_alpha - local->params->from_alpha), fraction);

    egui_view_set_alpha(self->target_view, value);
}

void egui_animation_alpha_params_set(egui_animation_alpha_t *self, const egui_animation_alpha_params_t *params)
{
    self->params = params;
}

const egui_animation_api_t egui_animation_alpha_t_api_table = {
        .on_start = egui_animation_alpha_on_start,
        .update = egui_animation_update,
        .on_update = egui_animation_alpha_on_update,
};

void egui_animation_alpha_init(egui_animation_t *self)
{
    egui_animation_alpha_t *local = (egui_animation_alpha_t *)self;
    EGUI_UNUSED(local);
    // call super init.
    egui_animation_init(self);
    // update api.
    self->api = &egui_animation_alpha_t_api_table;

    // init local data.
}

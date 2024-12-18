#include <stdio.h>
#include <assert.h>

#include "egui_animation_translate.h"
#include "widget/egui_view.h"
#include "core/egui_api.h"

void egui_animation_translate_on_start(egui_animation_t *self)
{
    egui_animation_translate_t *local = (egui_animation_translate_t *)self;
    if (local->params == NULL)
    {
        return;
    }

    local->current_x = 0;
    local->current_y = 0;
}

void egui_animation_translate_on_update(egui_animation_t *self, egui_float_t fraction)
{
    egui_animation_translate_t *local = (egui_animation_translate_t *)self;
    egui_dim_t delta_x = 0;
    egui_dim_t delta_y = 0;

    egui_dim_t delta = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT((local->params->to_x - local->params->from_x), fraction);
    egui_dim_t value = local->params->from_x + delta;
    if (local->current_x != value)
    {
        delta_x = value - local->current_x;
        local->current_x = value;
    }

    delta = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT((local->params->to_y - local->params->from_y), fraction);
    value = local->params->from_y + delta;
    if (local->current_y != value)
    {
        delta_y = value - local->current_y;
        local->current_y = value;
    }

    if ((delta_x != 0) || (delta_y != 0))
    {
        egui_view_scroll_by(self->target_view, delta_x, delta_y);
    }
}

void egui_animation_translate_params_set(egui_animation_translate_t *self, const egui_animation_translate_params_t *params)
{
    self->params = params;
}

const egui_animation_api_t egui_animation_translate_t_api_table = {
        .on_start = egui_animation_translate_on_start,
        .update = egui_animation_update,
        .on_update = egui_animation_translate_on_update,
};

void egui_animation_translate_init(egui_animation_t *self)
{
    egui_animation_translate_t *local = (egui_animation_translate_t *)self;
    EGUI_UNUSED(local);
    // call super init.
    egui_animation_init(self);
    // update api.
    self->api = &egui_animation_translate_t_api_table;

    // init local data.
}

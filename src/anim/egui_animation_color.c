#include <stdio.h>
#include <assert.h>

#include "egui_animation_color.h"
#include "widget/egui_view.h"
#include "widget/egui_view_label.h"
#include "core/egui_api.h"

void egui_animation_color_on_start(egui_animation_t *self)
{
    EGUI_LOCAL_INIT(egui_animation_color_t);
    if (local->params == NULL)
    {
        return;
    }
}

void egui_animation_color_on_update(egui_animation_t *self, egui_float_t fraction)
{
    EGUI_LOCAL_INIT(egui_animation_color_t);

    egui_color_t from = local->params->from_color;
    egui_color_t to = local->params->to_color;
    egui_color_t result;

    // Per-channel linear interpolation using native color fields.
    int16_t r = from.color.red + (int16_t)EGUI_FLOAT_MULT_LIMIT((to.color.red - from.color.red), fraction);
    int16_t g = from.color.green + (int16_t)EGUI_FLOAT_MULT_LIMIT((to.color.green - from.color.green), fraction);
    int16_t b = from.color.blue + (int16_t)EGUI_FLOAT_MULT_LIMIT((to.color.blue - from.color.blue), fraction);

    result.color.red = (uint16_t)r;
    result.color.green = (uint16_t)g;
    result.color.blue = (uint16_t)b;

    // Apply to target view as label font color
    egui_view_label_set_font_color(self->target_view, result, EGUI_ALPHA_100);
}

void egui_animation_color_params_set(egui_animation_color_t *self, const egui_animation_color_params_t *params)
{
    self->params = params;
}

const egui_animation_api_t egui_animation_color_t_api_table = {
        .on_start = egui_animation_color_on_start,
        .update = egui_animation_update,
        .on_update = egui_animation_color_on_update,
};

void egui_animation_color_init(egui_animation_t *self)
{
    EGUI_LOCAL_INIT(egui_animation_color_t);
    // call super init.
    egui_animation_init(self);
    // update api.
    self->api = &egui_animation_color_t_api_table;

    // init local data.
}

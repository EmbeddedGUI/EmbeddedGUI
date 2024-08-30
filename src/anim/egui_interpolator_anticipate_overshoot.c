#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_interpolator_anticipate_overshoot.h"
#include "core/egui_common.h"
#include "core/egui_api.h"
#include "core/egui_canvas.h"

void egui_interpolator_anticipate_overshoot_tension_set(egui_interpolator_t *self, egui_float_t tension)
{
    egui_interpolator_anticipate_overshoot_t *local = (egui_interpolator_anticipate_overshoot_t *)self;
    local->tension = EGUI_FLOAT_MULT(tension, EGUI_FLOAT_VALUE(1.5f));
}

static egui_float_t a(egui_float_t t, egui_float_t s)
{
    // t * t * ((s + 1) * t - s);
    return (EGUI_FLOAT_MULT_LIMIT(EGUI_FLOAT_MULT_LIMIT(t, t), (EGUI_FLOAT_MULT_LIMIT((s + EGUI_FLOAT_VALUE(1.0f)), t)) - s));
}

static egui_float_t o(egui_float_t t, egui_float_t s)
{
    // t * t * ((s + 1) * t + s);
    return (EGUI_FLOAT_MULT_LIMIT(EGUI_FLOAT_MULT_LIMIT(t, t), EGUI_FLOAT_MULT_LIMIT((s + EGUI_FLOAT_VALUE(1.0f)), t) + s));
}

egui_float_t egui_interpolator_anticipate_overshoot_get_interpolation(egui_interpolator_t *self, egui_float_t t)
{
    egui_interpolator_anticipate_overshoot_t *local = (egui_interpolator_anticipate_overshoot_t *)self;
    // a(t, s) = t * t * ((s + 1) * t - s)
    // o(t, s) = t * t * ((s + 1) * t + s)
    // f(t) = 0.5 * a(t * 2, tension * extraTension), when t < 0.5
    // f(t) = 0.5 * (o(t * 2 - 2, tension * extraTension) + 2), when t <= 1.0
    if (t < EGUI_FLOAT_VALUE(0.5f))
    {
        return EGUI_FLOAT_MULT_LIMIT(EGUI_FLOAT_VALUE(0.5f), a(EGUI_FLOAT_MULT_LIMIT(t, EGUI_FLOAT_VALUE(2.0f)), local->tension));
    }
    else
    {
        return EGUI_FLOAT_MULT_LIMIT(EGUI_FLOAT_VALUE(0.5f),
                                     (o(EGUI_FLOAT_MULT_LIMIT(t, EGUI_FLOAT_VALUE(2.0f)) - EGUI_FLOAT_VALUE(2.0f), local->tension) + EGUI_FLOAT_VALUE(2.0f)));
    }
}

// name must be _type##_api_table, it will be used by EGUI_VIEW_DEFINE to init api.
const egui_interpolator_api_t egui_interpolator_anticipate_overshoot_t_api_table = {
        .get_interpolation = egui_interpolator_anticipate_overshoot_get_interpolation,
};

void egui_interpolator_anticipate_overshoot_init(egui_interpolator_t *self)
{
    egui_interpolator_anticipate_overshoot_t *local = (egui_interpolator_anticipate_overshoot_t *)self;
    // call super init.
    egui_interpolator_init(self);
    // update api.
    self->api = &egui_interpolator_anticipate_overshoot_t_api_table;

    // init local data.
    local->tension = EGUI_FLOAT_VALUE(2.0f * 1.5f);
}

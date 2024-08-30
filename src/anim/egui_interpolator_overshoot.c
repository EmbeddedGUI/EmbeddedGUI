#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_interpolator_overshoot.h"
#include "core/egui_common.h"
#include "core/egui_api.h"
#include "core/egui_canvas.h"

void egui_interpolator_overshoot_tension_set(egui_interpolator_t *self, egui_float_t tension)
{
    egui_interpolator_overshoot_t *local = (egui_interpolator_overshoot_t *)self;
    local->tension = tension;
}

egui_float_t egui_interpolator_overshoot_get_interpolation(egui_interpolator_t *self, egui_float_t t)
{
    egui_interpolator_overshoot_t *local = (egui_interpolator_overshoot_t *)self;
    // _o(t) = t * t * ((tension + 1) * t + tension)
    // o(t) = _o(t - 1) + 1
    t = t - EGUI_FLOAT_VALUE(1.0f);
    return EGUI_FLOAT_MULT_LIMIT(EGUI_FLOAT_MULT_LIMIT(t, t), EGUI_FLOAT_MULT_LIMIT(local->tension + EGUI_FLOAT_VALUE(1.0f), t) + local->tension) +
           EGUI_FLOAT_VALUE(1.0f);
}

// name must be _type##_api_table, it will be used by EGUI_VIEW_DEFINE to init api.
const egui_interpolator_api_t egui_interpolator_overshoot_t_api_table = {
        .get_interpolation = egui_interpolator_overshoot_get_interpolation,
};

void egui_interpolator_overshoot_init(egui_interpolator_t *self)
{
    egui_interpolator_overshoot_t *local = (egui_interpolator_overshoot_t *)self;
    // call super init.
    egui_interpolator_init(self);
    // update api.
    self->api = &egui_interpolator_overshoot_t_api_table;

    // init local data.
    local->tension = EGUI_FLOAT_VALUE(2.0f);
}

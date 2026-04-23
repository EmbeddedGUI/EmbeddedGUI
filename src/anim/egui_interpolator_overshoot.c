#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_interpolator_overshoot.h"
#include "core/egui_common.h"
#include "core/egui_api.h"
#include "canvas/egui_canvas.h"

/**
 * @file egui_interpolator_overshoot.c
 * @brief Overshoot interpolator that goes past the final value and then settles back.
 */

/** Update the overshoot tension. Larger values push the curve farther past 1.0 before settling. */
void egui_interpolator_overshoot_tension_set(egui_interpolator_t *self, egui_float_t tension)
{
    EGUI_LOCAL_INIT(egui_interpolator_overshoot_t);
    local->tension = tension;
}

/** Apply the overshoot curve so late progress can exceed 1.0 before returning to the end state. */
egui_float_t egui_interpolator_overshoot_get_interpolation(egui_interpolator_t *self, egui_float_t t)
{
    EGUI_LOCAL_INIT(egui_interpolator_overshoot_t);
    t = t - EGUI_FLOAT_VALUE(1.0f);
    return EGUI_FLOAT_MULT_LIMIT(EGUI_FLOAT_MULT_LIMIT(t, t), EGUI_FLOAT_MULT_LIMIT(local->tension + EGUI_FLOAT_VALUE(1.0f), t) + local->tension) +
           EGUI_FLOAT_VALUE(1.0f);
}

const egui_interpolator_api_t egui_interpolator_overshoot_t_api_table = {
        .get_interpolation = egui_interpolator_overshoot_get_interpolation,
};

/** Initialize the overshoot interpolator and install the default tension of 2.0. */
void egui_interpolator_overshoot_init(egui_interpolator_t *self)
{
    EGUI_LOCAL_INIT(egui_interpolator_overshoot_t);
    egui_interpolator_init(self);
    self->api = &egui_interpolator_overshoot_t_api_table;
    local->tension = EGUI_FLOAT_VALUE(2.0f);
}

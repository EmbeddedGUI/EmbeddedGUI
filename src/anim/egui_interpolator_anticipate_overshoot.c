#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_interpolator_anticipate_overshoot.h"
#include "core/egui_common.h"
#include "core/egui_api.h"
#include "canvas/egui_canvas.h"

/**
 * @file egui_interpolator_anticipate_overshoot.c
 * @brief Interpolator that combines an initial backward pull with a final overshoot rebound.
 */

/** Update the base tension. Internally it is scaled by 1.5 to match the classic anticipate-overshoot shape. */
void egui_interpolator_anticipate_overshoot_tension_set(egui_interpolator_t *self, egui_float_t tension)
{
    EGUI_LOCAL_INIT(egui_interpolator_anticipate_overshoot_t);
    local->tension = EGUI_FLOAT_MULT(tension, EGUI_FLOAT_VALUE(1.5f));
}

/** Helper for the first half of the curve, which anticipates by moving backward. */
static egui_float_t a(egui_float_t t, egui_float_t s)
{
    return (EGUI_FLOAT_MULT_LIMIT(EGUI_FLOAT_MULT_LIMIT(t, t), (EGUI_FLOAT_MULT_LIMIT((s + EGUI_FLOAT_VALUE(1.0f)), t)) - s));
}

/** Helper for the second half of the curve, which overshoots before settling. */
static egui_float_t o(egui_float_t t, egui_float_t s)
{
    return (EGUI_FLOAT_MULT_LIMIT(EGUI_FLOAT_MULT_LIMIT(t, t), EGUI_FLOAT_MULT_LIMIT((s + EGUI_FLOAT_VALUE(1.0f)), t) + s));
}

/** Use the anticipate helper for the first half and the overshoot helper for the second half of the animation. */
egui_float_t egui_interpolator_anticipate_overshoot_get_interpolation(egui_interpolator_t *self, egui_float_t t)
{
    EGUI_LOCAL_INIT(egui_interpolator_anticipate_overshoot_t);
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

const egui_interpolator_api_t egui_interpolator_anticipate_overshoot_t_api_table = {
        .get_interpolation = egui_interpolator_anticipate_overshoot_get_interpolation,
};

/** Initialize the anticipate-overshoot interpolator with the default effective tension of 3.0. */
void egui_interpolator_anticipate_overshoot_init(egui_interpolator_t *self)
{
    EGUI_LOCAL_INIT(egui_interpolator_anticipate_overshoot_t);
    egui_interpolator_init(self);
    self->api = &egui_interpolator_anticipate_overshoot_t_api_table;
    local->tension = EGUI_FLOAT_VALUE(2.0f * 1.5f);
}

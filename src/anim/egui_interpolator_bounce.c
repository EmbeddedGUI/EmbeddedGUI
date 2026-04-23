#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_interpolator_bounce.h"
#include "core/egui_common.h"
#include "core/egui_api.h"
#include "canvas/egui_canvas.h"

/**
 * @file egui_interpolator_bounce.c
 * @brief Bounce interpolator that simulates a few diminishing rebounds near the end of the motion.
 */

/** Primitive parabola used by each bounce segment. */
static egui_float_t bounce(egui_float_t t)
{
    return EGUI_FLOAT_MULT(EGUI_FLOAT_MULT(t, t), EGUI_FLOAT_VALUE(8.0f));
}

/** Evaluate the piecewise bounce curve, scaling input first and then choosing one rebound segment. */
egui_float_t egui_interpolator_bounce_get_interpolation(egui_interpolator_t *self, egui_float_t t)
{
    EGUI_UNUSED(self);

    t = EGUI_FLOAT_MULT(t, EGUI_FLOAT_VALUE(1.1226f));
    if (t < EGUI_FLOAT_VALUE(0.3535f))
        return bounce(t);
    else if (t < EGUI_FLOAT_VALUE(0.7408f))
        return bounce(t - EGUI_FLOAT_VALUE(0.54719f)) + EGUI_FLOAT_VALUE(0.7f);
    else if (t < EGUI_FLOAT_VALUE(0.9644f))
        return bounce(t - EGUI_FLOAT_VALUE(0.8526f)) + EGUI_FLOAT_VALUE(0.9f);
    else
        return bounce(t - EGUI_FLOAT_VALUE(1.0435f)) + EGUI_FLOAT_VALUE(0.95f);
}

const egui_interpolator_api_t egui_interpolator_bounce_t_api_table = {
        .get_interpolation = egui_interpolator_bounce_get_interpolation,
};

/** Initialize the bounce interpolator by swapping in its piecewise-curve API table. */
void egui_interpolator_bounce_init(egui_interpolator_t *self)
{
    egui_interpolator_init(self);
    self->api = &egui_interpolator_bounce_t_api_table;
}

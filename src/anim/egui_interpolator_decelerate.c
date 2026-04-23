#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_interpolator_decelerate.h"
#include "core/egui_common.h"
#include "core/egui_api.h"
#include "canvas/egui_canvas.h"

/**
 * @file egui_interpolator_decelerate.c
 * @brief Decelerate interpolator that moves quickly at first and eases into the final value.
 */

/** Update the deceleration factor. Larger values stretch more easing into the end of the curve. */
void egui_interpolator_decelerate_factor_set(egui_interpolator_t *self, egui_float_t factor)
{
    EGUI_LOCAL_INIT(egui_interpolator_decelerate_t);
    local->factor = factor;
}

/** Map input progress through a decelerating power curve using the configured factor. */
egui_float_t egui_interpolator_decelerate_get_interpolation(egui_interpolator_t *self, egui_float_t input)
{
    EGUI_LOCAL_INIT(egui_interpolator_decelerate_t);
    if (local->factor == EGUI_FLOAT_VALUE(1.0f))
    {
        return EGUI_FLOAT_VALUE(1.0f) - EGUI_FLOAT_MULT_LIMIT(EGUI_FLOAT_VALUE(1.0f) - input, EGUI_FLOAT_VALUE(1.0f) - input);
    }
    else
    {
        return EGUI_FLOAT_VALUE(1.0f) - EGUI_FLOAT_POWER(EGUI_FLOAT_VALUE(1.0f) - input, local->factor * 2);
    }
}

const egui_interpolator_api_t egui_interpolator_decelerate_t_api_table = {
        .get_interpolation = egui_interpolator_decelerate_get_interpolation,
};

/** Initialize the decelerate interpolator and install the default factor of 1.0. */
void egui_interpolator_decelerate_init(egui_interpolator_t *self)
{
    EGUI_LOCAL_INIT(egui_interpolator_decelerate_t);
    egui_interpolator_init(self);
    self->api = &egui_interpolator_decelerate_t_api_table;
    local->factor = EGUI_FLOAT_VALUE(1.0f);
}

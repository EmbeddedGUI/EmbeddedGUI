#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_interpolator_accelerate.h"
#include "core/egui_common.h"
#include "core/egui_api.h"
#include "canvas/egui_canvas.h"

/**
 * @file egui_interpolator_accelerate.c
 * @brief Accelerate interpolator that starts slowly and speeds up toward the end.
 */

/** Update the acceleration factor. Larger values keep the early portion of the curve flatter. */
void egui_interpolator_accelerate_factor_set(egui_interpolator_t *self, egui_float_t factor)
{
    EGUI_LOCAL_INIT(egui_interpolator_accelerate_t);
    local->factor = factor;
}

/** Map input progress through an accelerating power curve using the configured factor. */
egui_float_t egui_interpolator_accelerate_get_interpolation(egui_interpolator_t *self, egui_float_t input)
{
    EGUI_LOCAL_INIT(egui_interpolator_accelerate_t);
    if (local->factor == EGUI_FLOAT_VALUE(1.0f))
    {
        return EGUI_FLOAT_MULT_LIMIT(input, input);
    }
    else
    {
        return EGUI_FLOAT_POWER(input, local->factor * 2);
    }
}

const egui_interpolator_api_t egui_interpolator_accelerate_t_api_table = {
        .get_interpolation = egui_interpolator_accelerate_get_interpolation,
};

/** Initialize the accelerate interpolator and install the default factor of 1.0. */
void egui_interpolator_accelerate_init(egui_interpolator_t *self)
{
    EGUI_LOCAL_INIT(egui_interpolator_accelerate_t);
    egui_interpolator_init(self);
    self->api = &egui_interpolator_accelerate_t_api_table;
    local->factor = EGUI_FLOAT_VALUE(1.0f);
}

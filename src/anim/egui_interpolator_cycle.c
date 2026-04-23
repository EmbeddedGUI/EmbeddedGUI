#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_interpolator_cycle.h"
#include "core/egui_common.h"
#include "core/egui_api.h"
#include "canvas/egui_canvas.h"

/**
 * @file egui_interpolator_cycle.c
 * @brief Cycle interpolator that outputs a sine wave for oscillating animation values.
 */

/** Update how many sine-wave cycles are completed across the 0..1 input range. */
void egui_interpolator_cycle_tension_set(egui_interpolator_t *self, egui_float_t cycles)
{
    EGUI_LOCAL_INIT(egui_interpolator_cycle_t);
    local->cycles = cycles;
}

/** Convert normalized progress into a sine wave so the animated value can oscillate around zero. */
egui_float_t egui_interpolator_cycle_get_interpolation(egui_interpolator_t *self, egui_float_t input)
{
    EGUI_LOCAL_INIT(egui_interpolator_cycle_t);
    return EGUI_FLOAT_SIN(EGUI_FLOAT_MULT(2 * local->cycles, EGUI_FLOAT_MULT(input, EGUI_FLOAT_PI)));
}

const egui_interpolator_api_t egui_interpolator_cycle_t_api_table = {
        .get_interpolation = egui_interpolator_cycle_get_interpolation,
};

/** Initialize the cycle interpolator and install the default cycle count of 1.0. */
void egui_interpolator_cycle_init(egui_interpolator_t *self)
{
    EGUI_LOCAL_INIT(egui_interpolator_cycle_t);
    egui_interpolator_init(self);
    self->api = &egui_interpolator_cycle_t_api_table;
    local->cycles = EGUI_FLOAT_VALUE(1.0f);
}

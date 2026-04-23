#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_interpolator_anticipate.h"
#include "core/egui_common.h"
#include "core/egui_api.h"
#include "canvas/egui_canvas.h"

/**
 * @file egui_interpolator_anticipate.c
 * @brief Anticipate interpolator that briefly moves backward before heading toward the final value.
 */

/** Update the anticipate tension. Larger values increase the initial backward pull. */
void egui_interpolator_anticipate_tension_set(egui_interpolator_t *self, egui_float_t tension)
{
    EGUI_LOCAL_INIT(egui_interpolator_anticipate_t);
    local->tension = tension;
}

/** Apply the anticipate curve so early progress can dip below zero before accelerating forward. */
egui_float_t egui_interpolator_anticipate_get_interpolation(egui_interpolator_t *self, egui_float_t input)
{
    EGUI_LOCAL_INIT(egui_interpolator_anticipate_t);
    return (EGUI_FLOAT_MULT_LIMIT(EGUI_FLOAT_MULT_LIMIT(input, input), EGUI_FLOAT_MULT_LIMIT((local->tension + EGUI_FLOAT_VALUE(1.0f)), input)) -
            local->tension);
}

const egui_interpolator_api_t egui_interpolator_anticipate_t_api_table = {
        .get_interpolation = egui_interpolator_anticipate_get_interpolation,
};

/** Initialize the anticipate interpolator and install the default tension of 2.0. */
void egui_interpolator_anticipate_init(egui_interpolator_t *self)
{
    EGUI_LOCAL_INIT(egui_interpolator_anticipate_t);
    egui_interpolator_init(self);
    self->api = &egui_interpolator_anticipate_t_api_table;
    local->tension = EGUI_FLOAT_VALUE(2.0f);
}

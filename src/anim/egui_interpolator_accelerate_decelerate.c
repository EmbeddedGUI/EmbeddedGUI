#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_interpolator_accelerate_decelerate.h"
#include "core/egui_common.h"
#include "core/egui_api.h"
#include "canvas/egui_canvas.h"

/**
 * @file egui_interpolator_accelerate_decelerate.c
 * @brief Symmetric ease-in/ease-out interpolator based on a cosine curve.
 */

/** Ease in during the first half and ease out during the second half using one cosine wave segment. */
egui_float_t egui_interpolator_accelerate_decelerate_get_interpolation(egui_interpolator_t *self, egui_float_t input)
{
    EGUI_UNUSED(self);
    return (egui_float_t)(EGUI_FLOAT_DIV(EGUI_FLOAT_COS(EGUI_FLOAT_MULT((input + EGUI_FLOAT_VALUE(1.0f)), EGUI_FLOAT_PI)), EGUI_FLOAT_VALUE(2.0f))) +
           EGUI_FLOAT_VALUE(0.5f);
}

const egui_interpolator_api_t egui_interpolator_accelerate_decelerate_t_api_table = {
        .get_interpolation = egui_interpolator_accelerate_decelerate_get_interpolation,
};

/** Initialize the accelerate-decelerate interpolator by swapping in its cosine-based API table. */
void egui_interpolator_accelerate_decelerate_init(egui_interpolator_t *self)
{
    egui_interpolator_init(self);
    self->api = &egui_interpolator_accelerate_decelerate_t_api_table;
}

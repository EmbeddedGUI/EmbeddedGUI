#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_interpolator_linear.h"
#include "core/egui_common.h"
#include "core/egui_api.h"
#include "canvas/egui_canvas.h"

/**
 * @file egui_interpolator_linear.c
 * @brief Linear interpolator that leaves animation progress unchanged.
 */

/** Return the input fraction directly, producing a constant-speed animation curve. */
egui_float_t egui_interpolator_linear_get_interpolation(egui_interpolator_t *self, egui_float_t input)
{
    EGUI_UNUSED(self);
    return input;
}

const egui_interpolator_api_t egui_interpolator_linear_t_api_table = {
        .get_interpolation = egui_interpolator_linear_get_interpolation,
};

/** Initialize the concrete linear interpolator by swapping in its API table. */
void egui_interpolator_linear_init(egui_interpolator_t *self)
{
    egui_interpolator_init(self);
    self->api = &egui_interpolator_linear_t_api_table;
}

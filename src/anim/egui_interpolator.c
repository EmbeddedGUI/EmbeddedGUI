#include <stdio.h>
#include <assert.h>

#include "egui_interpolator.h"
#include "widget/egui_view.h"

/**
 * @file egui_interpolator.c
 * @brief Base interpolator implementation that leaves animation progress unchanged unless a subclass overrides it.
 */

/** Default interpolation: return the input fraction unchanged. */
egui_float_t egui_interpolator_get_interpolation(egui_interpolator_t *self, egui_float_t input)
{
    EGUI_UNUSED(self);
    return input;
}

const egui_interpolator_api_t egui_interpolator_t_api_table = {
        .get_interpolation = egui_interpolator_get_interpolation,
};

/** Initialize the base interpolator object with the default linear mapping API table. */
void egui_interpolator_init(egui_interpolator_t *self)
{
    self->api = &egui_interpolator_t_api_table;
}

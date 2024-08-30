#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_interpolator_accelerate_decelerate.h"
#include "core/egui_common.h"
#include "core/egui_api.h"
#include "core/egui_canvas.h"

egui_float_t egui_interpolator_accelerate_decelerate_get_interpolation(egui_interpolator_t *self, egui_float_t input)
{
    egui_interpolator_accelerate_decelerate_t *local = (egui_interpolator_accelerate_decelerate_t *)self;
    // (float)(Math.cos((input + 1) * Math.PI) / 2.0f) + 0.5f;
    return (egui_float_t)(EGUI_FLOAT_DIV(EGUI_FLOAT_COS(EGUI_FLOAT_MULT((input + EGUI_FLOAT_VALUE(1.0f)), EGUI_FLOAT_PI)), EGUI_FLOAT_VALUE(2.0f))) +
           EGUI_FLOAT_VALUE(0.5f);
}

// name must be _type##_api_table, it will be used by EGUI_VIEW_DEFINE to init api.
const egui_interpolator_api_t egui_interpolator_accelerate_decelerate_t_api_table = {
        .get_interpolation = egui_interpolator_accelerate_decelerate_get_interpolation,
};

void egui_interpolator_accelerate_decelerate_init(egui_interpolator_t *self)
{
    egui_interpolator_accelerate_decelerate_t *local = (egui_interpolator_accelerate_decelerate_t *)self;
    // call super init.
    egui_interpolator_init(self);
    // update api.
    self->api = &egui_interpolator_accelerate_decelerate_t_api_table;

    // init local data.
}

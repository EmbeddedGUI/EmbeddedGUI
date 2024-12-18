#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_interpolator_linear.h"
#include "core/egui_common.h"
#include "core/egui_api.h"
#include "core/egui_canvas.h"

egui_float_t egui_interpolator_linear_get_interpolation(egui_interpolator_t *self, egui_float_t input)
{
    return input;
}

// name must be _type##_api_table, it will be used by EGUI_VIEW_DEFINE to init api.
const egui_interpolator_api_t egui_interpolator_linear_t_api_table = {
        .get_interpolation = egui_interpolator_linear_get_interpolation,
};

void egui_interpolator_linear_init(egui_interpolator_t *self)
{
    egui_interpolator_linear_t *local = (egui_interpolator_linear_t *)self;
    // call super init.
    egui_interpolator_init(self);
    // update api.
    self->api = &egui_interpolator_linear_t_api_table;

    // init local data.
}

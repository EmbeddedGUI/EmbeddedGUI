#include <stdio.h>
#include <assert.h>

#include "egui_interpolator.h"
#include "widget/egui_view.h"

egui_float_t egui_interpolator_get_interpolation(egui_interpolator_t *self, egui_float_t input)
{
    return input;
}

const egui_interpolator_api_t egui_interpolator_t_api_table = {
        .get_interpolation = egui_interpolator_get_interpolation,
};

void egui_interpolator_init(egui_interpolator_t *self)
{
    // init api
    self->api = &egui_interpolator_t_api_table;
}

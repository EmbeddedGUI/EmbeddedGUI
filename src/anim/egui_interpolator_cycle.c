#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_interpolator_cycle.h"
#include "core/egui_common.h"
#include "core/egui_api.h"
#include "core/egui_canvas.h"

void egui_interpolator_cycle_cycles_set(egui_interpolator_t *self, egui_float_t cycles)
{
    egui_interpolator_cycle_t *local = (egui_interpolator_cycle_t *)self;
    local->cycles = cycles;
}

egui_float_t egui_interpolator_cycle_get_interpolation(egui_interpolator_t *self, egui_float_t input)
{
    egui_interpolator_cycle_t *local = (egui_interpolator_cycle_t *)self;
    // (float)(Math.sin(2 * mCycles * Math.PI * input));
    return EGUI_FLOAT_SIN(EGUI_FLOAT_MULT(2 * local->cycles, EGUI_FLOAT_MULT(input, EGUI_FLOAT_PI)));
}

// name must be _type##_api_table, it will be used by EGUI_VIEW_DEFINE to init api.
const egui_interpolator_api_t egui_interpolator_cycle_t_api_table = {
        .get_interpolation = egui_interpolator_cycle_get_interpolation,
};

void egui_interpolator_cycle_init(egui_interpolator_t *self)
{
    egui_interpolator_cycle_t *local = (egui_interpolator_cycle_t *)self;
    // call super init.
    egui_interpolator_init(self);
    // update api.
    self->api = &egui_interpolator_cycle_t_api_table;

    // init local data.
    local->cycles = EGUI_FLOAT_VALUE(1.0f);
}

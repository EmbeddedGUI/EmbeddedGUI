#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_interpolator_anticipate.h"
#include "core/egui_common.h"
#include "core/egui_api.h"
#include "core/egui_canvas.h"

void egui_interpolator_anticipate_tension_set(egui_interpolator_t *self, egui_float_t tension)
{
    egui_interpolator_anticipate_t *local = (egui_interpolator_anticipate_t *)self;
    local->tension = tension;
}

egui_float_t egui_interpolator_anticipate_get_interpolation(egui_interpolator_t *self, egui_float_t input)
{
    egui_interpolator_anticipate_t *local = (egui_interpolator_anticipate_t *)self;
    // a(t) = t * t * ((tension + 1) * t - tension)
    return (EGUI_FLOAT_MULT_LIMIT(EGUI_FLOAT_MULT_LIMIT(input, input), EGUI_FLOAT_MULT_LIMIT((local->tension + EGUI_FLOAT_VALUE(1.0f)), input)) -
            local->tension);
}

// name must be _type##_api_table, it will be used by EGUI_VIEW_DEFINE to init api.
const egui_interpolator_api_t egui_interpolator_anticipate_t_api_table = {
        .get_interpolation = egui_interpolator_anticipate_get_interpolation,
};

void egui_interpolator_anticipate_init(egui_interpolator_t *self)
{
    egui_interpolator_anticipate_t *local = (egui_interpolator_anticipate_t *)self;
    // call super init.
    egui_interpolator_init(self);
    // update api.
    self->api = &egui_interpolator_anticipate_t_api_table;

    // init local data.
    local->tension = EGUI_FLOAT_VALUE(2.0f);
}

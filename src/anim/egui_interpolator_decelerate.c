#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_interpolator_decelerate.h"
#include "core/egui_common.h"
#include "core/egui_api.h"
#include "core/egui_canvas.h"

void egui_interpolator_decelerate_factor_set(egui_interpolator_t *self, egui_float_t factor)
{
    egui_interpolator_decelerate_t *local = (egui_interpolator_decelerate_t *)self;
    local->factor = factor;
}

egui_float_t egui_interpolator_decelerate_get_interpolation(egui_interpolator_t *self, egui_float_t input)
{
    egui_interpolator_decelerate_t *local = (egui_interpolator_decelerate_t *)self;
    if (local->factor == EGUI_FLOAT_VALUE(1.0f))
    {
        // (float)(1.0f - (1.0f - input) * (1.0f - input));
        return EGUI_FLOAT_VALUE(1.0f) - EGUI_FLOAT_MULT_LIMIT(EGUI_FLOAT_VALUE(1.0f) - input, EGUI_FLOAT_VALUE(1.0f) - input);
    }
    else
    {
        // (float)(1.0f - Math.pow((1.0f - input), 2 * mFactor));
        return EGUI_FLOAT_VALUE(1.0f) - EGUI_FLOAT_POWER(EGUI_FLOAT_VALUE(1.0f) - input, local->factor * 2);
    }
}

// name must be _type##_api_table, it will be used by EGUI_VIEW_DEFINE to init api.
const egui_interpolator_api_t egui_interpolator_decelerate_t_api_table = {
        .get_interpolation = egui_interpolator_decelerate_get_interpolation,
};

void egui_interpolator_decelerate_init(egui_interpolator_t *self)
{
    egui_interpolator_decelerate_t *local = (egui_interpolator_decelerate_t *)self;
    // call super init.
    egui_interpolator_init(self);
    // update api.
    self->api = &egui_interpolator_decelerate_t_api_table;

    // init local data.
    local->factor = EGUI_FLOAT_VALUE(1.0f);
}

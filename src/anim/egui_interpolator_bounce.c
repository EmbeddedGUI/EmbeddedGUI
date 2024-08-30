#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_interpolator_bounce.h"
#include "core/egui_common.h"
#include "core/egui_api.h"
#include "core/egui_canvas.h"

static egui_float_t bounce(egui_float_t t)
{
    // t * t * 8.0f;
    return EGUI_FLOAT_MULT(EGUI_FLOAT_MULT(t, t), EGUI_FLOAT_VALUE(8.0f));
}

egui_float_t egui_interpolator_bounce_get_interpolation(egui_interpolator_t *self, egui_float_t t)
{
    egui_interpolator_bounce_t *local = (egui_interpolator_bounce_t *)self;
    // _b(t) = t * t * 8
    // bs(t) = _b(t) for t < 0.3535
    // bs(t) = _b(t - 0.54719) + 0.7 for t < 0.7408
    // bs(t) = _b(t - 0.8526) + 0.9 for t < 0.9644
    // bs(t) = _b(t - 1.0435) + 0.95 for t <= 1.0
    // b(t) = bs(t * 1.1226)

    t = EGUI_FLOAT_MULT(t, EGUI_FLOAT_VALUE(1.1226f));
    if (t < EGUI_FLOAT_VALUE(0.3535f))
        return bounce(t);
    else if (t < EGUI_FLOAT_VALUE(0.7408f))
        return bounce(t - EGUI_FLOAT_VALUE(0.54719f)) + EGUI_FLOAT_VALUE(0.7f);
    else if (t < EGUI_FLOAT_VALUE(0.9644f))
        return bounce(t - EGUI_FLOAT_VALUE(0.8526f)) + EGUI_FLOAT_VALUE(0.9f);
    else
        return bounce(t - EGUI_FLOAT_VALUE(1.0435f)) + EGUI_FLOAT_VALUE(0.95f);
}

// name must be _type##_api_table, it will be used by EGUI_VIEW_DEFINE to init api.
const egui_interpolator_api_t egui_interpolator_bounce_t_api_table = {
        .get_interpolation = egui_interpolator_bounce_get_interpolation,
};

void egui_interpolator_bounce_init(egui_interpolator_t *self)
{
    egui_interpolator_bounce_t *local = (egui_interpolator_bounce_t *)self;
    // call super init.
    egui_interpolator_init(self);
    // update api.
    self->api = &egui_interpolator_bounce_t_api_table;

    // init local data.
}

#include <string.h>

#include "egui_background_gradient.h"
#include "core/egui_common.h"
#include "core/egui_api.h"
#include "core/egui_canvas.h"

void egui_background_gradient_on_draw(egui_background_t *self, egui_region_t *region, const void *param)
{
    EGUI_LOCAL_INIT(egui_background_gradient_t);
    const egui_background_gradient_param_t *grad_param = param;

    egui_dim_t w = region->size.width;
    egui_dim_t h = region->size.height;

    /* Delegate to canvas gradient for unified implementation */
    egui_gradient_stop_t stops[2] = {
            {.position = 0, .color = grad_param->start_color},
            {.position = 255, .color = grad_param->end_color},
    };
    egui_gradient_t gradient = {
            .type = (grad_param->direction == EGUI_BACKGROUND_GRADIENT_DIR_VERTICAL) ? EGUI_GRADIENT_TYPE_LINEAR_VERTICAL
                                                                                     : EGUI_GRADIENT_TYPE_LINEAR_HORIZONTAL,
            .stop_count = 2,
            .alpha = grad_param->alpha,
            .stops = stops,
    };
    egui_canvas_draw_rectangle_fill_gradient(0, 0, w, h, &gradient);
}

const egui_background_api_t egui_background_gradient_t_api_table = {
        .draw = egui_background_draw,
        .on_draw = egui_background_gradient_on_draw,
};

void egui_background_gradient_init(egui_background_t *self)
{
    EGUI_LOCAL_INIT(egui_background_gradient_t);
    egui_background_init(self);
    self->api = &egui_background_gradient_t_api_table;
}

void egui_background_gradient_init_with_params(egui_background_t *self, const egui_background_params_t *params)
{
    egui_background_gradient_init(self);
    egui_background_set_params(self, params);
}

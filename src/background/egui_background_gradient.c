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

#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_GRADIENT
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
#else
    if (grad_param->direction == EGUI_BACKGROUND_GRADIENT_DIR_VERTICAL)
    {
        /* Draw vertical gradient: one hline per row */
        for (egui_dim_t y = 0; y < h; y++)
        {
            /* alpha = y * 255 / (h - 1), clamped */
            egui_alpha_t mix = (h > 1) ? (egui_alpha_t)((uint32_t)y * 255 / (h - 1)) : 0;
            egui_color_t line_color;
            egui_rgb_mix_ptr((egui_color_t *)&grad_param->start_color, (egui_color_t *)&grad_param->end_color, &line_color, mix);
            egui_canvas_draw_rectangle_fill(0, y, w, 1, line_color, grad_param->alpha);
        }
    }
    else /* HORIZONTAL */
    {
        /* Draw horizontal gradient: one vline per column */
        for (egui_dim_t x = 0; x < w; x++)
        {
            egui_alpha_t mix = (w > 1) ? (egui_alpha_t)((uint32_t)x * 255 / (w - 1)) : 0;
            egui_color_t col_color;
            egui_rgb_mix_ptr((egui_color_t *)&grad_param->start_color, (egui_color_t *)&grad_param->end_color, &col_color, mix);
            egui_canvas_draw_rectangle_fill(x, 0, 1, h, col_color, grad_param->alpha);
        }
    }
#endif
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

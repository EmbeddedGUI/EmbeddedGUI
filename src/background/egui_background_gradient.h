#ifndef _EGUI_BACKGROUND_GRADIENT_H_
#define _EGUI_BACKGROUND_GRADIENT_H_

#include "egui_background.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_BACKGROUND_GRADIENT_DIR_VERTICAL   0
#define EGUI_BACKGROUND_GRADIENT_DIR_HORIZONTAL 1

#define EGUI_BACKGROUND_GRADIENT_PARAM_INIT(_name, _dir, _start_color, _end_color, _alpha)                                                                     \
    static const egui_background_gradient_param_t _name = {.direction = _dir, .alpha = _alpha, .start_color = _start_color, .end_color = _end_color}

extern const egui_background_api_t egui_background_gradient_t_api_table;

#define EGUI_BACKGROUND_GRADIENT_STATIC_CONST_INIT(_name, _params)                                                                                             \
    static const egui_background_gradient_t _name = {.base = {.api = &egui_background_gradient_t_api_table, .params = _params}}

typedef struct egui_background_gradient_param egui_background_gradient_param_t;
struct egui_background_gradient_param
{
    uint8_t direction;
    egui_alpha_t alpha;
    egui_color_t start_color;
    egui_color_t end_color;
};

typedef struct egui_background_gradient egui_background_gradient_t;
struct egui_background_gradient
{
    egui_background_t base;
};

void egui_background_gradient_on_draw(egui_background_t *self, egui_region_t *region, const void *param);
void egui_background_gradient_init(egui_background_t *self);
void egui_background_gradient_init_with_params(egui_background_t *self, const egui_background_params_t *params);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_BACKGROUND_GRADIENT_H_ */

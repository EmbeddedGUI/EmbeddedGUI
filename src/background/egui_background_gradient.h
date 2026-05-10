#ifndef _EGUI_BACKGROUND_GRADIENT_H_
#define _EGUI_BACKGROUND_GRADIENT_H_

#include "egui_background.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Draw a top-to-bottom two-stop linear gradient. */
#define EGUI_BACKGROUND_GRADIENT_DIR_VERTICAL   0
/** Draw a left-to-right two-stop linear gradient. */
#define EGUI_BACKGROUND_GRADIENT_DIR_HORIZONTAL 1

/** Build one two-stop gradient parameter block. */
#define EGUI_BACKGROUND_GRADIENT_PARAM_INIT_COLOR(_name, _dir, _start_color, _end_color, _alpha)                                                               \
    static const egui_background_gradient_param_t _name = {.direction = _dir, .alpha = _alpha, .start_color = _start_color, .end_color = _end_color}

#if defined(_MSC_VER)
#define EGUI_BACKGROUND_GRADIENT_PARAM_INIT(_name, _dir, _start_color, _end_color, _alpha)                                                                     \
    EGUI_BACKGROUND_GRADIENT_PARAM_INIT_COLOR(_name, _dir, _start_color##_INIT, _end_color##_INIT, _alpha)
#else
#define EGUI_BACKGROUND_GRADIENT_PARAM_INIT(_name, _dir, _start_color, _end_color, _alpha)                                                                     \
    EGUI_BACKGROUND_GRADIENT_PARAM_INIT_COLOR(_name, _dir, (_start_color), (_end_color), _alpha)
#endif

extern const egui_background_api_t egui_background_gradient_t_api_table;

/** Build one static gradient-background object that reuses a shared parameter table. */
#define EGUI_BACKGROUND_GRADIENT_STATIC_CONST_INIT(_name, _params)                                                                                             \
    static const egui_background_gradient_t _name = {.base = {.api = &egui_background_gradient_t_api_table, .params = _params}}

typedef struct egui_background_gradient_param egui_background_gradient_param_t;

/**
 * @brief Parameter block for one background gradient state.
 */
struct egui_background_gradient_param
{
    uint8_t direction;
    egui_alpha_t alpha;
    egui_color_t start_color;
    egui_color_t end_color;
};

typedef struct egui_background_gradient egui_background_gradient_t;

/**
 * @brief Concrete background subtype that renders simple two-stop gradients.
 */
struct egui_background_gradient
{
    egui_background_t base;
};

/**
 * @brief Draw the selected two-stop gradient inside the supplied region.
 */
void egui_background_gradient_on_draw(egui_background_t *self, egui_canvas_t *canvas, egui_region_t *region, const void *param);

/**
 * @brief Initialize a gradient-backed background implementation.
 */
void egui_background_gradient_init(egui_background_t *self);

/**
 * @brief Initialize a gradient-backed background and immediately bind its state table.
 */
void egui_background_gradient_init_with_params(egui_background_t *self, const egui_background_params_t *params);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_BACKGROUND_GRADIENT_H_ */

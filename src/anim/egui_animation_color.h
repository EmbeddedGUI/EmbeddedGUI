#ifndef _EGUI_ANIMATION_COLOR_H_
#define _EGUI_ANIMATION_COLOR_H_

#include "egui_animation.h"
#include "core/egui_canvas.h"
#include "core/egui_region.h"
#include "core/egui_timer.h"
#include "utils/egui_slist.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Color animation: interpolates between two colors (R/G/B per-channel).
 * Applies the result to the target view's label font color.
 */
#define EGUI_ANIMATION_COLOR_PARAMS_INIT(_name, _from_color, _to_color)                                                                                        \
    static const egui_animation_color_params_t _name = {                                                                                                       \
            .from_color = _from_color,                                                                                                                         \
            .to_color = _to_color,                                                                                                                             \
    }

typedef struct egui_animation_color_params egui_animation_color_params_t;
struct egui_animation_color_params
{
    egui_color_t from_color; // start color
    egui_color_t to_color;   // end color
};

typedef struct egui_animation_color egui_animation_color_t;
struct egui_animation_color
{
    egui_animation_t base;

    const egui_animation_color_params_t *params;
};

void egui_animation_color_params_set(egui_animation_color_t *self, const egui_animation_color_params_t *params);
void egui_animation_color_init(egui_animation_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_ANIMATION_COLOR_H_ */

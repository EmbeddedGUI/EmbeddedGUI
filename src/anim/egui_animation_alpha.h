#ifndef _EGUI_ANIMATION_ALPHA_H_
#define _EGUI_ANIMATION_ALPHA_H_

#include "egui_animation.h"
#include "core/egui_canvas.h"
#include "core/egui_region.h"
#include "core/egui_timer.h"
#include "utils/egui_slist.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_ANIMATION_ALPHA_PARAMS_INIT(_name, _from_alpha, _to_alpha)                                                                                        \
    static const egui_animation_alpha_params_t _name = {                                                                                                       \
            .from_alpha = _from_alpha,                                                                                                                         \
            .to_alpha = _to_alpha,                                                                                                                             \
    }

typedef struct egui_animation_alpha_params egui_animation_alpha_params_t;
struct egui_animation_alpha_params
{
    egui_alpha_t from_alpha; // start value of the animation
    egui_alpha_t to_alpha;   // end value of the animation
};

typedef struct egui_animation_alpha egui_animation_alpha_t;
struct egui_animation_alpha
{
    egui_animation_t base;

    const egui_animation_alpha_params_t *params; // type of interpolator
};

void egui_animation_alpha_params_set(egui_animation_alpha_t *self, const egui_animation_alpha_params_t *params);
void egui_animation_alpha_init(egui_animation_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_ANIMATION_ALPHA_H_ */

#ifndef _EGUI_ANIMATION_TRANSLATE_H_
#define _EGUI_ANIMATION_TRANSLATE_H_

#include "egui_animation.h"
#include "canvas/egui_canvas.h"
#include "core/egui_region.h"
#include "core/egui_timer.h"
#include "utils/egui_slist.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Build a static translate-animation parameter block from one start offset and end offset. */
#define EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(_name, _from_x, _to_x, _from_y, _to_y)                                                                            \
    static const egui_animation_translate_params_t _name = {                                                                                                   \
            .from_x = _from_x,                                                                                                                                 \
            .to_x = _to_x,                                                                                                                                     \
            .from_y = _from_y,                                                                                                                                 \
            .to_y = _to_y,                                                                                                                                     \
    }

typedef struct egui_animation_translate_params egui_animation_translate_params_t;
struct egui_animation_translate_params
{
    egui_dim_t from_x; // start value of the animation
    egui_dim_t to_x;   // end value of the animation
    egui_dim_t from_y; // start value of the animation
    egui_dim_t to_y;   // end value of the animation
};

typedef struct egui_animation_translate egui_animation_translate_t;
struct egui_animation_translate
{
    egui_animation_t base;

    egui_dim_t current_x; // current value of the animation
    egui_dim_t current_y; // current value of the animation

    const egui_animation_translate_params_t *params; // borrowed animation parameters
};

/** Reset the accumulated delta state before the first translate update. Mostly used by the animation framework. */
void egui_animation_translate_start(egui_animation_t *self);
/** Bind the start/end translation offsets used by this animation. The parameter pointer is borrowed. */
void egui_animation_translate_params_set(egui_animation_translate_t *self, const egui_animation_translate_params_t *params);
/** Initialize an animation that moves the target view by incremental scroll offsets. */
void egui_animation_translate_init(egui_animation_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_ANIMATION_TRANSLATE_H_ */

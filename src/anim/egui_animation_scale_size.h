#ifndef _EGUI_ANIMATION_SCALE_SIZE_H_
#define _EGUI_ANIMATION_SCALE_SIZE_H_

#include "egui_animation.h"
#include "canvas/egui_canvas.h"
#include "core/egui_region.h"
#include "core/egui_timer.h"
#include "utils/egui_slist.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Build a static scale-size parameter block from one start scale and end scale. */
#define EGUI_ANIMATION_SCALE_SIZE_PARAMS_INIT(_name, _from_scale, _to_scale)                                                                                   \
    static const egui_animation_scale_size_params_t _name = {                                                                                                  \
            .from_scale = _from_scale,                                                                                                                         \
            .to_scale = _to_scale,                                                                                                                             \
    }

typedef struct egui_animation_scale_size_params egui_animation_scale_size_params_t;
struct egui_animation_scale_size_params
{
    egui_float_t from_scale; // start value of the animation
    egui_float_t to_scale;   // end value of the animation
};

typedef struct egui_animation_scale_size egui_animation_scale_size_t;
struct egui_animation_scale_size
{
    egui_animation_t base;

    egui_region_t origin_region; // original target region captured on start

    const egui_animation_scale_size_params_t *params; // borrowed animation parameters
};

/** Snapshot the current target region before the first scale-size update. Mostly used by the animation framework. */
void egui_animation_scale_size_start(egui_animation_t *self);
/** Bind the start/end scale factors used by this animation. The parameter pointer is borrowed. */
void egui_animation_scale_size_params_set(egui_animation_scale_size_t *self, const egui_animation_scale_size_params_t *params);
/** Initialize an animation that scales a view around its center using the captured start region. */
void egui_animation_scale_size_init(egui_animation_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_ANIMATION_SCALE_SIZE_H_ */

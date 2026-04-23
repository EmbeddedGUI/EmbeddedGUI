#ifndef _EGUI_ANIMATION_SET_H_
#define _EGUI_ANIMATION_SET_H_

#include "egui_animation.h"
#include "canvas/egui_canvas.h"
#include "core/egui_region.h"
#include "core/egui_timer.h"
#include "utils/egui_slist.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Animation set that starts and tracks multiple child animations as one group. */
typedef struct egui_animation_set egui_animation_set_t;
struct egui_animation_set
{
    egui_animation_t base;

    uint8_t is_mask_repeat_count : 1;
    uint8_t is_mask_repeat_mode : 1;
    uint8_t is_mask_duration : 1;
    uint8_t is_mask_target_view : 1;
    uint8_t is_mask_interpolator : 1;

    egui_slist_t childs; // list of animation
};

/** Choose which base properties from the set should be copied into each child before it starts. */
void egui_animation_set_set_mask(egui_animation_set_t *self, int is_mask_repeat_count, int is_mask_repeat_mode, int is_mask_duration, int is_mask_target_view,
                                 int is_mask_interpolator);
/** Append one child animation to the set. Added children are marked as being owned by an animation set. */
void egui_animation_set_add_animation(egui_animation_set_t *self, egui_animation_t *anim);
/** Initialize an animation set that starts and monitors multiple child animations together. */
void egui_animation_set_init(egui_animation_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_ANIMATION_SET_H_ */

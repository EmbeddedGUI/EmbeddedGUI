#ifndef _EGUI_ANIMATION_SET_H_
#define _EGUI_ANIMATION_SET_H_

#include "egui_animation.h"
#include "core/egui_canvas.h"
#include "core/egui_region.h"
#include "core/egui_timer.h"
#include "utils/egui_slist.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

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

void egui_animation_set_set_mask(egui_animation_set_t *self, int is_mask_repeat_count, int is_mask_repeat_mode, int is_mask_duration, int is_mask_target_view,
                                 int is_mask_interpolator);
void egui_animation_set_add_animation(egui_animation_set_t *self, egui_animation_t *anim);
void egui_animation_set_init(egui_animation_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_ANIMATION_SET_H_ */

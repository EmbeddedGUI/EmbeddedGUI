#ifndef _EGUI_ANIMATION_VALUE_H_
#define _EGUI_ANIMATION_VALUE_H_

#include "egui_animation.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Callback type invoked on each animation frame with the current interpolated value. */
typedef void (*egui_animation_value_on_value_t)(egui_animation_t *self, int32_t value);

typedef struct egui_animation_value egui_animation_value_t;
/**
 * @brief Generic integer value animator, equivalent to LVGL lv_anim_t with set_values / set_exec_cb.
 *
 * Interpolates an int32_t from `from` to `to` over the animation duration and fires
 * `on_value` each frame.  The caller is responsible for applying the value to the
 * target (e.g. move a widget, update a progress bar, rotate an image).
 *
 * Usage:
 *   egui_animation_value_init(EGUI_ANIM_OF(&anim));
 *   egui_animation_value_set_range(&anim, 0, 100);
 *   egui_animation_value_set_on_value(&anim, my_callback);
 *   egui_animation_target_view_set(EGUI_ANIM_OF(&anim), view);
 *   egui_animation_duration_set(EGUI_ANIM_OF(&anim), 500);
 *   egui_animation_start(EGUI_ANIM_OF(&anim));
 */
struct egui_animation_value
{
    egui_animation_t base;

    int32_t from;    /* Start value of the range. */
    int32_t to;      /* End value of the range. */
    int32_t current; /* Last emitted value (not directly usable; kept for dedup). */

    egui_animation_value_on_value_t on_value; /* Callback invoked each frame with the current value. */
};

/** Set the start and end values for the animation. */
void egui_animation_value_set_range(egui_animation_value_t *self, int32_t from, int32_t to);
/** Register the per-frame callback that receives the interpolated value. */
void egui_animation_value_set_on_value(egui_animation_value_t *self, egui_animation_value_on_value_t cb);
/** Initialize the value animator and install its update callbacks. */
void egui_animation_value_init(egui_animation_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_ANIMATION_VALUE_H_ */

#ifndef _EGUI_ANIMATION_H_
#define _EGUI_ANIMATION_H_

#include "canvas/egui_canvas.h"
#include "core/egui_region.h"
#include "core/egui_timer.h"
#include "utils/egui_slist.h"
#include "egui_interpolator.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

enum
{
    EGUI_ANIMATION_REPEAT_MODE_RESTART,
    EGUI_ANIMATION_REPEAT_MODE_REVERSE,
};

typedef struct egui_animation_api egui_animation_api_t;
struct egui_animation_api
{
    void (*on_start)(egui_animation_t *self);
    void (*update)(egui_animation_t *self, uint32_t current_time);
    void (*on_update)(egui_animation_t *self, egui_float_t fraction);
};

typedef struct egui_animation_handle egui_animation_handle_t;
struct egui_animation_handle
{
    void (*start)(egui_animation_t *self);
    void (*repeat)(egui_animation_t *self);
    void (*end)(egui_animation_t *self);
};

struct egui_animation
{
    egui_snode_t node; // used for linked list

    uint8_t is_running : 1;          // is the animation running?
    uint8_t is_started : 1;          // is the animation started?
    uint8_t is_ended : 1;            // is the animation ended?
    uint8_t is_cycle_flip : 1;       // is the animation cycle flip?
    uint8_t is_fill_before : 1;      // is the animation end with the first value?
    uint8_t is_fill_after : 1;       // is the animation start with the last value?
    uint8_t is_inside_animation : 1; // is the animation in animation set?
    int8_t repeat_count;             // The number of times the animation must repeat. By default, an animation repeats indefinitely.
    int8_t repeated;                 // Indicates how many times the animation was repeated.
    uint8_t repeat_mode;             // The behavior of the animation when it repeats. The repeat mode is either EGUI_ANIMATION_REPEAT_MODE_RESTART,
                                     // EGUI_ANIMATION_REPEAT_MODE_REVERSE.

    uint16_t duration; // duration of the animation in milliseconds

#if EGUI_CONFIG_FUNCTION_ANIM_DELAY
    uint16_t delay_ms;           /* delay before the animation begins after egui_animation_start() */
    uint8_t is_delay_passed : 1; /* internal: 1 once the delay has elapsed */
#endif

    uint32_t start_time;      // start time of the animation in milliseconds
    egui_view_t *target_view; // target view of the animation
    egui_core_t *queue_core;  // core that currently owns this animation in scene.anims

    egui_interpolator_t *interpolator; // interpolator used for the animation

    const egui_animation_handle_t *handle; // api of the view
    const egui_animation_api_t *api;       // api of the view

#if EGUI_CONFIG_FUNCTION_ANIM_COMPLETE_CB
    /** Optional simple completion callback invoked once when the animation ends. */
    void (*on_complete_cb)(struct egui_animation *self, void *user_data);
    void *on_complete_user_data;
#endif

#if EGUI_CONFIG_FUNCTION_ANIM_PAUSE_RESUME
    uint8_t is_paused : 1;  /* 1 while the animation is suspended by pause() */
    uint32_t pause_elapsed; /* ms elapsed since start when pause() was called */
#endif
};

/** Invoke the optional start callback handle. Mostly used by the animation system itself. */
void egui_animation_notify_start(egui_animation_t *self);
/** Apply fill behavior and invoke the optional end callback handle. */
void egui_animation_notify_end(egui_animation_t *self);
/** Invoke the optional repeat callback handle after one cycle completes. */
void egui_animation_notify_repeat(egui_animation_t *self);
/** Set how repeated cycles continue: restart from the beginning or reverse direction. */
void egui_animation_repeat_mode_set(egui_animation_t *self, uint8_t repeat_mode);
/** Return the configured repeat mode. Returns 0 when self is NULL. */
uint8_t egui_animation_repeat_mode_get(egui_animation_t *self);
/** Set how many extra cycles run after the first play. Use 0 for a single play only. */
void egui_animation_repeat_count_set(egui_animation_t *self, uint8_t repeat_count);
/** Return the configured repeat count. */
uint8_t egui_animation_repeat_count_get(egui_animation_t *self);
/** Install optional lifecycle callbacks for start, repeat, and end events. */
void egui_animation_handle_set(egui_animation_t *self, const egui_animation_handle_t *handle);
/** Set the duration of one animation cycle in milliseconds. */
void egui_animation_duration_set(egui_animation_t *self, uint16_t duration);
/** Return the configured duration of one animation cycle in milliseconds. */
uint16_t egui_animation_duration_get(egui_animation_t *self);
/** Set the interpolator used to remap the normalized progress fraction. */
void egui_animation_interpolator_set(egui_animation_t *self, egui_interpolator_t *interpolator);
/** Return the interpolator currently set on the animation, or NULL when none is set or self is NULL. */
egui_interpolator_t *egui_animation_interpolator_get(egui_animation_t *self);
/** Bind the animation to a target view so it can join the correct core animation queue. */
void egui_animation_target_view_set(egui_animation_t *self, egui_view_t *view);
/** Return the target view bound to this animation, or NULL when none is set or self is NULL. */
egui_view_t *egui_animation_target_view_get(egui_animation_t *self);
/** End the animation at the starting value when it finishes. */
void egui_animation_is_fill_before_set(egui_animation_t *self, int is_fill_before);
/** Return 1 if the animation is configured to end at the starting value, 0 otherwise. */
uint8_t egui_animation_is_fill_before_get(egui_animation_t *self);
/** End the animation at the final value when it finishes. */
void egui_animation_is_fill_after_set(egui_animation_t *self, int is_fill_after);
/** Return 1 if the animation is configured to end at the final value, 0 otherwise. */
uint8_t egui_animation_is_fill_after_get(egui_animation_t *self);
/** Start the animation and append it to the owning core queue if needed. */
void egui_animation_start(egui_animation_t *self);
/** Jump to the final state of the current cycle and finish immediately. */
void egui_animation_complete(egui_animation_t *self);
/** Stop the animation without forcing an end-state update. */
void egui_animation_stop(egui_animation_t *self);
/** Advance the animation with the current timestamp. Normally called by the core. */
void egui_animation_update(egui_animation_t *self, uint32_t current_time);
/** Initialize the base animation fields before applying subclass-specific settings. */
void egui_animation_init(egui_animation_t *self);
/** Return 1 if the animation is currently running, 0 otherwise. */
uint8_t egui_animation_is_running(egui_animation_t *self);

#if EGUI_CONFIG_FUNCTION_ANIM_PAUSE_RESUME
/** Suspend the animation mid-flight; elapsed time is preserved for resume(). */
void egui_animation_pause(egui_animation_t *self);
/** Resume a paused animation from where it was suspended. */
void egui_animation_resume(egui_animation_t *self);
/** Return 1 if the animation is currently paused, 0 otherwise. */
uint8_t egui_animation_is_paused(egui_animation_t *self);
#endif

#if EGUI_CONFIG_FUNCTION_ANIM_DELAY
/** Set a delay (ms) applied between egui_animation_start() and the first visual update. */
void egui_animation_set_delay(egui_animation_t *self, uint16_t delay_ms);
/** Return the configured start delay in milliseconds. */
uint16_t egui_animation_get_delay(egui_animation_t *self);
#endif

#if EGUI_CONFIG_FUNCTION_ANIM_COMPLETE_CB
/** Typedef for the simple animation completion callback. */
typedef void (*egui_animation_on_complete_cb_t)(egui_animation_t *self, void *user_data);
/** Register a callback invoked once when the animation ends (after fill-after update). */
void egui_animation_set_on_complete(egui_animation_t *self, egui_animation_on_complete_cb_t cb, void *user_data);
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_ANIMATION_H_ */

#ifndef _EGUI_ANIMATION_H_
#define _EGUI_ANIMATION_H_

#include "core/egui_canvas.h"
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

typedef struct egui_view egui_view_t;

typedef struct egui_animation egui_animation_t;

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

    uint32_t start_time;      // start time of the animation in milliseconds
    egui_view_t *target_view; // target view of the animation

    egui_interpolator_t *interpolator; // interpolator used for the animation

    const egui_animation_handle_t *handle; // api of the view
    const egui_animation_api_t *api;       // api of the view
};

void egui_animation_notify_start(egui_animation_t *self);
void egui_animation_notify_end(egui_animation_t *self);
void egui_animation_notify_repeat(egui_animation_t *self);
void egui_animation_repeat_mode_set(egui_animation_t *self, uint8_t repeat_mode);
void egui_animation_repeat_count_set(egui_animation_t *self, uint8_t repeat_count);
void egui_animation_handle_set(egui_animation_t *self, const egui_animation_handle_t *handle);
void egui_animation_duration_set(egui_animation_t *self, uint16_t duration);
void egui_animation_interpolator_set(egui_animation_t *self, egui_interpolator_t *interpolator);
void egui_animation_target_view_set(egui_animation_t *self, egui_view_t *view);
void egui_animation_is_fill_before_set(egui_animation_t *self, int is_fill_before);
void egui_animation_is_fill_after_set(egui_animation_t *self, int is_fill_after);
void egui_animation_start(egui_animation_t *self);
void egui_animation_stop(egui_animation_t *self);
void egui_animation_update(egui_animation_t *self, uint32_t current_time);
void egui_animation_init(egui_animation_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_ANIMATION_H_ */

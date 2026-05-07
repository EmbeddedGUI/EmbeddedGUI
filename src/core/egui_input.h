#ifndef _EGUI_INPUT_H_
#define _EGUI_INPUT_H_

#include "egui_common.h"
#include "utils/egui_slist.h"
#include "utils/simple_ringbuffer/simple_pool.h"
#include "egui_motion_event.h"
#include "egui_velocity_tracker.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
#include "egui_key_event.h"
#endif

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_input egui_input_t;
/** Per-core queued input state reused by touch polling and key dispatch. */
struct egui_input
{
    egui_slist_t motion_list; // queued motion events in screen coordinates

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    simple_pool_t motion_pool;                                            // fixed-capacity pool backing motion-event allocation
    void *motion_pool_fifo_storage[EGUI_CONFIG_INPUT_MOTION_CACHE_COUNT]; // pool queue storage for free motion-event nodes
    uint8_t motion_pool_data_storage[EGUI_CONFIG_INPUT_MOTION_CACHE_COUNT][MROUND(sizeof(egui_motion_event_t))]; // raw storage for motion events
#endif

#if EGUI_CONFIG_FUNCTION_INPUT_VELOCITY_TRACKER
    egui_velocity_tracker_t velocity_tracker; // recent motion history used to estimate fling velocity
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
    egui_slist_t key_list;                                                                              // key event queue
    simple_pool_t key_pool;                                                                             // fixed-capacity pool backing key-event allocation
    void *key_pool_fifo_storage[EGUI_CONFIG_INPUT_KEY_CACHE_COUNT];                                     // pool queue storage for free key-event nodes
    uint8_t key_pool_data_storage[EGUI_CONFIG_INPUT_KEY_CACHE_COUNT][MROUND(sizeof(egui_key_event_t))]; // raw storage for key events
#endif
};

/** Queue one single-pointer motion event. Returns non-zero when the event was queued or merged successfully. */
int egui_input_add_motion(egui_core_t *core, uint8_t type, egui_dim_t x, egui_dim_t y);
/** Get the last computed X velocity from the optional velocity tracker. Returns `0` when tracking is disabled. */
egui_float_t egui_input_get_velocity_x(egui_core_t *core);
/** Get the last computed Y velocity from the optional velocity tracker. Returns `0` when tracking is disabled. */
egui_float_t egui_input_get_velocity_y(egui_core_t *core);
/** Check whether the motion-event queue is currently empty. */
int egui_input_check_idle(egui_core_t *core);
/** Poll the touch driver once, then drain and dispatch every queued motion event into the core. */
void egui_input_polling_work(egui_core_t *core);
/** Initialize motion queues, pools, previous-touch state, and optional velocity tracking. */
void egui_input_init(egui_core_t *core);

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
/** Queue one multi-pointer motion event from an array of screen-coordinate locations. */
int egui_input_add_motion_points(egui_core_t *core, uint8_t type, uint8_t pointer_count, const egui_location_t *locations);
/** Queue one scroll-wheel style event carrying the wheel delta in `delta`. */
int egui_input_add_scroll(egui_core_t *core, egui_dim_t x, egui_dim_t y, int16_t delta);
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
/** Queue one key event with optional Shift/Ctrl modifiers. Returns non-zero when storage was available. */
int egui_input_add_key(egui_core_t *core, uint8_t type, uint8_t key_code, uint8_t is_shift, uint8_t is_ctrl);
/** Check whether the queued key-event list is currently empty. */
int egui_input_check_key_idle(egui_core_t *core);
/** Drain queued key events and dispatch them into the core in FIFO order. */
void egui_input_key_dispatch_work(egui_core_t *core);
/** Initialize key-event queues and fixed-capacity key-event storage. */
void egui_input_key_init(egui_core_t *core);
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_INPUT_H_ */

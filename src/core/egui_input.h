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
struct egui_input
{
    egui_slist_t motion_list; // the position in screen coordinates

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    simple_pool_t motion_pool;
    void *motion_pool_fifo_storage[EGUI_CONFIG_INPUT_MOTION_CACHE_COUNT];
    uint8_t motion_pool_data_storage[EGUI_CONFIG_INPUT_MOTION_CACHE_COUNT][MROUND(sizeof(egui_motion_event_t))];
#endif

#if EGUI_CONFIG_INPUT_VELOCITY_TRACKER_ENABLE
    egui_velocity_tracker_t velocity_tracker;
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
    egui_slist_t key_list; // key event queue
    simple_pool_t key_pool;
    void *key_pool_fifo_storage[EGUI_CONFIG_INPUT_KEY_CACHE_COUNT];
    uint8_t key_pool_data_storage[EGUI_CONFIG_INPUT_KEY_CACHE_COUNT][MROUND(sizeof(egui_key_event_t))];
#endif
};

int egui_input_add_motion(egui_core_t *core, uint8_t type, egui_dim_t x, egui_dim_t y);
egui_float_t egui_input_get_velocity_x(egui_core_t *core);
egui_float_t egui_input_get_velocity_y(egui_core_t *core);
int egui_input_check_idle(egui_core_t *core);
void egui_input_polling_work(egui_core_t *core);
void egui_input_init(egui_core_t *core);

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
int egui_input_add_motion_multi(egui_core_t *core, uint8_t type, uint8_t pointer_count, egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2);
int egui_input_add_scroll(egui_core_t *core, egui_dim_t x, egui_dim_t y, int16_t delta);
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
int egui_input_add_key(egui_core_t *core, uint8_t type, uint8_t key_code, uint8_t is_shift, uint8_t is_ctrl);
int egui_input_check_key_idle(egui_core_t *core);
void egui_input_key_dispatch_work(egui_core_t *core);
void egui_input_key_init(egui_core_t *core);
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_INPUT_H_ */

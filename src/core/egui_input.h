#ifndef _EGUI_INPUT_H_
#define _EGUI_INPUT_H_

#include "egui_common.h"
#include "utils/egui_slist.h"
#include "egui_motion_event.h"
#include "egui_velocity_tracker.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_input egui_input_t;
struct egui_input
{
    egui_slist_t motion_list; // the position in screen coordinates

#if EGUI_CONFIG_INPUT_VELOCITY_TRACKER_ENABLE
    egui_velocity_tracker_t velocity_tracker;
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
    egui_slist_t key_list; // key event queue
#endif
};

int egui_input_add_motion(uint8_t type, egui_dim_t x, egui_dim_t y);
egui_float_t egui_input_get_velocity_x(void);
egui_float_t egui_input_get_velocity_y(void);
int egui_input_check_idle(void);
void egui_input_polling_work(void);
void egui_input_init(void);

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
int egui_input_add_motion_multi(uint8_t type, uint8_t pointer_count, egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2);
int egui_input_add_scroll(egui_dim_t x, egui_dim_t y, int16_t delta);
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
int egui_input_add_key(uint8_t type, uint8_t key_code, uint8_t is_shift, uint8_t is_ctrl);
int egui_input_check_key_idle(void);
void egui_input_key_dispatch_work(void);
void egui_input_key_init(void);
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_INPUT_H_ */

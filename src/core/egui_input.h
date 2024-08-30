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
    egui_velocity_tracker_t velocity_tracker;
};

int egui_input_add_motion(uint8_t type, egui_dim_t x, egui_dim_t y);
egui_float_t egui_input_get_velocity_x(void);
egui_float_t egui_input_get_velocity_y(void);
int egui_input_check_idle(void);
void egui_input_polling_work(void);
void egui_input_init(void);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_INPUT_H_ */

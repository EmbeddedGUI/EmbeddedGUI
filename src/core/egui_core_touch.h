#ifndef _EGUI_CORE_TOUCH_H_
#define _EGUI_CORE_TOUCH_H_

#include "egui_motion_event.h"
#include "egui_typedef.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

void egui_core_touch_init(egui_core_t *core);
void egui_core_touch_record_motion(egui_core_t *core, const egui_motion_event_t *motion_event);
void egui_core_touch_draw_trace(egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CORE_TOUCH_H_ */

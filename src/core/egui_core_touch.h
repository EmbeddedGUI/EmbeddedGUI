#ifndef _EGUI_CORE_TOUCH_H_
#define _EGUI_CORE_TOUCH_H_

#include "egui_motion_event.h"
#include "egui_typedef.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Initialize optional touch-trace debug state for one core. */
void egui_core_touch_init(egui_core_t *core);
/** Feed one motion event into the optional debug touch trace recorder. */
void egui_core_touch_record_motion(egui_core_t *core, const egui_motion_event_t *motion_event);
/** Draw the recorded touch trace into the current frame when touch tracing is enabled. */
void egui_core_touch_draw_trace(egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CORE_TOUCH_H_ */

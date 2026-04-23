#ifndef _EGUI_CORE_ACTIVITY_H_
#define _EGUI_CORE_ACTIVITY_H_

#include "egui_core.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Get the top-most activity in the stack, or `NULL` when the stack is empty. */
egui_activity_t *egui_core_activity_get_current(egui_core_t *core);
/** Get the activity that should currently be treated as active after pending transitions are considered. */
egui_activity_t *egui_core_activity_get_current_active(egui_core_t *core);
/** Immediately stop and destroy every activity in the stack without transition animations. */
void egui_core_activity_force_finish_all(egui_core_t *core);
/** Immediately finish activities above `activity` and bring that target activity back to the foreground. */
void egui_core_activity_force_finish_to_activity(egui_core_t *core, egui_activity_t *activity);
/** Return non-zero when `activity` is currently linked in the core's activity stack. */
int egui_core_activity_check_in_process(egui_core_t *core, egui_activity_t *activity);
/** Append one activity node to the core's internal activity stack. */
void egui_core_activity_append(egui_core_t *core, egui_activity_t *activity);
/** Remove one activity node from the core's internal activity stack. */
void egui_core_activity_remove(egui_core_t *core, egui_activity_t *activity);
/** Start `self` and pause the previous activity, resolving `prev_activity` automatically when it is `NULL`. */
void egui_core_activity_start(egui_core_t *core, egui_activity_t *self, egui_activity_t *prev_activity);
/** Configure the open/close animations used when a new activity is started. */
void egui_core_activity_set_start_anim(egui_core_t *core, egui_animation_t *open_anim, egui_animation_t *close_anim);
/** Configure the open/close animations used when the current activity finishes. */
void egui_core_activity_set_finish_anim(egui_core_t *core, egui_animation_t *open_anim, egui_animation_t *close_anim);
/** Finish one activity and resume the nearest previous activity that can continue. */
void egui_core_activity_finish(egui_core_t *core, egui_activity_t *self);
/** Walk up from `view` and return the owning activity whose root contains it. */
egui_activity_t *egui_core_activity_get_by_view(egui_core_t *core, egui_view_t *view);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CORE_ACTIVITY_H_ */

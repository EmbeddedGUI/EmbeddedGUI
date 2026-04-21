#ifndef _EGUI_CORE_ACTIVITY_H_
#define _EGUI_CORE_ACTIVITY_H_

#include "egui_core.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

egui_activity_t *egui_core_activity_get_current(egui_core_t *core);
egui_activity_t *egui_core_activity_get_current_active(egui_core_t *core);
void egui_core_activity_force_finish_all(egui_core_t *core);
void egui_core_activity_force_finish_to_activity(egui_core_t *core, egui_activity_t *activity);
int egui_core_activity_check_in_process(egui_core_t *core, egui_activity_t *activity);
void egui_core_activity_append(egui_core_t *core, egui_activity_t *activity);
void egui_core_activity_remove(egui_core_t *core, egui_activity_t *activity);
void egui_core_activity_start(egui_core_t *core, egui_activity_t *self, egui_activity_t *prev_activity);
void egui_core_activity_set_start_anim(egui_core_t *core, egui_animation_t *open_anim, egui_animation_t *close_anim);
void egui_core_activity_set_finish_anim(egui_core_t *core, egui_animation_t *open_anim, egui_animation_t *close_anim);
void egui_core_activity_finish(egui_core_t *core, egui_activity_t *self);
egui_activity_t *egui_core_activity_get_by_view(egui_core_t *core, egui_view_t *view);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CORE_ACTIVITY_H_ */

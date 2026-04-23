#ifndef _EGUI_CORE_DIALOG_H_
#define _EGUI_CORE_DIALOG_H_

#include "egui_core.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Get the dialog currently owned by the core, or `NULL` when none is showing. */
egui_dialog_t *egui_core_dialog_get(egui_core_t *core);
/** Configure the open/close animations used by dialog transitions. */
void egui_core_dialog_set_anim(egui_core_t *core, egui_animation_t *open_anim, egui_animation_t *close_anim);
/** Show `self` above `activity`, pausing that activity until the dialog is dismissed. */
void egui_core_dialog_start(egui_core_t *core, egui_activity_t *activity, egui_dialog_t *self);
/** Return non-zero when `dialog` is the dialog currently tracked by the core. */
int egui_core_dialog_check_in_process(egui_core_t *core, egui_dialog_t *dialog);
/** Finish the active dialog and resume the activity it was bound to. */
void egui_core_dialog_finish(egui_core_t *core, egui_dialog_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CORE_DIALOG_H_ */

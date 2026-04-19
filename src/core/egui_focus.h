#ifndef _EGUI_FOCUS_H_
#define _EGUI_FOCUS_H_

#include "egui_common.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS

struct egui_focus_manager
{
    egui_view_t *focused_view;
};

void egui_focus_manager_init(egui_core_t *core);
void egui_focus_manager_set_focus(egui_core_t *core, egui_view_t *view);
void egui_focus_manager_clear_focus(egui_core_t *core);
egui_view_t *egui_focus_manager_get_focused_view(egui_core_t *core);
void egui_focus_manager_move_focus_next(egui_core_t *core);
void egui_focus_manager_move_focus_prev(egui_core_t *core);

#endif // EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_FOCUS_H_ */

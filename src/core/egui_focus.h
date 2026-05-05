#ifndef _EGUI_FOCUS_H_
#define _EGUI_FOCUS_H_

#include "egui_common.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS

/** Minimal focus manager that tracks the one currently focused view inside a core. */
struct egui_focus_manager
{
    egui_view_t *focused_view; // currently focused view, or `NULL` when no view owns focus
};

/** Initialize focus tracking for one core with no focused view selected. */
void egui_focus_manager_init(egui_core_t *core);
/** Move focus to one view, invalidate both old/new targets, and fire `on_focus_changed` hooks as needed. */
void egui_focus_manager_set_focus(egui_core_t *core, egui_view_t *view);
/** Clear the current focused view, if any. */
void egui_focus_manager_clear_focus(egui_core_t *core);
/** Get the view that is currently focused in this core, or `NULL` when nothing is focused. */
egui_view_t *egui_focus_manager_get_focused_view(egui_core_t *core);
/** Return whether the view and all of its ancestors allow focus. */
int egui_focus_view_is_focusable(egui_view_t *view);
/** Move focus to the next enabled, visible, focusable view in root-tree traversal order, wrapping at the end. */
void egui_focus_manager_move_focus_next(egui_core_t *core);
/** Move focus to the previous enabled, visible, focusable view in root-tree traversal order, wrapping at the beginning. */
void egui_focus_manager_move_focus_prev(egui_core_t *core);
/** Move focus spatially in the requested arrow-key direction. Returns non-zero when focus changed or was assigned. */
int egui_focus_manager_move_focus_direction(egui_core_t *core, uint8_t key_code);

#endif // EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_FOCUS_H_ */

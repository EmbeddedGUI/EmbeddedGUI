#include "egui_core.h"
#include "egui_core_internal.h"
#include "egui_key_event.h"
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
#include "egui_focus.h"
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
/**
 * @file egui_core_key.c
 * @brief Core-side keyboard dispatch that bridges focus navigation and root fallback delivery.
 */

/** Dispatch one key event, giving focus navigation priority before falling back to normal view delivery. */
void egui_core_process_input_key(egui_core_t *core, egui_key_event_t *key_event)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    // Tab key triggers focus navigation
    if (key_event->type == EGUI_KEY_EVENT_ACTION_UP)
    {
        if (key_event->key_code == EGUI_KEY_CODE_TAB)
        {
            if (key_event->is_shift)
            {
                egui_focus_manager_move_focus_prev(core);
            }
            else
            {
                egui_focus_manager_move_focus_next(core);
            }
            return;
        }
    }

    // Focused views get the first chance to consume ordinary key input.
    egui_view_t *focused = egui_focus_manager_get_focused_view(core);
    if (focused != NULL)
    {
        focused->api->dispatch_key_event(focused, key_event);
        return;
    }
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS

    // Without focus support, or when nothing is focused, route the event through the root view tree.
    egui_view_group_dispatch_key_event((egui_view_t *)egui_core_get_root_view(core), key_event);
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_KEY

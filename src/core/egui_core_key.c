#include "egui_core.h"
#include "egui_core_internal.h"
#include "egui_key_event.h"
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
#include "egui_focus.h"
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
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

    // Dispatch to focused view if available
    egui_view_t *focused = egui_focus_manager_get_focused_view(core);
    if (focused != NULL)
    {
        focused->api->dispatch_key_event(focused, key_event);
        return;
    }
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS

    // Fallback: dispatch to root view group
    egui_view_group_dispatch_key_event((egui_view_t *)egui_core_get_root_view(core), key_event);
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_KEY

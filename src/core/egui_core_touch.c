#include "egui_core_touch.h"

#include "egui_core.h"

void egui_core_process_input_motion(egui_core_t *core, egui_motion_event_t *motion_event)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
#if EGUI_CONFIG_DEBUG_TOUCH_TRACE && EGUI_CONFIG_DEBUG_TOUCH_TRACE_MAX_POINTS > 0
    egui_core_touch_record_motion(core, motion_event);
#endif
    egui_view_group_dispatch_touch_event((egui_view_t *)egui_core_get_root_view(core), motion_event);
#else
    EGUI_UNUSED(core);
    EGUI_UNUSED(motion_event);
#endif
}

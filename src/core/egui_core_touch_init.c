#include "egui_core_touch.h"

#include "egui_core.h"

void egui_core_touch_init(egui_core_t *core)
{
    if (core == NULL)
    {
        return;
    }

#if EGUI_CONFIG_DEBUG_TOUCH_TRACE && EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH && EGUI_CONFIG_DEBUG_TOUCH_TRACE_MAX_POINTS > 0
    core->touch.trace_record.point_count = 0;
    core->touch.trace_record.active = 0;
    egui_region_init_empty(&core->touch.trace_record.bounds);
#endif
}

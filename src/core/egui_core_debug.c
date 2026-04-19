#include "egui_api.h"
#include "egui_core.h"
#include "egui_core_internal.h"

#if EGUI_CONFIG_DEBUG_VIEW_ID
uint16_t egui_core_get_unique_id(egui_core_t *core)
{
    return core->unique_id++;
}
#endif

#if EGUI_DEBUG_MONITOR_SHOW
void egui_debug_init_monitors(egui_core_t *core)
{
#if EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW
    egui_debug_perf_init_monitor(core);
#endif

#if EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW
    egui_debug_mem_init_monitor(core);
#endif
}

void egui_debug_update_monitors(egui_core_t *core, uint32_t timestamp)
{
#if EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW
    egui_debug_perf_update_monitor(core, timestamp);
#endif
#if EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW
    egui_debug_mem_update_monitor(core, timestamp);
#endif
}
#endif

#include "egui_api.h"
#include "egui_core.h"
#include "egui_core_internal.h"

#if EGUI_DEBUG_MONITOR_SHOW && EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW
#define debug_mem_overlay (core->debug.debug_mem_overlay)

static void egui_debug_bytes_to_kb_tenth(uint32_t bytes, uint32_t *kb_int, uint32_t *kb_tenth)
{
    uint32_t whole = bytes / 1024U;

    if (kb_int != NULL)
    {
        *kb_int = whole;
    }

    if (kb_tenth != NULL)
    {
        *kb_tenth = (bytes - whole * 1024U) / 102U;
    }
}

static uint8_t egui_debug_set_mem_text(egui_core_t *core, const egui_mem_monitor_t *monitor)
{
    char next_string[EGUI_DEBUG_MONITOR_TEXT_MAX_LEN];
    uint32_t used_size = 0U;
    uint32_t used_kb = 0U;
    uint32_t used_kb_tenth = 0U;
    uint32_t max_used_kb = 0U;
    uint32_t max_used_kb_tenth = 0U;

    if (monitor != NULL)
    {
        used_size = (uint32_t)monitor->used_size;
    }

    egui_debug_bytes_to_kb_tenth(used_size, &used_kb, &used_kb_tenth);
    egui_debug_bytes_to_kb_tenth(monitor != NULL ? (uint32_t)monitor->max_used : 0U, &max_used_kb, &max_used_kb_tenth);

    egui_api_sprintf(next_string, "%lu.%lu kB\n%lu.%lu kB max", (unsigned long)used_kb, (unsigned long)used_kb_tenth, (unsigned long)max_used_kb,
                     (unsigned long)max_used_kb_tenth);

    return egui_debug_overlay_set_text(&debug_mem_overlay, next_string);
}

void egui_debug_mem_init_monitor(egui_core_t *core)
{
    egui_mem_monitor_t monitor;

    egui_debug_overlay_init(&debug_mem_overlay, EGUI_CONFIG_DEBUG_MEM_MONITOR_POS, EGUI_CONFIG_DEBUG_MEM_MONITOR_OFFSET_X,
                            EGUI_CONFIG_DEBUG_MEM_MONITOR_OFFSET_Y);
    if (!egui_api_get_mem_monitor(core, &monitor))
    {
        egui_api_memset(&monitor, 0, (int)sizeof(monitor));
    }
    egui_debug_set_mem_text(core, &monitor);
}

void egui_debug_mem_update_monitor(egui_core_t *core, uint32_t timestamp)
{
    egui_mem_monitor_t monitor;
    EGUI_UNUSED(timestamp);

    if (egui_api_get_mem_monitor(core, &monitor) && egui_debug_set_mem_text(core, &monitor))
    {
        egui_debug_mark_overlay_dirty(core, &debug_mem_overlay);
    }
}
#endif

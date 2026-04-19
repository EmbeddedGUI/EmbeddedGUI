#include "egui_api.h"
#include "egui_core.h"
#include "egui_core_internal.h"

#if EGUI_DEBUG_MONITOR_SHOW && EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW
#define debug_perf_stats   (core->debug.debug_perf_stats)
#define debug_perf_overlay (core->debug.debug_perf_overlay)

static uint8_t egui_debug_set_perf_text(egui_core_t *core, uint32_t fps, uint32_t cpu_percent, uint32_t render_avg_time_ms, uint32_t flush_avg_time_ms)
{
    char next_string[EGUI_DEBUG_MONITOR_TEXT_MAX_LEN];
    uint32_t refr_avg_time_ms = render_avg_time_ms + flush_avg_time_ms;

    if (cpu_percent > 100U)
    {
        cpu_percent = 100U;
    }

    egui_api_sprintf(next_string, "%lu FPS, %lu%% CPU\n%lu ms (%lu | %lu)", (unsigned long)fps, (unsigned long)cpu_percent, (unsigned long)refr_avg_time_ms,
                     (unsigned long)render_avg_time_ms, (unsigned long)flush_avg_time_ms);

    return egui_debug_overlay_set_text(&debug_perf_overlay, next_string);
}

static void egui_debug_reset_perf_stats(egui_core_t *core)
{
    egui_api_memset(&debug_perf_stats, 0, (int)sizeof(debug_perf_stats));
}

void egui_debug_perf_init_monitor(egui_core_t *core)
{
    egui_debug_overlay_init(&debug_perf_overlay, EGUI_CONFIG_DEBUG_PERF_MONITOR_POS, EGUI_CONFIG_DEBUG_PERF_MONITOR_OFFSET_X,
                            EGUI_CONFIG_DEBUG_PERF_MONITOR_OFFSET_Y);
    egui_debug_set_perf_text(core, 0, 0, 0, 0);
    egui_debug_reset_perf_stats(core);
    debug_perf_stats.window_start_time = egui_api_timer_get_current_core(core);
    debug_perf_stats.initialized = 1;
}

void egui_debug_perf_update_monitor(egui_core_t *core, uint32_t timestamp)
{
    uint32_t elapsed;
    uint32_t fps;
    uint32_t cpu_percent;
    uint32_t render_avg_time_ms;
    uint32_t flush_avg_time_ms;
    uint32_t total_work_time;

    if (!debug_perf_stats.initialized)
    {
        debug_perf_stats.window_start_time = timestamp;
        debug_perf_stats.initialized = 1;
    }

    debug_perf_stats.refr_count++;

    elapsed = timestamp - debug_perf_stats.window_start_time;
    if (elapsed < EGUI_CONFIG_DEBUG_MONITOR_REFR_PERIOD || debug_perf_stats.refr_count == 0)
    {
        return;
    }

    fps = (debug_perf_stats.refr_count * 1000U + elapsed / 2U) / elapsed;
    if (fps > EGUI_CONFIG_MAX_FPS)
    {
        fps = EGUI_CONFIG_MAX_FPS;
    }

    total_work_time = debug_perf_stats.total_render_time + debug_perf_stats.total_flush_time;
    cpu_percent = (total_work_time * 100U + elapsed / 2U) / elapsed;
    if (debug_perf_stats.render_count > 0)
    {
        render_avg_time_ms = (debug_perf_stats.total_render_time + debug_perf_stats.render_count / 2U) / debug_perf_stats.render_count;
        flush_avg_time_ms = (debug_perf_stats.total_flush_time + debug_perf_stats.render_count / 2U) / debug_perf_stats.render_count;
    }
    else
    {
        render_avg_time_ms = 0;
        flush_avg_time_ms = 0;
    }

    if (egui_debug_set_perf_text(core, fps, cpu_percent, render_avg_time_ms, flush_avg_time_ms))
    {
        egui_debug_mark_overlay_dirty(core, &debug_perf_overlay);
    }

    debug_perf_stats.window_start_time = timestamp;
    debug_perf_stats.refr_count = 0;
    debug_perf_stats.render_count = 0;
    debug_perf_stats.total_render_time = 0;
    debug_perf_stats.total_flush_time = 0;
}

void egui_debug_perf_record_work_time(egui_core_t *core, uint32_t render_time, uint32_t flush_time, uint32_t timestamp)
{
    if (!debug_perf_stats.initialized)
    {
        debug_perf_stats.window_start_time = timestamp;
        debug_perf_stats.initialized = 1;
    }

    debug_perf_stats.render_count++;
    debug_perf_stats.total_render_time += render_time;
    debug_perf_stats.total_flush_time += flush_time;
}
#endif

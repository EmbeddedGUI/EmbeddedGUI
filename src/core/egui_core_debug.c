#include <string.h>

#include "egui_api.h"
#include "canvas/egui_canvas.h"
#include "egui_core.h"
#include "egui_core_internal.h"
#include "font/egui_font.h"
#include "resource/egui_resource.h"

/**
 * @file egui_core_debug.c
 * @brief Debug-only helpers for overlay text, performance sampling, and memory monitor rendering.
 */

#if EGUI_CONFIG_DEBUG_VIEW_ID
/** Return a monotonically increasing per-core view id for debug builds. */
uint16_t egui_core_get_unique_id(egui_core_t *core)
{
    return core->unique_id++;
}
#endif

#if EGUI_DEBUG_MONITOR_SHOW
#define EGUI_DEBUG_MONITOR_OVERLAY_PADDING  3
#define EGUI_DEBUG_MONITOR_OVERLAY_BG_ALPHA 128
#define EGUI_DEBUG_MONITOR_FONT()           ((const egui_font_t *)EGUI_CONFIG_DEBUG_MONITOR_FONT)

/** Initialize one overlay slot with empty text and no previously committed region. */
void egui_debug_overlay_init(egui_debug_overlay_t *overlay, uint8_t align_type, egui_dim_t offset_x, egui_dim_t offset_y)
{
    if (overlay == NULL)
    {
        return;
    }

    overlay->text[0] = '\0';
    egui_region_init_empty(&overlay->region_last);
    overlay->region_last_valid = 0;
    overlay->align_type = align_type;
    overlay->offset_x = offset_x;
    overlay->offset_y = offset_y;
}

/** Replace overlay text only when the content actually changed. */
uint8_t egui_debug_overlay_set_text(egui_debug_overlay_t *overlay, const char *next_text)
{
    if (overlay == NULL || next_text == NULL)
    {
        return 0;
    }

    if (strcmp(overlay->text, next_text) == 0)
    {
        return 0;
    }

    strcpy(overlay->text, next_text);
    return 1;
}

/** Compute the on-screen rectangle needed by one overlay based on its text metrics and alignment. */
int egui_debug_get_overlay_region(egui_core_t *core, const egui_debug_overlay_t *overlay, egui_region_t *region)
{
    egui_dim_t text_width = 0;
    egui_dim_t text_height = 0;
    egui_dim_t overlay_width;
    egui_dim_t overlay_height;
    egui_dim_t overlay_x;
    egui_dim_t overlay_y;
    const egui_font_t *font = EGUI_DEBUG_MONITOR_FONT();

    if (overlay == NULL || region == NULL || core->screen_width <= 0 || core->screen_height <= 0 || overlay->text[0] == '\0')
    {
        return 0;
    }

    egui_font_get_str_size_with_core(font, core, overlay->text, 1, 0, &text_width, &text_height);

    overlay_width = text_width + (EGUI_DEBUG_MONITOR_OVERLAY_PADDING << 1);
    overlay_height = text_height + (EGUI_DEBUG_MONITOR_OVERLAY_PADDING << 1);
    overlay_width = EGUI_MIN(overlay_width, (egui_dim_t)core->screen_width);
    overlay_height = EGUI_MIN(overlay_height, (egui_dim_t)core->screen_height);

    if (overlay_width <= 0 || overlay_height <= 0)
    {
        return 0;
    }

    egui_common_align_get_x_y(core->screen_width, core->screen_height, overlay_width, overlay_height, overlay->align_type, &overlay_x, &overlay_y);
    overlay_x += overlay->offset_x;
    overlay_y += overlay->offset_y;

    egui_region_init(region, overlay_x, overlay_y, overlay_width, overlay_height);
    return 1;
}

/** Mark both the previous and current overlay rectangles dirty so moved or resized text redraws cleanly. */
void egui_debug_mark_overlay_dirty(egui_core_t *core, egui_debug_overlay_t *overlay)
{
    egui_region_t overlay_region;
    egui_region_t refresh_region;
    int has_overlay_region = egui_debug_get_overlay_region(core, overlay, &overlay_region);

    if (overlay == NULL)
    {
        return;
    }

    if (!has_overlay_region && !overlay->region_last_valid)
    {
        return;
    }

    if (overlay->region_last_valid && has_overlay_region)
    {
        egui_region_union(&overlay->region_last, &overlay_region, &refresh_region);
    }
    else if (has_overlay_region)
    {
        egui_region_copy(&refresh_region, &overlay_region);
    }
    else
    {
        egui_region_copy(&refresh_region, &overlay->region_last);
    }

    if (!egui_region_is_empty(&refresh_region))
    {
        egui_core_update_region_dirty(core, &refresh_region);
    }
}

#if EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW
#define debug_perf_stats   (core->debug.debug_perf_stats)
#define debug_perf_overlay (core->debug.debug_perf_overlay)

/** Format the performance overlay text from the current aggregated counters. */
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

/** Reset the rolling performance counters that feed the performance overlay. */
static void egui_debug_reset_perf_stats(egui_core_t *core)
{
    egui_api_memset(&debug_perf_stats, 0, (int)sizeof(debug_perf_stats));
}

/** Initialize the performance monitor overlay and start its first sampling window. */
void egui_debug_perf_init_monitor(egui_core_t *core)
{
    egui_debug_overlay_init(&debug_perf_overlay, EGUI_CONFIG_DEBUG_PERF_MONITOR_POS, EGUI_CONFIG_DEBUG_PERF_MONITOR_OFFSET_X,
                            EGUI_CONFIG_DEBUG_PERF_MONITOR_OFFSET_Y);
    egui_debug_set_perf_text(core, 0, 0, 0, 0);
    egui_debug_reset_perf_stats(core);
    debug_perf_stats.window_start_time = egui_api_timer_get_current_core(core);
    debug_perf_stats.initialized = 1;
}

/** Update the performance overlay once enough time has elapsed to close the current sampling window. */
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

/** Record one frame's render and flush cost for the next performance-monitor update. */
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

#if EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW
#define debug_mem_overlay (core->debug.debug_mem_overlay)

/** Convert bytes into a `kB.tenth` style pair used by the memory overlay text. */
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

/** Format the memory overlay text from the current monitor snapshot. */
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

/** Initialize the memory monitor overlay from the current allocator snapshot. */
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

/** Refresh the memory monitor overlay when the tracked usage changed. */
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

/** Draw one overlay into the current PFB only when that PFB intersects the overlay's screen region. */
static void egui_debug_draw_overlay_for_current_pfb(egui_core_t *core, const egui_debug_overlay_t *overlay)
{
    egui_canvas_t *canvas = &core->canvas;
    egui_region_t overlay_region;
    egui_region_t text_region;
    egui_region_t *pfb_region = egui_canvas_get_pfb_region(canvas);
    EGUI_REGION_DEFINE(region_screen, 0, 0, core->screen_width, core->screen_height);

    if (!egui_debug_get_overlay_region(core, overlay, &overlay_region))
    {
        return;
    }

    if (pfb_region != NULL && !egui_region_is_intersect(&overlay_region, pfb_region))
    {
        return;
    }

    egui_canvas_calc_work_region(canvas, &region_screen);
    egui_canvas_draw_rectangle_fill(canvas, overlay_region.location.x, overlay_region.location.y, overlay_region.size.width, overlay_region.size.height,
                                    EGUI_COLOR_BLACK, EGUI_DEBUG_MONITOR_OVERLAY_BG_ALPHA);

    egui_region_copy(&text_region, &overlay_region);
    if (text_region.size.width <= (EGUI_DEBUG_MONITOR_OVERLAY_PADDING << 1) || text_region.size.height <= (EGUI_DEBUG_MONITOR_OVERLAY_PADDING << 1))
    {
        return;
    }

    text_region.location.x += EGUI_DEBUG_MONITOR_OVERLAY_PADDING;
    text_region.location.y += EGUI_DEBUG_MONITOR_OVERLAY_PADDING;
    text_region.size.width -= (EGUI_DEBUG_MONITOR_OVERLAY_PADDING << 1);
    text_region.size.height -= (EGUI_DEBUG_MONITOR_OVERLAY_PADDING << 1);
    egui_canvas_draw_text_in_rect(canvas, EGUI_DEBUG_MONITOR_FONT(), overlay->text, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, EGUI_COLOR_WHITE,
                                  EGUI_ALPHA_100);
}

/** Remember the overlay rectangle that was just rendered so future updates can dirty the old area too. */
static void egui_debug_commit_overlay_region(egui_core_t *core, egui_debug_overlay_t *overlay)
{
    egui_region_t overlay_region;

    if (overlay == NULL || !egui_debug_get_overlay_region(core, overlay, &overlay_region))
    {
        return;
    }

    egui_region_copy(&overlay->region_last, &overlay_region);
    overlay->region_last_valid = 1;
}

/** Draw every enabled debug overlay into the current PFB tile. */
void egui_debug_draw_overlays_for_current_pfb(egui_core_t *core)
{
#if EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW
    egui_debug_draw_overlay_for_current_pfb(core, &debug_perf_overlay);
#endif
#if EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW
    egui_debug_draw_overlay_for_current_pfb(core, &debug_mem_overlay);
#endif
}

/** Commit the last-drawn screen region for every enabled overlay. */
void egui_debug_commit_overlay_regions(egui_core_t *core)
{
#if EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW
    egui_debug_commit_overlay_region(core, &debug_perf_overlay);
#endif
#if EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW
    egui_debug_commit_overlay_region(core, &debug_mem_overlay);
#endif
}

/** Initialize every debug monitor enabled by the current build configuration. */
void egui_debug_init_monitors(egui_core_t *core)
{
#if EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW
    egui_debug_perf_init_monitor(core);
#endif

#if EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW
    egui_debug_mem_init_monitor(core);
#endif
}

/** Update every debug monitor enabled by the current build configuration. */
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

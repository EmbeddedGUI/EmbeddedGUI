#include <limits.h>

#include "egui_api.h"
#include "egui_core.h"
#include "egui_core_internal.h"

/**
 * @file egui_core_dirty.c
 * @brief Dirty-region bookkeeping, merge policy, and optional debug/statistics helpers for one GUI core.
 */

#if EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS
#define egui_dirty_region_stats (core->debug.dirty_region_stats)
#endif

#if EGUI_CONFIG_DEBUG_DIRTY_REGION_DETAIL || EGUI_CONFIG_DEBUG_DIRTY_REGION_TRACE || EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS
/** Return the pixel area of one region, treating empty or invalid rectangles as zero. */
static uint32_t egui_core_get_region_area(const egui_region_t *region)
{
    if (region == NULL || region->size.width <= 0 || region->size.height <= 0)
    {
        return 0;
    }

    return (uint32_t)region->size.width * (uint32_t)region->size.height;
}
#endif

/** Return non-zero when the candidate dirty region intersects any tracked dirty slot. */
int egui_core_check_region_dirty_intersect(egui_core_t *core, egui_region_t *region_dirty)
{
    int i;

    for (i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        egui_region_t *p_region = &core->scene.region_dirty_arr[i];

        if (egui_region_is_intersect(p_region, region_dirty))
        {
            return 1;
        }
    }

    return 0;
}

/** Return the monotonically increasing dirty epoch used to notice refresh cycles. */
uint32_t egui_core_get_dirty_epoch(egui_core_t *core)
{
    return core->scene.dirty_epoch;
}

/** Expose the dirty slot array so higher layers can iterate the coalesced dirty regions. */
egui_region_t *egui_core_get_region_dirty_arr(egui_core_t *core)
{
    return core->scene.region_dirty_arr;
}

/** Clear all dirty slots and bump the epoch so subsequent scans see a fresh frame. */
void egui_core_clear_region_dirty(egui_core_t *core)
{
    int i;

    for (i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        egui_region_init_empty(&core->scene.region_dirty_arr[i]);
    }

    core->scene.dirty_epoch++;
}

/**
 * Merge one new dirty rectangle into the bounded dirty slot array.
 * The algorithm first tries direct overlap/empty-slot insertion, then
 * coalesces later slots, and finally falls back to the slot whose union
 * causes the smallest area growth.
 */
void egui_core_update_region_dirty(egui_core_t *core, egui_region_t *region_dirty)
{
    int i, j;
    int is_changed = 0;

    // Clip incoming updates to the visible screen so off-screen invalidations do not consume dirty slots.
    EGUI_REGION_DEFINE(region_new_in_window, 0, 0, core->screen_width, core->screen_height);
    egui_region_intersect(&region_new_in_window, region_dirty, &region_new_in_window);

    if (egui_region_is_empty(&region_new_in_window))
    {
        return;
    }

#if EGUI_CONFIG_DEBUG_DIRTY_REGION_TRACE
    egui_core_debug_log_region_line("DIRTY_REGION_INCOMING", egui_core_debug_next_frame_index(core), -1, &region_new_in_window);
#endif

    for (i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        egui_region_t *p_region_dirty = &core->scene.region_dirty_arr[i];

        if (!is_changed)
        {
            // First fit: merge into the first overlapping slot, or claim the first empty slot.
            if (egui_region_is_intersect(p_region_dirty, &region_new_in_window) || egui_region_is_empty(p_region_dirty))
            {
#if EGUI_CONFIG_DEBUG_DIRTY_REGION_TRACE
                egui_region_t region_before;

                egui_region_copy(&region_before, p_region_dirty);
#endif
                egui_region_union(p_region_dirty, &region_new_in_window, p_region_dirty);
                is_changed = 1;

#if EGUI_CONFIG_DEBUG_DIRTY_REGION_TRACE
                egui_api_log("DIRTY_REGION_TRACE:frame=%lu,event=slot_update,slot=%d,slot_before_x=%d,slot_before_y=%d,slot_before_w=%d,slot_before_h=%d,slot_"
                             "after_x=%d,"
                             "slot_after_y=%d,slot_after_w=%d,slot_after_h=%d\r\n",
                             (unsigned long)egui_core_debug_next_frame_index(core), i, region_before.location.x, region_before.location.y,
                             region_before.size.width, region_before.size.height, p_region_dirty->location.x, p_region_dirty->location.y,
                             p_region_dirty->size.width, p_region_dirty->size.height);
#endif
            }
        }

        if (!is_changed)
        {
            continue;
        }

        for (j = i + 1; j < EGUI_CONFIG_DIRTY_AREA_COUNT; j++)
        {
            egui_region_t *p_region_merge = &core->scene.region_dirty_arr[j];

            if (egui_region_is_empty(p_region_merge))
            {
                continue;
            }

            if (egui_region_is_empty(p_region_dirty))
            {
                // Compact later non-empty slots forward so early slots stay densely packed.
#if EGUI_CONFIG_DEBUG_DIRTY_REGION_TRACE
                egui_region_t region_moved;

                egui_region_copy(&region_moved, p_region_merge);
#endif
                egui_region_copy(p_region_dirty, p_region_merge);
                egui_region_init_empty(p_region_merge);

#if EGUI_CONFIG_DEBUG_DIRTY_REGION_TRACE
                egui_api_log("DIRTY_REGION_TRACE:frame=%lu,event=compact_move,target_slot=%d,moved_slot=%d,moved_x=%d,moved_y=%d,moved_w=%d,moved_h=%d,target_"
                             "after_x=%d,"
                             "target_after_y=%d,target_after_w=%d,target_after_h=%d\r\n",
                             (unsigned long)egui_core_debug_next_frame_index(core), i, j, region_moved.location.x, region_moved.location.y,
                             region_moved.size.width, region_moved.size.height, p_region_dirty->location.x, p_region_dirty->location.y,
                             p_region_dirty->size.width, p_region_dirty->size.height);
#endif
                continue;
            }

            if (egui_region_is_intersect(p_region_dirty, p_region_merge))
            {
                // After the new region changes one slot, collapse any later slot that now overlaps it.
#if EGUI_CONFIG_DEBUG_DIRTY_REGION_TRACE
                egui_region_t region_merged;

                egui_region_copy(&region_merged, p_region_merge);
#endif
                egui_region_union(p_region_dirty, p_region_merge, p_region_dirty);
                egui_region_init_empty(p_region_merge);

#if EGUI_CONFIG_DEBUG_DIRTY_REGION_TRACE
                egui_api_log("DIRTY_REGION_TRACE:frame=%lu,event=coalesce,target_slot=%d,merged_slot=%d,merged_x=%d,merged_y=%d,merged_w=%d,merged_h=%d,target_"
                             "after_x=%d,"
                             "target_after_y=%d,target_after_w=%d,target_after_h=%d\r\n",
                             (unsigned long)egui_core_debug_next_frame_index(core), i, j, region_merged.location.x, region_merged.location.y,
                             region_merged.size.width, region_merged.size.height, p_region_dirty->location.x, p_region_dirty->location.y,
                             p_region_dirty->size.width, p_region_dirty->size.height);
#endif
            }
        }
    }

    if (!is_changed)
    {
        int best_idx = 0;
        int32_t best_area = INT32_MAX;

        for (i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
        {
            egui_region_t *p_region = &core->scene.region_dirty_arr[i];

            if (egui_region_is_empty(p_region))
            {
                egui_region_copy(p_region, &region_new_in_window);
                best_area = 0;
                break;
            }

            EGUI_REGION_DEFINE_EMPTY(tmp_union);
            egui_region_union(p_region, &region_new_in_window, &tmp_union);

            {
                // When no overlap exists anywhere, sacrifice the slot whose expanded union is smallest.
                int32_t area = (int32_t)tmp_union.size.width * tmp_union.size.height;

                if (area < best_area)
                {
                    best_area = area;
                    best_idx = i;
                }
            }
        }

        if (best_area != 0)
        {
#if EGUI_CONFIG_DEBUG_DIRTY_REGION_TRACE
            egui_region_t region_before;

            egui_region_copy(&region_before, &core->scene.region_dirty_arr[best_idx]);
#endif
            egui_region_union(&core->scene.region_dirty_arr[best_idx], &region_new_in_window, &core->scene.region_dirty_arr[best_idx]);

#if EGUI_CONFIG_DEBUG_DIRTY_REGION_TRACE
            egui_api_log("DIRTY_REGION_TRACE:frame=%lu,event=fallback_union,slot=%d,best_union_area=%ld,slot_before_x=%d,slot_before_y=%d,slot_before_w=%d,"
                         "slot_before_h=%d,slot_after_x=%d,slot_after_y=%d,slot_after_w=%d,slot_after_h=%d\r\n",
                         (unsigned long)egui_core_debug_next_frame_index(core), best_idx, (long)best_area, region_before.location.x, region_before.location.y,
                         region_before.size.width, region_before.size.height, core->scene.region_dirty_arr[best_idx].location.x,
                         core->scene.region_dirty_arr[best_idx].location.y, core->scene.region_dirty_arr[best_idx].size.width,
                         core->scene.region_dirty_arr[best_idx].size.height);
#endif
        }
    }
}

/** Mark the whole screen dirty so the next frame redraws everything. */
void egui_core_update_region_dirty_all(egui_core_t *core)
{
    EGUI_REGION_DEFINE(region_screen, 0, 0, core->screen_width, core->screen_height);
    egui_core_update_region_dirty(core, &region_screen);
}

/** Public alias for forcing a full-screen redraw. */
void egui_core_force_refresh(egui_core_t *core)
{
    egui_core_update_region_dirty_all(core);
}

/** Return the next frame index used by dirty-region debug logs. */
uint32_t egui_core_debug_next_frame_index(egui_core_t *core)
{
#if EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS
    return egui_dirty_region_stats.frame_index + 1;
#else
    EGUI_UNUSED(core);
    return 0;
#endif
}

/** Emit one formatted dirty-region debug line when detail or trace logging is enabled. */
void egui_core_debug_log_region_line(const char *tag, uint32_t frame_index, int slot, const egui_region_t *region)
{
#if EGUI_CONFIG_DEBUG_DIRTY_REGION_DETAIL || EGUI_CONFIG_DEBUG_DIRTY_REGION_TRACE
    uint32_t area;

    if (tag == NULL || region == NULL)
    {
        return;
    }

    area = egui_core_get_region_area(region);
    egui_api_log("%s:frame=%lu,slot=%d,x=%d,y=%d,w=%d,h=%d,area=%lu\r\n", tag, (unsigned long)frame_index, slot, region->location.x, region->location.y,
                 region->size.width, region->size.height, (unsigned long)area);
#else
    EGUI_UNUSED(tag);
    EGUI_UNUSED(frame_index);
    EGUI_UNUSED(slot);
    EGUI_UNUSED(region);
#endif
}

/** Snapshot the current dirty-slot state before rendering starts for this frame. */
void egui_core_dirty_region_stats_begin_frame(egui_core_t *core)
{
#if EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS
    int i;

    egui_dirty_region_stats.frame_index++;
    egui_dirty_region_stats.current_region_count = 0;
    egui_dirty_region_stats.current_dirty_area = 0;
    egui_dirty_region_stats.current_tile_count = 0;

    for (i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        egui_region_t *p_region_dirty = &core->scene.region_dirty_arr[i];

        if (egui_region_is_empty(p_region_dirty))
        {
            break;
        }

        egui_dirty_region_stats.current_region_count++;
        egui_dirty_region_stats.current_dirty_area += egui_core_get_region_area(p_region_dirty);
    }

    egui_dirty_region_stats.is_collecting_frame = 1;
#else
    EGUI_UNUSED(core);
#endif
}

/** Count one rendered tile for dirty-region statistics. */
void egui_core_dirty_region_stats_count_tile(egui_core_t *core, const egui_region_t *region)
{
#if EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS
    if (!egui_dirty_region_stats.is_collecting_frame || region == NULL || region->size.width <= 0 || region->size.height <= 0)
    {
        return;
    }

    egui_dirty_region_stats.current_tile_count++;
#else
    EGUI_UNUSED(core);
    EGUI_UNUSED(region);
#endif
}

/** Finalize and optionally print the per-frame dirty-region statistics after rendering completes. */
void egui_core_dirty_region_stats_end_frame(egui_core_t *core)
{
#if EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS
    egui_dirty_region_stats.is_collecting_frame = 0;
    egui_dirty_region_stats.frame_count++;
    egui_dirty_region_stats.total_dirty_area += egui_dirty_region_stats.current_dirty_area;
    egui_dirty_region_stats.total_tile_count += egui_dirty_region_stats.current_tile_count;

    egui_api_log("DIRTY_REGION_STATS:frame=%lu,regions=%lu,dirty_area=%lu,screen_area=%lu,pfb_tiles=%lu,total_frames=%lu,total_dirty_area=%llu,total_pfb_tiles="
                 "%llu\r\n",
                 (unsigned long)egui_dirty_region_stats.frame_index, (unsigned long)egui_dirty_region_stats.current_region_count,
                 (unsigned long)egui_dirty_region_stats.current_dirty_area, (unsigned long)((uint32_t)core->screen_width * (uint32_t)core->screen_height),
                 (unsigned long)egui_dirty_region_stats.current_tile_count, (unsigned long)egui_dirty_region_stats.frame_count,
                 (unsigned long long)egui_dirty_region_stats.total_dirty_area, (unsigned long long)egui_dirty_region_stats.total_tile_count);

#if EGUI_CONFIG_DEBUG_DIRTY_REGION_DETAIL
    int i;

    for (i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        egui_region_t *p_region_dirty = &core->scene.region_dirty_arr[i];

        if (egui_region_is_empty(p_region_dirty))
        {
            break;
        }

        egui_core_debug_log_region_line("DIRTY_REGION_DETAIL", egui_dirty_region_stats.frame_index, i, p_region_dirty);
    }
#endif
#else
    EGUI_UNUSED(core);
#endif
}

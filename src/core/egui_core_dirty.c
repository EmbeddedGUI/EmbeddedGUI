#include <limits.h>

#include "egui_api.h"
#include "egui_core.h"
#include "egui_core_internal.h"

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

uint32_t egui_core_get_dirty_epoch(egui_core_t *core)
{
    return core->scene.dirty_epoch;
}

egui_region_t *egui_core_get_region_dirty_arr(egui_core_t *core)
{
    return core->scene.region_dirty_arr;
}

void egui_core_clear_region_dirty(egui_core_t *core)
{
    int i;

    for (i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        egui_region_init_empty(&core->scene.region_dirty_arr[i]);
    }

    core->scene.dirty_epoch++;
}

void egui_core_update_region_dirty(egui_core_t *core, egui_region_t *region_dirty)
{
    int i, j;
    int is_changed = 0;

    // change to the window dirty region
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

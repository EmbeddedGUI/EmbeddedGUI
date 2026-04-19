#include "egui_api.h"
#include "egui_core.h"
#include "egui_core_internal.h"

#if EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS
#define egui_dirty_region_stats (core->debug.dirty_region_stats)
#endif

#if EGUI_CONFIG_DEBUG_DIRTY_REGION_DETAIL || EGUI_CONFIG_DEBUG_DIRTY_REGION_TRACE || EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS
static uint32_t egui_core_get_region_area(const egui_region_t *region)
{
    if (region == NULL || region->size.width <= 0 || region->size.height <= 0)
    {
        return 0;
    }

    return (uint32_t)region->size.width * (uint32_t)region->size.height;
}
#endif

uint32_t egui_core_debug_next_frame_index(egui_core_t *core)
{
#if EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS
    return egui_dirty_region_stats.frame_index + 1;
#else
    EGUI_UNUSED(core);
    return 0;
#endif
}

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

void egui_core_dirty_region_stats_end_frame(egui_core_t *core)
{
#if EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS
    int i;

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

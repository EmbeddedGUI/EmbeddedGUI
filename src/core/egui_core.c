#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_api.h"
#include "egui_common.h"
#include "egui_core.h"
#include "egui_core_internal.h"
#include "egui_timer.h"
#include "egui_input.h"
#include "egui_display_driver.h"
#include "egui_touch_driver.h"
#include "egui_rotation.h"
#include "egui_canvas.h"
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
#include "egui_key_event.h"
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
#include "egui_focus.h"
#endif
#include "font/egui_font_std.h"
#include "image/egui_image_std.h"
#include "mask/egui_mask_circle.h"
#include "resource/egui_resource.h"
#include "widget/egui_view.h"
#include "utils/egui_slist.h"
#include "utils/egui_dlist.h"

#define EGUI_CORE_REFRESH_INTERVAL_MS (1000 / EGUI_CONFIG_MAX_FPS)

__EGUI_WEAK__ void egui_port_notify_frame_render_complete(void)
{
}

__EGUI_WEAK__ egui_dim_t egui_core_get_logical_pfb_target_width_hint(void)
{
    return 0;
}

egui_core_t egui_core;

#if EGUI_CONFIG_CORE_SEPARATE_USER_ROOT_GROUP_ENABLE
#define EGUI_CORE_USER_ROOT_VIEW_PTR() ((egui_view_t *)&egui_core.user_root_view_group)
#else
#define EGUI_CORE_USER_ROOT_VIEW_PTR() ((egui_view_t *)&egui_core.root_view_group)
#endif

#if EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW || EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW
#define EGUI_DEBUG_MONITOR_SHOW 1
#else
#define EGUI_DEBUG_MONITOR_SHOW 0
#endif

#if EGUI_DEBUG_MONITOR_SHOW
static void egui_debug_draw_overlays_for_current_pfb(void);
static void egui_debug_commit_overlay_regions(void);
#endif

static egui_dim_t egui_core_get_logical_pfb_probe_width(uint32_t pfb_total_pixel_count)
{
    egui_dim_t target_width = 0;
    int32_t candidate_width;
    egui_dim_t hint_width = egui_core_get_logical_pfb_target_width_hint();

#if EGUI_CONFIG_CORE_LOGICAL_PFB_PROBE_ENABLE
    target_width = EGUI_CONFIG_CORE_LOGICAL_PFB_PROBE_TARGET_WIDTH;
#endif
    if (hint_width > target_width)
    {
        target_width = hint_width;
    }

    if (target_width <= egui_core.pfb_width)
    {
        return 0;
    }

    if ((uint32_t)target_width > pfb_total_pixel_count)
    {
        target_width = (egui_dim_t)pfb_total_pixel_count;
    }

    if (target_width > egui_core.screen_width)
    {
        target_width = egui_core.screen_width;
    }

    for (candidate_width = target_width; candidate_width > egui_core.pfb_width; candidate_width--)
    {
        uint32_t candidate_height;

        if ((pfb_total_pixel_count % (uint32_t)candidate_width) != 0U)
        {
            continue;
        }

        candidate_height = pfb_total_pixel_count / (uint32_t)candidate_width;
        if (candidate_height == 0U || candidate_height > (uint32_t)egui_core.pfb_height)
        {
            continue;
        }

        return (egui_dim_t)candidate_width;
    }

    return 0;
}

static void egui_core_apply_logical_pfb_probe_shape(egui_dim_t *pfb_width, egui_dim_t *pfb_height, uint32_t pfb_total_pixel_count)
{
    egui_dim_t logical_width = egui_core_get_logical_pfb_probe_width(pfb_total_pixel_count);

    if (logical_width <= 0)
    {
        return;
    }

    *pfb_width = logical_width;
    *pfb_height = (egui_dim_t)(pfb_total_pixel_count / (uint32_t)logical_width);
}

#if EGUI_CONFIG_SOFTWARE_ROTATION
static egui_color_int_t egui_rotation_scratch[EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT];
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

#if EGUI_CONFIG_DEBUG_DIRTY_REGION_DETAIL || EGUI_CONFIG_DEBUG_DIRTY_REGION_TRACE
static void egui_core_debug_log_region_line(const char *tag, uint32_t frame_index, int slot, const egui_region_t *region)
{
    uint32_t area = egui_core_get_region_area(region);

    egui_api_log("%s:frame=%lu,slot=%d,x=%d,y=%d,w=%d,h=%d,area=%lu\r\n", tag, (unsigned long)frame_index, slot, region->location.x, region->location.y,
                 region->size.width, region->size.height, (unsigned long)area);
}
#endif

#if EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS
typedef struct egui_dirty_region_stats
{
    uint32_t frame_index;
    uint32_t frame_count;
    uint32_t current_region_count;
    uint32_t current_dirty_area;
    uint32_t current_tile_count;
    uint8_t is_collecting_frame;
    uint64_t total_dirty_area;
    uint64_t total_tile_count;
} egui_dirty_region_stats_t;

static egui_dirty_region_stats_t egui_dirty_region_stats;

#if EGUI_CONFIG_DEBUG_DIRTY_REGION_DETAIL || EGUI_CONFIG_DEBUG_DIRTY_REGION_TRACE
static uint32_t egui_core_debug_next_frame_index(void)
{
    return egui_dirty_region_stats.frame_index + 1;
}
#endif

static void egui_core_dirty_region_stats_begin_frame(void)
{
    int i;

    egui_dirty_region_stats.frame_index++;
    egui_dirty_region_stats.current_region_count = 0;
    egui_dirty_region_stats.current_dirty_area = 0;
    egui_dirty_region_stats.current_tile_count = 0;

    for (i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        egui_region_t *p_region_dirty = &egui_core.region_dirty_arr[i];

        if (egui_region_is_empty(p_region_dirty))
        {
            break;
        }

        egui_dirty_region_stats.current_region_count++;
        egui_dirty_region_stats.current_dirty_area += egui_core_get_region_area(p_region_dirty);
    }

    egui_dirty_region_stats.is_collecting_frame = 1;
}

static void egui_core_dirty_region_stats_count_tile(const egui_region_t *region)
{
    if (!egui_dirty_region_stats.is_collecting_frame || region == NULL || region->size.width <= 0 || region->size.height <= 0)
    {
        return;
    }

    egui_dirty_region_stats.current_tile_count++;
}

static void egui_core_dirty_region_stats_end_frame(void)
{
    int i;

    egui_dirty_region_stats.is_collecting_frame = 0;
    egui_dirty_region_stats.frame_count++;
    egui_dirty_region_stats.total_dirty_area += egui_dirty_region_stats.current_dirty_area;
    egui_dirty_region_stats.total_tile_count += egui_dirty_region_stats.current_tile_count;

    egui_api_log("DIRTY_REGION_STATS:frame=%lu,regions=%lu,dirty_area=%lu,screen_area=%lu,pfb_tiles=%lu,total_frames=%lu,total_dirty_area=%llu,total_pfb_tiles="
                 "%llu\r\n",
                 (unsigned long)egui_dirty_region_stats.frame_index, (unsigned long)egui_dirty_region_stats.current_region_count,
                 (unsigned long)egui_dirty_region_stats.current_dirty_area,
                 (unsigned long)((uint32_t)egui_core.screen_width * (uint32_t)egui_core.screen_height),
                 (unsigned long)egui_dirty_region_stats.current_tile_count, (unsigned long)egui_dirty_region_stats.frame_count,
                 (unsigned long long)egui_dirty_region_stats.total_dirty_area, (unsigned long long)egui_dirty_region_stats.total_tile_count);

#if EGUI_CONFIG_DEBUG_DIRTY_REGION_DETAIL
    for (i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        egui_region_t *p_region_dirty = &egui_core.region_dirty_arr[i];

        if (egui_region_is_empty(p_region_dirty))
        {
            break;
        }

        egui_core_debug_log_region_line("DIRTY_REGION_DETAIL", egui_dirty_region_stats.frame_index, i, p_region_dirty);
    }
#endif
}
#endif

#if (EGUI_CONFIG_DEBUG_DIRTY_REGION_DETAIL || EGUI_CONFIG_DEBUG_DIRTY_REGION_TRACE) && !EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS
static uint32_t egui_core_debug_next_frame_index(void)
{
    return 0;
}
#endif

#if EGUI_CONFIG_DEBUG_TOUCH_TRACE && EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH && EGUI_CONFIG_DEBUG_TOUCH_TRACE_MAX_POINTS > 0
typedef struct egui_core_touch_trace_record
{
    uint16_t point_count;
    uint8_t active;
    egui_region_t bounds;
    egui_location_t points[EGUI_CONFIG_DEBUG_TOUCH_TRACE_MAX_POINTS];
} egui_core_touch_trace_record_t;

static egui_core_touch_trace_record_t egui_core_touch_trace_record;

static void egui_core_debug_touch_trace_reset_record(egui_core_touch_trace_record_t *record)
{
    if (record == NULL)
    {
        return;
    }

    record->point_count = 0;
    record->active = 0;
    egui_region_init_empty(&record->bounds);
}

static void egui_core_debug_touch_trace_reset_storage(void)
{
    egui_core_debug_touch_trace_reset_record(&egui_core_touch_trace_record);
}

static void egui_core_debug_touch_trace_get_point_region(egui_dim_t x, egui_dim_t y, egui_region_t *region)
{
    if (region == NULL)
    {
        return;
    }

    egui_region_init(region, x - 1, y - 1, 3, 3);
}

static void egui_core_debug_touch_trace_get_segment_region(const egui_location_t *from, const egui_location_t *to, egui_region_t *region)
{
    egui_dim_t min_x;
    egui_dim_t min_y;
    egui_dim_t max_x;
    egui_dim_t max_y;

    if (from == NULL || to == NULL || region == NULL)
    {
        return;
    }

    min_x = EGUI_MIN(from->x, to->x) - 1;
    min_y = EGUI_MIN(from->y, to->y) - 1;
    max_x = EGUI_MAX(from->x, to->x) + 1;
    max_y = EGUI_MAX(from->y, to->y) + 1;
    egui_region_init(region, min_x, min_y, max_x - min_x + 1, max_y - min_y + 1);
}

static void egui_core_debug_touch_trace_invalidate_record(const egui_core_touch_trace_record_t *record)
{
    if (record == NULL || egui_region_is_empty((egui_region_t *)&record->bounds))
    {
        return;
    }

    egui_core_update_region_dirty((egui_region_t *)&record->bounds);
}

static void egui_core_debug_touch_trace_clear_last(void)
{
    egui_core_debug_touch_trace_invalidate_record(&egui_core_touch_trace_record);
    egui_core_debug_touch_trace_reset_storage();
}

static void egui_core_debug_touch_trace_append_point(egui_core_touch_trace_record_t *record, egui_dim_t x, egui_dim_t y)
{
    egui_region_t dirty_region;
    egui_location_t point = {x, y};

    if (record == NULL)
    {
        return;
    }

    if (record->point_count > 0)
    {
        egui_location_t *last = &record->points[record->point_count - 1U];
        if (last->x == x && last->y == y)
        {
            return;
        }

        egui_core_debug_touch_trace_get_segment_region(last, &point, &dirty_region);
        if (record->point_count < EGUI_CONFIG_DEBUG_TOUCH_TRACE_MAX_POINTS)
        {
            record->points[record->point_count] = point;
            record->point_count++;
        }
        else
        {
            record->points[record->point_count - 1U] = point;
        }
    }
    else
    {
        record->points[0] = point;
        record->point_count = 1;
        egui_core_debug_touch_trace_get_point_region(x, y, &dirty_region);
    }

    if (egui_region_is_empty(&record->bounds))
    {
        egui_region_copy(&record->bounds, &dirty_region);
    }
    else
    {
        egui_region_union(&record->bounds, &dirty_region, &record->bounds);
    }

    egui_core_update_region_dirty(&dirty_region);
}

static void egui_core_debug_touch_trace_begin(egui_dim_t x, egui_dim_t y)
{
    egui_core_debug_touch_trace_clear_last();
    egui_core_touch_trace_record.active = 1;
    egui_core_debug_touch_trace_append_point(&egui_core_touch_trace_record, x, y);
}

static void egui_core_debug_touch_trace_move(egui_dim_t x, egui_dim_t y)
{
    if (!egui_core_touch_trace_record.active)
    {
        egui_core_debug_touch_trace_begin(x, y);
        return;
    }

    egui_core_debug_touch_trace_append_point(&egui_core_touch_trace_record, x, y);
}

static void egui_core_debug_touch_trace_end(egui_dim_t x, egui_dim_t y)
{
    if (!egui_core_touch_trace_record.active)
    {
        return;
    }

    egui_core_debug_touch_trace_append_point(&egui_core_touch_trace_record, x, y);
    egui_core_touch_trace_record.active = 0;
}

static void egui_core_debug_touch_trace_record_motion(const egui_motion_event_t *motion_event)
{
    if (motion_event == NULL)
    {
        return;
    }

    switch (motion_event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        egui_core_debug_touch_trace_begin(motion_event->location.x, motion_event->location.y);
        break;
    case EGUI_MOTION_EVENT_ACTION_MOVE:
        egui_core_debug_touch_trace_move(motion_event->location.x, motion_event->location.y);
        break;
    case EGUI_MOTION_EVENT_ACTION_UP:
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        egui_core_debug_touch_trace_end(motion_event->location.x, motion_event->location.y);
        break;
    default:
        break;
    }
}

static void egui_core_debug_touch_trace_draw(void)
{
    uint16_t j;
    EGUI_REGION_DEFINE(region_screen, 0, 0, egui_core.screen_width, egui_core.screen_height);
    egui_region_t *pfb_region = egui_canvas_get_pfb_region();

    if (egui_core_touch_trace_record.point_count == 0)
    {
        return;
    }

    egui_canvas_calc_work_region(&region_screen);

    if (pfb_region != NULL && !egui_region_is_intersect(&egui_core_touch_trace_record.bounds, pfb_region))
    {
        return;
    }

    if (egui_core_touch_trace_record.point_count == 1)
    {
        egui_canvas_draw_point(egui_core_touch_trace_record.points[0].x, egui_core_touch_trace_record.points[0].y, EGUI_COLOR_RED, EGUI_ALPHA_100);
        return;
    }

    for (j = 1; j < egui_core_touch_trace_record.point_count; j++)
    {
        egui_canvas_draw_line(egui_core_touch_trace_record.points[j - 1U].x, egui_core_touch_trace_record.points[j - 1U].y,
                              egui_core_touch_trace_record.points[j].x, egui_core_touch_trace_record.points[j].y, 1, EGUI_COLOR_RED, EGUI_ALPHA_100);
    }
}
#endif

// EGUI_VIEW_SUB_DEFINE(egui_view_group_t, test_view_group_1);

// static egui_view_tree_t view_tree_main[] = {
//     EGUI_VIEW_TREE_INIT(&test_view, EGUI_ID_TEST_VIEW, 0, 0, 200, 200),
//     EGUI_VIEW_TREE_INIT(&test_view_group_1, EGUI_ID_TEST_VIEW_GROUP_1, 50, 50, 100, 100),
// };

egui_view_group_t *egui_core_get_root_view(void)
{
    return (egui_view_group_t *)&egui_core.root_view_group;
}

void egui_core_set_pfb_scan_direction(uint8_t reverse_x, uint8_t reverse_y)
{
    egui_core.pfb_scan_reverse_x = reverse_x ? 1U : 0U;
    egui_core.pfb_scan_reverse_y = reverse_y ? 1U : 0U;
}

void egui_core_reset_pfb_scan_direction(void)
{
    egui_core_set_pfb_scan_direction(0U, 0U);
}

uint8_t egui_core_get_pfb_scan_reverse_x(void)
{
    return egui_core.pfb_scan_reverse_x;
}

uint8_t egui_core_get_pfb_scan_reverse_y(void)
{
    return egui_core.pfb_scan_reverse_y;
}

void egui_core_add_root_view(egui_view_t *view)
{
    egui_view_group_add_child((egui_view_t *)&egui_core.root_view_group, view);
}

int egui_core_check_region_dirty_intersect(egui_region_t *region_dirty)
{
    int i;
    for (i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        egui_region_t *p_region = &egui_core.region_dirty_arr[i];
        if (egui_region_is_intersect(p_region, region_dirty))
        {
            return 1;
        }
    }

    return 0;
}

egui_region_t *egui_core_get_region_dirty_arr(void)
{
    return egui_core.region_dirty_arr;
}

uint32_t egui_core_get_dirty_epoch(void)
{
    return egui_core.dirty_epoch;
}

void egui_core_update_region_dirty(egui_region_t *region_dirty)
{
    int i, j;
    int is_changed = 0;

    // change to the window dirty region
    EGUI_REGION_DEFINE(region_new_in_window, 0, 0, egui_core.screen_width, egui_core.screen_height);
    egui_region_intersect(&region_new_in_window, region_dirty, &region_new_in_window);

    if (egui_region_is_empty(&region_new_in_window))
    {
        // EGUI_LOG_WRN("region_new_in_window is empty\r\n"); // change EGUI_CONFIG_DIRTY_AREA_COUNT
        return;
    }

#if EGUI_CONFIG_DEBUG_DIRTY_REGION_TRACE
    egui_core_debug_log_region_line("DIRTY_REGION_INCOMING", egui_core_debug_next_frame_index(), -1, &region_new_in_window);
#endif

    for (i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        egui_region_t *p_region_dirty = &egui_core.region_dirty_arr[i];
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
                             (unsigned long)egui_core_debug_next_frame_index(), i, region_before.location.x, region_before.location.y, region_before.size.width,
                             region_before.size.height, p_region_dirty->location.x, p_region_dirty->location.y, p_region_dirty->size.width,
                             p_region_dirty->size.height);
#endif
            }
        }

        // merge other region
        if (!is_changed)
        {
            continue;
        }

        for (j = i + 1; j < EGUI_CONFIG_DIRTY_AREA_COUNT; j++)
        {
            egui_region_t *p_region_merge = &egui_core.region_dirty_arr[j];

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
                             (unsigned long)egui_core_debug_next_frame_index(), i, j, region_moved.location.x, region_moved.location.y, region_moved.size.width,
                             region_moved.size.height, p_region_dirty->location.x, p_region_dirty->location.y, p_region_dirty->size.width,
                             p_region_dirty->size.height);
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
                // clear the intersect region
                egui_region_init_empty(p_region_merge);

#if EGUI_CONFIG_DEBUG_DIRTY_REGION_TRACE
                egui_api_log("DIRTY_REGION_TRACE:frame=%lu,event=coalesce,target_slot=%d,merged_slot=%d,merged_x=%d,merged_y=%d,merged_w=%d,merged_h=%d,target_"
                             "after_x=%d,"
                             "target_after_y=%d,target_after_w=%d,target_after_h=%d\r\n",
                             (unsigned long)egui_core_debug_next_frame_index(), i, j, region_merged.location.x, region_merged.location.y,
                             region_merged.size.width, region_merged.size.height, p_region_dirty->location.x, p_region_dirty->location.y,
                             p_region_dirty->size.width, p_region_dirty->size.height);
#endif
            }
        }
    }

    // if no region intersect, merge with the region that produces the smallest union area
    if (!is_changed)
    {
        int best_idx = 0;
        int32_t best_area = INT32_MAX;
        for (i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
        {
            egui_region_t *p_region = &egui_core.region_dirty_arr[i];
            if (egui_region_is_empty(p_region))
            {
                // empty slot found, just use it
                egui_region_copy(p_region, &region_new_in_window);
                best_area = 0;
                break;
            }
            EGUI_REGION_DEFINE_EMPTY(tmp_union);
            egui_region_union(p_region, &region_new_in_window, &tmp_union);
            int32_t area = (int32_t)tmp_union.size.width * tmp_union.size.height;
            if (area < best_area)
            {
                best_area = area;
                best_idx = i;
            }
        }
        if (best_area != 0)
        {
#if EGUI_CONFIG_DEBUG_DIRTY_REGION_TRACE
            egui_region_t region_before;

            egui_region_copy(&region_before, &egui_core.region_dirty_arr[best_idx]);
#endif
            egui_region_union(&egui_core.region_dirty_arr[best_idx], &region_new_in_window, &egui_core.region_dirty_arr[best_idx]);

#if EGUI_CONFIG_DEBUG_DIRTY_REGION_TRACE
            egui_api_log("DIRTY_REGION_TRACE:frame=%lu,event=fallback_union,slot=%d,best_union_area=%ld,slot_before_x=%d,slot_before_y=%d,slot_before_w=%d,"
                         "slot_before_h=%d,slot_after_x=%d,slot_after_y=%d,slot_after_w=%d,slot_after_h=%d\r\n",
                         (unsigned long)egui_core_debug_next_frame_index(), best_idx, (long)best_area, region_before.location.x, region_before.location.y,
                         region_before.size.width, region_before.size.height, egui_core.region_dirty_arr[best_idx].location.x,
                         egui_core.region_dirty_arr[best_idx].location.y, egui_core.region_dirty_arr[best_idx].size.width,
                         egui_core.region_dirty_arr[best_idx].size.height);
#endif
        }
    }
}

void egui_core_clear_region_dirty(void)
{

    int i;
    for (i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        egui_region_init_empty(&egui_core.region_dirty_arr[i]);
    }

    egui_core.dirty_epoch++;
}

void egui_core_update_region_dirty_all(void)
{
    EGUI_REGION_DEFINE(region_screen, 0, 0, egui_core.screen_width, egui_core.screen_height);
    egui_core_update_region_dirty(&region_screen);
}

void egui_core_force_refresh(void)
{
    egui_core_update_region_dirty_all();
}

egui_view_group_t *egui_core_get_user_root_view(void)
{
    return (egui_view_group_t *)EGUI_CORE_USER_ROOT_VIEW_PTR();
}

void egui_core_add_user_root_view(egui_view_t *view)
{
    egui_view_group_add_child(EGUI_CORE_USER_ROOT_VIEW_PTR(), view);
}

void egui_core_remove_user_root_view(egui_view_t *view)
{
    egui_view_group_remove_child(EGUI_CORE_USER_ROOT_VIEW_PTR(), view);

    egui_core_update_region_dirty_all();
}

void egui_core_layout_childs_user_root_view(uint8_t is_orientation_horizontal, uint8_t align_type)
{
    egui_view_group_layout_childs(EGUI_CORE_USER_ROOT_VIEW_PTR(), is_orientation_horizontal, 0, 0, align_type);
}

void egui_core_draw_data(egui_region_t *p_region)
{
    int16_t x = p_region->location.x;
    int16_t y = p_region->location.y;
    int16_t w = p_region->size.width;
    int16_t h = p_region->size.height;
    const egui_color_int_t *data = egui_core.pfb;

#if EGUI_CONFIG_SOFTWARE_ROTATION
    egui_display_driver_t *drv = egui_display_driver_get();
    // Apply software rotation if hardware doesn't support it
    if (drv != NULL && drv->rotation != EGUI_DISPLAY_ROTATION_0 && drv->ops->set_rotation == NULL)
    {
        // PFB manager async DMA sends from mgr->buffers[] (i.e. egui_core.pfb),
        // not from the data pointer. So we must ensure the rotated pixels end up
        // in egui_core.pfb. Use scratch as intermediate, then copy back.
        egui_rotation_transform_pfb(drv->rotation, drv->physical_width, drv->physical_height, &x, &y, &w, &h, data, egui_rotation_scratch,
                                    EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT);
        egui_memcpy((void *)data, egui_rotation_scratch, w * h * sizeof(egui_color_int_t));
    }
#endif

    // Submit to PFB ring buffer manager.
    // Single buffer: synchronous draw.
    // Multi-buffer: async DMA with ring queue.
    egui_pfb_manager_submit(&egui_core.pfb_mgr, x, y, w, h, data);
}

void egui_core_draw_view_group(egui_region_t *p_region_dirty, int is_debug_mode)
{
    egui_view_group_t *view_group = (egui_view_group_t *)&egui_core.root_view_group;
    egui_dim_t x, y, x_pos, y_pos;
    egui_dim_t x_pos_base = p_region_dirty->location.x;
    egui_dim_t y_pos_base = p_region_dirty->location.y;
    egui_dim_t width_dirty = p_region_dirty->size.width;
    egui_dim_t height_dirty = p_region_dirty->size.height;
    egui_dim_t pfb_width = egui_core.pfb_width;
    egui_dim_t pfb_height = egui_core.pfb_height;
    egui_dim_t tmp_pfb_width;
    egui_dim_t tmp_pfb_height;
    egui_dim_t pfb_width_count, pfb_height_count;
    uint32_t pfb_total_pixel_count = (uint32_t)egui_core.pfb_total_buffer_size / sizeof(egui_color_int_t);
    uint8_t reverse_x = egui_core.pfb_scan_reverse_x;
    uint8_t reverse_y = egui_core.pfb_scan_reverse_y;

    EGUI_LOG_DBG("region_dirty, x: %d, y: %d, width: %d, height: %d\n", p_region_dirty->location.x, p_region_dirty->location.y, p_region_dirty->size.width,
                 p_region_dirty->size.height);

    egui_core_apply_logical_pfb_probe_shape(&pfb_width, &pfb_height, pfb_total_pixel_count);

    // change pfb size to fit the dirty region
    if (pfb_width > width_dirty)
    {
        pfb_width = width_dirty;
        pfb_height = (egui_dim_t)(pfb_total_pixel_count / (uint32_t)pfb_width);
    }
    else if (pfb_height > height_dirty)
    {
        pfb_height = height_dirty;
        pfb_width = (egui_dim_t)(pfb_total_pixel_count / (uint32_t)pfb_height);
    }

    pfb_width_count = (width_dirty + pfb_width - 1) / pfb_width;
    pfb_height_count = (height_dirty + pfb_height - 1) / pfb_height;

    EGUI_LOG_DBG("pfb_update, pfb_width_count: %d, pfb_height_count: %d, pfb_width: %d, pfb_height: %d\n", pfb_width_count, pfb_height_count, pfb_width,
                 pfb_height);

    // start draw
    for (y = 0; y < pfb_height_count; y++)
    {
        egui_dim_t tile_y = reverse_y ? (pfb_height_count - 1 - y) : y;
        y_pos = y_pos_base + tile_y * pfb_height;

        for (x = 0; x < pfb_width_count; x++)
        {
            egui_dim_t tile_x = reverse_x ? (pfb_width_count - 1 - x) : x;

            x_pos = x_pos_base + tile_x * pfb_width;
            tmp_pfb_width = EGUI_MIN(pfb_width, x_pos_base + width_dirty - x_pos);
            tmp_pfb_height = EGUI_MIN(pfb_height, y_pos_base + height_dirty - y_pos);
            EGUI_LOG_DBG("pfb_region, x_pos: %d, y_pos: %d, pfb_width: %d, pfb_height: %d\n", x_pos, y_pos, tmp_pfb_width, tmp_pfb_height);

            EGUI_REGION_DEFINE(region, x_pos, y_pos, tmp_pfb_width, tmp_pfb_height);

#if EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS
            egui_core_dirty_region_stats_count_tile(&region);
#endif

            egui_core.pfb = egui_pfb_manager_get_render_buffer(&egui_core.pfb_mgr);

            egui_canvas_init(egui_core.pfb, &region);

            int pfb_total_buffer_size = region.size.width * region.size.height * sizeof(egui_color_int_t);
            egui_api_pfb_clear(egui_core.pfb, pfb_total_buffer_size);

            view_group->base.api->draw((egui_view_t *)view_group);

#if EGUI_CONFIG_DEBUG_PFB_REFRESH || EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
            if (is_debug_mode)
            {
                egui_region_t *p_region;
                // change to screen coordinate
                EGUI_REGION_DEFINE(region_screen, 0, 0, egui_core.screen_width, egui_core.screen_height);
                egui_canvas_calc_work_region(&region_screen);

#if EGUI_CONFIG_DEBUG_PFB_REFRESH
                p_region = egui_canvas_get_pfb_region();
                egui_canvas_draw_rectangle(p_region->location.x, p_region->location.y, p_region->size.width, p_region->size.height, 1, EGUI_COLOR_RED,
                                           EGUI_ALPHA_100);
#endif // EGUI_CONFIG_DEBUG_PFB_REFRESH

#if EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
                p_region = p_region_dirty;
                egui_canvas_draw_rectangle(p_region->location.x, p_region->location.y, p_region->size.width, p_region->size.height, 2, EGUI_COLOR_BLUE,
                                           EGUI_ALPHA_100);
#endif // EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
            }
#endif // EGUI_CONFIG_DEBUG_PFB_REFRESH || EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH

#if EGUI_CONFIG_DEBUG_TOUCH_TRACE && EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH && EGUI_CONFIG_DEBUG_TOUCH_TRACE_MAX_POINTS > 0
            egui_core_debug_touch_trace_draw();
#endif

#if EGUI_DEBUG_MONITOR_SHOW
            egui_debug_draw_overlays_for_current_pfb();
#endif

            egui_core_draw_data(&region);

#if EGUI_CONFIG_DEBUG_PFB_REFRESH || EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
            if (is_debug_mode)
            {
                egui_api_delay(EGUI_CONFIG_DEBUG_REFRESH_DELAY);
                egui_api_refresh_display();
            }
#endif // EGUI_CONFIG_DEBUG_PFB_REFRESH || EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
        }
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
void egui_core_process_input_motion(egui_motion_event_t *motion_event)
{
#if EGUI_CONFIG_DEBUG_TOUCH_TRACE && EGUI_CONFIG_DEBUG_TOUCH_TRACE_MAX_POINTS > 0
    egui_core_debug_touch_trace_record_motion(motion_event);
#endif
    egui_view_group_dispatch_touch_event((egui_view_t *)egui_core_get_root_view(), motion_event);
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
void egui_core_process_input_key(egui_key_event_t *key_event)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    // Tab key triggers focus navigation
    if (key_event->type == EGUI_KEY_EVENT_ACTION_UP)
    {
        if (key_event->key_code == EGUI_KEY_CODE_TAB)
        {
            if (key_event->is_shift)
            {
                egui_focus_manager_move_focus_prev();
            }
            else
            {
                egui_focus_manager_move_focus_next();
            }
            return;
        }
    }

    // Dispatch to focused view if available
    egui_view_t *focused = egui_focus_manager_get_focused_view();
    if (focused != NULL)
    {
        focused->api->dispatch_key_event(focused, key_event);
        return;
    }
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS

    // Fallback: dispatch to root view group
    egui_view_group_dispatch_key_event((egui_view_t *)egui_core_get_root_view(), key_event);
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_KEY

#if EGUI_CONFIG_DEBUG_VIEW_ID
uint16_t egui_core_get_unique_id(void)
{
    return egui_core.unique_id++;
}
#endif

#if EGUI_DEBUG_MONITOR_SHOW
#define EGUI_DEBUG_MONITOR_TEXT_MAX_LEN     100
#define EGUI_DEBUG_MONITOR_OVERLAY_PADDING  3
#define EGUI_DEBUG_MONITOR_OVERLAY_BG_ALPHA 128
#define EGUI_DEBUG_MONITOR_FONT()           ((const egui_font_t *)EGUI_CONFIG_DEBUG_MONITOR_FONT)

typedef struct egui_debug_overlay
{
    char text[EGUI_DEBUG_MONITOR_TEXT_MAX_LEN];
    egui_region_t region_last;
    uint8_t region_last_valid;
    uint8_t align_type;
    egui_dim_t offset_x;
    egui_dim_t offset_y;
} egui_debug_overlay_t;

#if EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW
typedef struct egui_debug_perf_stats
{
    uint32_t window_start_time;
    uint32_t refr_count;
    uint32_t render_count;
    uint32_t total_render_time;
    uint32_t total_flush_time;
    uint8_t initialized;
} egui_debug_perf_stats_t;

static egui_debug_perf_stats_t debug_perf_stats;
static egui_debug_overlay_t debug_perf_overlay;
#endif

#if EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW
static egui_debug_overlay_t debug_mem_overlay;
#endif

static void egui_debug_overlay_init(egui_debug_overlay_t *overlay, uint8_t align_type, egui_dim_t offset_x, egui_dim_t offset_y)
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

static int egui_debug_get_overlay_region(const egui_debug_overlay_t *overlay, egui_region_t *region)
{
    egui_dim_t text_width = 0;
    egui_dim_t text_height = 0;
    egui_dim_t overlay_width;
    egui_dim_t overlay_height;
    egui_dim_t overlay_x;
    egui_dim_t overlay_y;
    const egui_font_t *font = EGUI_DEBUG_MONITOR_FONT();

    if (overlay == NULL || region == NULL || egui_core.screen_width <= 0 || egui_core.screen_height <= 0 || overlay->text[0] == '\0')
    {
        return 0;
    }

    font->api->get_str_size(font, overlay->text, 1, 0, &text_width, &text_height);

    overlay_width = text_width + (EGUI_DEBUG_MONITOR_OVERLAY_PADDING << 1);
    overlay_height = text_height + (EGUI_DEBUG_MONITOR_OVERLAY_PADDING << 1);
    overlay_width = EGUI_MIN(overlay_width, (egui_dim_t)egui_core.screen_width);
    overlay_height = EGUI_MIN(overlay_height, (egui_dim_t)egui_core.screen_height);

    if (overlay_width <= 0 || overlay_height <= 0)
    {
        return 0;
    }

    egui_common_align_get_x_y(egui_core.screen_width, egui_core.screen_height, overlay_width, overlay_height, overlay->align_type, &overlay_x, &overlay_y);
    overlay_x += overlay->offset_x;
    overlay_y += overlay->offset_y;

    egui_region_init(region, overlay_x, overlay_y, overlay_width, overlay_height);
    return 1;
}

static uint8_t egui_debug_overlay_set_text(egui_debug_overlay_t *overlay, const char *next_text)
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

static void egui_debug_mark_overlay_dirty(egui_debug_overlay_t *overlay)
{
    egui_region_t overlay_region;
    egui_region_t refresh_region;
    int has_overlay_region = egui_debug_get_overlay_region(overlay, &overlay_region);

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
        egui_core_update_region_dirty(&refresh_region);
    }
}

static void egui_debug_draw_overlay_for_current_pfb(const egui_debug_overlay_t *overlay)
{
    egui_region_t overlay_region;
    egui_region_t text_region;
    egui_region_t *pfb_region = egui_canvas_get_pfb_region();
    EGUI_REGION_DEFINE(region_screen, 0, 0, egui_core.screen_width, egui_core.screen_height);

    if (!egui_debug_get_overlay_region(overlay, &overlay_region))
    {
        return;
    }

    if (pfb_region != NULL && !egui_region_is_intersect(&overlay_region, pfb_region))
    {
        return;
    }

    egui_canvas_calc_work_region(&region_screen);
    egui_canvas_draw_rectangle_fill(overlay_region.location.x, overlay_region.location.y, overlay_region.size.width, overlay_region.size.height, EGUI_COLOR_BLACK,
                                    EGUI_DEBUG_MONITOR_OVERLAY_BG_ALPHA);

    egui_region_copy(&text_region, &overlay_region);
    if (text_region.size.width <= (EGUI_DEBUG_MONITOR_OVERLAY_PADDING << 1) || text_region.size.height <= (EGUI_DEBUG_MONITOR_OVERLAY_PADDING << 1))
    {
        return;
    }

    text_region.location.x += EGUI_DEBUG_MONITOR_OVERLAY_PADDING;
    text_region.location.y += EGUI_DEBUG_MONITOR_OVERLAY_PADDING;
    text_region.size.width -= (EGUI_DEBUG_MONITOR_OVERLAY_PADDING << 1);
    text_region.size.height -= (EGUI_DEBUG_MONITOR_OVERLAY_PADDING << 1);
    egui_canvas_draw_text_in_rect(EGUI_DEBUG_MONITOR_FONT(), overlay->text, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

static void egui_debug_commit_overlay_region(egui_debug_overlay_t *overlay)
{
    egui_region_t overlay_region;

    if (overlay == NULL || !egui_debug_get_overlay_region(overlay, &overlay_region))
    {
        return;
    }

    egui_region_copy(&overlay->region_last, &overlay_region);
    overlay->region_last_valid = 1;
}

#if EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW
static uint8_t egui_debug_set_perf_text(uint32_t fps, uint32_t cpu_percent, uint32_t render_avg_time_ms, uint32_t flush_avg_time_ms)
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

static void egui_debug_reset_perf_stats(void)
{
    egui_api_memset(&debug_perf_stats, 0, (int)sizeof(debug_perf_stats));
}

static void egui_debug_perf_record_refresh_tick(uint32_t timestamp)
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

    if (egui_debug_set_perf_text(fps, cpu_percent, render_avg_time_ms, flush_avg_time_ms))
    {
        egui_debug_mark_overlay_dirty(&debug_perf_overlay);
    }

    debug_perf_stats.window_start_time = timestamp;
    debug_perf_stats.refr_count = 0;
    debug_perf_stats.render_count = 0;
    debug_perf_stats.total_render_time = 0;
    debug_perf_stats.total_flush_time = 0;
}

static void egui_debug_perf_record_work_time(uint32_t render_time, uint32_t flush_time, uint32_t timestamp)
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
static void egui_debug_bytes_to_kb_tenth(size_t bytes, uint64_t *kb_int, uint64_t *kb_tenth)
{
    uint64_t bytes_u64 = (uint64_t)bytes;
    uint64_t whole = bytes_u64 / 1024U;

    if (kb_int != NULL)
    {
        *kb_int = whole;
    }

    if (kb_tenth != NULL)
    {
        *kb_tenth = (bytes_u64 - whole * 1024U) / 102U;
    }
}

static uint8_t egui_debug_set_mem_text(const egui_mem_monitor_t *monitor)
{
    char next_string[EGUI_DEBUG_MONITOR_TEXT_MAX_LEN];
    size_t used_size = 0U;
    uint64_t used_kb = 0U;
    uint64_t used_kb_tenth = 0U;
    uint64_t max_used_kb = 0U;
    uint64_t max_used_kb_tenth = 0U;

    if (monitor != NULL)
    {
        used_size = monitor->used_size;
    }

    egui_debug_bytes_to_kb_tenth(used_size, &used_kb, &used_kb_tenth);
    egui_debug_bytes_to_kb_tenth(monitor != NULL ? monitor->max_used : 0U, &max_used_kb, &max_used_kb_tenth);

    egui_api_sprintf(next_string, "%llu.%llu kB\n%llu.%llu kB max", (unsigned long long)used_kb, (unsigned long long)used_kb_tenth,
                     (unsigned long long)max_used_kb, (unsigned long long)max_used_kb_tenth);

    return egui_debug_overlay_set_text(&debug_mem_overlay, next_string);
}

static void egui_debug_mem_refresh(uint32_t timestamp)
{
    egui_mem_monitor_t monitor;
    EGUI_UNUSED(timestamp);

    if (egui_api_get_mem_monitor(&monitor) && egui_debug_set_mem_text(&monitor))
    {
        egui_debug_mark_overlay_dirty(&debug_mem_overlay);
    }
}
#endif

static void egui_debug_draw_overlays_for_current_pfb(void)
{
#if EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW
    egui_debug_draw_overlay_for_current_pfb(&debug_perf_overlay);
#endif
#if EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW
    egui_debug_draw_overlay_for_current_pfb(&debug_mem_overlay);
#endif
}

static void egui_debug_commit_overlay_regions(void)
{
#if EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW
    egui_debug_commit_overlay_region(&debug_perf_overlay);
#endif
#if EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW
    egui_debug_commit_overlay_region(&debug_mem_overlay);
#endif
}

static void egui_debug_init_monitors(void)
{
#if EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW
    egui_debug_overlay_init(&debug_perf_overlay, EGUI_CONFIG_DEBUG_PERF_MONITOR_POS, EGUI_CONFIG_DEBUG_PERF_MONITOR_OFFSET_X, EGUI_CONFIG_DEBUG_PERF_MONITOR_OFFSET_Y);
    egui_debug_set_perf_text(0, 0, 0, 0);
    egui_debug_reset_perf_stats();
    debug_perf_stats.window_start_time = egui_api_timer_get_current();
    debug_perf_stats.initialized = 1;
#endif

#if EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW
    egui_mem_monitor_t monitor;

    egui_debug_overlay_init(&debug_mem_overlay, EGUI_CONFIG_DEBUG_MEM_MONITOR_POS, EGUI_CONFIG_DEBUG_MEM_MONITOR_OFFSET_X, EGUI_CONFIG_DEBUG_MEM_MONITOR_OFFSET_Y);
    if (!egui_api_get_mem_monitor(&monitor))
    {
        egui_api_memset(&monitor, 0, (int)sizeof(monitor));
    }
    egui_debug_set_mem_text(&monitor);
#endif
}

static void egui_debug_update_monitors(uint32_t timestamp)
{
#if EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW
    egui_debug_perf_record_refresh_tick(timestamp);
#endif
#if EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW
    egui_debug_mem_refresh(timestamp);
#endif
}
#endif

void egui_polling_refresh_display(void)
{
#if EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW
    uint32_t render_start_time = egui_api_timer_get_current();
    uint32_t render_end_time;
    uint32_t flush_end_time;
#endif

#if EGUI_CONFIG_DEBUG_PFB_REFRESH || EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
#if EGUI_CONFIG_DEBUG_PFB_DIRTY_REGION_CLEAR == 0
    // clear last all dirty region
    EGUI_REGION_DEFINE(region, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_core_draw_view_group(&region, false);
#endif
#endif // EGUI_CONFIG_DEBUG_PFB_REFRESH || EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH

#if EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS
    if (!egui_region_is_empty(&egui_core.region_dirty_arr[0]))
    {
        egui_core_dirty_region_stats_begin_frame();
    }
#endif

    for (int i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        egui_region_t *p_region_dirty = &egui_core.region_dirty_arr[i];

        if (egui_region_is_empty(p_region_dirty))
        {
            break;
        }

        egui_core_draw_view_group(p_region_dirty, EGUI_CONFIG_DEBUG_PFB_REFRESH || EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH);
    }

#if EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS
    if (egui_dirty_region_stats.is_collecting_frame)
    {
        egui_core_dirty_region_stats_end_frame();
    }
#endif

    // clear the dirty region
    egui_core_clear_region_dirty();

#if EGUI_DEBUG_MONITOR_SHOW
    egui_debug_commit_overlay_regions();
#endif

#if EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW
    render_end_time = egui_api_timer_get_current();
#endif

    // wait for all PFB flush complete before next frame, to avoid too many pending buffers in the PFB manager when the screen is updated frequently.
    egui_pfb_manager_wait_all_complete(&egui_core.pfb_mgr);

    /* Release per-frame heap caches after the full dirty-frame finishes.
     * They need to stay alive across PFB tiles within the same frame, but
     * should not remain resident once the frame is done. */
    egui_image_std_release_frame_cache();
    egui_canvas_transform_release_frame_cache();
    egui_mask_circle_release_frame_cache();
    egui_font_std_release_frame_cache();

#if EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW
    flush_end_time = egui_api_timer_get_current();
    egui_debug_perf_record_work_time(render_end_time - render_start_time, flush_end_time - render_end_time, flush_end_time);
#endif
}

int egui_check_need_refresh(void)
{
    int i;
    for (i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        if (!egui_region_is_empty(&egui_core.region_dirty_arr[i]))
        {
            return 1;
        }
    }

    return 0;
}

void egui_core_draw_view_group_pre_work(void)
{
    egui_view_group_t *view_group = (egui_view_group_t *)&egui_core.root_view_group;

    // Calculate the layout of the view group
    view_group->base.api->compute_scroll((egui_view_t *)view_group);

    // Calculate the layout of the view group
    view_group->base.api->calculate_layout((egui_view_t *)view_group);
}

void egui_polling_work(void)
{
    egui_timer_polling_work();

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    egui_input_polling_work();
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
    if (!egui_input_check_key_idle())
    {
        egui_input_key_dispatch_work();
    }
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_KEY
}

void egui_core_animation_append(egui_animation_t *anim)
{
    egui_slist_append(&egui_core.anims, &anim->node);
}

void egui_core_animation_remove(egui_animation_t *anim)
{
    egui_slist_find_and_remove(&egui_core.anims, &anim->node);
}

void egui_core_animation_polling_work(void)
{
    egui_snode_t *p_head;
    egui_snode_t *p_next;
    egui_animation_t *tmp;

    uint32_t anim_work_timestamp = egui_api_timer_get_current();

    if (!egui_slist_is_empty(&egui_core.anims))
    {
        EGUI_SLIST_FOR_EACH_NODE_SAFE(&egui_core.anims, p_head, p_next)
        {
            tmp = EGUI_SLIST_ENTRY(p_head, egui_animation_t, node);

            tmp->api->update(tmp, anim_work_timestamp);
        }
    }
}

void egui_core_refresh_screen(void)
{
#if EGUI_DEBUG_MONITOR_SHOW
    egui_debug_update_monitors(egui_api_timer_get_current());
#endif

    if (egui_core.is_suspended)
    {
        return;
    }
    egui_core_animation_polling_work();
    egui_core_draw_view_group_pre_work();
    if (egui_check_need_refresh())
    {
        // Frame sync: prevent tearing by aligning display updates with LCD refresh.
        egui_display_driver_t *drv = egui_display_driver_get();
        if (drv != NULL)
        {
            if (drv->frame_sync_enabled)
            {
                // Non-blocking: TE ISR sets frame_sync_ready via egui_display_notify_vsync().
                // If not ready yet, skip this frame — timer will retry next cycle.
                if (!drv->frame_sync_ready)
                {
                    return;
                }
                drv->frame_sync_ready = 0;
            }
            else if (drv->ops->wait_vsync != NULL)
            {
                // Blocking fallback: wait for VSync/TE signal.
                drv->ops->wait_vsync();
            }
        }

        egui_polling_refresh_display();

        // after drawing, refresh the display
        egui_api_refresh_display();
        egui_port_notify_frame_render_complete();

#if EGUI_CONFIG_DEBUG_PFB_REFRESH || EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
        // wait for a while to see the result
        egui_api_delay(EGUI_CONFIG_DEBUG_REFRESH_DELAY);
#endif // EGUI_CONFIG_DEBUG_PFB_REFRESH || EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
    }
}

static egui_timer_t egui_refresh_timer;
static void egui_refresh_timer_callback(egui_timer_t *timer)
{
    uint32_t start_time = egui_api_timer_get_current();
    egui_core_refresh_screen();
    // get the time used to refresh the screen.
    // avoid refresh too frequently.
    start_time = egui_api_timer_get_current() - start_time;
    if (start_time >= EGUI_CORE_REFRESH_INTERVAL_MS)
    {
        egui_timer_start_timer(&egui_refresh_timer, 1, 0);
    }
    else
    {
        egui_timer_start_timer(&egui_refresh_timer, EGUI_CORE_REFRESH_INTERVAL_MS - start_time, 0);
    }
}

void egui_core_stop_auto_refresh_screen(void)
{
    egui_timer_stop_timer(&egui_refresh_timer);
}

egui_color_int_t *egui_core_get_pfb_buffer_ptr(void)
{
    return egui_core.pfb;
}

void egui_pfb_notify_flush_complete(void)
{
    egui_pfb_manager_notify_flush_complete(&egui_core.pfb_mgr);
}

void egui_pfb_bus_acquire(void)
{
    egui_pfb_manager_bus_acquire(&egui_core.pfb_mgr);
}

void egui_pfb_bus_release(void)
{
    egui_pfb_manager_bus_release(&egui_core.pfb_mgr);
}

void egui_core_pfb_set_buffer(egui_color_int_t *pfb, uint16_t width, uint16_t height)
{
    egui_core.pfb = pfb;
    egui_core.pfb_width = width;
    egui_core.pfb_height = height;
    egui_core.pfb_total_buffer_size = width * height * egui_core.color_bytes;

    // Calculate the number of pixels in each row and column of the PFB
    egui_core.pfb_width_count = (EGUI_CONFIG_SCEEN_WIDTH + width - 1) / width;
    egui_core.pfb_height_count = (EGUI_CONFIG_SCEEN_HEIGHT + height - 1) / height;
}

void egui_core_power_off(void)
{
    egui_timer_stop_timer(&egui_refresh_timer);
}

void egui_core_power_on(void)
{
    egui_timer_start_timer(&egui_refresh_timer, 0, 0);
}

void egui_core_set_screen_size(int16_t width, int16_t height)
{
    egui_core.screen_width = width;
    egui_core.screen_height = height;

    // Recalculate PFB tile counts for the new screen size
    egui_core.pfb_width_count = (width + egui_core.pfb_width - 1) / egui_core.pfb_width;
    egui_core.pfb_height_count = (height + egui_core.pfb_height - 1) / egui_core.pfb_height;

    // Update root view group sizes
    egui_view_set_size((egui_view_t *)&egui_core.root_view_group, width, height);
#if EGUI_CONFIG_CORE_SEPARATE_USER_ROOT_GROUP_ENABLE
    egui_view_set_size((egui_view_t *)&egui_core.user_root_view_group, width, height);
#endif

    // Force full screen refresh
    egui_core_update_region_dirty_all();
}

void egui_core_suspend(void)
{
    egui_core.is_suspended = 1;
    egui_core_stop_auto_refresh_screen();
}

void egui_core_resume(void)
{
    egui_core.is_suspended = 0;
    egui_core_update_region_dirty_all();
    egui_core_power_on();
}

int egui_core_is_suspended(void)
{
    return egui_core.is_suspended;
}

void egui_core_clear_screen(void)
{
    int16_t screen_w = egui_display_get_width();
    int16_t screen_h = egui_display_get_height();
    int16_t pfb_w = egui_core.pfb_width;
    int16_t pfb_h = egui_core.pfb_height;

    // Clear PFB buffer to black
    egui_api_pfb_clear(egui_core.pfb, egui_core.pfb_total_buffer_size);

    // Send black tiles to cover entire screen
    for (int16_t y = 0; y < screen_h; y += pfb_h)
    {
        for (int16_t x = 0; x < screen_w; x += pfb_w)
        {
            int16_t w = (x + pfb_w > screen_w) ? (screen_w - x) : pfb_w;
            int16_t h = (y + pfb_h > screen_h) ? (screen_h - y) : pfb_h;
            EGUI_REGION_DEFINE(region, x, y, w, h);

            if (egui_core.pfb_mgr.buffer_count > 1)
            {
                egui_core.pfb = egui_pfb_manager_get_render_buffer(&egui_core.pfb_mgr);
                // Clear PFB buffer to black
                egui_api_pfb_clear(egui_core.pfb, egui_core.pfb_total_buffer_size);
            }

            egui_core_draw_data(&region);
        }
    }

    egui_pfb_manager_wait_all_complete(&egui_core.pfb_mgr);
}

void egui_screen_off(void)
{
    // 1. Suspend core: stop rendering and refresh timer
    egui_core_suspend();

    // 2. Turn off display hardware
    egui_display_set_power(0);
}

void egui_screen_on(void)
{
    // 1. Turn on display hardware
    egui_display_set_power(1);

    // 2. Clear screen to avoid garbage (GRAM may contain random data)
    egui_core_clear_screen();

    // 3. Resume core: mark all dirty, restart refresh timer
    // The first refresh cycle will redraw the entire UI
    egui_core_resume();
}

void egui_init(egui_color_int_t pfb[][EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT])
{
    egui_core.screen_width = EGUI_CONFIG_SCEEN_WIDTH;
    egui_core.screen_height = EGUI_CONFIG_SCEEN_HEIGHT;
    egui_core.color_bytes = EGUI_CONFIG_COLOR_DEPTH >> 3;
    egui_core_reset_pfb_scan_direction();

    egui_core_pfb_set_buffer(pfb[0], EGUI_CONFIG_PFB_WIDTH, EGUI_CONFIG_PFB_HEIGHT);

    // Initialize PFB manager with first buffer
    egui_pfb_manager_init(&egui_core.pfb_mgr, pfb[0], EGUI_CONFIG_PFB_WIDTH, EGUI_CONFIG_PFB_HEIGHT, egui_core.color_bytes);

    // Auto-add extra buffers based on EGUI_CONFIG_PFB_BUFFER_COUNT
    {
        int i;
        for (i = 1; i < EGUI_CONFIG_PFB_BUFFER_COUNT; i++)
        {
            egui_pfb_manager_add_buffer(&egui_core.pfb_mgr, pfb[i]);
        }
    }

    // reset the unique id
#if EGUI_CONFIG_DEBUG_VIEW_ID
    egui_core.unique_id = 0;
#endif
    egui_core.is_suspended = 1; // Start suspended; user calls egui_screen_on() when ready
    egui_core.dirty_epoch = 0;

    egui_timer_init();
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    egui_input_init();
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
    egui_input_key_init();
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_KEY

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_focus_manager_init();
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS

#if EGUI_CONFIG_DEBUG_TOUCH_TRACE && EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH && EGUI_CONFIG_DEBUG_TOUCH_TRACE_MAX_POINTS > 0
    egui_core_debug_touch_trace_reset_storage();
#endif

    egui_core_update_region_dirty_all();

    egui_slist_init(&egui_core.anims);
    egui_dlist_init(&egui_core.activitys);

    egui_core.activity_anim_start_open = NULL;
    egui_core.activity_anim_start_close = NULL;
    egui_core.activity_anim_finish_open = NULL;
    egui_core.activity_anim_finish_close = NULL;

    egui_core.dialog_anim_start = NULL;
    egui_core.dialog_anim_finish = NULL;
    egui_core.dialog = NULL;

    egui_core.toast = NULL;

    // Initialize the root view group
    egui_view_root_group_init((egui_view_t *)&egui_core.root_view_group);
    egui_view_set_position((egui_view_t *)&egui_core.root_view_group, 0, 0);
    egui_view_set_size((egui_view_t *)&egui_core.root_view_group, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_view_dispatch_attach_to_window((egui_view_t *)&egui_core.root_view_group);

#if EGUI_CONFIG_CORE_SEPARATE_USER_ROOT_GROUP_ENABLE
    egui_view_root_group_init((egui_view_t *)&egui_core.user_root_view_group);
    egui_view_set_position((egui_view_t *)&egui_core.user_root_view_group, 0, 0);
    egui_view_set_size((egui_view_t *)&egui_core.user_root_view_group, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);

    egui_core_add_root_view((egui_view_t *)&egui_core.user_root_view_group);
#endif

#if EGUI_DEBUG_MONITOR_SHOW
    egui_debug_init_monitors();
#endif

    // Initialize registered drivers
    egui_display_driver_t *drv = egui_display_driver_get();
    if (drv != NULL && drv->ops->init != NULL)
    {
        drv->ops->init();
    }

    // Apply initial display configuration from driver struct
    if (drv != NULL)
    {
        if (drv->ops->set_rotation != NULL)
        {
            drv->ops->set_rotation(drv->rotation);
        }
        if (drv->ops->set_brightness != NULL)
        {
            drv->ops->set_brightness(drv->brightness);
        }
        // power_on is not applied here — egui_screen_on() handles power, clear screen, and resume together
    }

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    egui_touch_driver_t *touch = egui_touch_driver_get();
    if (touch != NULL && touch->ops->init != NULL)
    {
        touch->ops->init();
    }
#endif

    // Prepare refresh timer callback (not started yet — egui_screen_on() will start it)
    egui_refresh_timer.callback = egui_refresh_timer_callback;
}

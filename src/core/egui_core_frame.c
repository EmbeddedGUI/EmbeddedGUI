#include "egui_api.h"
#include "egui_core.h"
#include "egui_core_internal.h"
#include "font/egui_font_std.h"
#include "image/egui_image_svg.h"
#include "image/egui_image_std.h"
#include "mask/egui_mask_circle.h"

#if EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS
#define egui_dirty_region_stats (core->debug.dirty_region_stats)
#endif

void egui_polling_refresh_display(egui_core_t *core)
{
#if EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW
    uint32_t render_start_time = egui_api_timer_get_current_core(core);
    uint32_t render_end_time;
    uint32_t flush_end_time;
#endif

#if EGUI_CONFIG_DEBUG_PFB_REFRESH || EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
#if EGUI_CONFIG_DEBUG_PFB_DIRTY_REGION_CLEAR == 0
    // clear last all dirty region
    EGUI_REGION_DEFINE(region, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_core_draw_view_group(core, &region, false);
#endif
#endif // EGUI_CONFIG_DEBUG_PFB_REFRESH || EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH

#if EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS
    if (!egui_region_is_empty(&core->scene.region_dirty_arr[0]))
    {
        egui_core_dirty_region_stats_begin_frame(core);
    }
#endif

    for (int i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        egui_region_t *p_region_dirty = &core->scene.region_dirty_arr[i];

        if (egui_region_is_empty(p_region_dirty))
        {
            break;
        }

        egui_core_draw_view_group(core, p_region_dirty, EGUI_CONFIG_DEBUG_PFB_REFRESH || EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH);
    }

#if EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS
    if (egui_dirty_region_stats.is_collecting_frame)
    {
        egui_core_dirty_region_stats_end_frame(core);
    }
#endif

    // clear the dirty region
    egui_core_clear_region_dirty(core);

#if EGUI_DEBUG_MONITOR_SHOW
    egui_debug_commit_overlay_regions(core);
#endif

#if EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW
    render_end_time = egui_api_timer_get_current_core(core);
#endif

    // wait for all PFB flush complete before next frame, to avoid too many pending buffers in the PFB manager when the screen is updated frequently.
    egui_pfb_manager_wait_all_complete(&core->render.pfb_mgr);

    /* Release per-frame heap caches after the full dirty-frame finishes.
     * They need to stay alive across PFB tiles within the same frame, but
     *
     * should not remain resident once the frame is done. */
    egui_image_std_release_frame_cache(core);
    egui_image_svg_release_frame_cache();
    egui_canvas_transform_release_frame_cache(&core->canvas);
    egui_mask_circle_release_frame_cache(core);
    egui_font_std_release_frame_cache(core);

#if EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW
    flush_end_time = egui_api_timer_get_current_core(core);
    egui_debug_perf_record_work_time(core, render_end_time - render_start_time, flush_end_time - render_end_time, flush_end_time);
#endif
}

#include <string.h>

#include "egui_api.h"
#include "canvas/egui_canvas.h"
#include "egui_core.h"
#include "egui_core_internal.h"
#include "egui_core_touch.h"
#include "egui_display_driver.h"
#include "egui_input.h"
#include "egui_platform.h"
#include "egui_timer.h"
#include "font/egui_font_std.h"
#include "image/egui_image_std.h"
#include "image/egui_image_svg.h"
#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK
#include "mask/egui_mask_circle.h"
#endif

/**
 * @file egui_core.c
 * @brief Main per-core runtime loop: root view ownership, dirty-tile rendering, refresh scheduling, and power/display bootstrap.
 */

#if EGUI_CONFIG_CORE_SEPARATE_USER_ROOT_GROUP_ENABLE
#define EGUI_CORE_USER_ROOT_VIEW_PTR(_core) ((egui_view_t *)&(_core)->scene.user_root_view_group)
#else
#define EGUI_CORE_USER_ROOT_VIEW_PTR(_core) ((egui_view_t *)&(_core)->scene.root_view_group)
#endif

/** Return the built-in root view group that ultimately owns every visible tree on this core. */
egui_view_group_t *egui_core_get_root_view(egui_core_t *core)
{
    return (egui_view_group_t *)&core->scene.root_view_group;
}

/** Attach a view directly under the core root, moving it from any previous parent first. */
void egui_core_add_root_view(egui_core_t *core, egui_view_t *view)
{
    egui_view_group_t *target_parent;

    if (core == NULL || view == NULL)
    {
        return;
    }

    target_parent = (egui_view_group_t *)EGUI_VIEW_OF(&core->scene.root_view_group);
    if (view->parent != NULL && view->parent != target_parent)
    {
        egui_view_group_remove_child(EGUI_VIEW_OF(view->parent), view);
    }

    EGUI_ASSERT(egui_view_get_core(view) == core);

    if (view->parent == NULL)
    {
        egui_view_group_add_child(EGUI_VIEW_OF(target_parent), view);
    }
}

/** Return the user-facing root group used for activities, dialogs, toast roots, and app-owned content. */
egui_view_group_t *egui_core_get_user_root_view(egui_core_t *core)
{
    return (egui_view_group_t *)EGUI_CORE_USER_ROOT_VIEW_PTR(core);
}

/** Attach a view under the core's user root using the view's already bound core pointer. */
void egui_core_add_user_root_view(egui_view_t *view)
{
    egui_core_t *core;
    egui_view_group_t *target_parent;

    if (view == NULL)
    {
        return;
    }

    core = egui_view_get_core(view);
    if (core == NULL)
    {
        return;
    }

    target_parent = (egui_view_group_t *)EGUI_CORE_USER_ROOT_VIEW_PTR(core);
    if (view->parent != NULL && view->parent != target_parent)
    {
        egui_view_group_remove_child(EGUI_VIEW_OF(view->parent), view);
    }

    if (view->parent == NULL)
    {
        egui_view_group_add_child(EGUI_VIEW_OF(target_parent), view);
    }
}

/** Remove a child from the user root and dirty the scene so the gap is repainted. */
void egui_core_remove_user_root_view(egui_core_t *core, egui_view_t *view)
{
    if (core == NULL || view == NULL)
    {
        return;
    }

    egui_view_invalidate_visible_tree(view);
    egui_view_group_remove_child(EGUI_CORE_USER_ROOT_VIEW_PTR(core), view);
}

/** Run the generic group-layout helper on the user root container. */
void egui_core_layout_childs_user_root_view(egui_core_t *core, uint8_t is_orientation_horizontal, uint8_t align_type)
{
    egui_view_group_layout_childs(EGUI_CORE_USER_ROOT_VIEW_PTR(core), is_orientation_horizontal, 0, 0, align_type);
}

/** Run per-frame pre-work on the root tree before any dirty tiles are rendered. */
void egui_core_draw_view_group_pre_work(egui_core_t *core)
{
    egui_view_group_t *view_group = (egui_view_group_t *)&core->scene.root_view_group;

#if EGUI_CONFIG_FUNCTION_CORE_PRE_COMPUTE_SCROLL
    // Scroll offsets can affect layout and hit testing, so update them first.
    view_group->base.api->compute_scroll((egui_view_t *)view_group);
#endif

    // Recalculate layout once per frame before dirty rectangles are split into tiles.
    view_group->base.api->calculate_layout((egui_view_t *)view_group);
}

/** Draw optional debug overlays on top of the tile that was just rendered. */
void egui_core_render_tile_debug_decorate(egui_core_t *core, const egui_region_t *p_region_dirty, int is_debug_mode)
{
#if EGUI_CONFIG_DEBUG_PFB_REFRESH || EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
    if (is_debug_mode)
    {
        egui_canvas_t *canvas = &core->canvas;
        egui_region_t *p_region;
        // change to screen coordinate
        EGUI_REGION_DEFINE(region_screen, 0, 0, core->screen_width, core->screen_height);
        egui_canvas_calc_work_region(canvas, &region_screen);

#if EGUI_CONFIG_DEBUG_PFB_REFRESH
        p_region = egui_canvas_get_pfb_region(canvas);
        egui_canvas_draw_rectangle(canvas, p_region->location.x, p_region->location.y, p_region->size.width, p_region->size.height, 1, EGUI_COLOR_RED,
                                   EGUI_ALPHA_100);
#endif // EGUI_CONFIG_DEBUG_PFB_REFRESH

#if EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
        p_region = (egui_region_t *)p_region_dirty;
        egui_canvas_draw_rectangle(canvas, p_region->location.x, p_region->location.y, p_region->size.width, p_region->size.height, 2, EGUI_COLOR_BLUE,
                                   EGUI_ALPHA_100);
#endif // EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
    }
#else
    EGUI_UNUSED(core);
    EGUI_UNUSED(p_region_dirty);
    EGUI_UNUSED(is_debug_mode);
#endif // EGUI_CONFIG_DEBUG_PFB_REFRESH || EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH

#if EGUI_CONFIG_DEBUG_TOUCH_TRACE && EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH && EGUI_CONFIG_DEBUG_TOUCH_TRACE_MAX_POINTS > 0
    egui_core_touch_draw_trace(core);
#endif

#if EGUI_DEBUG_MONITOR_SHOW
    egui_debug_draw_overlays_for_current_pfb(core);
#endif
}

/** Submit one rendered tile and optionally pause/refresh immediately in debug stepping modes. */
void egui_core_render_tile_present(egui_core_t *core, egui_region_t *region, int is_debug_mode)
{
    egui_core_draw_data(core, region);

#if EGUI_CONFIG_DEBUG_PFB_REFRESH || EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
    if (is_debug_mode)
    {
        egui_api_delay_core(core, EGUI_CONFIG_DEBUG_REFRESH_DELAY);
        egui_api_refresh_display(core);
    }
#else
    EGUI_UNUSED(is_debug_mode);
#endif // EGUI_CONFIG_DEBUG_PFB_REFRESH || EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
}

/**
 * Render one dirty region by splitting it into PFB-sized tiles.
 * The tile probe can be reshaped logically to reuse the same pixel budget while
 * scanning the dirty rectangle in the configured X/Y order.
 */
void egui_core_draw_view_group(egui_core_t *core, egui_region_t *p_region_dirty, int is_debug_mode)
{
    egui_view_group_t *view_group = (egui_view_group_t *)&core->scene.root_view_group;
    egui_dim_t x, y, x_pos, y_pos;
    egui_dim_t x_pos_base = p_region_dirty->location.x;
    egui_dim_t y_pos_base = p_region_dirty->location.y;
    egui_dim_t width_dirty = p_region_dirty->size.width;
    egui_dim_t height_dirty = p_region_dirty->size.height;
    egui_dim_t pfb_width = core->pfb_width;
    egui_dim_t pfb_height = core->pfb_height;
    egui_dim_t tmp_pfb_width;
    egui_dim_t tmp_pfb_height;
    egui_dim_t pfb_width_count, pfb_height_count;
    uint32_t pfb_total_pixel_count = (uint32_t)core->pfb_total_buffer_size / sizeof(egui_color_int_t);
    uint8_t reverse_x = core->pfb_scan_reverse_x;
    uint8_t reverse_y = core->pfb_scan_reverse_y;

    EGUI_LOG_DBG("region_dirty, x: %d, y: %d, width: %d, height: %d\n", p_region_dirty->location.x, p_region_dirty->location.y, p_region_dirty->size.width,
                 p_region_dirty->size.height);

    // Some ports prefer a different logical tile aspect ratio while keeping the same total pixel count.
    egui_core_apply_logical_pfb_probe_shape(core, &pfb_width, &pfb_height, pfb_total_pixel_count);

    // Shrink the probe if the dirty rectangle is smaller than the nominal tile shape.
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

    // Render tile by tile so one small PFB buffer can cover an arbitrarily large dirty rectangle.
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
            egui_core_dirty_region_stats_count_tile(core, &region);
#endif

            // Acquire the next writable render buffer before binding the canvas to this tile.
            core->pfb = egui_pfb_manager_get_render_buffer(&core->render.pfb_mgr);

            egui_canvas_init(&core->canvas, core, core->pfb, &region);

            int pfb_total_buffer_size = region.size.width * region.size.height * sizeof(egui_color_int_t);
            egui_api_pfb_clear(core->pfb, pfb_total_buffer_size);

            view_group->base.api->draw((egui_view_t *)view_group);
            egui_core_render_tile_debug_decorate(core, p_region_dirty, is_debug_mode);
            egui_core_render_tile_present(core, &region, is_debug_mode);
        }
    }
}

/** Draw one dirty region after removing the app-owned punch region from it. */
static void egui_core_draw_view_group_without_punch(egui_core_t *core, egui_region_t *p_region_dirty, int is_debug_mode)
{
    egui_region_t rects[4];
    int rect_count;
    int i;

    if (core == NULL || p_region_dirty == NULL)
    {
        return;
    }

    rect_count = egui_region_subtract_rect(p_region_dirty, &core->punch_region, rects);
    for (i = 0; i < rect_count; i++)
    {
        egui_core_draw_view_group(core, &rects[i], is_debug_mode);
    }
}

void egui_core_set_punch_region(egui_core_t *core, int16_t x, int16_t y, int16_t w, int16_t h)
{
    egui_region_t old_region;

    if (core == NULL)
    {
        return;
    }

    old_region = core->punch_region;

    if (w < 0)
    {
        w = 0;
    }
    if (h < 0)
    {
        h = 0;
    }

    core->punch_region.location.x = x;
    core->punch_region.location.y = y;
    core->punch_region.size.width = w;
    core->punch_region.size.height = h;

    if (!egui_region_is_empty(&old_region) && !egui_region_equal(&old_region, &core->punch_region))
    {
        egui_core_update_region_dirty(core, &old_region);
    }
}

#define EGUI_CORE_REFRESH_INTERVAL_MS (1000 / EGUI_CONFIG_MAX_FPS)

/** Timer callback that keeps the auto-refresh loop close to the configured FPS cap. */
void egui_refresh_timer_callback(egui_timer_t *timer)
{
    egui_core_t *core = (egui_core_t *)timer->user_data;
    uint32_t start_time = egui_api_timer_get_current_core(core);

    egui_core_refresh_screen(core);
    start_time = egui_api_timer_get_current_core(core) - start_time;

    // If one frame already consumed the budget, wake again immediately on the next tick.
    if (start_time >= EGUI_CORE_REFRESH_INTERVAL_MS)
    {
        egui_timer_start_timer(core, timer, 1, 0);
    }
    else
    {
        egui_timer_start_timer(core, timer, EGUI_CORE_REFRESH_INTERVAL_MS - start_time, 0);
    }
}

/** Weak hook for ports that want a notification when one frame starts rendering. */
#if defined(_MSC_VER)
void egui_port_notify_frame_render_start_default(void)
#else
__EGUI_WEAK__ void egui_port_notify_frame_render_start(void)
#endif
{
}
__EGUI_MSVC_ALTERNATE_NAME(egui_port_notify_frame_render_start, egui_port_notify_frame_render_start_default)

/** Weak hook for ports that want a notification after one frame is fully rendered and flushed. */
#if defined(_MSC_VER)
void egui_port_notify_frame_render_complete_default(void)
#else
__EGUI_WEAK__ void egui_port_notify_frame_render_complete(void)
#endif
{
}
__EGUI_MSVC_ALTERNATE_NAME(egui_port_notify_frame_render_complete, egui_port_notify_frame_render_complete_default)

/** Weak hook for ports that need to delay an expensive refresh frame temporarily. */
#if defined(_MSC_VER)
int egui_port_should_defer_refresh_default(egui_core_t *core)
#else
__EGUI_WEAK__ int egui_port_should_defer_refresh(egui_core_t *core)
#endif
{
    EGUI_UNUSED(core);
    return 0;
}
__EGUI_MSVC_ALTERNATE_NAME(egui_port_should_defer_refresh, egui_port_should_defer_refresh_default)

/**
 * Render all pending dirty regions immediately.
 * This is the core of the polling-mode renderer and is also reused by the auto-refresh timer.
 */
void egui_polling_refresh_display(egui_core_t *core)
{
#if EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW
    uint32_t render_start_time = egui_api_timer_get_current_core(core);
    uint32_t render_end_time;
    uint32_t flush_end_time;
#endif

#if EGUI_CONFIG_DEBUG_PFB_REFRESH || EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
#if EGUI_CONFIG_DEBUG_PFB_DIRTY_REGION_CLEAR == 0
    // In stepped debug modes, clear the previous highlight pass before drawing the current dirty regions.
    EGUI_REGION_DEFINE(region, 0, 0, core->screen_width, core->screen_height);
    egui_core_draw_view_group_without_punch(core, &region, false);
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
            // Dirty slots are compacted, so the first empty slot means no more work this frame.
            break;
        }

        egui_core_draw_view_group_without_punch(core, p_region_dirty, EGUI_CONFIG_DEBUG_PFB_REFRESH || EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH);
    }

#if EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS
    if (core->debug.dirty_region_stats.is_collecting_frame)
    {
        egui_core_dirty_region_stats_end_frame(core);
    }
#endif

    // All queued dirty regions have been rendered for this frame.
    egui_core_clear_region_dirty(core);

#if EGUI_DEBUG_MONITOR_SHOW
    egui_debug_commit_overlay_regions(core);
#endif

#if EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW
    render_end_time = egui_api_timer_get_current_core(core);
#endif

    // Drain outstanding flushes so the next frame does not outrun the PFB manager queue.
    egui_pfb_manager_wait_all_complete(&core->render.pfb_mgr);

    /* Release per-frame heap caches after the full dirty-frame finishes.
     * They need to stay alive across PFB tiles within the same frame, but
     * should not remain resident once the frame is done. */
    egui_image_std_release_frame_cache(core);
#if EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG
    egui_image_svg_release_frame_cache();
#endif
    egui_canvas_transform_release_frame_cache(&core->canvas);
#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK
    egui_mask_circle_release_frame_cache(core);
#endif
    egui_font_std_release_frame_cache(core);

#if EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW
    flush_end_time = egui_api_timer_get_current_core(core);
    egui_debug_perf_record_work_time(core, render_end_time - render_start_time, flush_end_time - render_end_time, flush_end_time);
#endif
}

/** Return non-zero when at least one dirty slot still contains pending render work. */
int egui_check_need_refresh(egui_core_t *core)
{
    int i;

    for (i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        if (!egui_region_is_empty(&core->scene.region_dirty_arr[i]))
        {
            return 1;
        }
    }

    return 0;
}

/**
 * Execute one frame tick for this core.
 * The frame updates monitors, polls animations/layout, optionally waits for frame-sync policy, renders pending dirty regions, and finally kicks the panel
 * refresh.
 */
void egui_core_refresh_screen(egui_core_t *core)
{
#if EGUI_DEBUG_MONITOR_SHOW
    egui_debug_update_monitors(core, egui_api_timer_get_current_core(core));
#endif

    // Suspended cores keep their scene state but do not spend time on refresh work.
    if (core->system.is_suspended)
    {
        return;
    }

    egui_core_animation_polling_work(core);
    egui_core_draw_view_group_pre_work(core);

    if (egui_check_need_refresh(core))
    {
        egui_display_driver_t *drv = egui_display_driver_get(core);

        if (egui_port_should_defer_refresh(core))
        {
            return;
        }

        if (drv != NULL)
        {
            if (drv->frame_sync_enabled)
            {
                // Non-blocking frame sync: only render after the port notifies one ready slot.
                if (!drv->frame_sync_ready)
                {
                    return;
                }
                drv->frame_sync_ready = 0;
            }
            else if (drv->ops->wait_vsync != NULL)
            {
                // Blocking frame sync: let the port wait before rendering the next frame.
                drv->ops->wait_vsync(core);
            }
        }

        egui_port_notify_frame_render_start();
        egui_polling_refresh_display(core);
        egui_api_refresh_display(core);
        egui_port_notify_frame_render_complete();

#if EGUI_CONFIG_DEBUG_PFB_REFRESH || EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
        egui_api_delay_core(core, EGUI_CONFIG_DEBUG_REFRESH_DELAY);
#endif
    }
}

/** Stop the auto-refresh timer without otherwise changing the current scene state. */
void egui_core_stop_auto_refresh_screen(egui_core_t *core)
{
    egui_timer_stop_timer(core, &core->system.refresh_timer);
}

/** Poll timers plus touch/key/encoder input once for polling-mode ports. */
void egui_polling_work(egui_core_t *core)
{
    egui_timer_polling_work(core);

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    egui_input_polling_work(core);
#endif

#if EGUI_CONFIG_FUNCTION_LONG_PRESS && EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    /* Long-press detection: fire the listener once after the threshold has elapsed. */
    if (core != NULL && core->touch.prev_pressed)
    {
        egui_view_t *pressed_view;

#if EGUI_CONFIG_FUNCTION_VIEW_GROUP_TOUCH_CAPTURE_PATH
        pressed_view = (core->touch.view_group_touch_state.path_len > 0)
                               ? core->touch.view_group_touch_state.path[core->touch.view_group_touch_state.path_len - 1]
                               : NULL;
#else
        pressed_view = core->touch.view_group_touch_state.captured_view;
#endif

        egui_view_poll_long_press(pressed_view);
    }
#endif /* EGUI_CONFIG_FUNCTION_LONG_PRESS */

#if EGUI_CONFIG_FUNCTION_ENCODER
    egui_encoder_polling_work(core);
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
    if (!egui_input_check_key_idle(core))
    {
        egui_input_key_dispatch_work(core);
    }
#endif
}

/** Power helper that stops the refresh timer while the display is powered down. */
void egui_core_power_off(egui_core_t *core)
{
    egui_timer_stop_timer(core, &core->system.refresh_timer);
}

/** Power helper that restarts the refresh timer for an active display. */
void egui_core_power_on(egui_core_t *core)
{
    egui_timer_start_timer(core, &core->system.refresh_timer, 0, 0);
}

/** Suspend this core so refresh work stops while the scene tree remains allocated. */
void egui_core_suspend(egui_core_t *core)
{
    core->system.is_suspended = 1;
    egui_core_stop_auto_refresh_screen(core);
}

/** Resume this core, dirty the whole scene, and restart automatic refresh. */
void egui_core_resume(egui_core_t *core)
{
    core->system.is_suspended = 0;
    egui_core_update_region_dirty_all(core);
    egui_core_power_on(core);
}

/** Return non-zero when the core is currently suspended. */
int egui_core_is_suspended(egui_core_t *core)
{
    return core->system.is_suspended;
}

/** Clear the physical panel by repeatedly flushing black PFB tiles over the whole screen. */
void egui_core_clear_screen(egui_core_t *core)
{
    int16_t screen_w = egui_display_get_width(core);
    int16_t screen_h = egui_display_get_height(core);
    int16_t pfb_w = core->pfb_width;
    int16_t pfb_h = core->pfb_height;

    // Pre-clear the current PFB buffer once; single-buffer mode reuses it for every tile.
    egui_api_pfb_clear(core->pfb, core->pfb_total_buffer_size);

    // Cover the whole physical panel tile by tile so stale controller memory is overwritten.
    for (int16_t y = 0; y < screen_h; y += pfb_h)
    {
        for (int16_t x = 0; x < screen_w; x += pfb_w)
        {
            int16_t w = (x + pfb_w > screen_w) ? (screen_w - x) : pfb_w;
            int16_t h = (y + pfb_h > screen_h) ? (screen_h - y) : pfb_h;
            EGUI_REGION_DEFINE(region, x, y, w, h);

            if (core->render.pfb_mgr.buffer_count > 1)
            {
                core->pfb = egui_pfb_manager_get_render_buffer(&core->render.pfb_mgr);
                // Multi-buffer mode needs to clear whichever render buffer was acquired for this tile.
                egui_api_pfb_clear(core->pfb, core->pfb_total_buffer_size);
            }

            egui_core_draw_data(core, &region);
        }
    }

    egui_pfb_manager_wait_all_complete(&core->render.pfb_mgr);
}

/** Turn the display off in a safe order: suspend the core first, then power down the panel. */
void egui_screen_off(egui_core_t *core)
{
    // 1. Suspend core: stop rendering and refresh timer.
    egui_core_suspend(core);

    // 2. Turn off display hardware.
    egui_display_set_power(core, 0);
}

/** Turn the display on, clear stale pixels, then resume normal UI refresh. */
void egui_screen_on(egui_core_t *core)
{
    // 1. Turn on display hardware.
    egui_display_set_power(core, 1);

    // 2. Clear screen to avoid garbage left in controller memory.
    egui_core_clear_screen(core);

    // 3. Resume core: mark all dirty, restart refresh timer. The first refresh redraws the whole UI.
    egui_core_resume(core);
}

/** Update runtime screen size metadata and resize the built-in root groups to match. */
void egui_core_set_screen_size(egui_core_t *core, int16_t width, int16_t height)
{
    core->screen_width = width;
    core->screen_height = height;

    // Recalculate how many nominal PFB tiles cover the new logical screen.
    core->pfb_width_count = (width + core->pfb_width - 1) / core->pfb_width;
    core->pfb_height_count = (height + core->pfb_height - 1) / core->pfb_height;

    // Keep the built-in roots in lockstep with the logical display size.
    egui_view_set_size((egui_view_t *)&core->scene.root_view_group, width, height);
#if EGUI_CONFIG_CORE_SEPARATE_USER_ROOT_GROUP_ENABLE
    egui_view_set_size((egui_view_t *)&core->scene.user_root_view_group, width, height);
#endif

    // Layout and rendering need a full refresh after the logical screen size changes.
    egui_core_update_region_dirty_all(core);
}

#if EGUI_CONFIG_FUNCTION_SOFTWARE_ROTATION_ENABLE
static void egui_core_release_rotation_scratch(egui_core_t *core)
{
    if (core == NULL)
    {
        return;
    }

    if (core->render.rotation_scratch_owned && core->render.rotation_scratch != NULL)
    {
        egui_api_free(core, core->render.rotation_scratch);
    }

    core->render.rotation_scratch = NULL;
    core->render.rotation_scratch_owned = 0;
}
#endif

static void egui_core_apply_render_config(egui_core_t *core, const egui_core_render_config_t *config)
{
    uint8_t color_16_swap = EGUI_CONFIG_COLOR_16_SWAP ? 1U : 0U;
#if EGUI_CONFIG_FUNCTION_SOFTWARE_ROTATION_ENABLE
    uint8_t software_rotation = EGUI_CONFIG_FUNCTION_SOFTWARE_ROTATION_ENABLE ? 1U : 0U;
    egui_color_int_t *rotation_scratch = NULL;
#endif

    if (core == NULL)
    {
        return;
    }

    if (config != NULL)
    {
        color_16_swap = config->color_16_swap ? 1U : 0U;
#if EGUI_CONFIG_FUNCTION_SOFTWARE_ROTATION_ENABLE
        software_rotation = config->software_rotation ? 1U : 0U;
        rotation_scratch = config->rotation_scratch;
#endif
    }

    core->render.color_16_swap = color_16_swap;

#if !EGUI_CONFIG_FUNCTION_SOFTWARE_ROTATION_ENABLE
    core->render.rotation_scratch = NULL;
    core->render.rotation_scratch_owned = 0;
    core->render.software_rotation = 0;
    return;
#else
    if (!software_rotation)
    {
        egui_core_release_rotation_scratch(core);
        core->render.software_rotation = 0;
        return;
    }

    if (rotation_scratch != NULL)
    {
        egui_core_release_rotation_scratch(core);
        core->render.rotation_scratch = rotation_scratch;
        core->render.software_rotation = 1;
        return;
    }

    if (core->render.rotation_scratch == NULL || !core->render.rotation_scratch_owned)
    {
        egui_core_release_rotation_scratch(core);
        core->render.rotation_scratch = (egui_color_int_t *)egui_api_malloc(core, core->pfb_total_buffer_size);
        if (core->render.rotation_scratch == NULL)
        {
            EGUI_LOG_ERR("rotation scratch alloc failed, bytes=%d\n", core->pfb_total_buffer_size);
            core->render.software_rotation = 0;
            return;
        }
        core->render.rotation_scratch_owned = 1;
    }

    core->render.software_rotation = 1;
#endif
}

void egui_core_set_render_config(egui_core_t *core, const egui_core_render_config_t *config)
{
    int16_t new_w;
    int16_t new_h;

    EGUI_ASSERT(core != NULL);
    EGUI_ASSERT(core->screen_width > 0);
    EGUI_ASSERT(core->screen_height > 0);
    EGUI_ASSERT(core->pfb_total_buffer_size > 0);
    if (config != NULL)
    {
        EGUI_ASSERT(config->color_16_swap <= 1);
        EGUI_ASSERT(config->software_rotation <= 1);
    }

    egui_core_apply_render_config(core, config);
    new_w = egui_display_get_width(core);
    new_h = egui_display_get_height(core);
    if (new_w > 0 && new_h > 0 && (core->screen_width != new_w || core->screen_height != new_h))
    {
        egui_core_set_screen_size(core, new_w, new_h);
    }
    egui_core_force_refresh(core);
}

/** Finish the second half of display setup after the core and PFB manager are already initialized. */
void egui_core_setup_display_start(egui_core_t *core, const egui_display_setup_t *setup)
{
    egui_display_driver_register(core, setup->display_driver);

    if (setup->touch_register != NULL)
    {
        setup->touch_register(core);
    }

    if (setup->uicode_init != NULL)
    {
        setup->uicode_init(core);
    }

    egui_screen_on(core);
}

/** Initialize scene-side lists, built-in roots, debug hooks, and the refresh timer callback. */
void egui_core_init_display_scene(egui_core_t *core, int16_t screen_w, int16_t screen_h)
{
    egui_core_update_region_dirty_all(core);
    egui_slist_init(&core->scene.anims);

#if EGUI_CONFIG_FUNCTION_SUPPORT_ACTIVITY
    egui_dlist_init(&core->scene.activitys);

    core->scene.activity_anim_start_open = NULL;
    core->scene.activity_anim_start_close = NULL;
    core->scene.activity_anim_finish_open = NULL;
    core->scene.activity_anim_finish_close = NULL;
    core->scene.activity_anim_start_open_owner = NULL;
    core->scene.activity_anim_start_close_owner = NULL;
    core->scene.activity_anim_finish_open_owner = NULL;
    core->scene.activity_anim_finish_close_owner = NULL;
    core->scene.activity_open = NULL;
    core->scene.activity_close = NULL;
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_DIALOG
    core->scene.dialog_anim_start = NULL;
    core->scene.dialog_anim_finish = NULL;
    core->scene.dialog_anim_start_owner = NULL;
    core->scene.dialog_anim_finish_owner = NULL;
    core->scene.dialog = NULL;
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOAST
    core->scene.toast = NULL;
#endif

    egui_view_root_group_init((egui_view_t *)&core->scene.root_view_group, core);
    egui_view_set_position((egui_view_t *)&core->scene.root_view_group, 0, 0);
    egui_view_set_size((egui_view_t *)&core->scene.root_view_group, screen_w, screen_h);
    egui_view_dispatch_attach_to_window((egui_view_t *)&core->scene.root_view_group);

#if EGUI_CONFIG_CORE_SEPARATE_USER_ROOT_GROUP_ENABLE
    egui_view_root_group_init((egui_view_t *)&core->scene.user_root_view_group, core);
    egui_view_set_position((egui_view_t *)&core->scene.user_root_view_group, 0, 0);
    egui_view_set_size((egui_view_t *)&core->scene.user_root_view_group, screen_w, screen_h);
    egui_core_add_root_view(core, (egui_view_t *)&core->scene.user_root_view_group);
#endif

#if EGUI_DEBUG_MONITOR_SHOW
    egui_debug_init_monitors(core);
#endif

    core->system.refresh_timer.callback = egui_refresh_timer_callback;
    core->system.refresh_timer.user_data = core;
}

/** Initialize one core instance, bind its PFB manager/canvas, and bootstrap every runtime subsystem. */
void egui_init_display(egui_core_t *core, int16_t screen_w, int16_t screen_h, egui_color_int_t **pfb_bufs, int buf_count, int pfb_w, int pfb_h)
{
    int i;

    EGUI_ASSERT(core != NULL);
    EGUI_ASSERT(pfb_bufs != NULL);
    EGUI_ASSERT(pfb_bufs[0] != NULL);
    EGUI_ASSERT(buf_count > 0 && buf_count <= EGUI_PFB_BUFFER_MAX_COUNT);
    EGUI_ASSERT(screen_w > 0 && screen_h > 0);
    EGUI_ASSERT(pfb_w > 0 && pfb_h > 0);
    EGUI_ASSERT(pfb_w <= 32767 && pfb_h <= 32767);

    egui_api_memset(core, 0, sizeof(egui_core_t));
    core->render.pfb_mgr.core = core;
    core->canvas.core = core;
    core->screen_width = screen_w;
    core->screen_height = screen_h;
    core->color_bytes = EGUI_CONFIG_COLOR_DEPTH >> 3;
    egui_core_reset_pfb_scan_direction(core);

    egui_core_pfb_set_buffer(core, pfb_bufs[0], pfb_w, pfb_h);
    egui_pfb_manager_init(&core->render.pfb_mgr, pfb_bufs[0], pfb_w, pfb_h, core->color_bytes);

    for (i = 1; i < buf_count && i < EGUI_PFB_BUFFER_MAX_COUNT; i++)
    {
        EGUI_ASSERT(pfb_bufs[i] != NULL);
        egui_pfb_manager_add_buffer(&core->render.pfb_mgr, pfb_bufs[i]);
    }

#if EGUI_CONFIG_DEBUG_VIEW_ID
    core->unique_id = 0;
#endif
    core->system.is_suspended = 1;
    core->scene.dirty_epoch = 0;
    core->asset.theme_current = EGUI_CONFIG_THEME_DEFAULT;
    egui_core_apply_render_config(core, NULL);

    egui_timer_init(core);
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    egui_input_init(core);
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
    egui_input_key_init(core);
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_focus_manager_init(core);
#endif

#if EGUI_CONFIG_DEBUG_TOUCH_TRACE && EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH && EGUI_CONFIG_DEBUG_TOUCH_TRACE_MAX_POINTS > 0
    egui_core_touch_init(core);
#endif

    egui_core_init_display_scene(core, screen_w, screen_h);
}

/** One-call setup helper that initializes a core from a prepared setup descriptor and starts the display. */
void egui_setup_display(egui_core_t *core, const egui_display_setup_t *setup)
{
    EGUI_ASSERT(core != NULL);
    EGUI_ASSERT(setup != NULL);
    EGUI_ASSERT(egui_platform_get() != NULL);
    EGUI_ASSERT(setup->pfb_buffers != NULL);
    EGUI_ASSERT(setup->pfb_buffer_count > 0);
    EGUI_ASSERT(setup->pfb_buffer_count <= EGUI_PFB_BUFFER_MAX_COUNT);
    EGUI_ASSERT(setup->screen_width > 0);
    EGUI_ASSERT(setup->screen_height > 0);
    EGUI_ASSERT(setup->screen_width <= 32767);
    EGUI_ASSERT(setup->screen_height <= 32767);
    EGUI_ASSERT(setup->pfb_width > 0);
    EGUI_ASSERT(setup->pfb_height > 0);
    EGUI_ASSERT(setup->pfb_width <= 32767);
    EGUI_ASSERT(setup->pfb_height <= 32767);
    EGUI_ASSERT(setup->display_driver != NULL);
    EGUI_ASSERT(setup->display_id < EGUI_CONFIG_MAX_DISPLAY_COUNT);
    if (setup->render_config != NULL)
    {
        EGUI_ASSERT(setup->render_config->color_16_swap <= 1);
        EGUI_ASSERT(setup->render_config->software_rotation <= 1);
    }

    egui_init_display(core, (int16_t)setup->screen_width, (int16_t)setup->screen_height, setup->pfb_buffers, setup->pfb_buffer_count, setup->pfb_width,
                      setup->pfb_height);
    if (setup->render_config != NULL)
    {
        egui_core_set_render_config(core, setup->render_config);
    }
    core->id = setup->display_id;
    egui_core_setup_display_start(core, setup);
}

/** Primary-display convenience wrapper that builds the PFB pointer array from the compile-time buffer declaration. */
void egui_init(egui_core_t *core, egui_color_int_t pfb[][EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT])
{
    egui_color_int_t *pfb_bufs[EGUI_CONFIG_PFB_BUFFER_COUNT];
    int i;

    for (i = 0; i < EGUI_CONFIG_PFB_BUFFER_COUNT; i++)
    {
        pfb_bufs[i] = pfb[i];
    }

    egui_init_display(core, EGUI_CONFIG_SCREEN_WIDTH, EGUI_CONFIG_SCREEN_HEIGHT, pfb_bufs, EGUI_CONFIG_PFB_BUFFER_COUNT, EGUI_CONFIG_PFB_WIDTH,
                      EGUI_CONFIG_PFB_HEIGHT);
}

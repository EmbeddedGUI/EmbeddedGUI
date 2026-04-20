#include <string.h>

#include "egui_api.h"
#include "egui_canvas.h"
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
#include "mask/egui_mask_circle.h"

#if EGUI_CONFIG_CORE_SEPARATE_USER_ROOT_GROUP_ENABLE
#define EGUI_CORE_USER_ROOT_VIEW_PTR(_core) ((egui_view_t *)&(_core)->scene.user_root_view_group)
#else
#define EGUI_CORE_USER_ROOT_VIEW_PTR(_core) ((egui_view_t *)&(_core)->scene.root_view_group)
#endif

egui_view_group_t *egui_core_get_root_view(egui_core_t *core)
{
    return (egui_view_group_t *)&core->scene.root_view_group;
}

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

egui_view_group_t *egui_core_get_user_root_view(egui_core_t *core)
{
    return (egui_view_group_t *)EGUI_CORE_USER_ROOT_VIEW_PTR(core);
}

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

void egui_core_remove_user_root_view(egui_core_t *core, egui_view_t *view)
{
    egui_view_group_remove_child(EGUI_CORE_USER_ROOT_VIEW_PTR(core), view);
    egui_core_update_region_dirty_all(core);
}

void egui_core_layout_childs_user_root_view(egui_core_t *core, uint8_t is_orientation_horizontal, uint8_t align_type)
{
    egui_view_group_layout_childs(EGUI_CORE_USER_ROOT_VIEW_PTR(core), is_orientation_horizontal, 0, 0, align_type);
}

void egui_core_draw_view_group_pre_work(egui_core_t *core)
{
    egui_view_group_t *view_group = (egui_view_group_t *)&core->scene.root_view_group;

    // Calculate the layout of the view group
    view_group->base.api->compute_scroll((egui_view_t *)view_group);

    // Calculate the layout of the view group
    view_group->base.api->calculate_layout((egui_view_t *)view_group);
}

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

    egui_core_apply_logical_pfb_probe_shape(core, &pfb_width, &pfb_height, pfb_total_pixel_count);

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
            egui_core_dirty_region_stats_count_tile(core, &region);
#endif

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

#define EGUI_CORE_REFRESH_INTERVAL_MS (1000 / EGUI_CONFIG_MAX_FPS)

void egui_refresh_timer_callback(egui_timer_t *timer)
{
    egui_core_t *core = (egui_core_t *)timer->user_data;
    uint32_t start_time = egui_api_timer_get_current_core(core);

    egui_core_refresh_screen(core);
    start_time = egui_api_timer_get_current_core(core) - start_time;

    if (start_time >= EGUI_CORE_REFRESH_INTERVAL_MS)
    {
        egui_timer_start_timer(core, timer, 1, 0);
    }
    else
    {
        egui_timer_start_timer(core, timer, EGUI_CORE_REFRESH_INTERVAL_MS - start_time, 0);
    }
}

__EGUI_WEAK__ void egui_port_notify_frame_render_complete(void)
{
}

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
    if (core->debug.dirty_region_stats.is_collecting_frame)
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

void egui_core_refresh_screen(egui_core_t *core)
{
#if EGUI_DEBUG_MONITOR_SHOW
    egui_debug_update_monitors(core, egui_api_timer_get_current_core(core));
#endif

    if (core->system.is_suspended)
    {
        return;
    }

    egui_core_animation_polling_work(core);
    egui_core_draw_view_group_pre_work(core);

    if (egui_check_need_refresh(core))
    {
        egui_display_driver_t *drv = egui_display_driver_get(core);

        if (drv != NULL)
        {
            if (drv->frame_sync_enabled)
            {
                if (!drv->frame_sync_ready)
                {
                    return;
                }
                drv->frame_sync_ready = 0;
            }
            else if (drv->ops->wait_vsync != NULL)
            {
                drv->ops->wait_vsync(core);
            }
        }

        egui_polling_refresh_display(core);
        egui_api_refresh_display(core);
        egui_port_notify_frame_render_complete();

#if EGUI_CONFIG_DEBUG_PFB_REFRESH || EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
        egui_api_delay_core(core, EGUI_CONFIG_DEBUG_REFRESH_DELAY);
#endif
    }
}

void egui_core_stop_auto_refresh_screen(egui_core_t *core)
{
    egui_timer_stop_timer(core, &core->system.refresh_timer);
}

void egui_polling_work(egui_core_t *core)
{
    egui_timer_polling_work(core);

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    egui_input_polling_work(core);
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
    if (!egui_input_check_key_idle(core))
    {
        egui_input_key_dispatch_work(core);
    }
#endif
}

void egui_core_power_off(egui_core_t *core)
{
    egui_timer_stop_timer(core, &core->system.refresh_timer);
}

void egui_core_power_on(egui_core_t *core)
{
    egui_timer_start_timer(core, &core->system.refresh_timer, 0, 0);
}

void egui_core_suspend(egui_core_t *core)
{
    core->system.is_suspended = 1;
    egui_core_stop_auto_refresh_screen(core);
}

void egui_core_resume(egui_core_t *core)
{
    core->system.is_suspended = 0;
    egui_core_update_region_dirty_all(core);
    egui_core_power_on(core);
}

int egui_core_is_suspended(egui_core_t *core)
{
    return core->system.is_suspended;
}

void egui_core_clear_screen(egui_core_t *core)
{
    int16_t screen_w = egui_display_get_width(core);
    int16_t screen_h = egui_display_get_height(core);
    int16_t pfb_w = core->pfb_width;
    int16_t pfb_h = core->pfb_height;

    // Clear PFB buffer to black
    egui_api_pfb_clear(core->pfb, core->pfb_total_buffer_size);

    // Send black tiles to cover entire screen
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
                // Clear PFB buffer to black
                egui_api_pfb_clear(core->pfb, core->pfb_total_buffer_size);
            }

            egui_core_draw_data(core, &region);
        }
    }

    egui_pfb_manager_wait_all_complete(&core->render.pfb_mgr);
}

void egui_screen_off(egui_core_t *core)
{
    // 1. Suspend core: stop rendering and refresh timer
    egui_core_suspend(core);

    // 2. Turn off display hardware
    egui_display_set_power(core, 0);
}

void egui_screen_on(egui_core_t *core)
{
    // 1. Turn on display hardware
    egui_display_set_power(core, 1);

    // 2. Clear screen to avoid garbage (GRAM may contain random data)
    egui_core_clear_screen(core);

    // 3. Resume core: mark all dirty, restart refresh timer
    // The first refresh cycle will redraw the entire UI
    egui_core_resume(core);
}

void egui_core_set_screen_size(egui_core_t *core, int16_t width, int16_t height)
{
    core->screen_width = width;
    core->screen_height = height;

    // Recalculate PFB tile counts for the new screen size
    core->pfb_width_count = (width + core->pfb_width - 1) / core->pfb_width;
    core->pfb_height_count = (height + core->pfb_height - 1) / core->pfb_height;

    // Update root view group sizes
    egui_view_set_size((egui_view_t *)&core->scene.root_view_group, width, height);
#if EGUI_CONFIG_CORE_SEPARATE_USER_ROOT_GROUP_ENABLE
    egui_view_set_size((egui_view_t *)&core->scene.user_root_view_group, width, height);
#endif

    // Force full screen refresh
    egui_core_update_region_dirty_all(core);
}

void egui_core_setup_display_start(egui_core_t *core, const egui_display_setup_t *setup)
{
    egui_platform_register(core, setup->platform);
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

void egui_core_init_display_scene(egui_core_t *core, int16_t screen_w, int16_t screen_h)
{
    egui_core_update_region_dirty_all(core);
    egui_slist_init(&core->scene.anims);
    egui_dlist_init(&core->scene.activitys);

    core->scene.activity_anim_start_open = NULL;
    core->scene.activity_anim_start_close = NULL;
    core->scene.activity_anim_finish_open = NULL;
    core->scene.activity_anim_finish_close = NULL;
    core->scene.dialog_anim_start = NULL;
    core->scene.dialog_anim_finish = NULL;
    core->scene.dialog = NULL;
    core->scene.toast = NULL;

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

void egui_init_display(egui_core_t *core, int16_t screen_w, int16_t screen_h, egui_color_int_t **pfb_bufs, int buf_count, int pfb_w, int pfb_h)
{
    int i;

    memset(core, 0, sizeof(egui_core_t));
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
        egui_pfb_manager_add_buffer(&core->render.pfb_mgr, pfb_bufs[i]);
    }

#if EGUI_CONFIG_DEBUG_VIEW_ID
    core->unique_id = 0;
#endif
    core->system.is_suspended = 1;
    core->scene.dirty_epoch = 0;
    core->asset.theme_current = EGUI_CONFIG_THEME_DEFAULT;

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

void egui_setup_display(egui_core_t *core, const egui_display_setup_t *setup)
{
    EGUI_ASSERT(core != NULL);
    EGUI_ASSERT(setup != NULL);
    EGUI_ASSERT(setup->pfb_buffers != NULL);
    EGUI_ASSERT(setup->pfb_buffer_count > 0);
    EGUI_ASSERT(setup->display_driver != NULL);
    EGUI_ASSERT(setup->platform != NULL);

    egui_init_display(core, (int16_t)setup->screen_width, (int16_t)setup->screen_height, setup->pfb_buffers, setup->pfb_buffer_count, setup->pfb_width,
                      setup->pfb_height);
    core->id = setup->display_id;
    egui_core_setup_display_start(core, setup);
}

void egui_init(egui_core_t *core, egui_color_int_t pfb[][EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT])
{
    egui_color_int_t *pfb_bufs[EGUI_CONFIG_PFB_BUFFER_COUNT];
    int i;

    for (i = 0; i < EGUI_CONFIG_PFB_BUFFER_COUNT; i++)
    {
        pfb_bufs[i] = pfb[i];
    }

    egui_init_display(core, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT, pfb_bufs, EGUI_CONFIG_PFB_BUFFER_COUNT, EGUI_CONFIG_PFB_WIDTH,
                      EGUI_CONFIG_PFB_HEIGHT);
}

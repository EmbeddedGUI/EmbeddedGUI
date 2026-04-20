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

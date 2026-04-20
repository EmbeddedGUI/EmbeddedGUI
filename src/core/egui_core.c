#include <string.h>

#include "egui_api.h"
#include "egui_core.h"
#include "egui_core_internal.h"
#include "egui_core_touch.h"
#include "egui_display_driver.h"
#include "egui_input.h"
#include "egui_platform.h"
#include "egui_timer.h"

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

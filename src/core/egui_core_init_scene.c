#include "egui_api.h"
#include "egui_core.h"
#include "egui_core_internal.h"

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

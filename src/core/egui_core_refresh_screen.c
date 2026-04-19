#include "egui_api.h"
#include "egui_core.h"
#include "egui_core_internal.h"
#include "egui_display_driver.h"
#include "egui_timer.h"

__EGUI_WEAK__ void egui_port_notify_frame_render_complete(void)
{
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

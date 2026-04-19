#include <string.h>

#include "egui_api.h"
#include "egui_core.h"
#include "egui_core_internal.h"
#include "egui_core_touch.h"
#include "egui_display_driver.h"
#include "egui_input.h"
#include "egui_platform.h"
#include "egui_timer.h"

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

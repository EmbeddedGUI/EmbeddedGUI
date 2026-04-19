#include "egui_api.h"
#include "egui_core.h"
#include "egui_core_internal.h"
#include "egui_timer.h"

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

#include "egui_api.h"
#include "egui_core.h"
#include "egui_core_internal.h"

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

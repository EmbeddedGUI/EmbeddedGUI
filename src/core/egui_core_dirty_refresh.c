#include "egui_api.h"
#include "egui_core.h"
#include "egui_core_internal.h"

void egui_core_update_region_dirty_all(egui_core_t *core)
{
    EGUI_REGION_DEFINE(region_screen, 0, 0, core->screen_width, core->screen_height);
    egui_core_update_region_dirty(core, &region_screen);
}

void egui_core_force_refresh(egui_core_t *core)
{
    egui_core_update_region_dirty_all(core);
}

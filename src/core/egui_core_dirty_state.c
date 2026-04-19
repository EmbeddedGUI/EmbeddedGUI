#include "egui_api.h"
#include "egui_core.h"
#include "egui_core_internal.h"

int egui_core_check_region_dirty_intersect(egui_core_t *core, egui_region_t *region_dirty)
{
    int i;

    for (i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        egui_region_t *p_region = &core->scene.region_dirty_arr[i];

        if (egui_region_is_intersect(p_region, region_dirty))
        {
            return 1;
        }
    }

    return 0;
}

uint32_t egui_core_get_dirty_epoch(egui_core_t *core)
{
    return core->scene.dirty_epoch;
}

egui_region_t *egui_core_get_region_dirty_arr(egui_core_t *core)
{
    return core->scene.region_dirty_arr;
}

void egui_core_clear_region_dirty(egui_core_t *core)
{
    int i;

    for (i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        egui_region_init_empty(&core->scene.region_dirty_arr[i]);
    }

    core->scene.dirty_epoch++;
}

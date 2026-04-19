#include "egui_api.h"
#include "egui_core.h"
#include "egui_core_internal.h"

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

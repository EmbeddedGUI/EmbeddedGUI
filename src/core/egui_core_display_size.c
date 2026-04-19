#include "egui_api.h"
#include "egui_core.h"
#include "egui_core_internal.h"

void egui_core_set_screen_size(egui_core_t *core, int16_t width, int16_t height)
{
    core->screen_width = width;
    core->screen_height = height;

    // Recalculate PFB tile counts for the new screen size
    core->pfb_width_count = (width + core->pfb_width - 1) / core->pfb_width;
    core->pfb_height_count = (height + core->pfb_height - 1) / core->pfb_height;

    // Update root view group sizes
    egui_view_set_size((egui_view_t *)&core->scene.root_view_group, width, height);
#if EGUI_CONFIG_CORE_SEPARATE_USER_ROOT_GROUP_ENABLE
    egui_view_set_size((egui_view_t *)&core->scene.user_root_view_group, width, height);
#endif

    // Force full screen refresh
    egui_core_update_region_dirty_all(core);
}

#include "egui_api.h"
#include "egui_core.h"
#include "egui_core_internal.h"

void egui_core_draw_view_group_pre_work(egui_core_t *core)
{
    egui_view_group_t *view_group = (egui_view_group_t *)&core->scene.root_view_group;

    // Calculate the layout of the view group
    view_group->base.api->compute_scroll((egui_view_t *)view_group);

    // Calculate the layout of the view group
    view_group->base.api->calculate_layout((egui_view_t *)view_group);
}

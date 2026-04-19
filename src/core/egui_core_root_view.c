#include "egui_api.h"
#include "egui_core.h"
#include "egui_core_internal.h"

egui_view_group_t *egui_core_get_root_view(egui_core_t *core)
{
    return (egui_view_group_t *)&core->scene.root_view_group;
}

void egui_core_add_root_view(egui_core_t *core, egui_view_t *view)
{
    egui_view_group_t *target_parent;

    if (core == NULL || view == NULL)
    {
        return;
    }

    target_parent = (egui_view_group_t *)EGUI_VIEW_OF(&core->scene.root_view_group);
    if (view->parent != NULL && view->parent != target_parent)
    {
        egui_view_group_remove_child(EGUI_VIEW_OF(view->parent), view);
    }

    EGUI_ASSERT(egui_view_get_core(view) == core);

    if (view->parent == NULL)
    {
        egui_view_group_add_child(EGUI_VIEW_OF(target_parent), view);
    }
}

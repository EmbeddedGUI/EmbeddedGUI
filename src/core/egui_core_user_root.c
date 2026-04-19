#include "egui_api.h"
#include "egui_core.h"
#include "egui_core_internal.h"

#if EGUI_CONFIG_CORE_SEPARATE_USER_ROOT_GROUP_ENABLE
#define EGUI_CORE_USER_ROOT_VIEW_PTR(_core) ((egui_view_t *)&(_core)->scene.user_root_view_group)
#else
#define EGUI_CORE_USER_ROOT_VIEW_PTR(_core) ((egui_view_t *)&(_core)->scene.root_view_group)
#endif

egui_view_group_t *egui_core_get_user_root_view(egui_core_t *core)
{
    return (egui_view_group_t *)EGUI_CORE_USER_ROOT_VIEW_PTR(core);
}

void egui_core_add_user_root_view(egui_view_t *view)
{
    egui_core_t *core;
    egui_view_group_t *target_parent;

    if (view == NULL)
    {
        return;
    }

    core = egui_view_get_core(view);
    if (core == NULL)
    {
        return;
    }

    target_parent = (egui_view_group_t *)EGUI_CORE_USER_ROOT_VIEW_PTR(core);
    if (view->parent != NULL && view->parent != target_parent)
    {
        egui_view_group_remove_child(EGUI_VIEW_OF(view->parent), view);
    }

    if (view->parent == NULL)
    {
        egui_view_group_add_child(EGUI_VIEW_OF(target_parent), view);
    }
}

void egui_core_remove_user_root_view(egui_core_t *core, egui_view_t *view)
{
    egui_view_group_remove_child(EGUI_CORE_USER_ROOT_VIEW_PTR(core), view);
    egui_core_update_region_dirty_all(core);
}

void egui_core_layout_childs_user_root_view(egui_core_t *core, uint8_t is_orientation_horizontal, uint8_t align_type)
{
    egui_view_group_layout_childs(EGUI_CORE_USER_ROOT_VIEW_PTR(core), is_orientation_horizontal, 0, 0, align_type);
}

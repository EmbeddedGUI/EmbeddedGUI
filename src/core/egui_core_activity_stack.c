#include "egui_api.h"
#include "egui_common.h"
#include "egui_core.h"
#include "egui_core_internal.h"
#include "app/egui_activity.h"
#include "widget/egui_view.h"
#include "utils/egui_dlist.h"

egui_activity_t *egui_core_activity_get_by_view(egui_core_t *core, egui_view_t *view)
{
    if (core == NULL || view == NULL)
    {
        return NULL;
    }

    // Walk ancestors until user root and match any activity root_view on the path.
    while (view != NULL)
    {
        egui_dnode_t *p_head;
        egui_activity_t *tmp;

        if (view == (egui_view_t *)&core->scene.user_root_view_group)
        {
            break;
        }

        if (!egui_dlist_is_empty(&core->scene.activitys))
        {
            EGUI_DLIST_FOR_EACH_NODE(&core->scene.activitys, p_head)
            {
                tmp = EGUI_DLIST_ENTRY(p_head, egui_activity_t, node);

                if (view == EGUI_VIEW_OF(&tmp->root_view))
                {
                    return tmp;
                }
            }
        }

        view = (egui_view_t *)view->parent;
    }

    return NULL;
}

int egui_core_activity_check_in_process(egui_core_t *core, egui_activity_t *activity)
{
    egui_dnode_t *p_head;
    egui_activity_t *tmp;

    if (!egui_dlist_is_empty(&core->scene.activitys))
    {
        EGUI_DLIST_FOR_EACH_NODE(&core->scene.activitys, p_head)
        {
            tmp = EGUI_DLIST_ENTRY(p_head, egui_activity_t, node);

            if (activity == tmp)
            {
                return 1;
            }
        }
    }

    return 0;
}

void egui_core_activity_append(egui_core_t *core, egui_activity_t *activity)
{
    egui_dlist_append(&core->scene.activitys, &activity->node);
}

void egui_core_activity_remove(egui_core_t *core, egui_activity_t *activity)
{
    egui_dlist_remove(&activity->node);
}

egui_activity_t *egui_core_activity_get_current(egui_core_t *core)
{
    egui_dnode_t *tmp = egui_dlist_peek_tail(&core->scene.activitys);
    if (tmp == NULL)
    {
        return NULL;
    }

    return EGUI_DLIST_ENTRY(tmp, egui_activity_t, node);
}

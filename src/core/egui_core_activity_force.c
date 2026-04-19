#include "egui_api.h"
#include "egui_common.h"
#include "egui_core.h"
#include "egui_core_internal.h"
#include "app/egui_activity.h"
#include "widget/egui_view.h"
#include "utils/egui_dlist.h"

void egui_core_activity_force_finish_to_activity(egui_core_t *core, egui_activity_t *activity)
{
    // find the activity in the activitys list
    egui_dnode_t *p_head;
    egui_dnode_t *p_next;
    egui_activity_t *tmp;

    EGUI_LOG_DBG("egui_core_activity_force_finish_to_activity(), %d\n", egui_dlist_is_empty(&core->scene.activitys));
    if (!egui_dlist_is_empty(&core->scene.activitys))
    {
        EGUI_LOG_DBG("egui_core_activity_force_finish_to_activity() work\n");
        EGUI_DLIST_FOR_EACH_NODE_REVERSE_SAFE(&core->scene.activitys, p_head, p_next)
        {
            tmp = EGUI_DLIST_ENTRY(p_head, egui_activity_t, node);
            if (tmp == activity)
            {
                break;
            }
#if EGUI_CONFIG_DEBUG_CLASS_NAME
            EGUI_LOG_INF("force finish activity: %s, state: %d\n", tmp->name, tmp->state);
#endif
            if (tmp->state < EGUI_ACTIVITY_STATE_STOP)
            {
                tmp->api->on_stop(tmp);
            }
            if (tmp->state < EGUI_ACTIVITY_STATE_DESTROY)
            {
                tmp->api->on_destroy(tmp);
            }
        }
    }

#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_INF("restart activity: %s, state: %d\n", activity->name, activity->state);
#endif
    if (activity->state == EGUI_ACTIVITY_STATE_STOP)
    {
        activity->api->on_start(activity);
    }
    if (activity->state == EGUI_ACTIVITY_STATE_START)
    {
        activity->api->on_resume(activity);

        egui_view_set_position((egui_view_t *)&activity->root_view, 0, 0);
        egui_view_set_size((egui_view_t *)&activity->root_view, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    }

    // refresh the screen
    egui_core_update_region_dirty_all(core);
}

void egui_core_activity_force_finish_all(egui_core_t *core)
{
    // find the activity in the activitys list
    egui_dnode_t *p_head;
    egui_dnode_t *p_next;
    egui_activity_t *tmp;

    EGUI_LOG_DBG("egui_core_activity_force_finish_all(), %d\n", egui_dlist_is_empty(&core->scene.activitys));
    if (!egui_dlist_is_empty(&core->scene.activitys))
    {
        EGUI_LOG_DBG("egui_core_activity_force_finish_all() work\n");
        EGUI_DLIST_FOR_EACH_NODE_SAFE(&core->scene.activitys, p_head, p_next)
        {
            tmp = EGUI_DLIST_ENTRY(p_head, egui_activity_t, node);
#if EGUI_CONFIG_DEBUG_CLASS_NAME
            EGUI_LOG_DBG("force finish activity: %s, state: %d\n", tmp->name, tmp->state);
#endif
            if (tmp->state < EGUI_ACTIVITY_STATE_STOP)
            {
                tmp->api->on_stop(tmp);
            }
            if (tmp->state < EGUI_ACTIVITY_STATE_DESTROY)
            {
                tmp->api->on_destroy(tmp);
            }
        }
    }

    // refresh the screen
    egui_core_update_region_dirty_all(core);
}

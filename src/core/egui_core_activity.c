#include "egui_api.h"
#include "egui_common.h"
#include "egui_core.h"
#include "egui_core_internal.h"
#include "anim/egui_animation.h"
#include "app/egui_activity.h"
#include "widget/egui_view.h"
#include "utils/egui_dlist.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_ACTIVITY

egui_activity_t *egui_core_activity_get_by_view(egui_view_t *view)
{
    egui_view_t *view_activity = NULL;
    int found = 0;
    // the activity is in the parent view group, and is in user_root_view_group
    while (view)
    {
        if (view == (egui_view_t *)&egui_core.user_root_view_group)
        {
            found = 1;
            break;
        }

        view_activity = view;
        view = (egui_view_t *)view->parent;
    }

    if (found)
    {
        // find the activity in the activitys list
        egui_dnode_t *p_head;
        egui_activity_t *tmp;

        if (!egui_dlist_is_empty(&egui_core.activitys))
        {
            EGUI_DLIST_FOR_EACH_NODE(&egui_core.activitys, p_head)
            {
                tmp = EGUI_DLIST_ENTRY(p_head, egui_activity_t, node);

                if (view_activity == (egui_view_t *)&tmp->root_view)
                {
                    return tmp;
                }
            }
        }
    }

    return NULL;
}

int egui_core_activity_check_in_process(egui_activity_t *activity)
{
    // find the activity in the activitys list
    egui_dnode_t *p_head;
    egui_activity_t *tmp;

    if (!egui_dlist_is_empty(&egui_core.activitys))
    {
        EGUI_DLIST_FOR_EACH_NODE(&egui_core.activitys, p_head)
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

void egui_core_activity_append(egui_activity_t *activity)
{
    egui_dlist_append(&egui_core.activitys, &activity->node);
}

void egui_core_activity_remove(egui_activity_t *activity)
{
    egui_dlist_remove(&activity->node);
}

void egui_core_activity_start(egui_activity_t *self, egui_activity_t *prev_activity)
{
    egui_core.activity_open = self;
    egui_core.activity_close = prev_activity;

    if (self == prev_activity)
    {
        return;
    }

    self->api->on_create(self);

    if (egui_core.activity_anim_start_open != NULL)
    {
        egui_animation_target_view_set(egui_core.activity_anim_start_open, (egui_view_t *)&self->root_view);
        egui_animation_start(egui_core.activity_anim_start_open);
    }
    else
    {
        self->api->on_resume(self);
    }

    if (prev_activity)
    {
        prev_activity->api->on_pause(prev_activity);
        // check anim
        if (egui_core.activity_anim_start_close != NULL)
        {
            egui_animation_target_view_set(egui_core.activity_anim_start_close, (egui_view_t *)&prev_activity->root_view);
            egui_animation_start(egui_core.activity_anim_start_close);
        }
        else
        {
            prev_activity->api->on_stop(prev_activity);
        }
    }
}

void egui_core_activity_start_with_current(egui_activity_t *self)
{
    egui_core_activity_start(self, egui_core_activity_get_current());
}

void egui_core_activity_finish(egui_activity_t *self)
{
    // find a last activity to start
    egui_dnode_t *p_prev = egui_dlist_peek_prev(&egui_core.activitys, &self->node);
    egui_core.activity_close = self;

    // avoid enter twice
    if (self->is_need_finish)
    {
        return;
    }
    self->api->on_pause(self);
    self->is_need_finish = true;
    if (self->state < EGUI_ACTIVITY_STATE_STOP)
    {
        // check anim
        if (egui_core.activity_anim_finish_close != NULL)
        {
            egui_animation_target_view_set(egui_core.activity_anim_finish_close, (egui_view_t *)&self->root_view);
            egui_animation_start(egui_core.activity_anim_finish_close);
        }
        else
        {
            self->api->on_stop(self);
        }
    }
    else
    {
        // something error.
        if (self->state < EGUI_ACTIVITY_STATE_DESTROY)
        {
            self->api->on_destroy(self);
        }
    }

    if (p_prev)
    {
        egui_activity_t *tmp = EGUI_DLIST_ENTRY(p_prev, egui_activity_t, node);
        egui_core.activity_open = tmp;

        tmp->api->on_start(tmp);

        if (egui_core.activity_anim_finish_open != NULL)
        {
            egui_animation_target_view_set(egui_core.activity_anim_finish_open, (egui_view_t *)&tmp->root_view);
            egui_animation_start(egui_core.activity_anim_finish_open);
        }
        else
        {
            tmp->api->on_resume(tmp);
        }
    }

    // refresh the screen
    egui_core_update_region_dirty_all();
}

egui_activity_t *egui_core_activity_get_current(void)
{
    egui_dnode_t *tmp = egui_dlist_peek_tail(&egui_core.activitys);
    if (tmp == NULL)
    {
        return NULL;
    }
    egui_activity_t *p_activity = EGUI_DLIST_ENTRY(tmp, egui_activity_t, node);
    return p_activity;
}

void egui_core_activity_force_finish_to_activity(egui_activity_t *activity)
{
    // find the activity in the activitys list
    egui_dnode_t *p_head;
    egui_dnode_t *p_next;
    egui_activity_t *tmp;

    EGUI_LOG_DBG("egui_core_activity_force_finish_to_activity(), %d\n", egui_dlist_is_empty(&egui_core.activitys));
    if (!egui_dlist_is_empty(&egui_core.activitys))
    {
        EGUI_LOG_DBG("egui_core_activity_force_finish_to_activity() work\n");
        EGUI_DLIST_FOR_EACH_NODE_REVERSE_SAFE(&egui_core.activitys, p_head, p_next)
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
    egui_core_update_region_dirty_all();
}

void egui_core_activity_force_finish_all(void)
{
    // find the activity in the activitys list
    egui_dnode_t *p_head;
    egui_dnode_t *p_next;
    egui_activity_t *tmp;

    EGUI_LOG_DBG("egui_core_activity_force_finish_all(), %d\n", egui_dlist_is_empty(&egui_core.activitys));
    if (!egui_dlist_is_empty(&egui_core.activitys))
    {
        EGUI_LOG_DBG("egui_core_activity_force_finish_all() work\n");
        EGUI_DLIST_FOR_EACH_NODE_SAFE(&egui_core.activitys, p_head, p_next)
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
    egui_core_update_region_dirty_all();
}

static void on_activity_anim_start_open_end(egui_animation_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_activity_anim_start_open_end\n");
#endif
    if (egui_core.activity_open)
    {
        egui_core.activity_open->api->on_resume(egui_core.activity_open);
    }
}

static const egui_animation_handle_t activity_anim_start_open_hanlde = {
        .start = NULL,
        .end = on_activity_anim_start_open_end,
        .repeat = NULL,
};

static void on_activity_anim_start_close_end(egui_animation_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_activity_anim_start_close_end\n");
#endif
    if (egui_core.activity_close)
    {
        egui_core.activity_close->api->on_stop(egui_core.activity_close);
    }
}

static const egui_animation_handle_t activity_anim_start_close_hanlde = {
        .start = NULL,
        .end = on_activity_anim_start_close_end,
        .repeat = NULL,
};

static void on_activity_anim_finish_open_end(egui_animation_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_activity_anim_finish_open_end\n");
#endif
    if (egui_core.activity_open)
    {
        egui_core.activity_open->api->on_resume(egui_core.activity_open);
    }
}

static const egui_animation_handle_t activity_anim_finish_open_hanlde = {
        .start = NULL,
        .end = on_activity_anim_finish_open_end,
        .repeat = NULL,
};

static void on_activity_anim_finish_close_end(egui_animation_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_activity_anim_finish_close_end\n");
#endif

    if (egui_core.activity_close)
    {
        egui_core.activity_close->api->on_stop(egui_core.activity_close);
    }
}

static const egui_animation_handle_t activity_anim_finish_close_hanlde = {
        .start = NULL,
        .end = on_activity_anim_finish_close_end,
        .repeat = NULL,
};

void egui_core_activity_set_start_anim(egui_animation_t *open_anim, egui_animation_t *close_anim)
{
    egui_core.activity_anim_start_open = open_anim;
    egui_core.activity_anim_start_close = close_anim;

    if (open_anim)
    {
        egui_animation_handle_set(open_anim, &activity_anim_start_open_hanlde);
    }
    if (close_anim)
    {
        egui_animation_handle_set(close_anim, &activity_anim_start_close_hanlde);
    }
}

void egui_core_activity_set_finish_anim(egui_animation_t *open_anim, egui_animation_t *close_anim)
{
    egui_core.activity_anim_finish_open = open_anim;
    egui_core.activity_anim_finish_close = close_anim;

    if (open_anim)
    {
        egui_animation_handle_set(open_anim, &activity_anim_finish_open_hanlde);
    }
    if (close_anim)
    {
        egui_animation_handle_set(close_anim, &activity_anim_finish_close_hanlde);
    }
}

#endif

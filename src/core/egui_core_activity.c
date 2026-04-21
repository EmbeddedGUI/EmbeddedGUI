#include "egui_api.h"
#include "egui_common.h"
#include "egui_core.h"
#include "egui_core_activity.h"
#include "egui_core_internal.h"
#include "anim/egui_animation.h"
#include "app/egui_activity.h"
#include "widget/egui_view.h"
#include "utils/egui_dlist.h"

static int egui_core_activity_is_active_candidate(const egui_activity_t *activity)
{
    return (activity != NULL) && (!activity->is_need_finish) && (activity->state < EGUI_ACTIVITY_STATE_STOP);
}

static int egui_core_activity_is_resume_candidate(const egui_activity_t *activity)
{
    return (activity != NULL) && (!activity->is_need_finish) && (activity->state > EGUI_ACTIVITY_STATE_NONE) &&
           (activity->state < EGUI_ACTIVITY_STATE_DESTROY);
}

static egui_activity_t *egui_core_activity_find_previous_candidate(egui_core_t *core, egui_activity_t *activity,
                                                                   int (*is_candidate)(const egui_activity_t *activity))
{
    egui_dnode_t *node;

    if (core == NULL || is_candidate == NULL || egui_dlist_is_empty(&core->scene.activitys))
    {
        return NULL;
    }

    if (activity != NULL && egui_core_activity_check_in_process(core, activity))
    {
        node = egui_dlist_peek_prev(&core->scene.activitys, &activity->node);
    }
    else
    {
        node = egui_dlist_peek_tail(&core->scene.activitys);
    }

    while (node != NULL)
    {
        egui_activity_t *candidate = EGUI_DLIST_ENTRY(node, egui_activity_t, node);

        if (is_candidate(candidate))
        {
            return candidate;
        }

        node = egui_dlist_peek_prev(&core->scene.activitys, node);
    }

    return NULL;
}

static egui_activity_t *egui_core_activity_find_previous_active(egui_core_t *core, egui_activity_t *activity)
{
    return egui_core_activity_find_previous_candidate(core, activity, egui_core_activity_is_active_candidate);
}

static egui_activity_t *egui_core_activity_find_previous_resume(egui_core_t *core, egui_activity_t *activity)
{
    return egui_core_activity_find_previous_candidate(core, activity, egui_core_activity_is_resume_candidate);
}

static egui_activity_t *egui_core_activity_resolve_prev_activity(egui_core_t *core, egui_activity_t *activity)
{
    if (activity == NULL)
    {
        return NULL;
    }

    if (egui_activity_get_core(activity) != core || !egui_core_activity_check_in_process(core, activity))
    {
        return NULL;
    }

    if (egui_core_activity_is_active_candidate(activity))
    {
        return activity;
    }

    return egui_core_activity_find_previous_active(core, activity);
}

static void egui_core_activity_cancel_anim_slot(egui_animation_t *anim, egui_activity_t **owner_slot, egui_activity_t *activity)
{
    if (owner_slot == NULL || *owner_slot != activity)
    {
        return;
    }

    if (anim != NULL && anim->is_running)
    {
        egui_animation_complete(anim);
    }

    *owner_slot = NULL;
}

static void egui_core_activity_flush_resume_slot(egui_animation_t *anim, egui_activity_t **owner_slot)
{
    egui_activity_t *activity;

    if (owner_slot == NULL)
    {
        return;
    }

    activity = *owner_slot;
    if (activity == NULL)
    {
        return;
    }

    if (anim != NULL && anim->is_running)
    {
        egui_animation_complete(anim);
    }

    if (*owner_slot != activity)
    {
        return;
    }

    *owner_slot = NULL;

    if (egui_core_activity_is_active_candidate(activity))
    {
        activity->api->on_resume(activity);
    }
}

static void egui_core_activity_flush_stop_slot(egui_animation_t *anim, egui_activity_t **owner_slot)
{
    egui_activity_t *activity;

    if (owner_slot == NULL)
    {
        return;
    }

    activity = *owner_slot;
    if (activity == NULL)
    {
        return;
    }

    if (anim != NULL && anim->is_running)
    {
        egui_animation_complete(anim);
    }

    if (*owner_slot != activity)
    {
        return;
    }

    *owner_slot = NULL;

    if (activity->state < EGUI_ACTIVITY_STATE_STOP)
    {
        activity->api->on_stop(activity);
    }
}

static void egui_core_activity_cancel_pending(egui_core_t *core, egui_activity_t *activity)
{
    if (core == NULL || activity == NULL)
    {
        return;
    }

    egui_core_activity_cancel_anim_slot(core->scene.activity_anim_start_open, &core->scene.activity_anim_start_open_owner, activity);
    egui_core_activity_cancel_anim_slot(core->scene.activity_anim_start_close, &core->scene.activity_anim_start_close_owner, activity);
    egui_core_activity_cancel_anim_slot(core->scene.activity_anim_finish_open, &core->scene.activity_anim_finish_open_owner, activity);
    egui_core_activity_cancel_anim_slot(core->scene.activity_anim_finish_close, &core->scene.activity_anim_finish_close_owner, activity);

    if (core->scene.activity_open == activity)
    {
        core->scene.activity_open = NULL;
    }
    if (core->scene.activity_close == activity)
    {
        core->scene.activity_close = NULL;
    }
}

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
    egui_dnode_t *tmp;

    if (core == NULL)
    {
        return NULL;
    }

    tmp = egui_dlist_peek_tail(&core->scene.activitys);
    if (tmp == NULL)
    {
        return NULL;
    }

    return EGUI_DLIST_ENTRY(tmp, egui_activity_t, node);
}

egui_activity_t *egui_core_activity_get_current_active(egui_core_t *core)
{
    return egui_core_activity_find_previous_active(core, NULL);
}

static void on_activity_anim_start_open_end(egui_animation_t *self)
{
    egui_core_t *core = egui_view_get_core(self->target_view);
    egui_activity_t *target = NULL;

    if (core != NULL)
    {
        target = core->scene.activity_anim_start_open_owner;
        core->scene.activity_anim_start_open_owner = NULL;
    }
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_activity_anim_start_open_end\n");
#endif
    if (core == NULL || !egui_core_activity_is_active_candidate(target))
    {
        return;
    }

    target->api->on_resume(target);
}

static const egui_animation_handle_t activity_anim_start_open_hanlde = {
        .start = NULL,
        .end = on_activity_anim_start_open_end,
        .repeat = NULL,
};

static void on_activity_anim_start_close_end(egui_animation_t *self)
{
    egui_core_t *core = egui_view_get_core(self->target_view);
    egui_activity_t *target = NULL;

    if (core != NULL)
    {
        target = core->scene.activity_anim_start_close_owner;
        core->scene.activity_anim_start_close_owner = NULL;
    }
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_activity_anim_start_close_end\n");
#endif
    if (core == NULL || target == NULL || target->state >= EGUI_ACTIVITY_STATE_STOP)
    {
        return;
    }

    egui_core_activity_cancel_pending(core, target);
    target->api->on_stop(target);
}

static const egui_animation_handle_t activity_anim_start_close_hanlde = {
        .start = NULL,
        .end = on_activity_anim_start_close_end,
        .repeat = NULL,
};

static void on_activity_anim_finish_open_end(egui_animation_t *self)
{
    egui_core_t *core = egui_view_get_core(self->target_view);
    egui_activity_t *target = NULL;

    if (core != NULL)
    {
        target = core->scene.activity_anim_finish_open_owner;
        core->scene.activity_anim_finish_open_owner = NULL;
    }
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_activity_anim_finish_open_end\n");
#endif
    if (core == NULL || !egui_core_activity_is_active_candidate(target))
    {
        return;
    }

    target->api->on_resume(target);
}

static const egui_animation_handle_t activity_anim_finish_open_hanlde = {
        .start = NULL,
        .end = on_activity_anim_finish_open_end,
        .repeat = NULL,
};

static void on_activity_anim_finish_close_end(egui_animation_t *self)
{
    egui_core_t *core = egui_view_get_core(self->target_view);
    egui_activity_t *target = NULL;

    if (core != NULL)
    {
        target = core->scene.activity_anim_finish_close_owner;
        core->scene.activity_anim_finish_close_owner = NULL;
    }
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_activity_anim_finish_close_end\n");
#endif
    if (core == NULL || target == NULL || target->state >= EGUI_ACTIVITY_STATE_STOP)
    {
        return;
    }

    egui_core_activity_cancel_pending(core, target);
    target->api->on_stop(target);
}

static const egui_animation_handle_t activity_anim_finish_close_hanlde = {
        .start = NULL,
        .end = on_activity_anim_finish_close_end,
        .repeat = NULL,
};

void egui_core_activity_set_start_anim(egui_core_t *core, egui_animation_t *open_anim, egui_animation_t *close_anim)
{
    egui_core_activity_flush_resume_slot(core->scene.activity_anim_start_open, &core->scene.activity_anim_start_open_owner);
    egui_core_activity_flush_stop_slot(core->scene.activity_anim_start_close, &core->scene.activity_anim_start_close_owner);

    core->scene.activity_anim_start_open = open_anim;
    core->scene.activity_anim_start_close = close_anim;

    if (open_anim)
    {
        egui_animation_handle_set(open_anim, &activity_anim_start_open_hanlde);
    }
    if (close_anim)
    {
        egui_animation_handle_set(close_anim, &activity_anim_start_close_hanlde);
    }
}

void egui_core_activity_set_finish_anim(egui_core_t *core, egui_animation_t *open_anim, egui_animation_t *close_anim)
{
    egui_core_activity_flush_resume_slot(core->scene.activity_anim_finish_open, &core->scene.activity_anim_finish_open_owner);
    egui_core_activity_flush_stop_slot(core->scene.activity_anim_finish_close, &core->scene.activity_anim_finish_close_owner);

    core->scene.activity_anim_finish_open = open_anim;
    core->scene.activity_anim_finish_close = close_anim;

    if (open_anim)
    {
        egui_animation_handle_set(open_anim, &activity_anim_finish_open_hanlde);
    }
    if (close_anim)
    {
        egui_animation_handle_set(close_anim, &activity_anim_finish_close_hanlde);
    }
}

void egui_core_activity_start(egui_core_t *core, egui_activity_t *self, egui_activity_t *prev_activity)
{
    egui_activity_t *resolved_prev;

    if (core == NULL || self == NULL)
    {
        return;
    }

    resolved_prev = egui_core_activity_resolve_prev_activity(core, prev_activity);

    if (self == resolved_prev)
    {
        return;
    }

    if (egui_activity_get_core(self) != core || egui_view_get_core(EGUI_VIEW_OF(&self->root_view)) != core)
    {
        return;
    }

    egui_core_activity_cancel_pending(core, self);
    egui_core_activity_cancel_pending(core, resolved_prev);
    core->scene.activity_open = self;
    core->scene.activity_close = resolved_prev;
    self->api->on_create(self);

    if (core->scene.activity_anim_start_open != NULL)
    {
        egui_core_activity_flush_resume_slot(core->scene.activity_anim_start_open, &core->scene.activity_anim_start_open_owner);
        core->scene.activity_anim_start_open_owner = self;
        egui_animation_target_view_set(core->scene.activity_anim_start_open, (egui_view_t *)&self->root_view);
        egui_animation_start(core->scene.activity_anim_start_open);
    }
    else
    {
        core->scene.activity_anim_start_open_owner = NULL;
        self->api->on_resume(self);
    }

    if (resolved_prev)
    {
        resolved_prev->api->on_pause(resolved_prev);
        // check anim
        if (core->scene.activity_anim_start_close != NULL)
        {
            egui_core_activity_flush_stop_slot(core->scene.activity_anim_start_close, &core->scene.activity_anim_start_close_owner);
            core->scene.activity_anim_start_close_owner = resolved_prev;
            egui_animation_target_view_set(core->scene.activity_anim_start_close, (egui_view_t *)&resolved_prev->root_view);
            egui_animation_start(core->scene.activity_anim_start_close);
        }
        else
        {
            core->scene.activity_anim_start_close_owner = NULL;
            resolved_prev->api->on_stop(resolved_prev);
        }
    }
    else
    {
        core->scene.activity_anim_start_close_owner = NULL;
    }
}

void egui_core_activity_finish(egui_core_t *core, egui_activity_t *self)
{
    // find a last activity to start
    egui_activity_t *prev_activity = egui_core_activity_find_previous_resume(core, self);

    // avoid enter twice
    if (self->is_need_finish)
    {
        return;
    }

    egui_core_activity_cancel_pending(core, self);
    core->scene.activity_close = self;
    self->api->on_pause(self);
    self->is_need_finish = true;
    if (self->state < EGUI_ACTIVITY_STATE_STOP)
    {
        // check anim
        if (core->scene.activity_anim_finish_close != NULL)
        {
            egui_core_activity_flush_stop_slot(core->scene.activity_anim_finish_close, &core->scene.activity_anim_finish_close_owner);
            core->scene.activity_anim_finish_close_owner = self;
            egui_animation_target_view_set(core->scene.activity_anim_finish_close, (egui_view_t *)&self->root_view);
            egui_animation_start(core->scene.activity_anim_finish_close);
        }
        else
        {
            core->scene.activity_anim_finish_close_owner = NULL;
            self->api->on_stop(self);
        }
    }
    else
    {
        core->scene.activity_anim_finish_close_owner = NULL;
        // something error.
        if (self->state < EGUI_ACTIVITY_STATE_DESTROY)
        {
            self->api->on_destroy(self);
        }
    }

    if (prev_activity)
    {
        egui_core_activity_cancel_pending(core, prev_activity);
        core->scene.activity_open = prev_activity;

        prev_activity->api->on_start(prev_activity);

        if (core->scene.activity_anim_finish_open != NULL)
        {
            egui_core_activity_flush_resume_slot(core->scene.activity_anim_finish_open, &core->scene.activity_anim_finish_open_owner);
            core->scene.activity_anim_finish_open_owner = prev_activity;
            egui_animation_target_view_set(core->scene.activity_anim_finish_open, (egui_view_t *)&prev_activity->root_view);
            egui_animation_start(core->scene.activity_anim_finish_open);
        }
        else
        {
            core->scene.activity_anim_finish_open_owner = NULL;
            prev_activity->api->on_resume(prev_activity);
        }
    }
    else
    {
        core->scene.activity_open = NULL;
        core->scene.activity_anim_finish_open_owner = NULL;
    }

    // refresh the screen
    egui_core_update_region_dirty_all(core);
}

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
            egui_core_activity_cancel_pending(core, tmp);
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
            egui_core_activity_cancel_pending(core, tmp);
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

#include "egui_api.h"
#include "egui_common.h"
#include "egui_core.h"
#include "egui_core_activity.h"
#include "egui_core_internal.h"
#include "anim/egui_animation.h"
#include "app/egui_activity.h"
#include "widget/egui_view.h"
#include "utils/egui_dlist.h"

/**
 * @file egui_core_activity.c
 * @brief Core-side activity stack management, including transition animation ownership and lifecycle handoff.
 */

#if EGUI_CONFIG_FUNCTION_SUPPORT_ACTIVITY

/** Return non-zero when the activity can be treated as the currently visible or resumable foreground page. */
static int egui_core_activity_is_active_candidate(const egui_activity_t *activity)
{
    return (activity != NULL) && (!activity->is_need_finish) && (activity->state < EGUI_ACTIVITY_STATE_STOP);
}

/** Return non-zero when the activity is still alive enough to be restarted after the top activity finishes. */
static int egui_core_activity_is_resume_candidate(const egui_activity_t *activity)
{
    return (activity != NULL) && (!activity->is_need_finish) && (activity->state > EGUI_ACTIVITY_STATE_NONE) && (activity->state < EGUI_ACTIVITY_STATE_DESTROY);
}

/** Walk backward through the activity stack and return the first entry that matches the supplied predicate. */
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

/** Find the nearest earlier activity that is still considered active. */
static egui_activity_t *egui_core_activity_find_previous_active(egui_core_t *core, egui_activity_t *activity)
{
    return egui_core_activity_find_previous_candidate(core, activity, egui_core_activity_is_active_candidate);
}

/** Find the nearest earlier activity that can be started or resumed again. */
static egui_activity_t *egui_core_activity_find_previous_resume(egui_core_t *core, egui_activity_t *activity)
{
    return egui_core_activity_find_previous_candidate(core, activity, egui_core_activity_is_resume_candidate);
}

/** Validate or repair an explicit previous-activity hint before a start transition uses it. */
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

/** Cancel one animation slot only when it still belongs to the target activity. */
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

/** Flush a pending open animation slot so its owner reaches on_resume before the slot is replaced. */
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

/** Flush a pending close animation slot so its owner reaches on_stop before the slot is replaced. */
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

/** Remove every pending transition reference that still points at the supplied activity. */
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

/** Resolve which activity owns a view by walking ancestors until one of the registered activity roots is found. */
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

        if (view == (egui_view_t *)egui_core_get_user_root_view(core))
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

/** Return non-zero when the activity is currently tracked inside this core's activity stack. */
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

/** Append one activity to the tail of the core-owned activity stack. */
void egui_core_activity_append(egui_core_t *core, egui_activity_t *activity)
{
    egui_dlist_append(&core->scene.activitys, &activity->node);
}

/** Remove one activity from the core-owned activity stack. */
void egui_core_activity_remove(egui_core_t *core, egui_activity_t *activity)
{
    EGUI_UNUSED(core);
    egui_dlist_remove(&activity->node);
}

/** Return the tail entry of the activity stack, which is the newest stacked activity. */
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

/** Return the newest activity that is still eligible to stay active. */
egui_activity_t *egui_core_activity_get_current_active(egui_core_t *core)
{
    return egui_core_activity_find_previous_active(core, NULL);
}

/** Complete the start-open transition by resuming the activity that just became visible. */
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

/** Complete the start-close transition by stopping the activity that just moved behind the new foreground page. */
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

/** Complete the finish-open transition by resuming the activity revealed after the top page finishes. */
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

/** Complete the finish-close transition by stopping the activity that is being removed from the stack. */
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

/** Install the animations used when pushing a new activity on top of the stack. */
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

/** Install the animations used when finishing the top activity and revealing the previous one. */
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

/**
 * Start a new activity transition.
 * The new activity is created first, then it resumes immediately or after its open animation, while the previous foreground activity pauses and later stops.
 */
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
        // The open animation owns the deferred on_resume callback for the entering activity.
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
        // The previous foreground activity either stops immediately or after its close animation ends.
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

/**
 * Finish the current activity and reveal the nearest resumable activity underneath it.
 * The closing activity pauses first, then stops or destroys after the configured finish-close transition, while the revealed activity starts/resumes on the way
 * back.
 */
void egui_core_activity_finish(egui_core_t *core, egui_activity_t *self)
{
    // Find the activity that should become foreground once the current one finishes.
    egui_activity_t *prev_activity = egui_core_activity_find_previous_resume(core, self);

    // Ignore duplicate finish requests.
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
        // The finishing activity is stopped immediately or by the finish-close animation callback.
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
            // The reveal animation owns the deferred on_resume callback for the uncovered activity.
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

    // Transition animations and root attach/detach now invalidate the affected visible subtrees.
}

/** Force-destroy every activity above the target one, then restart/resume the target as the foreground page. */
void egui_core_activity_force_finish_to_activity(egui_core_t *core, egui_activity_t *activity)
{
    // Walk backward until the requested activity is reached, finishing everything stacked above it.
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
        egui_view_set_size((egui_view_t *)&activity->root_view, core->screen_width, core->screen_height);
    }

    // Force a full redraw because the visible foreground activity may have changed completely.
    egui_core_update_region_dirty_all(core);
}

/** Force-stop and destroy every tracked activity, leaving the stack visually invalidated for a full redraw. */
void egui_core_activity_force_finish_all(egui_core_t *core)
{
    // Walk the whole activity list and finish everything regardless of stack position.
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

    // Force a full redraw because all activity-owned content may have been detached.
    egui_core_update_region_dirty_all(core);
}

#else

egui_activity_t *egui_core_activity_get_current(egui_core_t *core)
{
    EGUI_UNUSED(core);
    return NULL;
}

egui_activity_t *egui_core_activity_get_current_active(egui_core_t *core)
{
    EGUI_UNUSED(core);
    return NULL;
}

void egui_core_activity_force_finish_all(egui_core_t *core)
{
    EGUI_UNUSED(core);
}

void egui_core_activity_force_finish_to_activity(egui_core_t *core, egui_activity_t *activity)
{
    EGUI_UNUSED(core);
    EGUI_UNUSED(activity);
}

int egui_core_activity_check_in_process(egui_core_t *core, egui_activity_t *activity)
{
    EGUI_UNUSED(core);
    EGUI_UNUSED(activity);
    return 0;
}

void egui_core_activity_append(egui_core_t *core, egui_activity_t *activity)
{
    EGUI_UNUSED(core);
    EGUI_UNUSED(activity);
}

void egui_core_activity_remove(egui_core_t *core, egui_activity_t *activity)
{
    EGUI_UNUSED(core);
    EGUI_UNUSED(activity);
}

void egui_core_activity_start(egui_core_t *core, egui_activity_t *self, egui_activity_t *prev_activity)
{
    EGUI_UNUSED(core);
    EGUI_UNUSED(self);
    EGUI_UNUSED(prev_activity);
}

void egui_core_activity_set_start_anim(egui_core_t *core, egui_animation_t *open_anim, egui_animation_t *close_anim)
{
    EGUI_UNUSED(core);
    EGUI_UNUSED(open_anim);
    EGUI_UNUSED(close_anim);
}

void egui_core_activity_set_finish_anim(egui_core_t *core, egui_animation_t *open_anim, egui_animation_t *close_anim)
{
    EGUI_UNUSED(core);
    EGUI_UNUSED(open_anim);
    EGUI_UNUSED(close_anim);
}

void egui_core_activity_finish(egui_core_t *core, egui_activity_t *self)
{
    EGUI_UNUSED(core);
    EGUI_UNUSED(self);
}

egui_activity_t *egui_core_activity_get_by_view(egui_core_t *core, egui_view_t *view)
{
    EGUI_UNUSED(core);
    EGUI_UNUSED(view);
    return NULL;
}

#endif

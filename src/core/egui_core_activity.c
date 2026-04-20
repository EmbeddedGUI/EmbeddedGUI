#include "egui_api.h"
#include "egui_common.h"
#include "egui_core.h"
#include "egui_core_internal.h"
#include "anim/egui_animation.h"
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

static void on_activity_anim_start_open_end(egui_animation_t *self)
{
    egui_core_t *core = egui_view_get_core(self->target_view);
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_activity_anim_start_open_end\n");
#endif
    if (core == NULL)
    {
        return;
    }

    if (core->scene.activity_open)
    {
        core->scene.activity_open->api->on_resume(core->scene.activity_open);
    }
}

static const egui_animation_handle_t activity_anim_start_open_hanlde = {
        .start = NULL,
        .end = on_activity_anim_start_open_end,
        .repeat = NULL,
};

static void on_activity_anim_start_close_end(egui_animation_t *self)
{
    egui_core_t *core = egui_view_get_core(self->target_view);
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_activity_anim_start_close_end\n");
#endif
    if (core == NULL)
    {
        return;
    }

    if (core->scene.activity_close)
    {
        core->scene.activity_close->api->on_stop(core->scene.activity_close);
    }
}

static const egui_animation_handle_t activity_anim_start_close_hanlde = {
        .start = NULL,
        .end = on_activity_anim_start_close_end,
        .repeat = NULL,
};

static void on_activity_anim_finish_open_end(egui_animation_t *self)
{
    egui_core_t *core = egui_view_get_core(self->target_view);
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_activity_anim_finish_open_end\n");
#endif
    if (core == NULL)
    {
        return;
    }

    if (core->scene.activity_open)
    {
        core->scene.activity_open->api->on_resume(core->scene.activity_open);
    }
}

static const egui_animation_handle_t activity_anim_finish_open_hanlde = {
        .start = NULL,
        .end = on_activity_anim_finish_open_end,
        .repeat = NULL,
};

static void on_activity_anim_finish_close_end(egui_animation_t *self)
{
    egui_core_t *core = egui_view_get_core(self->target_view);
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_activity_anim_finish_close_end\n");
#endif
    if (core == NULL)
    {
        return;
    }

    if (core->scene.activity_close)
    {
        core->scene.activity_close->api->on_stop(core->scene.activity_close);
    }
}

static const egui_animation_handle_t activity_anim_finish_close_hanlde = {
        .start = NULL,
        .end = on_activity_anim_finish_close_end,
        .repeat = NULL,
};

void egui_core_activity_set_start_anim(egui_core_t *core, egui_animation_t *open_anim, egui_animation_t *close_anim)
{
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
    if (core == NULL || self == NULL)
    {
        return;
    }

    core->scene.activity_open = self;
    core->scene.activity_close = prev_activity;

    if (self == prev_activity)
    {
        return;
    }

    if (egui_activity_get_core(self) != core || egui_view_get_core(EGUI_VIEW_OF(&self->root_view)) != core)
    {
        return;
    }
    self->api->on_create(self);

    if (core->scene.activity_anim_start_open != NULL)
    {
        egui_animation_target_view_set(core->scene.activity_anim_start_open, (egui_view_t *)&self->root_view);
        egui_animation_start(core->scene.activity_anim_start_open);
    }
    else
    {
        self->api->on_resume(self);
    }

    if (prev_activity)
    {
        prev_activity->api->on_pause(prev_activity);
        // check anim
        if (core->scene.activity_anim_start_close != NULL)
        {
            egui_animation_target_view_set(core->scene.activity_anim_start_close, (egui_view_t *)&prev_activity->root_view);
            egui_animation_start(core->scene.activity_anim_start_close);
        }
        else
        {
            prev_activity->api->on_stop(prev_activity);
        }
    }
}

void egui_core_activity_finish(egui_core_t *core, egui_activity_t *self)
{
    // find a last activity to start
    egui_dnode_t *p_prev = egui_dlist_peek_prev(&core->scene.activitys, &self->node);
    core->scene.activity_close = self;

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
        if (core->scene.activity_anim_finish_close != NULL)
        {
            egui_animation_target_view_set(core->scene.activity_anim_finish_close, (egui_view_t *)&self->root_view);
            egui_animation_start(core->scene.activity_anim_finish_close);
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
        core->scene.activity_open = tmp;

        tmp->api->on_start(tmp);

        if (core->scene.activity_anim_finish_open != NULL)
        {
            egui_animation_target_view_set(core->scene.activity_anim_finish_open, (egui_view_t *)&tmp->root_view);
            egui_animation_start(core->scene.activity_anim_finish_open);
        }
        else
        {
            tmp->api->on_resume(tmp);
        }
    }

    // refresh the screen
    egui_core_update_region_dirty_all(core);
}

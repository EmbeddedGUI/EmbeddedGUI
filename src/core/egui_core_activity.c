#include "egui_api.h"
#include "egui_common.h"
#include "egui_core.h"
#include "egui_core_internal.h"
#include "anim/egui_animation.h"
#include "app/egui_activity.h"
#include "widget/egui_view.h"
#include "utils/egui_dlist.h"

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

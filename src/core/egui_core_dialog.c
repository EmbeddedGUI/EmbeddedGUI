#include "egui_api.h"
#include "egui_common.h"
#include "egui_core.h"
#include "egui_core_dialog.h"
#include "egui_core_internal.h"
#include "anim/egui_animation.h"
#include "app/egui_activity.h"
#include "app/egui_dialog.h"
#include "widget/egui_view.h"

egui_dialog_t *egui_core_dialog_get(egui_core_t *core)
{
    return core->scene.dialog;
}

void egui_core_dialog_start(egui_core_t *core, egui_activity_t *activity, egui_dialog_t *self)
{
    if (core == NULL || activity == NULL || self == NULL)
    {
        return;
    }

    if (egui_dialog_get_core(self) != core || egui_view_get_core(EGUI_VIEW_OF(&self->root_view)) != core ||
        egui_view_get_core(EGUI_VIEW_OF(&self->user_root_view)) != core)
    {
        return;
    }

    core->scene.dialog = self;
    self->is_need_finish = 0;

    self->bind_activity = activity;
    self->api->on_create(self);
    activity->api->on_pause(activity);

    if (core->scene.dialog_anim_start != NULL)
    {
        egui_animation_target_view_set(core->scene.dialog_anim_start, (egui_view_t *)&self->user_root_view);
        egui_animation_start(core->scene.dialog_anim_start);
    }
    else
    {
        self->api->on_resume(self);
    }
}

int egui_core_dialog_check_in_process(egui_core_t *core, egui_dialog_t *dialog)
{
    if (core->scene.dialog == dialog)
    {
        return 1;
    }
    return 0;
}

void egui_core_dialog_finish(egui_core_t *core, egui_dialog_t *self)
{
    core->scene.dialog = self;
    EGUI_LOG_DBG("egui_core_dialog_finish %p, self->is_need_finish: %d\n", self, self->is_need_finish);
    // avoid enter twice
    if (self->is_need_finish)
    {
        return;
    }
    if (self->state == EGUI_DIALOG_STATE_NONE)
    {
        return;
    }

    self->is_need_finish = true;
    self->api->on_pause(self);

    if (self->state < EGUI_DIALOG_STATE_STOP)
    {
        // check anim
        if (core->scene.dialog_anim_finish != NULL)
        {
            egui_animation_target_view_set(core->scene.dialog_anim_finish, (egui_view_t *)&self->user_root_view);
            egui_animation_start(core->scene.dialog_anim_finish);
        }
        else
        {
            self->api->on_stop(self);
            self->bind_activity->api->on_resume(self->bind_activity);
            core->scene.dialog = NULL;
        }
    }
    else
    {
        // something error.
        if (self->state < EGUI_DIALOG_STATE_DESTROY)
        {
            self->api->on_destroy(self);
        }
    }

    // refresh the screen
    egui_core_update_region_dirty(core, &self->root_view.base.base.region);
}

static void on_dialog_anim_start_end(egui_animation_t *self)
{
    egui_core_t *core = egui_view_get_core(self->target_view);
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_dialog_anim_start_end\n");
#endif
    if (core == NULL)
    {
        return;
    }

    if (core->scene.dialog)
    {
        core->scene.dialog->api->on_resume(core->scene.dialog);
    }
}

static const egui_animation_handle_t dialog_anim_start_hanlde = {
        .start = NULL,
        .end = on_dialog_anim_start_end,
        .repeat = NULL,
};

static void on_dialog_anim_finish_end(egui_animation_t *self)
{
    egui_core_t *core = egui_view_get_core(self->target_view);
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_dialog_anim_finish_end\n");
#endif
    if (core == NULL)
    {
        return;
    }

    if (core->scene.dialog)
    {
        core->scene.dialog->bind_activity->api->on_resume(core->scene.dialog->bind_activity);

        core->scene.dialog->api->on_stop(core->scene.dialog);
        core->scene.dialog = NULL;
    }
}

static const egui_animation_handle_t dialog_anim_finish_hanlde = {
        .start = NULL,
        .end = on_dialog_anim_finish_end,
        .repeat = NULL,
};

void egui_core_dialog_set_anim(egui_core_t *core, egui_animation_t *open_anim, egui_animation_t *close_anim)
{
    core->scene.dialog_anim_start = open_anim;
    core->scene.dialog_anim_finish = close_anim;

    if (open_anim)
    {
        egui_animation_handle_set(open_anim, &dialog_anim_start_hanlde);
    }
    if (close_anim)
    {
        egui_animation_handle_set(close_anim, &dialog_anim_finish_hanlde);
    }
}

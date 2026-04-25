#include "egui_api.h"
#include "egui_common.h"
#include "egui_core.h"
#include "egui_core_dialog.h"
#include "egui_core_internal.h"
#include "anim/egui_animation.h"
#include "app/egui_activity.h"
#include "app/egui_dialog.h"
#include "widget/egui_view.h"

/**
 * @file egui_core_dialog.c
 * @brief Core-side dialog ownership and lifecycle handoff on top of the current activity.
 */

#if EGUI_CONFIG_FUNCTION_SUPPORT_DIALOG

/** Cancel one dialog animation slot only when it still belongs to the target dialog. */
static void egui_core_dialog_cancel_anim_slot(egui_animation_t *anim, egui_dialog_t **owner_slot, egui_dialog_t *dialog)
{
    if (owner_slot == NULL || *owner_slot != dialog)
    {
        return;
    }

    if (anim != NULL && anim->is_running)
    {
        egui_animation_complete(anim);
    }

    *owner_slot = NULL;
}

/** Remove every pending transition reference that still points at the supplied dialog. */
static void egui_core_dialog_cancel_pending(egui_core_t *core, egui_dialog_t *dialog)
{
    if (core == NULL || dialog == NULL)
    {
        return;
    }

    egui_core_dialog_cancel_anim_slot(core->scene.dialog_anim_start, &core->scene.dialog_anim_start_owner, dialog);
    egui_core_dialog_cancel_anim_slot(core->scene.dialog_anim_finish, &core->scene.dialog_anim_finish_owner, dialog);

    if (core->scene.dialog == dialog)
    {
        core->scene.dialog = NULL;
    }
}

/** Return the dialog currently attached to this core, or NULL when no dialog is active. */
egui_dialog_t *egui_core_dialog_get(egui_core_t *core)
{
    return core->scene.dialog;
}

/**
 * Show a dialog above the current activity.
 * The dialog is created first, the bound activity pauses, and the dialog then resumes immediately or after its open animation completes.
 */
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

    egui_core_dialog_cancel_pending(core, self);
    core->scene.dialog = self;
    self->is_need_finish = 0;

    self->bind_activity = activity;
    self->api->on_create(self);
    activity->api->on_pause(activity);

    if (core->scene.dialog_anim_start != NULL)
    {
        // The open animation owns the deferred on_resume callback for the entering dialog.
        core->scene.dialog_anim_start_owner = self;
        egui_animation_target_view_set(core->scene.dialog_anim_start, (egui_view_t *)&self->user_root_view);
        egui_animation_start(core->scene.dialog_anim_start);
    }
    else
    {
        core->scene.dialog_anim_start_owner = NULL;
        self->api->on_resume(self);
    }
}

/** Return non-zero when the supplied dialog is still the active dialog tracked by this core. */
int egui_core_dialog_check_in_process(egui_core_t *core, egui_dialog_t *dialog)
{
    if (core->scene.dialog == dialog)
    {
        return 1;
    }
    return 0;
}

/**
 * Begin closing the active dialog.
 * The dialog pauses first, then stops immediately or after its close animation,
 * and the previously paused activity resumes once the dialog is gone.
 */
void egui_core_dialog_finish(egui_core_t *core, egui_dialog_t *self)
{
    EGUI_LOG_DBG("egui_core_dialog_finish %p, self->is_need_finish: %d\n", self, self->is_need_finish);
    // Ignore duplicate finish requests.
    if (self->is_need_finish)
    {
        return;
    }
    if (self->state == EGUI_DIALOG_STATE_NONE)
    {
        return;
    }

    egui_core_dialog_cancel_pending(core, self);
    core->scene.dialog = self;
    self->is_need_finish = true;
    self->api->on_pause(self);

    if (self->state < EGUI_DIALOG_STATE_STOP)
    {
        // The close animation owns the deferred stop/resume handoff when configured.
        if (core->scene.dialog_anim_finish != NULL)
        {
            core->scene.dialog_anim_finish_owner = self;
            egui_animation_target_view_set(core->scene.dialog_anim_finish, (egui_view_t *)&self->user_root_view);
            egui_animation_start(core->scene.dialog_anim_finish);
        }
        else
        {
            core->scene.dialog_anim_finish_owner = NULL;
            self->api->on_stop(self);
            self->bind_activity->api->on_resume(self->bind_activity);
            core->scene.dialog = NULL;
        }
    }
    else
    {
        core->scene.dialog_anim_finish_owner = NULL;
        // something error.
        if (self->state < EGUI_DIALOG_STATE_DESTROY)
        {
            self->api->on_destroy(self);
        }
    }

    // Dirty the dialog area so the overlay and the re-exposed activity content are redrawn.
    egui_core_update_region_dirty(core, &self->root_view.base.base.region);
}

/** Complete the dialog open transition by resuming the dialog that just became interactive. */
static void on_dialog_anim_start_end(egui_animation_t *self)
{
    egui_core_t *core = egui_view_get_core(self->target_view);
    egui_dialog_t *dialog = NULL;

    if (core != NULL)
    {
        dialog = core->scene.dialog_anim_start_owner;
        core->scene.dialog_anim_start_owner = NULL;
    }
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_dialog_anim_start_end\n");
#endif
    if (core == NULL || dialog == NULL || dialog->is_need_finish || dialog->state >= EGUI_DIALOG_STATE_STOP)
    {
        return;
    }

    dialog->api->on_resume(dialog);
}

static const egui_animation_handle_t dialog_anim_start_hanlde = {
        .start = NULL,
        .end = on_dialog_anim_start_end,
        .repeat = NULL,
};

/** Complete the dialog close transition by resuming the bound activity and stopping the dialog. */
static void on_dialog_anim_finish_end(egui_animation_t *self)
{
    egui_core_t *core = egui_view_get_core(self->target_view);
    egui_dialog_t *dialog = NULL;

    if (core != NULL)
    {
        dialog = core->scene.dialog_anim_finish_owner;
        core->scene.dialog_anim_finish_owner = NULL;
    }
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_dialog_anim_finish_end\n");
#endif
    if (core == NULL || dialog == NULL || dialog->state >= EGUI_DIALOG_STATE_STOP)
    {
        return;
    }

    dialog->bind_activity->api->on_resume(dialog->bind_activity);
    dialog->api->on_stop(dialog);
    if (core->scene.dialog == dialog)
    {
        core->scene.dialog = NULL;
    }
}

static const egui_animation_handle_t dialog_anim_finish_hanlde = {
        .start = NULL,
        .end = on_dialog_anim_finish_end,
        .repeat = NULL,
};

/** Install the open/close animations used for dialogs on this core. */
void egui_core_dialog_set_anim(egui_core_t *core, egui_animation_t *open_anim, egui_animation_t *close_anim)
{
    egui_core_dialog_cancel_anim_slot(core->scene.dialog_anim_start, &core->scene.dialog_anim_start_owner, core->scene.dialog_anim_start_owner);
    egui_core_dialog_cancel_anim_slot(core->scene.dialog_anim_finish, &core->scene.dialog_anim_finish_owner, core->scene.dialog_anim_finish_owner);

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

#else

egui_dialog_t *egui_core_dialog_get(egui_core_t *core)
{
    EGUI_UNUSED(core);
    return NULL;
}

void egui_core_dialog_set_anim(egui_core_t *core, egui_animation_t *open_anim, egui_animation_t *close_anim)
{
    EGUI_UNUSED(core);
    EGUI_UNUSED(open_anim);
    EGUI_UNUSED(close_anim);
}

void egui_core_dialog_start(egui_core_t *core, egui_activity_t *activity, egui_dialog_t *self)
{
    EGUI_UNUSED(core);
    EGUI_UNUSED(activity);
    EGUI_UNUSED(self);
}

int egui_core_dialog_check_in_process(egui_core_t *core, egui_dialog_t *dialog)
{
    EGUI_UNUSED(core);
    EGUI_UNUSED(dialog);
    return 0;
}

void egui_core_dialog_finish(egui_core_t *core, egui_dialog_t *self)
{
    EGUI_UNUSED(core);
    EGUI_UNUSED(self);
}

#endif

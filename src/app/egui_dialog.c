#include <stdio.h>
#include <assert.h>

#include "egui_dialog.h"
#include "egui_toast.h"
#include "widget/egui_view.h"
#include "core/egui_core_dialog.h"
#include "core/egui_core_toast.h"
#include "core/egui_core_internal.h"
#include "core/egui_api.h"
#include "background/egui_background_color.h"

/**
 * @file egui_dialog.c
 * @brief Public dialog helpers and the default modal-dialog lifecycle layered above the current activity.
 */

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_dialog_param_normal, EGUI_COLOR_BLACK, EGUI_ALPHA_50);
EGUI_BACKGROUND_PARAM_INIT(bg_dialog_params, &bg_dialog_param_normal, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_dialog, &bg_dialog_params);

/** Human-readable names for debug logs that print dialog state transitions. */
const char dialog_state_str[][16] = {
        "NONE", "CREATE", "START", "RESUME", "PAUSE", "STOP", "DESTROY",
};

/** Return a debug-friendly string for one dialog state enum value. */
const char *egui_dialog_state_str(int state)
{
    return dialog_state_str[state];
}

/** Apply a layout rectangle to the dialog's user-content root. */
void egui_dialog_set_layout(egui_dialog_t *self, egui_region_t *layout)
{
    egui_view_layout((egui_view_t *)&self->user_root_view, layout);
}

/** Add one child view into the dialog's user-content root. */
void egui_dialog_add_view(egui_dialog_t *self, egui_view_t *view)
{
    egui_view_group_add_child((egui_view_t *)&self->user_root_view, view);
}

/** Store a debug-only dialog name when class-name logging is enabled. */
void egui_dialog_set_name(egui_dialog_t *self, const char *name)
{
    EGUI_UNUSED(name);
    EGUI_UNUSED(self);
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    self->name = name;
#endif
}

/** Return the core currently bound to this dialog. */
egui_core_t *egui_dialog_get_core(egui_dialog_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }

    return self->core;
}

/** Return the default toast currently registered on the same core as this dialog. */
egui_toast_t *egui_dialog_get_toast(egui_dialog_t *self)
{
    egui_core_t *core = egui_dialog_get_core(self);

    if (core == NULL)
    {
        return NULL;
    }

    return egui_core_toast_get(core);
}

/** Show an info toast from the dialog using an explicit duration override. */
void egui_dialog_show_toast_info_with_duration(egui_dialog_t *self, const char *text, uint16_t duration)
{
    egui_toast_t *toast = egui_dialog_get_toast(self);

    if (toast == NULL)
    {
        return;
    }

    egui_toast_show_info_with_duration(toast, text, duration);
}

/** Show an info toast from the dialog using the configured default duration. */
void egui_dialog_show_toast_info(egui_dialog_t *self, const char *text)
{
    egui_dialog_show_toast_info_with_duration(self, text, EGUI_CONFIG_PARAM_TOAST_DEFAULT_SHOW_TIME);
}

/** Start a timer owned by the same core as this dialog. */
int egui_dialog_start_timer(egui_dialog_t *self, egui_timer_t *handle, uint32_t ms, uint32_t period)
{
    egui_core_t *core = egui_dialog_get_core(self);

    if (core == NULL)
    {
        return -1;
    }

    return egui_timer_start_timer(core, handle, ms, period);
}

/** Stop one timer through the core bound to this dialog. */
void egui_dialog_stop_timer(egui_dialog_t *self, egui_timer_t *handle)
{
    egui_core_t *core = egui_dialog_get_core(self);

    if (core == NULL)
    {
        return;
    }

    egui_timer_stop_timer(core, handle);
}

/** Return non-zero when the supplied timer is currently started on this dialog's core. */
int egui_dialog_check_timer_start(egui_dialog_t *self, egui_timer_t *handle)
{
    egui_core_t *core = egui_dialog_get_core(self);

    if (core == NULL)
    {
        return 0;
    }

    return egui_timer_check_timer_start(core, handle);
}

/** Install the animations used when this dialog opens and closes. */
void egui_dialog_set_anim(egui_dialog_t *self, egui_animation_t *open_anim, egui_animation_t *close_anim)
{
    egui_core_t *core = egui_dialog_get_core(self);

    if (core == NULL)
    {
        return;
    }

    egui_core_dialog_set_anim(core, open_anim, close_anim);
}

/** Ask the core dialog manager to show this dialog above the supplied activity. */
void egui_dialog_start(egui_dialog_t *self, egui_activity_t *activity)
{
    egui_core_t *core = egui_dialog_get_core(self);
    if (core == NULL)
    {
        return;
    }

    egui_core_dialog_start(core, activity, self);
}

/** Return non-zero when this dialog is currently tracked as the active dialog on its core. */
int egui_dialog_check_in_process(egui_dialog_t *self)
{
    egui_core_t *core = egui_dialog_get_core(self);

    if (core == NULL)
    {
        return 0;
    }

    return egui_core_dialog_check_in_process(core, self);
}

/** Ask the core dialog manager to finish this dialog. */
void egui_dialog_finish(egui_dialog_t *self)
{
    egui_core_t *core = egui_dialog_get_core(self);

    if (core == NULL)
    {
        return;
    }

    egui_core_dialog_finish(core, self);
}

/** Default click handler for the dimmed background around the dialog content. */
static void bg_click_cb(egui_view_t *self)
{
    EGUI_LOG_DBG("bg_click_cb\n");
    egui_dialog_t *p_dialog = egui_view_get_dialog(self);
    if (p_dialog)
    {
        if (p_dialog->is_cancel_on_touch_outside)
        {
            egui_dialog_finish(p_dialog);
        }
    }
}

/** Eat clicks inside the user-content root so they do not dismiss the dialog background. */
static void bg_user_click_cb(egui_view_t *self)
{
    EGUI_UNUSED(self);
    EGUI_LOG_DBG("bg_user_click_cb\n");
}

/** Default on_create handler: switch state, enter on_start, then attach the dialog root to the scene. */
void egui_dialog_on_create(egui_dialog_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_create, name: %s, last_state: %s\n", self->name, egui_dialog_state_str(self->state));
#endif
    egui_core_t *core;

    self->state = EGUI_DIALOG_STATE_CREATE;
    self->api->on_start(self);

    core = egui_dialog_get_core(self);
    if (core == NULL)
    {
        return;
    }

    egui_core_add_user_root_view((egui_view_t *)&self->root_view);
}

/** Default on_start handler: show the dialog root and attach it to the window when parented. */
void egui_dialog_on_start(egui_dialog_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_start, name: %s, last_state: %s\n", self->name, egui_dialog_state_str(self->state));
#endif
    self->state = EGUI_DIALOG_STATE_START;
    egui_view_set_visible((egui_view_t *)&self->root_view, true);
    if (EGUI_VIEW_OF(&self->root_view)->parent != NULL)
    {
        egui_view_dispatch_attach_to_window((egui_view_t *)&self->root_view);
    }
}

/** Default on_resume handler: mark the dialog interactive again. */
void egui_dialog_on_resume(egui_dialog_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_resume, name: %s, last_state: %s\n", self->name, egui_dialog_state_str(self->state));
#endif
    self->state = EGUI_DIALOG_STATE_RESUME;
    egui_view_group_set_disallow_process_touch_event((egui_view_t *)&self->root_view, 0);
}

/** Default on_pause handler: keep the dialog alive but temporarily block touch delivery. */
void egui_dialog_on_pause(egui_dialog_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_pause, name: %s, last_state: %s\n", self->name, egui_dialog_state_str(self->state));
#endif
    self->state = EGUI_DIALOG_STATE_PAUSE;
    egui_view_group_set_disallow_process_touch_event((egui_view_t *)&self->root_view, 1);
}

/** Default on_stop handler: detach and hide the dialog root, then destroy immediately when finish was requested. */
void egui_dialog_on_stop(egui_dialog_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_stop, name: %s, last_state: %s\n", self->name, egui_dialog_state_str(self->state));
#endif
    self->state = EGUI_DIALOG_STATE_STOP;
    egui_view_dispatch_detach_from_window((egui_view_t *)&self->root_view);
    egui_view_set_visible((egui_view_t *)&self->root_view, false);

    if (self->is_need_finish)
    {
        self->api->on_destroy(self);
    }
}

/** Default on_destroy handler: remove the dialog root from the scene tree. */
void egui_dialog_on_destroy(egui_dialog_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_destroy, name: %s, last_state: %s\n", self->name, egui_dialog_state_str(self->state));
#endif
    self->state = EGUI_DIALOG_STATE_DESTROY;

    egui_core_t *core = egui_dialog_get_core(self);

    if (core == NULL)
    {
        return;
    }

    egui_core_remove_user_root_view(core, (egui_view_t *)&self->root_view);
}

static const egui_dialog_api_t EGUI_DIALOG_API_TABLE_NAME(egui_dialog_t) = {
        .on_create = egui_dialog_on_create,
        .on_start = egui_dialog_on_start,
        .on_resume = egui_dialog_on_resume,
        .on_pause = egui_dialog_on_pause,
        .on_stop = egui_dialog_on_stop,
        .on_destroy = egui_dialog_on_destroy,
};

/** Initialize a dialog with a dimmed full-screen root plus a nested user-content root. */
void egui_dialog_init(egui_dialog_t *self, egui_core_t *core)
{
    EGUI_ASSERT(core != NULL);

    self->state = EGUI_DIALOG_STATE_NONE;
    self->is_need_finish = false;
    self->is_cancel_on_touch_outside = true;
    self->core = core;
    self->bind_activity = NULL;

    // Root view group: the dimmed full-screen modal layer.
    egui_view_root_group_init((egui_view_t *)&self->root_view, core);
    egui_view_set_position((egui_view_t *)&self->root_view, 0, 0);
    egui_view_set_size((egui_view_t *)&self->root_view, core->screen_width, core->screen_height);

    egui_view_set_background((egui_view_t *)&self->root_view, (egui_background_t *)&bg_dialog);

    egui_view_set_view_name((egui_view_t *)&self->root_view, "egui_dialog_root_view");

    egui_view_set_on_click_listener((egui_view_t *)&self->root_view, bg_click_cb);

    // User view group: the actual dialog content tree that sits above the modal background.
    egui_view_root_group_init((egui_view_t *)&self->user_root_view, core);
    egui_view_set_position((egui_view_t *)&self->user_root_view, 0, 0);
    egui_view_set_size((egui_view_t *)&self->user_root_view, core->screen_width, core->screen_height);

    egui_view_set_view_name((egui_view_t *)&self->user_root_view, "egui_dialog_user_root_view");

    egui_view_group_add_child((egui_view_t *)&self->root_view, (egui_view_t *)&self->user_root_view);

    egui_view_set_on_click_listener((egui_view_t *)&self->user_root_view, bg_user_click_cb);

    self->api = &EGUI_DIALOG_API_TABLE_NAME(egui_dialog_t);
}

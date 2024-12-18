#include <stdio.h>
#include <assert.h>

#include "egui_dialog.h"
#include "widget/egui_view.h"
#include "core/egui_core.h"
#include "core/egui_api.h"
#include "background/egui_background_color.h"

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_dialog_param_normal, EGUI_COLOR_BLACK, EGUI_ALPHA_50);
EGUI_BACKGROUND_PARAM_INIT(bg_dialog_params, &bg_dialog_param_normal, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_dialog, &bg_dialog_params);

const char dialog_state_str[][16] = {
        "NONE", "CREATE", "START", "RESUME", "PAUSE", "STOP", "DESTROY",
};

const char *egui_dialog_state_str(int state)
{
    return dialog_state_str[state];
}

void egui_dialog_set_layout(egui_dialog_t *self, egui_region_t *layout)
{
    // set view group params
    egui_view_layout((egui_view_t *)&self->user_root_view, layout); // set user layout
}

void egui_dialog_add_view(egui_dialog_t *self, egui_view_t *view)
{
    egui_view_group_add_child((egui_view_t *)&self->user_root_view, view);
}

void egui_dialog_set_name(egui_dialog_t *self, const char *name)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    self->name = name;
#endif
}

static void bg_click_cb(egui_view_t *self)
{
    EGUI_LOG_DBG("bg_click_cb\n");
    egui_dialog_t *p_dialog = egui_core_dialog_get();
    if (p_dialog)
    {
        if (p_dialog->is_cancel_on_touch_outside)
        {
            egui_core_dialog_finish(p_dialog);
        }
    }
}

static void bg_user_click_cb(egui_view_t *self)
{
    EGUI_LOG_DBG("bg_user_click_cb\n");
    // do nothing
}

void egui_dialog_on_create(egui_dialog_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_create, name: %s, last_state: %s\n", self->name, egui_dialog_state_str(self->state));
#endif
    self->state = EGUI_DIALOG_STATE_CREATE;
    self->api->on_start(self);

    // start anim
    egui_core_add_user_root_view((egui_view_t *)&self->root_view);
}

void egui_dialog_on_start(egui_dialog_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_start, name: %s, last_state: %s\n", self->name, egui_dialog_state_str(self->state));
#endif
    self->state = EGUI_DIALOG_STATE_START;
    egui_view_set_visible((egui_view_t *)&self->root_view, true); // show view group
}

void egui_dialog_on_resume(egui_dialog_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_resume, name: %s, last_state: %s\n", self->name, egui_dialog_state_str(self->state));
#endif
    self->state = EGUI_DIALOG_STATE_RESUME;
    egui_view_group_set_disallow_process_touch_event((egui_view_t *)&self->root_view, 0); // allow process touch event
}

void egui_dialog_on_pause(egui_dialog_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_pause, name: %s, last_state: %s\n", self->name, egui_dialog_state_str(self->state));
#endif
    self->state = EGUI_DIALOG_STATE_PAUSE;
    egui_view_group_set_disallow_process_touch_event((egui_view_t *)&self->root_view, 1); // disallow process touch event
}

void egui_dialog_on_stop(egui_dialog_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_stop, name: %s, last_state: %s\n", self->name, egui_dialog_state_str(self->state));
#endif
    self->state = EGUI_DIALOG_STATE_STOP;
    egui_view_set_visible((egui_view_t *)&self->root_view, false); // hide view group

    if (self->is_need_finish)
    {
        self->api->on_destroy(self);
    }
}

void egui_dialog_on_destroy(egui_dialog_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_destroy, name: %s, last_state: %s\n", self->name, egui_dialog_state_str(self->state));
#endif
    self->state = EGUI_DIALOG_STATE_DESTROY;

    egui_core_remove_user_root_view((egui_view_t *)&self->root_view);
}

EGUI_DIALOG_API_DEFINE(egui_dialog_t, NULL, NULL, NULL, NULL, NULL, NULL);

void egui_dialog_init(egui_dialog_t *self)
{
    self->state = EGUI_DIALOG_STATE_NONE;
    self->is_need_finish = false;
    self->is_cancel_on_touch_outside = true;

    // root view group
    egui_view_group_init((egui_view_t *)&self->root_view); // init view group
    egui_view_set_position((egui_view_t *)&self->root_view, 0, 0);
    egui_view_set_size((egui_view_t *)&self->root_view, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);

    egui_view_set_background((egui_view_t *)&self->root_view, (egui_background_t *)&bg_dialog);

    egui_view_set_view_name((egui_view_t *)&self->root_view, "egui_dialog_root_view");

    egui_view_set_on_click_listener((egui_view_t *)&self->root_view, bg_click_cb);

    // user view group
    egui_view_group_init((egui_view_t *)&self->user_root_view); // init view group
    egui_view_set_position((egui_view_t *)&self->user_root_view, 0, 0);
    egui_view_set_size((egui_view_t *)&self->user_root_view, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);

    egui_view_set_view_name((egui_view_t *)&self->user_root_view, "egui_dialog_user_root_view");

    egui_view_group_add_child((egui_view_t *)&self->root_view, (egui_view_t *)&self->user_root_view); // add user view group to root view group

    egui_view_set_on_click_listener((egui_view_t *)&self->user_root_view, bg_user_click_cb);

    // init api
    self->api = &EGUI_DIALOG_API_TABLE_NAME(egui_dialog_t);
}

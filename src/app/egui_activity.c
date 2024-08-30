#include <stdio.h>
#include <assert.h>

#include "egui_activity.h"
#include "widget/egui_view.h"
#include "core/egui_core.h"
#include "core/egui_api.h"
#include "background/egui_background_color.h"

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_activity_param_normal, EGUI_COLOR_BLACK, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_activity_params, &bg_activity_param_normal, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_activity, &bg_activity_params);

const char activity_state_str[][16] = {
        "NONE", "CREATE", "START", "RESUME", "PAUSE", "STOP", "DESTROY",
};

const char *egui_activity_state_str(int state)
{
    return activity_state_str[state];
}

void egui_activity_set_layout(egui_activity_t *self, egui_region_t *layout)
{
    // set view group params
    egui_view_layout((egui_view_t *)&self->root_view, layout); // set layout for
}

void egui_activity_add_view(egui_activity_t *self, egui_view_t *view)
{
    egui_view_group_add_child((egui_view_t *)&self->root_view, view);
}

void egui_activity_set_name(egui_activity_t *self, const char *name)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    self->name = name;
#endif
}

void egui_activity_on_create(egui_activity_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_create, name: %s, last_state: %s\n", self->name, egui_activity_state_str(self->state));
#endif
    self->state = EGUI_ACTIVITY_STATE_CREATE;
    self->api->on_start(self);

    // start anim
    egui_core_add_user_root_view((egui_view_t *)&self->root_view);
    egui_core_activity_append(self);
}

void egui_activity_on_start(egui_activity_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_start, name: %s, last_state: %s\n", self->name, egui_activity_state_str(self->state));
#endif
    self->state = EGUI_ACTIVITY_STATE_START;
    egui_view_set_visible((egui_view_t *)&self->root_view, true); // show view group
}

void egui_activity_on_resume(egui_activity_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_resume, name: %s, last_state: %s\n", self->name, egui_activity_state_str(self->state));
#endif
    self->state = EGUI_ACTIVITY_STATE_RESUME;
    egui_view_group_set_disallow_process_touch_event((egui_view_t *)&self->root_view, 0); // allow process touch event
}

void egui_activity_on_pause(egui_activity_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_pause, name: %s, last_state: %s\n", self->name, egui_activity_state_str(self->state));
#endif
    self->state = EGUI_ACTIVITY_STATE_PAUSE;
    egui_view_group_set_disallow_process_touch_event((egui_view_t *)&self->root_view, 1); // disallow process touch event
}

void egui_activity_on_stop(egui_activity_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_stop, name: %s, last_state: %s\n", self->name, egui_activity_state_str(self->state));
#endif
    self->state = EGUI_ACTIVITY_STATE_STOP;
    egui_view_set_visible((egui_view_t *)&self->root_view, false); // hide view group

    if (self->is_need_finish)
    {
        self->api->on_destroy(self);
    }
}

void egui_activity_on_destroy(egui_activity_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_destroy, name: %s, last_state: %s\n", self->name, egui_activity_state_str(self->state));
#endif
    self->state = EGUI_ACTIVITY_STATE_DESTROY;

    egui_core_remove_user_root_view((egui_view_t *)&self->root_view);
    egui_core_activity_remove(self);
}

EGUI_ACTIVITY_API_DEFINE(egui_activity_t, NULL, NULL, NULL, NULL, NULL, NULL);

void egui_activity_init(egui_activity_t *self)
{
    self->state = EGUI_ACTIVITY_STATE_NONE;
    self->is_need_finish = false;

    egui_dlist_init(&self->node);
    egui_view_group_init((egui_view_t *)&self->root_view); // init view group
    egui_view_set_position((egui_view_t *)&self->root_view, 0, 0);
    egui_view_set_size((egui_view_t *)&self->root_view, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);

    // egui_view_set_background((egui_view_t *)&self->root_view, (egui_background_t *)&bg_activity);

    egui_view_set_view_name((egui_view_t *)&self->root_view, "egui_activity_root_view");
    // init api
    self->api = &EGUI_ACTIVITY_API_TABLE_NAME(egui_activity_t);
}

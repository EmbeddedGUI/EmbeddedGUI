#include <stdio.h>
#include <assert.h>

#include "egui_activity.h"
#include "egui_toast.h"
#include "widget/egui_view.h"
#include "core/egui_core_activity.h"
#include "core/egui_core_toast.h"
#include "core/egui_core_internal.h"
#include "core/egui_api.h"
#include "background/egui_background_color.h"

// EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_activity_param_normal, EGUI_COLOR_BLACK, EGUI_ALPHA_100);
// EGUI_BACKGROUND_PARAM_INIT(bg_activity_params, &bg_activity_param_normal, NULL, NULL);
// EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_activity, &bg_activity_params);

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

egui_core_t *egui_activity_get_core(egui_activity_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }

    return self->core;
}

egui_toast_t *egui_activity_get_toast(egui_activity_t *self)
{
    egui_core_t *core = egui_activity_get_core(self);

    if (core == NULL)
    {
        return NULL;
    }

    return egui_core_toast_get(core);
}

void egui_activity_show_toast_info_with_duration(egui_activity_t *self, const char *text, uint16_t duration)
{
    egui_toast_t *toast = egui_activity_get_toast(self);

    if (toast == NULL)
    {
        return;
    }

    egui_toast_show_info_with_duration(toast, text, duration);
}

void egui_activity_show_toast_info(egui_activity_t *self, const char *text)
{
    egui_activity_show_toast_info_with_duration(self, text, EGUI_CONFIG_PARAM_TOAST_DEFAULT_SHOW_TIME);
}

int egui_activity_start_timer(egui_activity_t *self, egui_timer_t *handle, uint32_t ms, uint32_t period)
{
    egui_core_t *core = egui_activity_get_core(self);

    if (core == NULL)
    {
        return -1;
    }

    return egui_timer_start_timer(core, handle, ms, period);
}

void egui_activity_stop_timer(egui_activity_t *self, egui_timer_t *handle)
{
    egui_core_t *core = egui_activity_get_core(self);

    if (core == NULL)
    {
        return;
    }

    egui_timer_stop_timer(core, handle);
}

int egui_activity_check_timer_start(egui_activity_t *self, egui_timer_t *handle)
{
    egui_core_t *core = egui_activity_get_core(self);

    if (core == NULL)
    {
        return 0;
    }

    return egui_timer_check_timer_start(core, handle);
}

void egui_activity_set_start_anim(egui_activity_t *self, egui_animation_t *open_anim, egui_animation_t *close_anim)
{
    egui_core_t *core = egui_activity_get_core(self);

    if (core == NULL)
    {
        return;
    }

    egui_core_activity_set_start_anim(core, open_anim, close_anim);
}

void egui_activity_set_finish_anim(egui_activity_t *self, egui_animation_t *open_anim, egui_animation_t *close_anim)
{
    egui_core_t *core = egui_activity_get_core(self);

    if (core == NULL)
    {
        return;
    }

    egui_core_activity_set_finish_anim(core, open_anim, close_anim);
}

void egui_activity_start(egui_activity_t *self, egui_activity_t *prev_activity)
{
    egui_core_t *core = egui_activity_get_core(self);
    if (core == NULL)
    {
        return;
    }

    egui_core_activity_start(core, self, prev_activity);
}

void egui_activity_finish(egui_activity_t *self)
{
    egui_core_t *core = egui_activity_get_core(self);

    if (core == NULL)
    {
        return;
    }

    egui_core_activity_finish(core, self);
}

int egui_activity_check_in_process(egui_activity_t *self)
{
    egui_core_t *core = egui_activity_get_core(self);

    if (core == NULL)
    {
        return 0;
    }

    return egui_core_activity_check_in_process(core, self);
}

void egui_activity_on_create(egui_activity_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_create, name: %s, last_state: %s\n", self->name, egui_activity_state_str(self->state));
#endif
    egui_core_t *core;

    self->state = EGUI_ACTIVITY_STATE_CREATE;
    self->api->on_start(self);

    // start anim
    core = egui_activity_get_core(self);
    if (core == NULL)
    {
        return;
    }

    egui_core_add_user_root_view((egui_view_t *)&self->root_view);
    egui_core_activity_append(core, self);
}

void egui_activity_on_start(egui_activity_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_start, name: %s, last_state: %s\n", self->name, egui_activity_state_str(self->state));
#endif
    self->state = EGUI_ACTIVITY_STATE_START;
    egui_view_set_visible((egui_view_t *)&self->root_view, true); // show view group
    if (EGUI_VIEW_OF(&self->root_view)->parent != NULL)
    {
        egui_view_dispatch_attach_to_window((egui_view_t *)&self->root_view);
    }
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
    egui_view_dispatch_detach_from_window((egui_view_t *)&self->root_view);
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
    egui_core_t *core;

    self->state = EGUI_ACTIVITY_STATE_DESTROY;

    core = egui_activity_get_core(self);
    if (core == NULL)
    {
        return;
    }

    egui_core_remove_user_root_view(core, (egui_view_t *)&self->root_view);
    egui_core_activity_remove(core, self);
}

static const egui_activity_api_t EGUI_ACTIVITY_API_TABLE_NAME(egui_activity_t) = {
        .on_create = egui_activity_on_create,
        .on_start = egui_activity_on_start,
        .on_resume = egui_activity_on_resume,
        .on_pause = egui_activity_on_pause,
        .on_stop = egui_activity_on_stop,
        .on_destroy = egui_activity_on_destroy,
};

void egui_activity_init(egui_activity_t *self, egui_core_t *core)
{
    EGUI_ASSERT(core != NULL);

    self->state = EGUI_ACTIVITY_STATE_NONE;
    self->is_need_finish = false;
    self->core = core;

    egui_dlist_init(&self->node);
    egui_view_root_group_init((egui_view_t *)&self->root_view, core); // init view group
    egui_view_set_position((egui_view_t *)&self->root_view, 0, 0);
    egui_view_set_size((egui_view_t *)&self->root_view, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);

    // egui_view_set_background((egui_view_t *)&self->root_view, (egui_background_t *)&bg_activity);

    egui_view_set_view_name((egui_view_t *)&self->root_view, "egui_activity_root_view");
    // init api
    self->api = &EGUI_ACTIVITY_API_TABLE_NAME(egui_activity_t);
}

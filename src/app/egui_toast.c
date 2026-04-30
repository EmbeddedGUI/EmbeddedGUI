#include <stdio.h>
#include <assert.h>

#include "egui_toast.h"
#include "widget/egui_view.h"
#include "core/egui_core_toast.h"
#include "core/egui_api.h"
#include "background/egui_background_color.h"

/**
 * @file egui_toast.c
 * @brief Base toast helpers that bridge timer/default-toast ownership and provide overridable show/hide hooks.
 */

/** Return the core currently bound to this toast instance. */
egui_core_t *egui_toast_get_core(egui_toast_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }

    return self->core;
}

/** Start a timer owned by the same core as this toast. */
int egui_toast_start_timer(egui_toast_t *self, egui_timer_t *handle, uint32_t ms, uint32_t period)
{
    egui_core_t *core = egui_toast_get_core(self);

    if (core == NULL)
    {
        return -1;
    }

    return egui_timer_start_timer(core, handle, ms, period);
}

/** Stop one timer through the core bound to this toast. */
void egui_toast_stop_timer(egui_toast_t *self, egui_timer_t *handle)
{
    egui_core_t *core = egui_toast_get_core(self);

    if (core == NULL)
    {
        return;
    }

    egui_timer_stop_timer(core, handle);
}

/** Return non-zero when the supplied timer is currently started on this toast's core. */
int egui_toast_check_timer_start(egui_toast_t *self, egui_timer_t *handle)
{
    egui_core_t *core = egui_toast_get_core(self);

    if (core == NULL)
    {
        return 0;
    }

    return egui_timer_check_timer_start(core, handle);
}

/** Register this toast as the default toast on its core and run the subclass default-setup hook. */
void egui_toast_set_as_default(egui_toast_t *self)
{
    egui_core_t *core = egui_toast_get_core(self);

    if (core == NULL)
    {
        return;
    }

    egui_core_toast_set(core, self);

    if (self->api && self->api->on_set_default)
    {
        self->api->on_set_default(self);
    }
}

/** Clear this toast as the core default only when it is still the currently registered one. */
void egui_toast_clear_as_default(egui_toast_t *self)
{
    egui_core_t *core = egui_toast_get_core(self);

    if (core == NULL)
    {
        return;
    }

    if (egui_core_toast_get(core) != self)
    {
        return;
    }

    egui_core_toast_set(core, NULL);
}

/** Show one toast message immediately and arm the hide timer for the current duration. */
void egui_toast_show(egui_toast_t *self, const char *text)
{
    if (self == NULL || egui_toast_get_core(self) == NULL)
    {
        return;
    }

    self->info = text;
    self->api->on_show(self, text);
    egui_toast_start_timer(self, &self->hide_timer, self->duration, 0);
}

/** Override the duration for one show call, then show the toast. */
void egui_toast_show_info_with_duration(egui_toast_t *self, const char *text, uint16_t duration)
{
    if (self == NULL)
    {
        return;
    }

    egui_toast_set_duration(self, duration);
    egui_toast_show(self, text);
}

/** Show the toast using the configured default duration. */
void egui_toast_show_info(egui_toast_t *self, const char *text)
{
    egui_toast_show_info_with_duration(self, text, EGUI_CONFIG_PARAM_TOAST_DEFAULT_SHOW_TIME);
}

/** Store a debug-only toast name when class-name logging is enabled. */
void egui_toast_set_name(egui_toast_t *self, const char *name)
{
    EGUI_UNUSED(name);
    EGUI_UNUSED(self);
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    self->name = name;
#endif
}

/** Update the duration used by subsequent `egui_toast_show()` calls. */
void egui_toast_set_duration(egui_toast_t *self, uint16_t duration)
{
    self->duration = duration;
}

/** Hide-timer callback that forwards to the subclass hide hook. */
static void hide_timer_callback(egui_timer_t *timer)
{
    egui_toast_t *toast = (egui_toast_t *)timer->user_data;
    toast->api->on_hide(toast);
}

/** Default show hook for subclasses that do not need custom behavior. */
void egui_toast_on_show(egui_toast_t *self, const char *text)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(text);
}

/** Default hide hook for subclasses that do not need custom behavior. */
void egui_toast_on_hide(egui_toast_t *self)
{
    EGUI_UNUSED(self);
}

/** Default string-buffer hook for subclasses that keep their own text storage. */
char *egui_toast_get_str_buf(egui_toast_t *self)
{
    EGUI_UNUSED(self);
    return NULL;
}

static const egui_toast_api_t EGUI_TOAST_API_TABLE_NAME(egui_toast_t) = {
        .on_set_default = NULL,
        .on_show = egui_toast_on_show,
        .on_hide = egui_toast_on_hide,
        .get_str_buf = egui_toast_get_str_buf,
};

/** Initialize the base toast object and bind its auto-hide timer to the supplied core. */
void egui_toast_init(egui_toast_t *self, egui_core_t *core)
{
    EGUI_ASSERT(core != NULL);

    self->core = core;
    self->info = NULL;
    egui_timer_init_timer(&self->hide_timer, self, hide_timer_callback);

    self->duration = 1000;

    self->api = &EGUI_TOAST_API_TABLE_NAME(egui_toast_t);
}

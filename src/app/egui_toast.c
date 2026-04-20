#include <stdio.h>
#include <assert.h>

#include "egui_toast.h"
#include "widget/egui_view.h"
#include "core/egui_core_toast.h"
#include "core/egui_api.h"
#include "background/egui_background_color.h"

egui_core_t *egui_toast_get_core(egui_toast_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }

    return self->core;
}

int egui_toast_start_timer(egui_toast_t *self, egui_timer_t *handle, uint32_t ms, uint32_t period)
{
    egui_core_t *core = egui_toast_get_core(self);

    if (core == NULL)
    {
        return -1;
    }

    return egui_timer_start_timer(core, handle, ms, period);
}

void egui_toast_stop_timer(egui_toast_t *self, egui_timer_t *handle)
{
    egui_core_t *core = egui_toast_get_core(self);

    if (core == NULL)
    {
        return;
    }

    egui_timer_stop_timer(core, handle);
}

int egui_toast_check_timer_start(egui_toast_t *self, egui_timer_t *handle)
{
    egui_core_t *core = egui_toast_get_core(self);

    if (core == NULL)
    {
        return 0;
    }

    return egui_timer_check_timer_start(core, handle);
}

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

void egui_toast_show_info_with_duration(egui_toast_t *self, const char *text, uint16_t duration)
{
    if (self == NULL)
    {
        return;
    }

    egui_toast_set_duration(self, duration);
    egui_toast_show(self, text);
}

void egui_toast_show_info(egui_toast_t *self, const char *text)
{
    egui_toast_show_info_with_duration(self, text, EGUI_CONFIG_PARAM_TOAST_DEFAULT_SHOW_TIME);
}

void egui_toast_set_name(egui_toast_t *self, const char *name)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    self->name = name;
#endif
}

void egui_toast_set_duration(egui_toast_t *self, uint16_t duration)
{
    self->duration = duration;
}

static void hide_timer_callback(egui_timer_t *timer)
{
    egui_toast_t *toast = (egui_toast_t *)timer->user_data;
    toast->api->on_hide(toast);
}

void egui_toast_on_show(egui_toast_t *self, const char *text)
{
    // implement by subclass
}

void egui_toast_on_hide(egui_toast_t *self)
{
    // implement by subclass
}

char *egui_toast_get_str_buf(egui_toast_t *self)
{
    // implement by subclass
    return NULL;
}

static const egui_toast_api_t EGUI_TOAST_API_TABLE_NAME(egui_toast_t) = {
        .on_set_default = NULL,
        .on_show = egui_toast_on_show,
        .on_hide = egui_toast_on_hide,
        .get_str_buf = egui_toast_get_str_buf,
};

void egui_toast_init(egui_toast_t *self, egui_core_t *core)
{
    EGUI_ASSERT(core != NULL);

    self->core = core;
    self->info = NULL;
    egui_timer_init_timer(&self->hide_timer, self, hide_timer_callback);

    self->duration = 1000; // default duration is 1s

    // init api
    self->api = &EGUI_TOAST_API_TABLE_NAME(egui_toast_t);
}

#include <stdio.h>
#include <assert.h>

#include "egui_toast.h"
#include "widget/egui_view.h"
#include "core/egui_core.h"
#include "core/egui_api.h"
#include "background/egui_background_color.h"

void egui_toast_show(egui_toast_t *self, const char *text)
{
    self->info = text;
    self->api->on_show(self, text);
    egui_timer_start_timer(&self->hide_timer, self->duration, 0);
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

EGUI_TOAST_API_DEFINE(egui_toast_t, NULL, NULL, NULL);

void egui_toast_init(egui_toast_t *self)
{
    self->info = NULL;
    self->hide_timer.callback = hide_timer_callback;
    self->hide_timer.user_data = self;

    self->duration = 1000; // default duration is 1s

    // init api
    self->api = &EGUI_TOAST_API_TABLE_NAME(egui_toast_t);
}

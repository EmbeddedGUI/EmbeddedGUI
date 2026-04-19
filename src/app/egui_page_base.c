#include "egui.h"
#include <stdlib.h>
#include <math.h>

#include "egui_page_base.h"
#include "core/egui_core_internal.h"

void egui_page_base_set_layout(egui_page_base_t *self, egui_region_t *layout)
{
    // set view group params
    egui_view_layout((egui_view_t *)&self->root_view, layout); // set layout for
}

void egui_page_base_add_view(egui_page_base_t *self, egui_view_t *view)
{
    egui_view_group_add_child((egui_view_t *)&self->root_view, view);
}

void egui_page_base_set_name(egui_page_base_t *self, const char *name)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    self->name = name;
#endif
}

egui_core_t *egui_page_base_get_core(egui_page_base_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }

    return self->core;
}

egui_toast_t *egui_page_base_get_toast(egui_page_base_t *self)
{
    egui_core_t *core = egui_page_base_get_core(self);

    if (core == NULL)
    {
        return NULL;
    }

    return egui_core_toast_get(core);
}

void egui_page_base_show_toast_info_with_duration(egui_page_base_t *self, const char *text, uint16_t duration)
{
    egui_toast_t *toast = egui_page_base_get_toast(self);

    if (toast == NULL)
    {
        return;
    }

    egui_toast_show_info_with_duration(toast, text, duration);
}

void egui_page_base_show_toast_info(egui_page_base_t *self, const char *text)
{
    egui_page_base_show_toast_info_with_duration(self, text, EGUI_CONFIG_PARAM_TOAST_DEFAULT_SHOW_TIME);
}

int egui_page_base_start_timer(egui_page_base_t *self, egui_timer_t *handle, uint32_t ms, uint32_t period)
{
    egui_core_t *core = egui_page_base_get_core(self);

    if (core == NULL)
    {
        return -1;
    }

    return egui_timer_start_timer(core, handle, ms, period);
}

void egui_page_base_stop_timer(egui_page_base_t *self, egui_timer_t *handle)
{
    egui_core_t *core = egui_page_base_get_core(self);

    if (core == NULL)
    {
        return;
    }

    egui_timer_stop_timer(core, handle);
}

int egui_page_base_check_timer_start(egui_page_base_t *self, egui_timer_t *handle)
{
    egui_core_t *core = egui_page_base_get_core(self);

    if (core == NULL)
    {
        return 0;
    }

    return egui_timer_check_timer_start(core, handle);
}

void egui_page_base_open(egui_page_base_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("open, name: %s\n", self->name);
#endif
    if (egui_page_base_get_core(self) == NULL)
    {
        return;
    }

    self->api->on_open(self);
}

void egui_page_base_close(egui_page_base_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("close, name: %s\n", self->name);
#endif
    self->api->on_close(self);
}

void egui_page_base_refresh(egui_page_base_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("refresh, name: %s\n", self->name);
#endif
    if (self->api->on_refresh)
    {
        self->api->on_refresh(self);
    }
}

void egui_page_base_key_pressed(egui_page_base_t *self, uint16_t keycode)
{
    self->api->on_key_pressed(self, keycode);
}

void egui_page_base_on_open(egui_page_base_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_open, name: %s\n", self->name);
#endif
    egui_core_t *core = egui_page_base_get_core(self);

    if (core == NULL)
    {
        return;
    }

    egui_core_add_user_root_view((egui_view_t *)&self->root_view);
}

void egui_page_base_on_close(egui_page_base_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_close, name: %s\n", self->name);
#endif
    egui_core_t *core = egui_page_base_get_core(self);

    if (core == NULL)
    {
        return;
    }

    egui_core_remove_user_root_view(core, (egui_view_t *)&self->root_view);
}

void egui_page_base_on_key_pressed(egui_page_base_t *self, uint16_t keycode)
{
}

static const egui_page_base_api_t EGUI_VIEW_API_TABLE_NAME(egui_page_base_t) = {
        .on_open = egui_page_base_on_open,
        .on_close = egui_page_base_on_close,

        .on_key_pressed = egui_page_base_on_key_pressed,
};

void egui_page_base_init(egui_page_base_t *self, egui_core_t *core)
{
    EGUI_ASSERT(core != NULL);

    self->core = core;
    egui_view_group_init((egui_view_t *)&self->root_view, core); // init view group
    egui_view_set_position((egui_view_t *)&self->root_view, 0, 0);
    egui_view_set_size((egui_view_t *)&self->root_view, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);

    // egui_view_set_background((egui_view_t *)&self->root_view, (egui_background_t *)&bg_page_base);

    egui_view_set_view_name((egui_view_t *)&self->root_view, "egui_page_base_root_view");
    // init api
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_page_base_t);
}

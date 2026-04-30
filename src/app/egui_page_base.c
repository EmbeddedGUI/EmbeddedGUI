#include "egui.h"
#include <stdlib.h>
#include <math.h>

#include "egui_page_base.h"
#include "egui_toast.h"
#include "core/egui_core_toast.h"
#include "core/egui_core_internal.h"

/**
 * @file egui_page_base.c
 * @brief Base page helpers that expose a lightweight open/close lifecycle for user-root pages outside the activity stack.
 */

/** Apply a layout rectangle to the page root view. */
void egui_page_base_set_layout(egui_page_base_t *self, egui_region_t *layout)
{
    egui_view_layout((egui_view_t *)&self->root_view, layout);
}

/** Add one child view into the page root container. */
void egui_page_base_add_view(egui_page_base_t *self, egui_view_t *view)
{
    egui_view_group_add_child((egui_view_t *)&self->root_view, view);
}

/** Store a debug-only page name when class-name logging is enabled. */
void egui_page_base_set_name(egui_page_base_t *self, const char *name)
{
    EGUI_UNUSED(name);
    EGUI_UNUSED(self);
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    self->name = name;
#endif
}

/** Return the core currently bound to this page. */
egui_core_t *egui_page_base_get_core(egui_page_base_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }

    return self->core;
}

/** Return the default toast currently registered on the same core as this page. */
egui_toast_t *egui_page_base_get_toast(egui_page_base_t *self)
{
    egui_core_t *core = egui_page_base_get_core(self);

    if (core == NULL)
    {
        return NULL;
    }

    return egui_core_toast_get(core);
}

/** Show an info toast from the page using an explicit duration override. */
void egui_page_base_show_toast_info_with_duration(egui_page_base_t *self, const char *text, uint16_t duration)
{
    egui_toast_t *toast = egui_page_base_get_toast(self);

    if (toast == NULL)
    {
        return;
    }

    egui_toast_show_info_with_duration(toast, text, duration);
}

/** Show an info toast from the page using the configured default duration. */
void egui_page_base_show_toast_info(egui_page_base_t *self, const char *text)
{
    egui_page_base_show_toast_info_with_duration(self, text, EGUI_CONFIG_PARAM_TOAST_DEFAULT_SHOW_TIME);
}

/** Start a timer owned by the same core as this page. */
int egui_page_base_start_timer(egui_page_base_t *self, egui_timer_t *handle, uint32_t ms, uint32_t period)
{
    egui_core_t *core = egui_page_base_get_core(self);

    if (core == NULL)
    {
        return -1;
    }

    return egui_timer_start_timer(core, handle, ms, period);
}

/** Stop one timer through the core bound to this page. */
void egui_page_base_stop_timer(egui_page_base_t *self, egui_timer_t *handle)
{
    egui_core_t *core = egui_page_base_get_core(self);

    if (core == NULL)
    {
        return;
    }

    egui_timer_stop_timer(core, handle);
}

/** Return non-zero when the supplied timer is currently started on this page's core. */
int egui_page_base_check_timer_start(egui_page_base_t *self, egui_timer_t *handle)
{
    egui_core_t *core = egui_page_base_get_core(self);

    if (core == NULL)
    {
        return 0;
    }

    return egui_timer_check_timer_start(core, handle);
}

/** Public entry point that opens the page through its lifecycle table. */
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

/** Public entry point that closes the page through its lifecycle table. */
void egui_page_base_close(egui_page_base_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("close, name: %s\n", self->name);
#endif
    self->api->on_close(self);
}

/** Run the optional refresh callback for pages that want to update themselves in place. */
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

/** Forward a key press to the page-specific callback implementation. */
void egui_page_base_key_pressed(egui_page_base_t *self, uint16_t keycode)
{
    self->api->on_key_pressed(self, keycode);
}

/** Default on_open hook: attach the page root into the user-root scene tree. */
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

/** Default on_close hook: remove the page root from the user-root scene tree. */
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

/** Default key hook for pages that do not care about keyboard input. */
void egui_page_base_on_key_pressed(egui_page_base_t *self, uint16_t keycode)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(keycode);
}

static const egui_page_base_api_t EGUI_VIEW_API_TABLE_NAME(egui_page_base_t) = {
        .on_open = egui_page_base_on_open,
        .on_close = egui_page_base_on_close,

        .on_key_pressed = egui_page_base_on_key_pressed,
};

/** Initialize a base page object with a full-screen root view and the default open/close callbacks. */
void egui_page_base_init(egui_page_base_t *self, egui_core_t *core)
{
    EGUI_ASSERT(core != NULL);

    self->core = core;
    egui_view_group_init((egui_view_t *)&self->root_view, core);
    egui_view_set_position((egui_view_t *)&self->root_view, 0, 0);
    egui_view_set_size((egui_view_t *)&self->root_view, core->screen_width, core->screen_height);

    egui_view_set_view_name((egui_view_t *)&self->root_view, "egui_page_base_root_view");
    egui_view_set_dirty_passthrough((egui_view_t *)&self->root_view, 1);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_page_base_t);
}

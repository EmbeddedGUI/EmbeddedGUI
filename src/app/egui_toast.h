#ifndef _EGUI_TOAST_H_
#define _EGUI_TOAST_H_

#include "widget/egui_view_group.h"
#include "core/egui_region.h"
#include "core/egui_timer.h"
#include "core/egui_typedef.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_toast_api egui_toast_api_t;
struct egui_toast_api
{
    void (*on_set_default)(egui_toast_t *self);
    void (*on_show)(egui_toast_t *self, const char *text);
    void (*on_hide)(egui_toast_t *self);
    char *(*get_str_buf)(egui_toast_t *self);
};

#define EGUI_TOAST_API_TABLE_NAME(_name) _name##_api_table

struct egui_toast
{
    egui_core_t *core;       // core pointer
    const char *info;        // show info
    uint16_t duration;       // show duration
    egui_timer_t hide_timer; // hide timer

#if EGUI_CONFIG_DEBUG_CLASS_NAME
    const char *name; // name of the view
#endif

    const egui_toast_api_t *api; // api of the view
};

/** Return the core that owns this toast instance. */
egui_core_t *egui_toast_get_core(egui_toast_t *self);
/** Register this toast as the default toast used by the current core. */
void egui_toast_set_as_default(egui_toast_t *self);
/** Remove this toast from the default-toast slot if it is currently registered. */
void egui_toast_clear_as_default(egui_toast_t *self);
/** Show a toast message immediately using the current duration. */
void egui_toast_show(egui_toast_t *self, const char *text);
/** Show a toast message and override the display duration for this call. */
void egui_toast_show_info_with_duration(egui_toast_t *self, const char *text, uint16_t duration);
/** Show a toast message using the default configured duration. */
void egui_toast_show_info(egui_toast_t *self, const char *text);
/** Set the debug name shown by tracing/log helpers. */
void egui_toast_set_name(egui_toast_t *self, const char *name);
/** Set the default display duration used by egui_toast_show(). */
void egui_toast_set_duration(egui_toast_t *self, uint16_t duration);
/** Start a timer that is automatically associated with this toast's core. */
int egui_toast_start_timer(egui_toast_t *self, egui_timer_t *handle, uint32_t ms, uint32_t period);
/** Stop a timer previously started from this toast. */
void egui_toast_stop_timer(egui_toast_t *self, egui_timer_t *handle);
/** Return non-zero if the timer is already running on this toast's core. */
int egui_toast_check_timer_start(egui_toast_t *self, egui_timer_t *handle);

/** Subclass hook that should create/show the toast visual. */
void egui_toast_on_show(egui_toast_t *self, const char *text);
/** Subclass hook that should hide the toast visual. */
void egui_toast_on_hide(egui_toast_t *self);
/** Subclass hook that returns a writable text buffer if needed by the implementation. */
char *egui_toast_get_str_buf(egui_toast_t *self);

/** Initialize the base toast object and its auto-hide timer. */
void egui_toast_init(egui_toast_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_TOAST_H_ */

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

egui_core_t *egui_toast_get_core(egui_toast_t *self);
void egui_toast_set_as_default(egui_toast_t *self);
void egui_toast_clear_as_default(egui_toast_t *self);
void egui_toast_show(egui_toast_t *self, const char *text);
void egui_toast_show_info_with_duration(egui_toast_t *self, const char *text, uint16_t duration);
void egui_toast_show_info(egui_toast_t *self, const char *text);
void egui_toast_set_name(egui_toast_t *self, const char *name);
void egui_toast_set_duration(egui_toast_t *self, uint16_t duration);
int egui_toast_start_timer(egui_toast_t *self, egui_timer_t *handle, uint32_t ms, uint32_t period);
void egui_toast_stop_timer(egui_toast_t *self, egui_timer_t *handle);
int egui_toast_check_timer_start(egui_toast_t *self, egui_timer_t *handle);

void egui_toast_on_show(egui_toast_t *self, const char *text);
void egui_toast_on_hide(egui_toast_t *self);
char *egui_toast_get_str_buf(egui_toast_t *self);

void egui_toast_init(egui_toast_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_TOAST_H_ */

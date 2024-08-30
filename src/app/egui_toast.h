#ifndef _EGUI_TOAST_H_
#define _EGUI_TOAST_H_

#include "widget/egui_view_group.h"
#include "core/egui_region.h"
#include "core/egui_timer.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_toast egui_toast_t;

typedef struct egui_toast_api egui_toast_api_t;
struct egui_toast_api
{
    void (*on_show)(egui_toast_t *self, const char *text);
    void (*on_hide)(egui_toast_t *self);
    char *(*get_str_buf)(egui_toast_t *self);
};

#define EGUI_TOAST_API_TABLE_NAME(_name) _name##_api_table

#define EGUI_TOAST_API_DEFINE(_name, _on_show, _on_hide, _get_str_buf)                                                                                         \
    static const egui_toast_api_t EGUI_TOAST_API_TABLE_NAME(_name) = {                                                                                         \
            .on_show = _on_show == NULL ? egui_toast_on_show : _on_show,                                                                                       \
            .on_hide = _on_hide == NULL ? egui_toast_on_hide : _on_hide,                                                                                       \
            .get_str_buf = _get_str_buf == NULL ? egui_toast_get_str_buf : _get_str_buf,                                                                       \
    };

struct egui_toast
{
    const char *info;        // show info
    uint16_t duration;       // show duration
    egui_timer_t hide_timer; // hide timer

#if EGUI_CONFIG_DEBUG_CLASS_NAME
    const char *name; // name of the view
#endif

    const egui_toast_api_t *api; // api of the view
};

void egui_toast_show(egui_toast_t *self, const char *text);
void egui_toast_set_name(egui_toast_t *self, const char *name);
void egui_toast_set_duration(egui_toast_t *self, uint16_t duration);

void egui_toast_on_show(egui_toast_t *self, const char *text);
void egui_toast_on_hide(egui_toast_t *self);
char *egui_toast_get_str_buf(egui_toast_t *self);

void egui_toast_init(egui_toast_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_TOAST_H_ */

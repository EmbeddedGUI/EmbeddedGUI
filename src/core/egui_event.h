#ifndef _EGUI_EVENT_H_
#define _EGUI_EVENT_H_

#include <stdint.h>

#include "config/egui_config.h"
#include "egui_typedef.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#if EGUI_CONFIG_FUNCTION_EVENT_LITE

typedef enum egui_event_code
{
    EGUI_EVENT_ALL = 0,
    EGUI_EVENT_PRESSED,
    EGUI_EVENT_RELEASED,
    EGUI_EVENT_CLICKED,
    EGUI_EVENT_VALUE_CHANGED,
    EGUI_EVENT_FOCUSED,
    EGUI_EVENT_DEFOCUSED,
    EGUI_EVENT_SIZE_CHANGED,
    EGUI_EVENT_LAYOUT_CHANGED,
    EGUI_EVENT_LANGUAGE_CHANGED,
    EGUI_EVENT_CUSTOM_BASE = 128,
} egui_event_code_t;

struct egui_event
{
    egui_event_code_t code;
    egui_view_t *target;
    egui_view_t *current_target;
    void *param;
    void *user_data;
};

typedef void (*egui_event_cb_t)(egui_event_t *event);

struct egui_event_listener
{
    egui_event_code_t code;
    egui_event_cb_t cb;
    void *user_data;
};

int egui_view_add_event_listener(egui_view_t *self, egui_event_code_t code, egui_event_cb_t cb, void *user_data);
int egui_view_remove_event_listener(egui_view_t *self, egui_event_code_t code, egui_event_cb_t cb, void *user_data);
void egui_view_clear_event_listeners(egui_view_t *self);
int egui_view_send_event(egui_view_t *self, egui_event_code_t code, void *param);
void egui_view_send_event_to_tree(egui_view_t *root, egui_event_code_t code, void *param);
void egui_core_send_event_to_tree(egui_core_t *core, egui_event_code_t code, void *param);

#endif /* EGUI_CONFIG_FUNCTION_EVENT_LITE */

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_EVENT_H_ */

#ifndef _EGUI_VIEW_SWITCH_H_
#define _EGUI_VIEW_SWITCH_H_

#include "egui_view.h"
#include "core/egui_theme.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*egui_view_on_checked_listener_t)(egui_view_t *self, int is_checked);

typedef struct egui_view_switch egui_view_switch_t;
struct egui_view_switch
{
    egui_view_t base;

    egui_view_on_checked_listener_t on_checked_changed;

    uint8_t is_checked;
    egui_alpha_t alpha;
    egui_color_t switch_color_on;
    egui_color_t switch_color_off;
    egui_color_t bk_color_on;
    egui_color_t bk_color_off;
};

void egui_view_switch_set_on_checked_listener(egui_view_t *self, egui_view_on_checked_listener_t listener);
void egui_view_switch_set_checked(egui_view_t *self, uint8_t is_checked);
void egui_view_switch_on_draw(egui_view_t *self);
void egui_view_switch_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_SWITCH_H_ */

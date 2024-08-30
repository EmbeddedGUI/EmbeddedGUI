#ifndef _EGUI_VIEW_PROGRESS_BAR_H_
#define _EGUI_VIEW_PROGRESS_BAR_H_

#include "egui_view.h"
#include "core/egui_theme.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*egui_view_on_progress_changed_listener_t)(egui_view_t *self, uint8_t progress);

typedef struct egui_view_progress_bar egui_view_progress_bar_t;
struct egui_view_progress_bar
{
    egui_view_t base;

    egui_view_on_progress_changed_listener_t on_progress_changed;

    uint8_t process;
    uint8_t is_show_control;
    egui_color_t control_color;
    egui_color_t bk_color;
    egui_color_t progress_color;
};

void egui_view_progress_bar_set_on_progress_listener(egui_view_t *self, egui_view_on_progress_changed_listener_t listener);
void egui_view_progress_bar_set_process(egui_view_t *self, uint8_t process);
void egui_view_progress_bar_on_draw(egui_view_t *self);
void egui_view_progress_bar_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_PROGRESS_BAR_H_ */

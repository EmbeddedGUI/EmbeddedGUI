#ifndef _EGUI_VIEW_BUTTON_H_
#define _EGUI_VIEW_BUTTON_H_

#include "egui_view_label.h"
#include "core/egui_theme.h"
#include "font/egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_button egui_view_button_t;
struct egui_view_button
{
    egui_view_label_t base;
};

void egui_view_button_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_BUTTON_H_ */

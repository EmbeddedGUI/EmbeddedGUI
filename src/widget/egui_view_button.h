#ifndef _EGUI_VIEW_BUTTON_H_
#define _EGUI_VIEW_BUTTON_H_

#include "egui_view_label.h"
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

// ============== Button Params (reuse Label) ==============
#define EGUI_VIEW_BUTTON_PARAMS_INIT        EGUI_VIEW_LABEL_PARAMS_INIT
#define EGUI_VIEW_BUTTON_PARAMS_INIT_SIMPLE EGUI_VIEW_LABEL_PARAMS_INIT_SIMPLE
#define egui_view_button_apply_params       egui_view_label_apply_params

void egui_view_button_init(egui_view_t *self);
void egui_view_button_init_with_params(egui_view_t *self, const egui_view_label_params_t *params);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_BUTTON_H_ */

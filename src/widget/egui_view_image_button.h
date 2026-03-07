#ifndef _EGUI_VIEW_IMAGE_BUTTON_H_
#define _EGUI_VIEW_IMAGE_BUTTON_H_

#include "egui_view_image.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_image_button egui_view_image_button_t;
struct egui_view_image_button
{
    egui_view_image_t base;
};

// ============== ImageButton Params (reuse Image) ==============
#define EGUI_VIEW_IMAGE_BUTTON_PARAMS_INIT  EGUI_VIEW_IMAGE_PARAMS_INIT
#define egui_view_image_button_apply_params egui_view_image_apply_params

void egui_view_image_button_on_draw(egui_view_t *self);
void egui_view_image_button_init(egui_view_t *self);
void egui_view_image_button_init_with_params(egui_view_t *self, const egui_view_image_params_t *params);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_IMAGE_BUTTON_H_ */

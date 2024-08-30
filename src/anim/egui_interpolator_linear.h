#ifndef _EGUI_INTERPOLATOR_LINEAR_H_
#define _EGUI_INTERPOLATOR_LINEAR_H_

#include "egui_interpolator.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_interpolator_linear egui_interpolator_linear_t;
struct egui_interpolator_linear
{
    egui_interpolator_t base;
};

void egui_interpolator_linear_draw(egui_interpolator_t *self, egui_canvas_t *canvas, egui_region_t *region, int is_disabled, int is_pressed);
void egui_interpolator_linear_init(egui_interpolator_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_INTERPOLATOR_LINEAR_H_ */

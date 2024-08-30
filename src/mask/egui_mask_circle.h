#ifndef _EGUI_MASK_COLOR_H_
#define _EGUI_MASK_COLOR_H_

#include "egui_mask.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_mask_circle egui_mask_circle_t;
struct egui_mask_circle
{
    egui_mask_t base;
};

void egui_mask_circle_mask_point(egui_mask_t *self, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha);
void egui_mask_circle_init(egui_mask_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_MASK_COLOR_H_ */

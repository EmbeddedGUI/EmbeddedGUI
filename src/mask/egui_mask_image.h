#ifndef _EGUI_MASK_IMAGE_H_
#define _EGUI_MASK_IMAGE_H_

#include "egui_mask.h"
#include "image/egui_image.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_mask_image egui_mask_image_t;
struct egui_mask_image
{
    egui_mask_t base;

    egui_image_t *img;
};

void egui_mask_image_set_image(egui_mask_t *self, egui_image_t *img);
void egui_mask_image_mask_point(egui_mask_t *self, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha);
void egui_mask_image_init(egui_mask_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_MASK_IMAGE_H_ */
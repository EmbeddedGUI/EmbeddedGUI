#ifndef _EGUI_UTILS_H_
#define _EGUI_UTILS_H_

#include "widget/egui_view_image.h"
#include "image/egui_image.h"
#include "image/egui_image_std.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

void egui_utils_view_image_std_normal_init(egui_view_t *self, egui_image_t *image, egui_dim_t x, egui_dim_t y);


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_UTILS_H_ */
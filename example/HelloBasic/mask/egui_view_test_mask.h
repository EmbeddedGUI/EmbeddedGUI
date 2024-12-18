#ifndef _EGUI_VIEW_TEST_MASK_H_
#define _EGUI_VIEW_TEST_MASK_H_

#include "widget/egui_view_image.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_test_mask egui_view_test_mask_t;
struct egui_view_test_mask
{
    egui_view_image_t base;

    egui_mask_t* mask;
};

void egui_view_test_mask_set_mask(egui_view_t *self, egui_mask_t *mask);
void egui_view_test_mask_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_TEST_MASK_H_ */

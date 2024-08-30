#ifndef _EGUI_VIEW_TEST_H_
#define _EGUI_VIEW_TEST_H_

#include "widget/egui_view.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_test egui_view_test_t;
struct egui_view_test
{
    egui_view_t base;

    int last_pos;
};

void egui_view_test_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_TEST_H_ */

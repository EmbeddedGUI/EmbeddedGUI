#ifndef _EFUI_ACTIVITY_TEST_H_
#define _EFUI_ACTIVITY_TEST_H_

#include "egui.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_activity_test egui_activity_test_t;
struct egui_activity_test
{
    egui_activity_t base;

    int index;
    char label_str[20];

    egui_view_linearlayout_t layout_1;
    egui_view_label_t label_1;
    egui_view_button_t button_1;
    egui_view_button_t button_2;
    egui_view_button_t button_3;
};

void egui_activity_test_set_index(egui_activity_t *self, int index);
void egui_activity_test_init(egui_activity_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EFUI_ACTIVITY_TEST_H_ */

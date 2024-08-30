#ifndef _EFUI_DIALOG_TEST_H_
#define _EFUI_DIALOG_TEST_H_

#include "egui.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_dialog_test egui_dialog_test_t;
struct egui_dialog_test
{
    egui_dialog_t base;

    egui_view_linearlayout_t layout_1;
    egui_view_label_t label_1;
    egui_view_button_t button_1;
};

void egui_dialog_test_set_index(egui_dialog_t *self, int index);
void egui_dialog_test_init(egui_dialog_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EFUI_DIALOG_TEST_H_ */

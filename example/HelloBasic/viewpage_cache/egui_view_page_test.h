#ifndef _EGUI_VIEW_PAGE_1_H_
#define _EGUI_VIEW_PAGE_1_H_

#include "egui.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_page_test egui_view_page_test_t;
struct egui_view_page_test
{
    egui_view_group_t base;

    int index;
    char label_str[20];

    egui_view_label_t label_1;
};

void egui_view_page_test_set_index(egui_view_t *self, int index);
void egui_view_page_test_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_PAGE_1_H_ */

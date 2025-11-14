#ifndef _EGUI_PAGE_1_H_
#define _EGUI_PAGE_1_H_

#include "egui.h"

#include "egui_page_base.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_page_1 egui_page_1_t;
struct egui_page_1
{
    egui_page_base_t base;

    egui_timer_t timer;

    egui_view_linearlayout_t layout_1;
    egui_view_label_t label_1;
    egui_view_button_t button_1;
    egui_view_button_t button_2;

    int index;
    char label_str[20];
};

void egui_page_1_set_index(egui_page_base_t *self, int index);
void egui_page_1_init(egui_page_base_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_PAGE_1_H_ */

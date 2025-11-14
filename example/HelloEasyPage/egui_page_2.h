#ifndef _EGUI_PAGE_2_H_
#define _EGUI_PAGE_2_H_

#include "egui.h"

#include "egui_page_base.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_page_2 egui_page_2_t;
struct egui_page_2
{
    egui_page_base_t base;

    int index;
    char label_str[20];

    egui_view_linearlayout_t layout_1;
    egui_view_label_t label_1;
    egui_view_button_t button_1;
    egui_view_button_t button_2;

    egui_timer_t timer;
};

void egui_page_2_set_index(egui_page_base_t *self, int index);
void egui_page_2_init(egui_page_base_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_PAGE_2_H_ */

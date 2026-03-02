#ifndef _EGUI_VIEW_CANVAS_H_
#define _EGUI_VIEW_CANVAS_H_

#include "widget/egui_view.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_CANVAS_PAGE_COUNT 11

typedef struct egui_view_canvas egui_view_canvas_t;
struct egui_view_canvas
{
    egui_view_t base;
    uint8_t current_page;
};

void egui_view_canvas_init(egui_view_t *self);
void egui_view_canvas_init_with_page(egui_view_t *self, uint8_t page);
void egui_view_canvas_set_page(egui_view_t *self, uint8_t page);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_CANVAS_H_ */

#ifndef _EGUI_VIEW_DYNAMIC_LABEL_H_
#define _EGUI_VIEW_DYNAMIC_LABEL_H_

#include "egui.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_DYNAMIC_LABEL_MAX_SIZE 20

typedef struct egui_view_dynamic_label egui_view_dynamic_label_t;
struct egui_view_dynamic_label
{
    egui_view_label_t base;

    char text_buffer[EGUI_VIEW_DYNAMIC_LABEL_MAX_SIZE];
};

void egui_view_dynamic_label_set_text(egui_view_t *self, const char *text);
void egui_view_dynamic_label_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_DYNAMIC_LABEL_H_ */

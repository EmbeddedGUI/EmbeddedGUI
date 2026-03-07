#ifndef _EGUI_VIEW_SAMPLE_WIDGET_H_
#define _EGUI_VIEW_SAMPLE_WIDGET_H_

#include "egui.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_sample_widget egui_view_sample_widget_t;
struct egui_view_sample_widget
{
    egui_view_label_t base;
    egui_color_t border_color;
    uint8_t border_width;
};

void egui_view_sample_widget_set_border(egui_view_t *self, egui_color_t color, uint8_t width);
void egui_view_sample_widget_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_SAMPLE_WIDGET_H_ */

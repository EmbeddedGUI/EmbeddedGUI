#ifndef _EGUI_VIEW_SAMPLE_CHART_H_
#define _EGUI_VIEW_SAMPLE_CHART_H_

#include "egui.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_sample_chart egui_view_sample_chart_t;
struct egui_view_sample_chart
{
    egui_view_t base;

    const egui_font_t *font;

    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t accent_color;
};

void egui_view_sample_chart_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_SAMPLE_CHART_H_ */

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
    // TODO: add widget fields
};

void egui_view_sample_chart_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_SAMPLE_CHART_H_ */

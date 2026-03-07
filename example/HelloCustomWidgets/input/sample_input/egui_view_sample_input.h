#ifndef _EGUI_VIEW_SAMPLE_INPUT_H_
#define _EGUI_VIEW_SAMPLE_INPUT_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_sample_input egui_view_sample_input_t;
struct egui_view_sample_input
{
    egui_view_linearlayout_t base;
    egui_view_label_t label;
    egui_view_slider_t slider;
};

void egui_view_sample_input_init(egui_view_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_SAMPLE_INPUT_H_ */

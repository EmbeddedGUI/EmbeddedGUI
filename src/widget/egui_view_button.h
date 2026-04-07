#ifndef _EGUI_VIEW_BUTTON_H_
#define _EGUI_VIEW_BUTTON_H_

#include "egui_view_label.h"
#include "font/egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_button egui_view_button_t;
struct egui_view_button
{
    egui_view_label_t base;

    const char *icon;
    const egui_font_t *icon_font;
    egui_dim_t icon_text_gap;
};

// ============== Button Params (reuse Label) ==============
#define EGUI_VIEW_BUTTON_PARAMS_INIT EGUI_VIEW_LABEL_PARAMS_INIT
#define EGUI_VIEW_BUTTON_PARAMS_INIT_SIMPLE(_name, _x, _y, _w, _h, _text)                                                                                      \
    static const egui_view_label_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}},                                                                     \
                                                   .align_type = EGUI_ALIGN_CENTER,                                                                            \
                                                   .text = (_text),                                                                                            \
                                                   .font = NULL,                                                                                               \
                                                   .color = EGUI_COLOR_WHITE,                                                                                  \
                                                   .alpha = EGUI_ALPHA_100}

void egui_view_button_apply_params(egui_view_t *self, const egui_view_label_params_t *params);
void egui_view_button_init(egui_view_t *self);
void egui_view_button_init_with_params(egui_view_t *self, const egui_view_label_params_t *params);
void egui_view_button_set_icon(egui_view_t *self, const char *icon);
void egui_view_button_set_icon_font(egui_view_t *self, const egui_font_t *font);
void egui_view_button_set_icon_text_gap(egui_view_t *self, egui_dim_t gap);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_BUTTON_H_ */

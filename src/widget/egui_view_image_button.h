#ifndef _EGUI_VIEW_IMAGE_BUTTON_H_
#define _EGUI_VIEW_IMAGE_BUTTON_H_

#include "egui_view_image.h"
#include "font/egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_image_button egui_view_image_button_t;
struct egui_view_image_button
{
    egui_view_image_t base;

    const char *icon;
    const char *text;
    const egui_font_t *font;
    const egui_font_t *icon_font;
    egui_color_t content_color;
    egui_alpha_t content_alpha;
    egui_dim_t icon_text_gap;
};

// ============== ImageButton Params (reuse Image) ==============
#define EGUI_VIEW_IMAGE_BUTTON_PARAMS_INIT  EGUI_VIEW_IMAGE_PARAMS_INIT
#define egui_view_image_button_apply_params egui_view_image_apply_params

void egui_view_image_button_on_draw(egui_view_t *self);
void egui_view_image_button_init(egui_view_t *self);
void egui_view_image_button_init_with_params(egui_view_t *self, const egui_view_image_params_t *params);
void egui_view_image_button_set_icon(egui_view_t *self, const char *icon);
void egui_view_image_button_set_text(egui_view_t *self, const char *text);
void egui_view_image_button_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_image_button_set_icon_font(egui_view_t *self, const egui_font_t *font);
void egui_view_image_button_set_content_color(egui_view_t *self, egui_color_t color, egui_alpha_t alpha);
void egui_view_image_button_set_icon_text_gap(egui_view_t *self, egui_dim_t gap);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_IMAGE_BUTTON_H_ */

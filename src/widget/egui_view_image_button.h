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
#define EGUI_VIEW_IMAGE_BUTTON_PARAMS_INIT EGUI_VIEW_IMAGE_PARAMS_INIT

/** Default draw hook used by the image-button API table. */
void egui_view_image_button_on_draw(egui_view_t *self);
/** Initialize an image-backed content button shell. */
void egui_view_image_button_init(egui_view_t *self, egui_core_t *core);
/** Apply image-style parameter fields to an image button. */
void egui_view_image_button_apply_params(egui_view_t *self, const egui_view_image_params_t *params);
/** Initialize an image button and immediately apply its parameter block. */
void egui_view_image_button_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_image_params_t *params);
/** Set an optional icon glyph shown in the content region. */
void egui_view_image_button_set_icon(egui_view_t *self, const char *icon);
/** Return the optional icon glyph shown in the content region. */
const char *egui_view_image_button_get_icon(egui_view_t *self);
/** Set optional text shown in the content region. */
void egui_view_image_button_set_text(egui_view_t *self, const char *text);
/** Return the optional text shown in the content region. */
const char *egui_view_image_button_get_text(egui_view_t *self);
/** Override the font used for the content text. */
void egui_view_image_button_set_font(egui_view_t *self, const egui_font_t *font);
/** Return the font override used for the content text. */
const egui_font_t *egui_view_image_button_get_font(egui_view_t *self);
/** Override the font used for the icon glyph. */
void egui_view_image_button_set_icon_font(egui_view_t *self, const egui_font_t *font);
/** Return the font override used for the icon glyph. */
const egui_font_t *egui_view_image_button_get_icon_font(egui_view_t *self);
/** Set the shared color/alpha used by icon and text overlays. */
void egui_view_image_button_set_content_color(egui_view_t *self, egui_color_t color, egui_alpha_t alpha);
/** Return the shared color used by icon and text overlays. */
egui_color_t egui_view_image_button_get_content_color(egui_view_t *self);
/** Return the shared alpha used by icon and text overlays. */
egui_alpha_t egui_view_image_button_get_content_alpha(egui_view_t *self);
/** Set the vertical spacing between icon and text when both are shown. */
void egui_view_image_button_set_icon_text_gap(egui_view_t *self, egui_dim_t gap);
/** Return the vertical spacing between icon and text when both are shown. */
egui_dim_t egui_view_image_button_get_icon_text_gap(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_IMAGE_BUTTON_H_ */

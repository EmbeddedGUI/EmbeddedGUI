#ifndef _EGUI_VIEW_LABEL_H_
#define _EGUI_VIEW_LABEL_H_

#include "egui_view.h"
#include "font/egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_label egui_view_label_t;
struct egui_view_label
{
    egui_view_t base;

    egui_dim_t line_space;

    uint8_t align_type;
    egui_alpha_t alpha;
    egui_color_t color;

    const egui_font_t *font;
    const char *text;
};

// ============== Label Params ==============
typedef struct egui_view_label_params egui_view_label_params_t;
struct egui_view_label_params
{
    egui_region_t region;
    uint8_t align_type;
    const char *text;
    const egui_font_t *font;
    egui_color_t color;
    egui_alpha_t alpha;
};

#define EGUI_VIEW_LABEL_PARAMS_INIT(_name, _x, _y, _w, _h, _text, _font, _color, _alpha)                                                                       \
    static const egui_view_label_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}},                                                                     \
                                                   .align_type = EGUI_ALIGN_CENTER,                                                                            \
                                                   .text = (_text),                                                                                            \
                                                   .font = (const egui_font_t *)(_font),                                                                       \
                                                   .color = (_color),                                                                                          \
                                                   .alpha = (_alpha)}

#define EGUI_VIEW_LABEL_PARAMS_INIT_SIMPLE(_name, _x, _y, _w, _h, _text)                                                                                       \
    static const egui_view_label_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}},                                                                     \
                                                   .align_type = EGUI_ALIGN_CENTER,                                                                            \
                                                   .text = (_text),                                                                                            \
                                                   .font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT,                                                      \
                                                   .color = EGUI_COLOR_WHITE,                                                                                  \
                                                   .alpha = EGUI_ALPHA_100}

void egui_view_label_apply_params(egui_view_t *self, const egui_view_label_params_t *params);
void egui_view_label_init_with_params(egui_view_t *self, const egui_view_label_params_t *params);

void egui_view_label_on_draw(egui_view_t *self);
void egui_view_label_set_line_space(egui_view_t *self, egui_dim_t line_space);
void egui_view_label_set_font_with_std_height(egui_view_t *self, const egui_font_t *font);
void egui_view_label_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_label_set_font_color(egui_view_t *self, egui_color_t color, egui_alpha_t alpha);
void egui_view_label_set_align_type(egui_view_t *self, uint8_t align_type);
void egui_view_label_set_text(egui_view_t *self, const char *text);
int egui_view_label_get_str_size(egui_view_t *self, const void *string, egui_dim_t *width, egui_dim_t *height);
int egui_view_label_get_str_size_with_padding(egui_view_t *self, const void *string, egui_dim_t *width, egui_dim_t *height);
void egui_view_label_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_LABEL_H_ */

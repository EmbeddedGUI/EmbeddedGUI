#ifndef _EGUI_VIEW_LABEL_H_
#define _EGUI_VIEW_LABEL_H_

#include "egui_view.h"
#include "font/egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_label egui_view_label_t;
/** Basic text view that draws one borrowed string inside its content region. */
struct egui_view_label
{
    egui_view_t base;

    egui_dim_t line_space; /* Extra spacing inserted between wrapped lines. */

    uint8_t align_type; /* Text alignment flags used by the canvas text helper. */
    egui_alpha_t alpha; /* Text alpha. */
    egui_color_t color; /* Text color. */

    const egui_font_t *font; /* Borrowed font pointer used for measurement and drawing. NULL can fall back to compact ASCII text when enabled. */
    const char *text;        /* Borrowed string pointer rendered by the label. */
};

// ============== Label Params ==============
typedef struct egui_view_label_params egui_view_label_params_t;
/** Construction-time parameters for a label widget. */
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

/** Apply a label parameter block after initialization. */
void egui_view_label_apply_params(egui_view_t *self, const egui_view_label_params_t *params);
/** Initialize a label and immediately apply its parameter block. */
void egui_view_label_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_label_params_t *params);

/** Default draw hook used by the label API table. */
void egui_view_label_on_draw(egui_view_t *self);
/** Set extra spacing inserted between wrapped text lines. */
void egui_view_label_set_line_space(egui_view_t *self, egui_dim_t line_space);
/** Set the font and resize the label height to the standard font height. */
void egui_view_label_set_font_with_std_height(egui_view_t *self, const egui_font_t *font);
/** Set the font used to render the label text. */
void egui_view_label_set_font(egui_view_t *self, const egui_font_t *font);
/** Set text color and alpha in one call. */
void egui_view_label_set_font_color(egui_view_t *self, egui_color_t color, egui_alpha_t alpha);
/** Set text alignment inside the label content region. */
void egui_view_label_set_align_type(egui_view_t *self, uint8_t align_type);
/** Replace the displayed text pointer and request a redraw. */
void egui_view_label_set_text(egui_view_t *self, const char *text);
/** Measure a string using the label font and line spacing. */
int egui_view_label_get_str_size(egui_view_t *self, const void *string, egui_dim_t *width, egui_dim_t *height);
/** Measure a string and then add the label padding to the result. */
int egui_view_label_get_str_size_with_padding(egui_view_t *self, const void *string, egui_dim_t *width, egui_dim_t *height);
/** Initialize the label base widget. */
void egui_view_label_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_LABEL_H_ */

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

#if EGUI_CONFIG_FUNCTION_LABEL_LETTER_SPACE
    egui_dim_t letter_space; /* Extra pixel gap inserted between characters (0 = default). */
#endif

    uint8_t align_type; /* Text alignment flags used by the canvas text helper. */
    egui_alpha_t alpha; /* Text alpha. */
    egui_color_t color; /* Text color. */

    const egui_font_t *font; /* Borrowed font pointer used for measurement and drawing. NULL can fall back to compact ASCII text when enabled. */
    const char *text;        /* Borrowed string pointer rendered by the label. */

#if EGUI_CONFIG_FUNCTION_LABEL_LONG_MODE
    uint8_t long_mode : 2; /* Text overflow mode: 0=CLIP (default), 1=DOTS. */
#endif

#if EGUI_CONFIG_FUNCTION_LABEL_RECOLOR
    uint8_t is_recolor : 1; /* 1 = parse inline #RRGGBB color tags in text. */
#endif

#if EGUI_CONFIG_FUNCTION_LABEL_TEXT_FMT
    char fmt_buf[EGUI_CONFIG_LABEL_FMT_BUF_SIZE]; /* Internal buffer used by set_text_fmt(). */
#endif
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

#define EGUI_VIEW_LABEL_PARAMS_INIT_COLOR(_name, _x, _y, _w, _h, _text, _font, _color, _alpha)                                                                 \
    static const egui_view_label_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}},                                                                     \
                                                   .align_type = EGUI_ALIGN_CENTER,                                                                            \
                                                   .text = (_text),                                                                                            \
                                                   .font = (const egui_font_t *)(_font),                                                                       \
                                                   .color = _color,                                                                                            \
                                                   .alpha = (_alpha)}

#if defined(_MSC_VER)
#define EGUI_VIEW_LABEL_PARAMS_INIT(_name, _x, _y, _w, _h, _text, _font, _color, _alpha)                                                                       \
    EGUI_VIEW_LABEL_PARAMS_INIT_COLOR(_name, _x, _y, _w, _h, _text, _font, _color##_INIT, _alpha)
#else
#define EGUI_VIEW_LABEL_PARAMS_INIT(_name, _x, _y, _w, _h, _text, _font, _color, _alpha)                                                                       \
    EGUI_VIEW_LABEL_PARAMS_INIT_COLOR(_name, _x, _y, _w, _h, _text, _font, (_color), _alpha)
#endif

#define EGUI_VIEW_LABEL_PARAMS_INIT_SIMPLE(_name, _x, _y, _w, _h, _text)                                                                                       \
    static const egui_view_label_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}},                                                                     \
                                                   .align_type = EGUI_ALIGN_CENTER,                                                                            \
                                                   .text = (_text),                                                                                            \
                                                   .font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT,                                                      \
                                                   .color = EGUI_COLOR_WHITE_INIT,                                                                             \
                                                   .alpha = EGUI_ALPHA_100}

/** Apply a label parameter block after initialization. */
void egui_view_label_apply_params(egui_view_t *self, const egui_view_label_params_t *params);
/** Initialize a label and immediately apply its parameter block. */
void egui_view_label_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_label_params_t *params);

/** Default draw hook used by the label API table. */
void egui_view_label_on_draw(egui_view_t *self);
/** Set extra spacing inserted between wrapped text lines. */
void egui_view_label_set_line_space(egui_view_t *self, egui_dim_t line_space);
/** Return the current extra line spacing in pixels. */
egui_dim_t egui_view_label_get_line_space(egui_view_t *self);
#if EGUI_CONFIG_FUNCTION_LABEL_LETTER_SPACE
/** Set extra pixel gap between consecutive characters (0 = default, no extra gap). */
void egui_view_label_set_letter_space(egui_view_t *self, egui_dim_t letter_space);
/** Return the current extra character spacing in pixels. Returns 0 when self is NULL or feature is disabled. */
egui_dim_t egui_view_label_get_letter_space(egui_view_t *self);
#endif /* EGUI_CONFIG_FUNCTION_LABEL_LETTER_SPACE */
/** Set the font and resize the label height to the standard font height. */
void egui_view_label_set_font_with_std_height(egui_view_t *self, const egui_font_t *font);
/** Set the font used to render the label text. */
void egui_view_label_set_font(egui_view_t *self, const egui_font_t *font);
/** Return the font currently used to render the label text. */
const egui_font_t *egui_view_label_get_font(egui_view_t *self);
/** Set text color and alpha in one call. */
void egui_view_label_set_font_color(egui_view_t *self, egui_color_t color, egui_alpha_t alpha);
/** Return the current text color. Returns a zeroed color when self is NULL. */
egui_color_t egui_view_label_get_font_color(egui_view_t *self);
/** Return the current text alpha. Returns 0 when self is NULL. */
egui_alpha_t egui_view_label_get_alpha(egui_view_t *self);
/** Set text alignment inside the label content region. */
void egui_view_label_set_align_type(egui_view_t *self, uint8_t align_type);
/** Return the current text alignment type. */
uint8_t egui_view_label_get_align_type(egui_view_t *self);
/** Replace the displayed text pointer and request a redraw. */
void egui_view_label_set_text(egui_view_t *self, const char *text);
/** Return the currently displayed text pointer, or NULL when self is NULL. */
const char *egui_view_label_get_text(egui_view_t *self);
#if EGUI_CONFIG_FUNCTION_LABEL_TEXT_FMT
/** Format text printf-style into an internal buffer and display it. */
void egui_view_label_set_text_fmt(egui_view_t *self, const char *fmt, ...);
#endif /* EGUI_CONFIG_FUNCTION_LABEL_TEXT_FMT */
/** Measure a string using the label font and line spacing. */
int egui_view_label_get_str_size(egui_view_t *self, const void *string, egui_dim_t *width, egui_dim_t *height);
/** Measure a string and then add the label padding to the result. */
int egui_view_label_get_str_size_with_padding(egui_view_t *self, const void *string, egui_dim_t *width, egui_dim_t *height);
/** Initialize the label base widget. */
void egui_view_label_init(egui_view_t *self, egui_core_t *core);

#if EGUI_CONFIG_FUNCTION_LABEL_LONG_MODE
/** Text overflow mode values for set_long_mode(). */
#define EGUI_LABEL_LONG_CLIP 0 /* Silent clip — default, zero overhead.   */
#define EGUI_LABEL_LONG_DOTS 1 /* Truncate text and append "...".          */
#if EGUI_CONFIG_FUNCTION_LABEL_WORD_WRAP
#define EGUI_LABEL_LONG_WRAP 2 /* Automatic word wrap; text folds at word boundaries. */
#endif                         /* EGUI_CONFIG_FUNCTION_LABEL_WORD_WRAP */

/** Set the text overflow mode. EGUI_LABEL_LONG_CLIP (0) or EGUI_LABEL_LONG_DOTS (1). */
void egui_view_label_set_long_mode(egui_view_t *self, uint8_t mode);
/** Return the current text overflow mode. */
uint8_t egui_view_label_get_long_mode(egui_view_t *self);
#endif /* EGUI_CONFIG_FUNCTION_LABEL_LONG_MODE */

#if EGUI_CONFIG_FUNCTION_LABEL_RECOLOR
/**
 * @brief Enable or disable inline color tags in the label text.
 *
 * When enabled, the label text may contain segments formatted as
 * #RRGGBB text# to draw that segment in the given RGB color.
 * Segments without tags are rendered in the label's default color.
 *
 * @param self        The label view.
 * @param is_recolor  1 to enable recolor parsing, 0 to disable (default).
 */
void egui_view_label_set_recolor(egui_view_t *self, uint8_t is_recolor);
/** Return whether inline color tag parsing is enabled. */
uint8_t egui_view_label_get_recolor(egui_view_t *self);
#endif /* EGUI_CONFIG_FUNCTION_LABEL_RECOLOR */

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_LABEL_H_ */

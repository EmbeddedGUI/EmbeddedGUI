#ifndef _EGUI_VIEW_BUTTON_H_
#define _EGUI_VIEW_BUTTON_H_

#include "egui_view_label.h"
#include "font/egui_font.h"

/**
 * @brief Clickable text button built on top of the label widget.
 *
 * The button reuses label text, font, alignment, and line-spacing behavior,
 * then adds a themed rounded background plus an optional icon string placed
 * before the label text. This makes it the base pattern for many other
 * button-like widgets in the library.
 */

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_button egui_view_button_t;
struct egui_view_button
{
    /* Base label that stores the main text, alignment, and text style. */
    egui_view_label_t base;

    /* Optional borrowed icon string rendered before the main label text. */
    const char *icon;
    /* Optional override font used for the icon string. */
    const egui_font_t *icon_font;
    /* Horizontal spacing between the icon string and main text. */
    egui_dim_t icon_text_gap;
};

// ============== Button Params (reuse Label) ==============
/** Reuse the full label parameter block when constructing a button. */
#define EGUI_VIEW_BUTTON_PARAMS_INIT EGUI_VIEW_LABEL_PARAMS_INIT
/** Build a centered white-text button parameter block with the default font. */
#define EGUI_VIEW_BUTTON_PARAMS_INIT_SIMPLE(_name, _x, _y, _w, _h, _text)                                                                                      \
    static const egui_view_label_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}},                                                                     \
                                                   .align_type = EGUI_ALIGN_CENTER,                                                                            \
                                                   .text = (_text),                                                                                            \
                                                   .font = NULL,                                                                                               \
                                                   .color = EGUI_COLOR_WHITE,                                                                                  \
                                                   .alpha = EGUI_ALPHA_100}

/** Apply label-style parameter fields to a button. */
void egui_view_button_apply_params(egui_view_t *self, const egui_view_label_params_t *params);
/** Initialize a clickable button with default button styling. */
void egui_view_button_init(egui_view_t *self, egui_core_t *core);
/** Initialize a button and immediately apply its label-style parameter block. */
void egui_view_button_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_label_params_t *params);
/** Set the icon text rendered before the button label text. */
void egui_view_button_set_icon(egui_view_t *self, const char *icon);
/** Override the font used to draw the icon text. */
void egui_view_button_set_icon_font(egui_view_t *self, const egui_font_t *font);
/** Set the horizontal gap between icon and label text. */
void egui_view_button_set_icon_text_gap(egui_view_t *self, egui_dim_t gap);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_BUTTON_H_ */

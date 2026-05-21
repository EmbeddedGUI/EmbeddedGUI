#ifndef _EGUI_VIEW_CHECKBOX_H_
#define _EGUI_VIEW_CHECKBOX_H_

#include "egui_view.h"
#include "egui_view_switch.h"

/**
 * @brief Checkbox widget with an optional label and square checked indicator.
 *
 * The checkbox owns one boolean checked state, can render either a vector
 * check mark or an icon-font glyph, and optionally shows label text to the
 * right of the indicator.
 */

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_checkbox egui_view_checkbox_t;

typedef enum
{
    EGUI_VIEW_CHECKBOX_MARK_STYLE_VECTOR = 0, /* Draw the check mark with vector lines. */
    EGUI_VIEW_CHECKBOX_MARK_STYLE_ICON = 1,   /* Draw the check mark with an icon font glyph. */
} egui_view_checkbox_mark_style_t;

/** Checkbox with an optional text label and a square checked indicator. */
struct egui_view_checkbox
{
    egui_view_t base;

    egui_view_on_checked_listener_t on_checked_changed; /* Notified when the checked state changes. */

    uint8_t is_checked;           /* Current logical checked state. */
    egui_alpha_t alpha;           /* Shared alpha for indicator and label. */
    egui_color_t box_color;       /* Outline color when unchecked. */
    egui_color_t check_color;     /* Check mark color when checked. */
    egui_color_t box_fill_color;  /* Filled indicator color when checked. */
    const char *text;             /* Optional label rendered to the right of the box. */
    const egui_font_t *font;      /* Optional label font override. */
    egui_color_t text_color;      /* Label color while enabled. */
    egui_dim_t text_gap;          /* Spacing between the indicator box and the label. */
    uint8_t mark_style;           /* One of egui_view_checkbox_mark_style_t. */
    const char *mark_icon;        /* Icon glyph used when mark_style selects icon rendering. */
    const egui_font_t *icon_font; /* Optional icon font override for the checked mark. */
};

// ============== Checkbox Params ==============
typedef struct egui_view_checkbox_params egui_view_checkbox_params_t;
/** Construction-time parameters for a checkbox. */
struct egui_view_checkbox_params
{
    egui_region_t region;
    uint8_t is_checked;
    const char *text;
};

/** Build a checkbox parameter block with region and initial checked state. */
#define EGUI_VIEW_CHECKBOX_PARAMS_INIT(_name, _x, _y, _w, _h, _checked)                                                                                        \
    static const egui_view_checkbox_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .is_checked = (_checked), .text = NULL}

/** Build a checkbox parameter block with region, initial checked state, and label text. */
#define EGUI_VIEW_CHECKBOX_PARAMS_INIT_WITH_TEXT(_name, _x, _y, _w, _h, _checked, _text)                                                                       \
    static const egui_view_checkbox_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .is_checked = (_checked), .text = (_text)}

/** Apply a checkbox parameter block after initialization. */
void egui_view_checkbox_apply_params(egui_view_t *self, const egui_view_checkbox_params_t *params);
/** Initialize a checkbox and immediately apply its parameter block. */
void egui_view_checkbox_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_checkbox_params_t *params);

/** Register the callback fired when the checked state changes. */
void egui_view_checkbox_set_on_checked_listener(egui_view_t *self, egui_view_on_checked_listener_t listener);
/** Return the callback fired when the checked state changes. */
egui_view_on_checked_listener_t egui_view_checkbox_get_on_checked_listener(egui_view_t *self);
/** Set the checked state programmatically. */
void egui_view_checkbox_set_checked(egui_view_t *self, uint8_t is_checked);
/** Return the optional label text pointer, or NULL when unset or self is NULL. */
const char *egui_view_checkbox_get_text(egui_view_t *self);
/** Return the current checked state (1 = checked, 0 = unchecked). */
uint8_t egui_view_checkbox_get_checked(egui_view_t *self);
/** Set the optional label text shown to the right of the box. */
void egui_view_checkbox_set_text(egui_view_t *self, const char *text);
/** Override the font used for the optional label text. */
void egui_view_checkbox_set_font(egui_view_t *self, const egui_font_t *font);
/** Return the optional label font override, or NULL when default font is used. */
const egui_font_t *egui_view_checkbox_get_font(egui_view_t *self);
/** Set the color used for the optional label text. */
void egui_view_checkbox_set_text_color(egui_view_t *self, egui_color_t color);
/** Return the color used for the optional label text. Returns zeroed color when self is NULL. */
egui_color_t egui_view_checkbox_get_text_color(egui_view_t *self);
/** Return the outline color used when unchecked. */
egui_color_t egui_view_checkbox_get_box_color(egui_view_t *self);
/** Return the check mark color shown when checked. */
egui_color_t egui_view_checkbox_get_check_color(egui_view_t *self);
/** Return the fill color of the indicator box when checked. */
egui_color_t egui_view_checkbox_get_box_fill_color(egui_view_t *self);
/** Return the shared alpha of the checkbox indicator and label. */
egui_alpha_t egui_view_checkbox_get_alpha(egui_view_t *self);
/** Return the horizontal gap between the indicator box and the label. */
egui_dim_t egui_view_checkbox_get_text_gap(egui_view_t *self);
/** Choose a vector check mark or an icon-font check mark. */
void egui_view_checkbox_set_mark_style(egui_view_t *self, egui_view_checkbox_mark_style_t style);
/** Return the configured check mark rendering style. */
egui_view_checkbox_mark_style_t egui_view_checkbox_get_mark_style(egui_view_t *self);
/** Set the icon glyph used when the mark style is `ICON`. */
void egui_view_checkbox_set_mark_icon(egui_view_t *self, const char *icon);
/** Return the icon glyph used when the mark style is `ICON`. */
const char *egui_view_checkbox_get_mark_icon(egui_view_t *self);
/** Override the font used for the icon-style check mark. */
void egui_view_checkbox_set_icon_font(egui_view_t *self, const egui_font_t *font);
/** Return the optional icon font override, or NULL when automatic icon font selection is used. */
const egui_font_t *egui_view_checkbox_get_icon_font(egui_view_t *self);
/** Set the horizontal gap between the indicator box and label text. */
void egui_view_checkbox_set_icon_text_gap(egui_view_t *self, egui_dim_t gap);
/** Return the horizontal gap between the indicator box and label text. */
egui_dim_t egui_view_checkbox_get_icon_text_gap(egui_view_t *self);
/** Default draw hook used by the checkbox API table. */
void egui_view_checkbox_on_draw(egui_view_t *self);
/** Initialize the checkbox widget with clickable toggle behavior. */
void egui_view_checkbox_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_CHECKBOX_H_ */

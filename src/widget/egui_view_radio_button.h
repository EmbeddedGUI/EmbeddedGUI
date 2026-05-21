#ifndef _EGUI_VIEW_RADIO_BUTTON_H_
#define _EGUI_VIEW_RADIO_BUTTON_H_

#include "egui_view.h"
#include "utils/egui_slist.h"

/**
 * @brief Radio-button widget plus lightweight group helper for mutual exclusion.
 *
 * Each radio button owns one checked state and optional label text. When
 * several buttons are attached to the same `egui_view_radio_group_t`, checking
 * one button automatically clears its peers.
 */

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Listener fired when a radio group selects a new button index. */
typedef void (*egui_view_on_radio_changed_listener_t)(egui_view_t *self, int index);

// ============== Radio Group ==============
typedef struct egui_view_radio_group egui_view_radio_group_t;
/** Lightweight group container that enforces single selection across member radio buttons. */
struct egui_view_radio_group
{
    egui_slist_t buttons;                             /* Member buttons in append order. */
    egui_view_on_radio_changed_listener_t on_changed; /* Group-level callback for the selected index. */
};

/** Initialize a radio group before adding any buttons. */
void egui_view_radio_group_init(egui_view_radio_group_t *group);
/** Add a radio button to the group. The append order defines the callback index. */
void egui_view_radio_group_add(egui_view_radio_group_t *group, egui_view_t *button);
/** Register the callback fired when a grouped button becomes checked. */
void egui_view_radio_group_set_on_changed_listener(egui_view_radio_group_t *group, egui_view_on_radio_changed_listener_t listener);
/** Return the callback fired when a grouped button becomes checked. */
egui_view_on_radio_changed_listener_t egui_view_radio_group_get_on_changed_listener(egui_view_radio_group_t *group);

// ============== Radio Button ==============
typedef struct egui_view_radio_button egui_view_radio_button_t;

typedef enum
{
    EGUI_VIEW_RADIO_BUTTON_MARK_STYLE_DOT = 0,  /* Draw the default filled inner dot. */
    EGUI_VIEW_RADIO_BUTTON_MARK_STYLE_ICON = 1, /* Draw an icon font glyph inside the ring. */
} egui_view_radio_button_mark_style_t;

/** Radio button with optional label text and optional membership in a mutual-exclusion group. */
struct egui_view_radio_button
{
    egui_view_t base;

    egui_snode_t group_node;        /* Link node used when the button joins a radio group. */
    egui_view_radio_group_t *group; /* Owning radio group, or NULL for standalone use. */

    uint8_t is_checked;           /* Current logical selection state. */
    egui_alpha_t alpha;           /* Shared alpha for ring, mark, and label. */
    egui_color_t circle_color;    /* Ring color while unchecked. */
    egui_color_t dot_color;       /* Inner mark color while checked. */
    const char *text;             /* Optional label rendered to the right of the ring. */
    const egui_font_t *font;      /* Optional label font override. */
    egui_color_t text_color;      /* Label color while enabled. */
    egui_dim_t text_gap;          /* Spacing between the ring and the label. */
    uint8_t mark_style;           /* One of egui_view_radio_button_mark_style_t. */
    const char *mark_icon;        /* Icon glyph used when mark_style selects icon rendering. */
    const egui_font_t *icon_font; /* Optional icon font override for the checked mark. */
};

// ============== Radio Button Params ==============
typedef struct egui_view_radio_button_params egui_view_radio_button_params_t;
/** Construction-time parameters for a radio button. */
struct egui_view_radio_button_params
{
    egui_region_t region;
    uint8_t is_checked;
    const char *text;
};

/** Build a radio-button parameter block with region and initial checked state. */
#define EGUI_VIEW_RADIO_BUTTON_PARAMS_INIT(_name, _x, _y, _w, _h, _checked)                                                                                    \
    static const egui_view_radio_button_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .is_checked = (_checked), .text = NULL}

/** Build a radio-button parameter block with region, initial checked state, and label text. */
#define EGUI_VIEW_RADIO_BUTTON_PARAMS_INIT_WITH_TEXT(_name, _x, _y, _w, _h, _checked, _text)                                                                   \
    static const egui_view_radio_button_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .is_checked = (_checked), .text = (_text)}

/** Apply a radio-button parameter block after initialization. */
void egui_view_radio_button_apply_params(egui_view_t *self, const egui_view_radio_button_params_t *params);
/** Initialize a radio button and immediately apply its parameter block. */
void egui_view_radio_button_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_radio_button_params_t *params);

/** Set the checked state. Checking a grouped button automatically clears its peers and notifies the group listener. */
void egui_view_radio_button_set_checked(egui_view_t *self, uint8_t is_checked);
/** Return the current checked state. */
uint8_t egui_view_radio_button_get_checked(egui_view_t *self);
/** Return the radio group this button belongs to, or NULL for standalone buttons. */
egui_view_radio_group_t *egui_view_radio_button_get_group(egui_view_t *self);
/** Set the optional label text shown to the right of the indicator. */
void egui_view_radio_button_set_text(egui_view_t *self, const char *text);
/** Return the optional label text pointer. */
const char *egui_view_radio_button_get_text(egui_view_t *self);
/** Override the font used for the optional label text. */
void egui_view_radio_button_set_font(egui_view_t *self, const egui_font_t *font);
/** Return the optional label font override, or NULL when default font is used. */
const egui_font_t *egui_view_radio_button_get_font(egui_view_t *self);
/** Set the color used for the optional label text. */
void egui_view_radio_button_set_text_color(egui_view_t *self, egui_color_t color);
/** Return the color used for the optional label text. Returns zeroed color when self is NULL. */
egui_color_t egui_view_radio_button_get_text_color(egui_view_t *self);
/** Choose between the default filled-dot mark and an icon-font mark. */
void egui_view_radio_button_set_mark_style(egui_view_t *self, egui_view_radio_button_mark_style_t style);
/** Return the configured checked mark rendering style. */
egui_view_radio_button_mark_style_t egui_view_radio_button_get_mark_style(egui_view_t *self);
/** Set the icon glyph used when the mark style is `ICON`. */
void egui_view_radio_button_set_mark_icon(egui_view_t *self, const char *icon);
/** Return the icon glyph used when the mark style is `ICON`. */
const char *egui_view_radio_button_get_mark_icon(egui_view_t *self);
/** Override the font used for the icon-style checked mark. */
void egui_view_radio_button_set_icon_font(egui_view_t *self, const egui_font_t *font);
/** Return the optional icon font override, or NULL when automatic icon font selection is used. */
const egui_font_t *egui_view_radio_button_get_icon_font(egui_view_t *self);
/** Set the horizontal gap between the indicator circle and label text. */
void egui_view_radio_button_set_icon_text_gap(egui_view_t *self, egui_dim_t gap);
/** Return the horizontal gap between the indicator circle and label text. */
egui_dim_t egui_view_radio_button_get_icon_text_gap(egui_view_t *self);
/** Default draw hook used by the radio-button API table. */
void egui_view_radio_button_on_draw(egui_view_t *self);
/** Initialize a clickable radio button widget. */
void egui_view_radio_button_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_RADIO_BUTTON_H_ */

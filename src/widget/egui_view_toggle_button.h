#ifndef _EGUI_VIEW_TOGGLE_BUTTON_H_
#define _EGUI_VIEW_TOGGLE_BUTTON_H_

#include "egui_view.h"
#include "font/egui_font.h"

/**
 * @brief Push-button style control whose whole face toggles between on and off states.
 *
 * Unlike a switch or checkbox, the entire button body is both the visual surface
 * and the interaction target. The widget stores only borrowed content pointers,
 * simple on or off state, and colors for the two background states.
 */

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Listener fired after the toggle state changes. */
typedef void (*egui_view_on_toggled_listener_t)(egui_view_t *self, uint8_t is_toggled);

/** Push-button style control whose whole face switches between on and off states. */
typedef struct egui_view_toggle_button egui_view_toggle_button_t;
struct egui_view_toggle_button
{
    egui_view_t base;

    egui_view_on_toggled_listener_t on_toggled; /* Notified when the toggle state changes. */
    uint8_t is_toggled;                         /* Current on/off state. */
    const char *icon;                           /* Optional icon shown before or instead of text. */
    const char *text;                           /* Optional button label. */
    const egui_font_t *font;                    /* Optional label font override. */
    const egui_font_t *icon_font;               /* Optional icon font override. */
    egui_color_t text_color;                    /* Content color while toggled on. */
    egui_color_t on_color;                      /* Background color when toggled on. */
    egui_color_t off_color;                     /* Background color when toggled off. */
    egui_dim_t corner_radius;                   /* Rounded corner radius for the button frame. */
    egui_dim_t icon_text_gap;                   /* Spacing between icon and label when both exist. */
};

// ============== ToggleButton Params ==============
typedef struct egui_view_toggle_button_params egui_view_toggle_button_params_t;
/** Construction-time parameters for a toggle button. */
struct egui_view_toggle_button_params
{
    /* Outer region occupied by the toggle button. */
    egui_region_t region;
    /* Borrowed label text shown by default. */
    const char *text;
    /* Initial on or off state applied during setup. */
    uint8_t is_toggled;
};

/** Build a toggle-button parameter block with region, text, and initial state. */
#define EGUI_VIEW_TOGGLE_BUTTON_PARAMS_INIT(_name, _x, _y, _w, _h, _text, _toggled)                                                                            \
    static const egui_view_toggle_button_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .text = (_text), .is_toggled = (_toggled)}

/** Apply a toggle-button parameter block after initialization. */
void egui_view_toggle_button_apply_params(egui_view_t *self, const egui_view_toggle_button_params_t *params);
/** Initialize a toggle button and immediately apply its parameter block. */
void egui_view_toggle_button_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_toggle_button_params_t *params);

/** Register the callback fired when the toggle state changes. */
void egui_view_toggle_button_set_on_toggled_listener(egui_view_t *self, egui_view_on_toggled_listener_t listener);
/** Return the callback fired when the toggle state changes. */
egui_view_on_toggled_listener_t egui_view_toggle_button_get_on_toggled_listener(egui_view_t *self);
/** Set the toggle state programmatically. This also fires the toggle listener on real changes. */
void egui_view_toggle_button_set_toggled(egui_view_t *self, uint8_t is_toggled);
/** Return whether the button is currently toggled on. */
uint8_t egui_view_toggle_button_get_toggled(egui_view_t *self);
/** Return whether the button is currently toggled on. */
uint8_t egui_view_toggle_button_is_toggled(egui_view_t *self);
/** Set the optional icon shown before or instead of the button text. */
void egui_view_toggle_button_set_icon(egui_view_t *self, const char *icon);
/** Return the optional icon pointer shown by the button. */
const char *egui_view_toggle_button_get_icon(egui_view_t *self);
/** Set the button text. */
void egui_view_toggle_button_set_text(egui_view_t *self, const char *text);
/** Return the current button text pointer. */
const char *egui_view_toggle_button_get_text(egui_view_t *self);
/** Override the font used for the button text. */
void egui_view_toggle_button_set_font(egui_view_t *self, const egui_font_t *font);
/** Return the text font override, or NULL when default font is used. */
const egui_font_t *egui_view_toggle_button_get_font(egui_view_t *self);
/** Override the icon font used by the optional icon. */
void egui_view_toggle_button_set_icon_font(egui_view_t *self, const egui_font_t *font);
/** Return the icon font override, or NULL when automatic icon-font resolution is used. */
const egui_font_t *egui_view_toggle_button_get_icon_font(egui_view_t *self);
/** Set the horizontal gap between icon and text. */
void egui_view_toggle_button_set_icon_text_gap(egui_view_t *self, egui_dim_t gap);
/** Return the horizontal gap between icon and text. */
egui_dim_t egui_view_toggle_button_get_icon_text_gap(egui_view_t *self);
/** Set the content color used when the button is toggled on. */
void egui_view_toggle_button_set_text_color(egui_view_t *self, egui_color_t color);
/** Return the content color used when the button is toggled on. */
egui_color_t egui_view_toggle_button_get_text_color(egui_view_t *self);
/** Default draw hook used by the toggle-button API table. */
void egui_view_toggle_button_on_draw(egui_view_t *self);
/** Initialize the tap-to-toggle button widget. */
void egui_view_toggle_button_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_TOGGLE_BUTTON_H_ */

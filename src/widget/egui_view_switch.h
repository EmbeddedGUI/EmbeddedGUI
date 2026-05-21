#ifndef _EGUI_VIEW_SWITCH_H_
#define _EGUI_VIEW_SWITCH_H_

#include "egui_view.h"
#include "font/egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Listener fired after the switch checked state changes. */
typedef void (*egui_view_on_checked_listener_t)(egui_view_t *self, int is_checked);

typedef struct egui_view_switch egui_view_switch_t;
/**
 * @brief Clickable on or off switch with optional icon glyphs in the thumb.
 *
 * The widget stores only checked state and presentation fields. Clicking uses
 * the normal view click pipeline, then flips `is_checked` in `perform_click`.
 */
struct egui_view_switch
{
    egui_view_t base;

    egui_view_on_checked_listener_t on_checked_changed;

    uint8_t is_checked;
    egui_alpha_t alpha;
    egui_color_t switch_color_on;
    egui_color_t switch_color_off;
    egui_color_t bk_color_on;
    egui_color_t bk_color_off;
    const char *icon_on;
    const char *icon_off;
    const egui_font_t *icon_font;
};

// ============== Switch Params ==============
typedef struct egui_view_switch_params egui_view_switch_params_t;
/**
 * @brief Construction-time parameter block for one switch widget.
 */
struct egui_view_switch_params
{
    egui_region_t region;
    uint8_t is_checked;
};

/** Build a switch parameter block with region and initial checked state. */
#define EGUI_VIEW_SWITCH_PARAMS_INIT(_name, _x, _y, _w, _h, _checked)                                                                                          \
    static const egui_view_switch_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .is_checked = (_checked)}

/** Apply a switch parameter block after initialization. */
void egui_view_switch_apply_params(egui_view_t *self, const egui_view_switch_params_t *params);
/** Initialize a switch and immediately apply its parameter block. */
void egui_view_switch_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_switch_params_t *params);

/** Register the callback fired when the checked state changes. */
void egui_view_switch_set_on_checked_listener(egui_view_t *self, egui_view_on_checked_listener_t listener);
/** Return the callback fired when the checked state changes. */
egui_view_on_checked_listener_t egui_view_switch_get_on_checked_listener(egui_view_t *self);
/** Set the checked state programmatically. This also fires the change listener on real changes. */
void egui_view_switch_set_checked(egui_view_t *self, uint8_t is_checked);
/** Return the current checked state (1 = on, 0 = off). */
uint8_t egui_view_switch_get_checked(egui_view_t *self);
/** Return the track (background capsule) color in the on state. */
egui_color_t egui_view_switch_get_bk_color_on(egui_view_t *self);
/** Return the track (background capsule) color in the off state. */
egui_color_t egui_view_switch_get_bk_color_off(egui_view_t *self);
/** Return the thumb knob color in the on state. */
egui_color_t egui_view_switch_get_switch_color_on(egui_view_t *self);
/** Return the thumb knob color in the off state. */
egui_color_t egui_view_switch_get_switch_color_off(egui_view_t *self);
/** Return the shared alpha of the switch widget. */
egui_alpha_t egui_view_switch_get_alpha(egui_view_t *self);
/** Set optional icon glyphs drawn inside the thumb for the on/off states. */
void egui_view_switch_set_state_icons(egui_view_t *self, const char *icon_on, const char *icon_off);
/** Return the optional icon glyph drawn in the on state. */
const char *egui_view_switch_get_icon_on(egui_view_t *self);
/** Return the optional icon glyph drawn in the off state. */
const char *egui_view_switch_get_icon_off(egui_view_t *self);
/** Override the icon font used for the optional state glyphs. */
void egui_view_switch_set_icon_font(egui_view_t *self, const egui_font_t *font);
/** Return the optional icon font override. */
const egui_font_t *egui_view_switch_get_icon_font(egui_view_t *self);
/** Default draw hook used by the switch API table. */
void egui_view_switch_on_draw(egui_view_t *self);
/** Initialize the switch widget with clickable toggle behavior. */
void egui_view_switch_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_SWITCH_H_ */

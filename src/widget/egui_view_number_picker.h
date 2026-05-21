#ifndef _EGUI_VIEW_NUMBER_PICKER_H_
#define _EGUI_VIEW_NUMBER_PICKER_H_

#include "egui_view.h"
#include "font/egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Listener fired after the picker value changes. */
typedef void (*egui_view_on_number_changed_listener_t)(egui_view_t *self, int16_t value);

/** Three-zone number picker with increment button, value label, and decrement button. */
typedef struct egui_view_number_picker egui_view_number_picker_t;
struct egui_view_number_picker
{
    egui_view_t base;

    egui_view_on_number_changed_listener_t on_value_changed; /* Notified after a real value change. */
    int16_t value;                                           /* Current displayed value. */
    int16_t min_value;                                       /* Lower clamp bound. */
    int16_t max_value;                                       /* Upper clamp bound. */
    int16_t step;                                            /* Amount added or subtracted per button press. */
    egui_alpha_t alpha;                                      /* Shared text/icon alpha. */
    egui_color_t text_color;                                 /* Middle value text color. */
    egui_color_t button_color;                               /* Divider and arrow icon color. */
    const egui_font_t *font;                                 /* Font for the numeric value. */
    const char *icon_inc;                                    /* Glyph drawn in the top increment zone. */
    const char *icon_dec;                                    /* Glyph drawn in the bottom decrement zone. */
    const egui_font_t *icon_font;                            /* Optional icon font override, auto-picked when NULL. */
    char text_buf[8];                                        /* Small formatting buffer for the current value string. */
    int8_t pressed_zone;                                     /* 0=none, 1=top zone pressed, -1=bottom zone pressed. */
};

// ============== NumberPicker Params ==============
typedef struct egui_view_number_picker_params egui_view_number_picker_params_t;
/** Construction-time parameters for a number picker. */
struct egui_view_number_picker_params
{
    egui_region_t region;
    int16_t value;
    int16_t min_value;
    int16_t max_value;
};

#define EGUI_VIEW_NUMBER_PICKER_PARAMS_INIT(_name, _x, _y, _w, _h, _val, _min, _max)                                                                           \
    static const egui_view_number_picker_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .value = (_val), .min_value = (_min), .max_value = (_max)}

/** Apply a number-picker parameter block after initialization. */
void egui_view_number_picker_apply_params(egui_view_t *self, const egui_view_number_picker_params_t *params);
/** Initialize a number picker and immediately apply its parameter block. */
void egui_view_number_picker_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_number_picker_params_t *params);

/** Register the callback fired when the current value changes. */
void egui_view_number_picker_set_on_value_changed_listener(egui_view_t *self, egui_view_on_number_changed_listener_t listener);
/** Return the callback fired when the current value changes. */
egui_view_on_number_changed_listener_t egui_view_number_picker_get_on_value_changed_listener(egui_view_t *self);
/** Set the current value. The value is clamped to the active min/max range and notifies the change listener on real changes. */
void egui_view_number_picker_set_value(egui_view_t *self, int16_t value);
/** Return the current picker value. */
int16_t egui_view_number_picker_get_value(egui_view_t *self);
/** Update the allowed range and clamp the current value if needed. */
void egui_view_number_picker_set_range(egui_view_t *self, int16_t min_value, int16_t max_value);
/** Return the configured minimum value. */
int16_t egui_view_number_picker_get_min_value(egui_view_t *self);
/** Return the configured maximum value. */
int16_t egui_view_number_picker_get_max_value(egui_view_t *self);
/** Set the increment/decrement step used by the top and bottom buttons. */
void egui_view_number_picker_set_step(egui_view_t *self, int16_t step);
/** Return the configured step. */
int16_t egui_view_number_picker_get_step(egui_view_t *self);
/** Return the value text color. */
egui_color_t egui_view_number_picker_get_text_color(egui_view_t *self);
/** Return the button divider and arrow icon color. */
egui_color_t egui_view_number_picker_get_button_color(egui_view_t *self);
/** Return the shared alpha value. */
egui_alpha_t egui_view_number_picker_get_alpha(egui_view_t *self);
/** Override the icon glyphs shown in the increment and decrement areas. */
void egui_view_number_picker_set_button_icons(egui_view_t *self, const char *icon_inc, const char *icon_dec);
/** Return the icon glyph shown in the increment area. */
const char *egui_view_number_picker_get_button_icon_inc(egui_view_t *self);
/** Return the icon glyph shown in the decrement area. */
const char *egui_view_number_picker_get_button_icon_dec(egui_view_t *self);
/** Override the icon font used for the increment and decrement glyphs. */
void egui_view_number_picker_set_icon_font(egui_view_t *self, const egui_font_t *font);
/** Return the explicit icon font override, or NULL when auto icon font selection is used. */
const egui_font_t *egui_view_number_picker_get_icon_font(egui_view_t *self);
/** Default draw hook used by the number-picker API table. */
void egui_view_number_picker_on_draw(egui_view_t *self);
/** Initialize the three-zone number-picker widget. */
void egui_view_number_picker_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_NUMBER_PICKER_H_ */

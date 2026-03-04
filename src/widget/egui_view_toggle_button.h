#ifndef _EGUI_VIEW_TOGGLE_BUTTON_H_
#define _EGUI_VIEW_TOGGLE_BUTTON_H_

#include "egui_view.h"
#include "core/egui_theme.h"
#include "font/egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*egui_view_on_toggled_listener_t)(egui_view_t *self, uint8_t is_toggled);

typedef struct egui_view_toggle_button egui_view_toggle_button_t;
struct egui_view_toggle_button
{
    egui_view_t base;

    egui_view_on_toggled_listener_t on_toggled;
    uint8_t is_toggled;
    const char *text;
    const egui_font_t *font;
    egui_color_t text_color;
    egui_color_t on_color;
    egui_color_t off_color;
    egui_dim_t corner_radius;
};

// ============== ToggleButton Params ==============
typedef struct egui_view_toggle_button_params egui_view_toggle_button_params_t;
struct egui_view_toggle_button_params
{
    egui_region_t region;
    const char *text;
    uint8_t is_toggled;
};

#define EGUI_VIEW_TOGGLE_BUTTON_PARAMS_INIT(_name, _x, _y, _w, _h, _text, _toggled)                                                                            \
    static const egui_view_toggle_button_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .text = (_text), .is_toggled = (_toggled)}

void egui_view_toggle_button_apply_params(egui_view_t *self, const egui_view_toggle_button_params_t *params);
void egui_view_toggle_button_init_with_params(egui_view_t *self, const egui_view_toggle_button_params_t *params);

void egui_view_toggle_button_set_on_toggled_listener(egui_view_t *self, egui_view_on_toggled_listener_t listener);
void egui_view_toggle_button_set_toggled(egui_view_t *self, uint8_t is_toggled);
uint8_t egui_view_toggle_button_is_toggled(egui_view_t *self);
void egui_view_toggle_button_set_text(egui_view_t *self, const char *text);
void egui_view_toggle_button_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_toggle_button_set_text_color(egui_view_t *self, egui_color_t color);
void egui_view_toggle_button_on_draw(egui_view_t *self);
void egui_view_toggle_button_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_TOGGLE_BUTTON_H_ */

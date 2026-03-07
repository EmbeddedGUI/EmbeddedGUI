#ifndef _EGUI_VIEW_CHECKBOX_H_
#define _EGUI_VIEW_CHECKBOX_H_

#include "egui_view.h"
#include "egui_view_switch.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_checkbox egui_view_checkbox_t;
struct egui_view_checkbox
{
    egui_view_t base;

    egui_view_on_checked_listener_t on_checked_changed;

    uint8_t is_checked;
    egui_alpha_t alpha;
    egui_color_t box_color;
    egui_color_t check_color;
    egui_color_t box_fill_color;
    const char *text;
    const egui_font_t *font;
    egui_color_t text_color;
    egui_dim_t text_gap;
};

// ============== Checkbox Params ==============
typedef struct egui_view_checkbox_params egui_view_checkbox_params_t;
struct egui_view_checkbox_params
{
    egui_region_t region;
    uint8_t is_checked;
    const char *text;
};

#define EGUI_VIEW_CHECKBOX_PARAMS_INIT(_name, _x, _y, _w, _h, _checked)                                                                                        \
    static const egui_view_checkbox_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .is_checked = (_checked), .text = NULL}

#define EGUI_VIEW_CHECKBOX_PARAMS_INIT_WITH_TEXT(_name, _x, _y, _w, _h, _checked, _text)                                                                       \
    static const egui_view_checkbox_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .is_checked = (_checked), .text = (_text)}

void egui_view_checkbox_apply_params(egui_view_t *self, const egui_view_checkbox_params_t *params);
void egui_view_checkbox_init_with_params(egui_view_t *self, const egui_view_checkbox_params_t *params);

void egui_view_checkbox_set_on_checked_listener(egui_view_t *self, egui_view_on_checked_listener_t listener);
void egui_view_checkbox_set_checked(egui_view_t *self, uint8_t is_checked);
void egui_view_checkbox_set_text(egui_view_t *self, const char *text);
void egui_view_checkbox_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_checkbox_set_text_color(egui_view_t *self, egui_color_t color);
void egui_view_checkbox_on_draw(egui_view_t *self);
void egui_view_checkbox_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_CHECKBOX_H_ */

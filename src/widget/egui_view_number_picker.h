#ifndef _EGUI_VIEW_NUMBER_PICKER_H_
#define _EGUI_VIEW_NUMBER_PICKER_H_

#include "egui_view.h"
#include "core/egui_theme.h"
#include "font/egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*egui_view_on_number_changed_listener_t)(egui_view_t *self, int16_t value);

typedef struct egui_view_number_picker egui_view_number_picker_t;
struct egui_view_number_picker
{
    egui_view_t base;

    egui_view_on_number_changed_listener_t on_value_changed;
    int16_t value;
    int16_t min_value;
    int16_t max_value;
    int16_t step;
    egui_alpha_t alpha;
    egui_color_t text_color;
    egui_color_t button_color;
    const egui_font_t *font;
    char text_buf[8];
    int8_t pressed_zone; /* 0=none, 1=top zone pressed, -1=bottom zone pressed */
};

// ============== NumberPicker Params ==============
typedef struct egui_view_number_picker_params egui_view_number_picker_params_t;
struct egui_view_number_picker_params
{
    egui_region_t region;
    int16_t value;
    int16_t min_value;
    int16_t max_value;
};

#define EGUI_VIEW_NUMBER_PICKER_PARAMS_INIT(_name, _x, _y, _w, _h, _val, _min, _max)                                                                           \
    static const egui_view_number_picker_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .value = (_val), .min_value = (_min), .max_value = (_max)}

void egui_view_number_picker_apply_params(egui_view_t *self, const egui_view_number_picker_params_t *params);
void egui_view_number_picker_init_with_params(egui_view_t *self, const egui_view_number_picker_params_t *params);

void egui_view_number_picker_set_on_value_changed_listener(egui_view_t *self, egui_view_on_number_changed_listener_t listener);
void egui_view_number_picker_set_value(egui_view_t *self, int16_t value);
int16_t egui_view_number_picker_get_value(egui_view_t *self);
void egui_view_number_picker_set_range(egui_view_t *self, int16_t min_value, int16_t max_value);
void egui_view_number_picker_set_step(egui_view_t *self, int16_t step);
void egui_view_number_picker_on_draw(egui_view_t *self);
void egui_view_number_picker_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_NUMBER_PICKER_H_ */

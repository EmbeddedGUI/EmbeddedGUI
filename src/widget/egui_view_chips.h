#ifndef _EGUI_VIEW_CHIPS_H_
#define _EGUI_VIEW_CHIPS_H_

#include "egui_view_button_matrix.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef egui_view_button_matrix_click_cb_t egui_view_on_chip_selected_listener_t;

typedef struct egui_view_chips egui_view_chips_t;
struct egui_view_chips
{
    egui_view_button_matrix_t matrix;
};

typedef struct egui_view_chips_params egui_view_chips_params_t;
struct egui_view_chips_params
{
    egui_region_t region;
    const char **labels;
    uint8_t chip_count;
    uint8_t cols;
    uint8_t gap;
};

#define EGUI_VIEW_CHIPS_PARAMS_INIT(_name, _x, _y, _w, _h, _labels, _count, _cols, _gap)                                                                       \
    static const egui_view_chips_params_t _name = {                                                                                                            \
            .region = {{(_x), (_y)}, {(_w), (_h)}}, .labels = (_labels), .chip_count = (_count), .cols = (_cols), .gap = (_gap)}

void egui_view_chips_apply_params(egui_view_t *self, const egui_view_chips_params_t *params);
void egui_view_chips_init_with_params(egui_view_t *self, const egui_view_chips_params_t *params);

void egui_view_chips_set_chips(egui_view_t *self, const char **labels, uint8_t count, uint8_t cols);
void egui_view_chips_set_on_selected_listener(egui_view_t *self, egui_view_on_chip_selected_listener_t listener);
void egui_view_chips_set_gap(egui_view_t *self, uint8_t gap);
void egui_view_chips_set_selected_index(egui_view_t *self, uint8_t index);
void egui_view_chips_clear_selection(egui_view_t *self);
uint8_t egui_view_chips_get_selected_index(egui_view_t *self);
void egui_view_chips_set_corner_radius(egui_view_t *self, uint8_t radius);
void egui_view_chips_set_bg_color(egui_view_t *self, egui_color_t color);
void egui_view_chips_set_selected_bg_color(egui_view_t *self, egui_color_t color);
void egui_view_chips_set_text_color(egui_view_t *self, egui_color_t color);
void egui_view_chips_set_border_color(egui_view_t *self, egui_color_t color);
void egui_view_chips_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_chips_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_CHIPS_H_ */

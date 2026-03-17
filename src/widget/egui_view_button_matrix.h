#ifndef _EGUI_VIEW_BUTTON_MATRIX_H_
#define _EGUI_VIEW_BUTTON_MATRIX_H_

#include "egui_view.h"
#include "font/egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_BUTTON_MATRIX_MAX_BUTTONS   16
#define EGUI_VIEW_BUTTON_MATRIX_PRESSED_NONE  0xFF
#define EGUI_VIEW_BUTTON_MATRIX_SELECTED_NONE 0xFF

typedef void (*egui_view_button_matrix_click_cb_t)(egui_view_t *self, uint8_t btn_index);

typedef struct egui_view_button_matrix egui_view_button_matrix_t;
struct egui_view_button_matrix
{
    egui_view_t base;

    const char **labels;
    const char **icons;
    uint8_t btn_count;
    uint8_t cols;
    uint8_t gap;
    uint8_t pressed_index;
    uint8_t selected_index;
    uint8_t selection_enabled;
    egui_color_t btn_color;
    egui_color_t btn_pressed_color;
    egui_color_t text_color;
    egui_color_t border_color;
    uint8_t corner_radius;
    const egui_font_t *font;
    const egui_font_t *icon_font;
    egui_dim_t icon_text_gap;
    egui_view_button_matrix_click_cb_t on_click;
};

// ============== ButtonMatrix Params ==============
typedef struct egui_view_button_matrix_params egui_view_button_matrix_params_t;
struct egui_view_button_matrix_params
{
    egui_region_t region;
    uint8_t cols;
    uint8_t gap;
};

#define EGUI_VIEW_BUTTON_MATRIX_PARAMS_INIT(_name, _x, _y, _w, _h, _cols, _gap)                                                                                \
    static const egui_view_button_matrix_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .cols = (_cols), .gap = (_gap)}

void egui_view_button_matrix_apply_params(egui_view_t *self, const egui_view_button_matrix_params_t *params);
void egui_view_button_matrix_init_with_params(egui_view_t *self, const egui_view_button_matrix_params_t *params);

void egui_view_button_matrix_set_labels(egui_view_t *self, const char **labels, uint8_t count, uint8_t cols);
void egui_view_button_matrix_set_on_click(egui_view_t *self, egui_view_button_matrix_click_cb_t callback);
void egui_view_button_matrix_set_selection_enabled(egui_view_t *self, uint8_t enabled);
void egui_view_button_matrix_set_selected_index(egui_view_t *self, uint8_t index);
uint8_t egui_view_button_matrix_get_selected_index(egui_view_t *self);
void egui_view_button_matrix_set_btn_color(egui_view_t *self, egui_color_t color);
void egui_view_button_matrix_set_btn_pressed_color(egui_view_t *self, egui_color_t color);
void egui_view_button_matrix_set_text_color(egui_view_t *self, egui_color_t color);
void egui_view_button_matrix_set_gap(egui_view_t *self, uint8_t gap);
void egui_view_button_matrix_set_corner_radius(egui_view_t *self, uint8_t radius);
void egui_view_button_matrix_set_border_color(egui_view_t *self, egui_color_t color);
void egui_view_button_matrix_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_button_matrix_set_icons(egui_view_t *self, const char **icons);
void egui_view_button_matrix_set_icon_font(egui_view_t *self, const egui_font_t *font);
void egui_view_button_matrix_set_icon_text_gap(egui_view_t *self, egui_dim_t gap);
void egui_view_button_matrix_on_draw(egui_view_t *self);
void egui_view_button_matrix_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_BUTTON_MATRIX_H_ */

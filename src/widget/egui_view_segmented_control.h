#ifndef _EGUI_VIEW_SEGMENTED_CONTROL_H_
#define _EGUI_VIEW_SEGMENTED_CONTROL_H_

#include "egui_view.h"
#include "font/egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_SEGMENTED_CONTROL_MAX_SEGMENTS 8
#define EGUI_VIEW_SEGMENTED_CONTROL_PRESSED_NONE 0xFF

typedef void (*egui_view_on_segment_changed_listener_t)(egui_view_t *self, uint8_t index);

typedef struct egui_view_segmented_control egui_view_segmented_control_t;
struct egui_view_segmented_control
{
    egui_view_t base;

    const char **segment_texts;
    uint8_t segment_count;
    uint8_t current_index;
    uint8_t pressed_index;
    uint8_t horizontal_padding;
    uint8_t segment_gap;
    uint8_t corner_radius;
    egui_alpha_t alpha;
    egui_color_t bg_color;
    egui_color_t selected_bg_color;
    egui_color_t text_color;
    egui_color_t selected_text_color;
    egui_color_t border_color;
    const egui_font_t *font;
    egui_view_on_segment_changed_listener_t on_segment_changed;
};

typedef struct egui_view_segmented_control_params egui_view_segmented_control_params_t;
struct egui_view_segmented_control_params
{
    egui_region_t region;
    const char **segment_texts;
    uint8_t segment_count;
};

#define EGUI_VIEW_SEGMENTED_CONTROL_PARAMS_INIT(_name, _x, _y, _w, _h, _texts, _count)                                                                         \
    static const egui_view_segmented_control_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .segment_texts = (_texts), .segment_count = (_count)}

void egui_view_segmented_control_apply_params(egui_view_t *self, const egui_view_segmented_control_params_t *params);
void egui_view_segmented_control_init_with_params(egui_view_t *self, const egui_view_segmented_control_params_t *params);

void egui_view_segmented_control_set_segments(egui_view_t *self, const char **segment_texts, uint8_t segment_count);
void egui_view_segmented_control_set_current_index(egui_view_t *self, uint8_t index);
uint8_t egui_view_segmented_control_get_current_index(egui_view_t *self);
void egui_view_segmented_control_set_on_segment_changed_listener(egui_view_t *self, egui_view_on_segment_changed_listener_t listener);

void egui_view_segmented_control_set_bg_color(egui_view_t *self, egui_color_t color);
void egui_view_segmented_control_set_selected_bg_color(egui_view_t *self, egui_color_t color);
void egui_view_segmented_control_set_text_color(egui_view_t *self, egui_color_t color);
void egui_view_segmented_control_set_selected_text_color(egui_view_t *self, egui_color_t color);
void egui_view_segmented_control_set_border_color(egui_view_t *self, egui_color_t color);
void egui_view_segmented_control_set_corner_radius(egui_view_t *self, uint8_t radius);
void egui_view_segmented_control_set_segment_gap(egui_view_t *self, uint8_t gap);
void egui_view_segmented_control_set_horizontal_padding(egui_view_t *self, uint8_t padding);
void egui_view_segmented_control_set_font(egui_view_t *self, const egui_font_t *font);

void egui_view_segmented_control_on_draw(egui_view_t *self);
void egui_view_segmented_control_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_SEGMENTED_CONTROL_H_ */

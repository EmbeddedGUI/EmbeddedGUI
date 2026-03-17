#ifndef _EGUI_VIEW_NUMBER_BOX_H_
#define _EGUI_VIEW_NUMBER_BOX_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*egui_view_on_number_box_changed_listener_t)(egui_view_t *self, int16_t value);

typedef struct egui_view_number_box egui_view_number_box_t;
struct egui_view_number_box
{
    egui_view_t base;
    egui_view_on_number_box_changed_listener_t on_value_changed;
    const egui_font_t *font;
    const egui_font_t *meta_font;
    const char *label;
    const char *suffix;
    const char *helper;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t accent_color;
    int16_t value;
    int16_t min_value;
    int16_t max_value;
    int16_t step;
    uint8_t compact_mode;
    uint8_t locked_mode;
    uint8_t pressed_part;
    char value_buffer[20];
};

void egui_view_number_box_init(egui_view_t *self);
void egui_view_number_box_set_value(egui_view_t *self, int16_t value);
int16_t egui_view_number_box_get_value(egui_view_t *self);
void egui_view_number_box_set_range(egui_view_t *self, int16_t min_value, int16_t max_value);
void egui_view_number_box_set_step(egui_view_t *self, int16_t step);
void egui_view_number_box_set_label(egui_view_t *self, const char *label);
void egui_view_number_box_set_suffix(egui_view_t *self, const char *suffix);
void egui_view_number_box_set_helper(egui_view_t *self, const char *helper);
void egui_view_number_box_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_number_box_set_meta_font(egui_view_t *self, const egui_font_t *font);
void egui_view_number_box_set_on_value_changed_listener(egui_view_t *self, egui_view_on_number_box_changed_listener_t listener);
void egui_view_number_box_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_number_box_set_locked_mode(egui_view_t *self, uint8_t locked_mode);
void egui_view_number_box_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                      egui_color_t muted_text_color, egui_color_t accent_color);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_NUMBER_BOX_H_ */

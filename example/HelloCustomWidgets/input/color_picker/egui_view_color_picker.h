#ifndef _EGUI_VIEW_COLOR_PICKER_H_
#define _EGUI_VIEW_COLOR_PICKER_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_COLOR_PICKER_PART_PALETTE 0
#define EGUI_VIEW_COLOR_PICKER_PART_HUE     1
#define EGUI_VIEW_COLOR_PICKER_PART_NONE    0xFF

#define EGUI_VIEW_COLOR_PICKER_HUE_COUNT        12
#define EGUI_VIEW_COLOR_PICKER_SATURATION_COUNT 6
#define EGUI_VIEW_COLOR_PICKER_VALUE_COUNT      4

typedef void (*egui_view_on_color_picker_changed_listener_t)(egui_view_t *self, egui_color_t color, uint8_t hue_index, uint8_t saturation_index,
                                                             uint8_t value_index, uint8_t part);

typedef struct egui_view_color_picker egui_view_color_picker_t;
struct egui_view_color_picker
{
    egui_view_t base;
    const egui_font_t *font;
    const egui_font_t *meta_font;
    egui_view_on_color_picker_changed_listener_t on_changed;
    const char *label;
    const char *helper;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t accent_color;
    egui_color_t selected_color;
    uint8_t hue_index;
    uint8_t saturation_index;
    uint8_t value_index;
    uint8_t current_part;
    uint8_t pressed_part;
    uint8_t compact_mode;
    uint8_t read_only_mode;
    char hex_text[8];
};

void egui_view_color_picker_init(egui_view_t *self);
void egui_view_color_picker_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_color_picker_set_meta_font(egui_view_t *self, const egui_font_t *font);
void egui_view_color_picker_set_label(egui_view_t *self, const char *label);
void egui_view_color_picker_set_helper(egui_view_t *self, const char *helper);
void egui_view_color_picker_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                        egui_color_t muted_text_color, egui_color_t accent_color);
void egui_view_color_picker_set_selection(egui_view_t *self, uint8_t hue_index, uint8_t saturation_index, uint8_t value_index);
uint8_t egui_view_color_picker_get_hue_index(egui_view_t *self);
uint8_t egui_view_color_picker_get_saturation_index(egui_view_t *self);
uint8_t egui_view_color_picker_get_value_index(egui_view_t *self);
egui_color_t egui_view_color_picker_get_color(egui_view_t *self);
const char *egui_view_color_picker_get_hex_text(egui_view_t *self);
void egui_view_color_picker_set_current_part(egui_view_t *self, uint8_t part);
uint8_t egui_view_color_picker_get_current_part(egui_view_t *self);
void egui_view_color_picker_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_color_picker_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode);
void egui_view_color_picker_set_on_changed_listener(egui_view_t *self, egui_view_on_color_picker_changed_listener_t listener);
uint8_t egui_view_color_picker_get_part_region(egui_view_t *self, uint8_t part, egui_region_t *region);
uint8_t egui_view_color_picker_handle_navigation_key(egui_view_t *self, uint8_t key_code);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_COLOR_PICKER_H_ */

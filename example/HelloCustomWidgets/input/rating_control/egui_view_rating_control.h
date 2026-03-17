#ifndef _EGUI_VIEW_RATING_CONTROL_H_
#define _EGUI_VIEW_RATING_CONTROL_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_RATING_CONTROL_MAX_ITEMS 7

#define EGUI_VIEW_RATING_CONTROL_PART_CLEAR 0xFE
#define EGUI_VIEW_RATING_CONTROL_PART_NONE  0xFF

typedef void (*egui_view_on_rating_control_changed_listener_t)(egui_view_t *self, uint8_t value, uint8_t part);

typedef struct egui_view_rating_control egui_view_rating_control_t;
struct egui_view_rating_control
{
    egui_view_t base;
    const egui_font_t *font;
    const egui_font_t *meta_font;
    egui_view_on_rating_control_changed_listener_t on_changed;
    const char *title;
    const char *low_label;
    const char *high_label;
    const char **value_labels;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t accent_color;
    egui_color_t shadow_color;
    uint8_t label_count;
    uint8_t item_count;
    uint8_t current_value;
    uint8_t current_part;
    uint8_t compact_mode;
    uint8_t read_only_mode;
    uint8_t clear_enabled;
    uint8_t pressed_part;
};

void egui_view_rating_control_init(egui_view_t *self);
void egui_view_rating_control_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_rating_control_set_meta_font(egui_view_t *self, const egui_font_t *font);
void egui_view_rating_control_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                          egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t shadow_color);
void egui_view_rating_control_set_item_count(egui_view_t *self, uint8_t item_count);
void egui_view_rating_control_set_value(egui_view_t *self, uint8_t value);
uint8_t egui_view_rating_control_get_value(egui_view_t *self);
void egui_view_rating_control_set_current_part(egui_view_t *self, uint8_t part);
uint8_t egui_view_rating_control_get_current_part(egui_view_t *self);
void egui_view_rating_control_set_on_changed_listener(egui_view_t *self, egui_view_on_rating_control_changed_listener_t listener);
void egui_view_rating_control_set_title(egui_view_t *self, const char *title);
void egui_view_rating_control_set_low_label(egui_view_t *self, const char *label);
void egui_view_rating_control_set_high_label(egui_view_t *self, const char *label);
void egui_view_rating_control_set_value_labels(egui_view_t *self, const char **labels, uint8_t label_count);
void egui_view_rating_control_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_rating_control_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode);
void egui_view_rating_control_set_clear_enabled(egui_view_t *self, uint8_t clear_enabled);
uint8_t egui_view_rating_control_get_part_region(egui_view_t *self, uint8_t part, egui_region_t *region);
uint8_t egui_view_rating_control_handle_navigation_key(egui_view_t *self, uint8_t key_code);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_RATING_CONTROL_H_ */

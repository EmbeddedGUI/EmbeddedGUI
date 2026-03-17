#ifndef _EGUI_VIEW_FLIP_VIEW_H_
#define _EGUI_VIEW_FLIP_VIEW_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_FLIP_VIEW_PART_PREVIOUS 0
#define EGUI_VIEW_FLIP_VIEW_PART_SURFACE  1
#define EGUI_VIEW_FLIP_VIEW_PART_NEXT     2
#define EGUI_VIEW_FLIP_VIEW_PART_NONE     0xFF

typedef struct egui_view_flip_view_item egui_view_flip_view_item_t;
struct egui_view_flip_view_item
{
    const char *eyebrow;
    const char *title;
    const char *description;
    const char *footer;
    egui_color_t surface_color;
    egui_color_t accent_color;
};

typedef void (*egui_view_on_flip_view_changed_listener_t)(egui_view_t *self, uint8_t current_index, uint8_t item_count, uint8_t part);

typedef struct egui_view_flip_view egui_view_flip_view_t;
struct egui_view_flip_view
{
    egui_view_t base;
    egui_view_on_flip_view_changed_listener_t on_changed;
    const egui_view_flip_view_item_t *items;
    const egui_font_t *font;
    const egui_font_t *meta_font;
    const char *title;
    const char *helper;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t inactive_color;
    uint8_t item_count;
    uint8_t current_index;
    uint8_t current_part;
    uint8_t compact_mode;
    uint8_t read_only_mode;
    uint8_t pressed_part;
};

void egui_view_flip_view_init(egui_view_t *self);
void egui_view_flip_view_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_flip_view_set_meta_font(egui_view_t *self, const egui_font_t *font);
void egui_view_flip_view_set_title(egui_view_t *self, const char *title);
void egui_view_flip_view_set_helper(egui_view_t *self, const char *helper);
void egui_view_flip_view_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                     egui_color_t muted_text_color, egui_color_t inactive_color);
void egui_view_flip_view_set_items(egui_view_t *self, const egui_view_flip_view_item_t *items, uint8_t item_count, uint8_t current_index);
const egui_view_flip_view_item_t *egui_view_flip_view_get_current_item(egui_view_t *self);
uint8_t egui_view_flip_view_get_item_count(egui_view_t *self);
uint8_t egui_view_flip_view_get_current_index(egui_view_t *self);
void egui_view_flip_view_set_current_index(egui_view_t *self, uint8_t current_index);
void egui_view_flip_view_set_current_part(egui_view_t *self, uint8_t part);
uint8_t egui_view_flip_view_get_current_part(egui_view_t *self);
void egui_view_flip_view_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_flip_view_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode);
void egui_view_flip_view_set_on_changed_listener(egui_view_t *self, egui_view_on_flip_view_changed_listener_t listener);
uint8_t egui_view_flip_view_get_part_region(egui_view_t *self, uint8_t part, egui_region_t *region);
uint8_t egui_view_flip_view_handle_navigation_key(egui_view_t *self, uint8_t key_code);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_FLIP_VIEW_H_ */
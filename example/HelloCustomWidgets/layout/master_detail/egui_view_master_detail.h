#ifndef _EGUI_VIEW_MASTER_DETAIL_H_
#define _EGUI_VIEW_MASTER_DETAIL_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_MASTER_DETAIL_MAX_ITEMS 4

#define EGUI_VIEW_MASTER_DETAIL_TONE_ACCENT  0
#define EGUI_VIEW_MASTER_DETAIL_TONE_SUCCESS 1
#define EGUI_VIEW_MASTER_DETAIL_TONE_WARNING 2
#define EGUI_VIEW_MASTER_DETAIL_TONE_NEUTRAL 3

typedef struct egui_view_master_detail_item egui_view_master_detail_item_t;
struct egui_view_master_detail_item
{
    const char *master_glyph;
    const char *master_title;
    const char *master_meta;
    const char *detail_eyebrow;
    const char *detail_title;
    const char *detail_meta;
    const char *detail_body_primary;
    const char *detail_body_secondary;
    const char *detail_footer;
    uint8_t tone;
    uint8_t emphasized;
};

typedef void (*egui_view_on_master_detail_selection_changed_listener_t)(egui_view_t *self, uint8_t index);

typedef struct egui_view_master_detail egui_view_master_detail_t;
struct egui_view_master_detail
{
    egui_view_t base;
    const egui_view_master_detail_item_t *items;
    const egui_font_t *font;
    const egui_font_t *meta_font;
    egui_view_on_master_detail_selection_changed_listener_t on_selection_changed;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t section_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t accent_color;
    egui_color_t success_color;
    egui_color_t warning_color;
    egui_color_t neutral_color;
    uint8_t item_count;
    uint8_t current_index;
    uint8_t compact_mode;
    uint8_t read_only_mode;
    uint8_t pressed_index;
};

void egui_view_master_detail_init(egui_view_t *self);
void egui_view_master_detail_set_items(egui_view_t *self, const egui_view_master_detail_item_t *items, uint8_t item_count);
uint8_t egui_view_master_detail_get_item_count(egui_view_t *self);
void egui_view_master_detail_set_current_index(egui_view_t *self, uint8_t item_index);
uint8_t egui_view_master_detail_get_current_index(egui_view_t *self);
void egui_view_master_detail_set_on_selection_changed_listener(egui_view_t *self, egui_view_on_master_detail_selection_changed_listener_t listener);
void egui_view_master_detail_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_master_detail_set_meta_font(egui_view_t *self, const egui_font_t *font);
void egui_view_master_detail_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_master_detail_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode);
void egui_view_master_detail_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t section_color,
                                         egui_color_t text_color, egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t success_color,
                                         egui_color_t warning_color, egui_color_t neutral_color);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_MASTER_DETAIL_H_ */

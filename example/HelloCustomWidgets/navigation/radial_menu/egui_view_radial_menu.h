#ifndef _EGUI_VIEW_RADIAL_MENU_H_
#define _EGUI_VIEW_RADIAL_MENU_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_RADIAL_MENU_MAX_ITEMS  8
#define EGUI_VIEW_RADIAL_MENU_INDEX_NONE 0xFF

typedef void (*egui_view_radial_menu_on_selection_changed_listener_t)(egui_view_t *self, uint8_t index);

typedef struct egui_view_radial_menu egui_view_radial_menu_t;
struct egui_view_radial_menu
{
    egui_view_t base;
    const char **item_texts;
    const egui_font_t *font;
    egui_view_radial_menu_on_selection_changed_listener_t on_selection_changed;
    egui_color_t base_color;
    egui_color_t accent_color;
    egui_color_t center_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    uint8_t item_count;
    uint8_t current_index;
    uint8_t hot_index;
    uint8_t is_expanded;
    uint8_t is_tracking;
    uint8_t expanded_on_down;
};

void egui_view_radial_menu_init(egui_view_t *self);
void egui_view_radial_menu_set_items(egui_view_t *self, const char **item_texts, uint8_t item_count);
void egui_view_radial_menu_set_current_index(egui_view_t *self, uint8_t index);
uint8_t egui_view_radial_menu_get_current_index(egui_view_t *self);
void egui_view_radial_menu_set_on_selection_changed_listener(egui_view_t *self, egui_view_radial_menu_on_selection_changed_listener_t listener);
void egui_view_radial_menu_set_palette(egui_view_t *self, egui_color_t base_color, egui_color_t accent_color, egui_color_t center_color);
void egui_view_radial_menu_set_decoration_colors(egui_view_t *self, egui_color_t border_color, egui_color_t text_color, egui_color_t muted_text_color);
void egui_view_radial_menu_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_radial_menu_set_expanded(egui_view_t *self, uint8_t expanded);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_RADIAL_MENU_H_ */

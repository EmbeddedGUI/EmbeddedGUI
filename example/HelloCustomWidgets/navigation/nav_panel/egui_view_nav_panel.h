#ifndef _EGUI_VIEW_NAV_PANEL_H_
#define _EGUI_VIEW_NAV_PANEL_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_NAV_PANEL_MAX_ITEMS 4

typedef struct egui_view_nav_panel_item egui_view_nav_panel_item_t;
struct egui_view_nav_panel_item
{
    const char *title;
    const char *badge;
};

typedef void (*egui_view_on_nav_panel_selection_changed_listener_t)(egui_view_t *self, uint8_t index);

typedef struct egui_view_nav_panel egui_view_nav_panel_t;
struct egui_view_nav_panel
{
    egui_view_t base;
    egui_view_on_nav_panel_selection_changed_listener_t on_selection_changed;
    const egui_font_t *font;
    const egui_font_t *meta_font;
    const egui_view_nav_panel_item_t *items;
    const char *header_text;
    const char *footer_text;
    const char *footer_badge;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t accent_color;
    uint8_t item_count;
    uint8_t current_index;
    uint8_t compact_mode;
    uint8_t locked_mode;
    uint8_t pressed_index;
};

void egui_view_nav_panel_init(egui_view_t *self);
void egui_view_nav_panel_set_items(egui_view_t *self, const egui_view_nav_panel_item_t *items, uint8_t item_count);
void egui_view_nav_panel_set_current_index(egui_view_t *self, uint8_t index);
uint8_t egui_view_nav_panel_get_current_index(egui_view_t *self);
void egui_view_nav_panel_set_header_text(egui_view_t *self, const char *text);
void egui_view_nav_panel_set_footer_text(egui_view_t *self, const char *text);
void egui_view_nav_panel_set_footer_badge(egui_view_t *self, const char *text);
void egui_view_nav_panel_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_nav_panel_set_meta_font(egui_view_t *self, const egui_font_t *font);
void egui_view_nav_panel_set_on_selection_changed_listener(egui_view_t *self, egui_view_on_nav_panel_selection_changed_listener_t listener);
void egui_view_nav_panel_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_nav_panel_set_locked_mode(egui_view_t *self, uint8_t locked_mode);
void egui_view_nav_panel_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                     egui_color_t muted_text_color, egui_color_t accent_color);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_NAV_PANEL_H_ */

#ifndef _EGUI_VIEW_MENU_BAR_H_
#define _EGUI_VIEW_MENU_BAR_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_MENU_BAR_MAX_SNAPSHOTS   6
#define EGUI_VIEW_MENU_BAR_MAX_MENUS       5
#define EGUI_VIEW_MENU_BAR_MAX_PANEL_ITEMS 6
#define EGUI_VIEW_MENU_BAR_HIT_NONE        0xFF
#define EGUI_VIEW_MENU_BAR_ITEM_NONE       EGUI_VIEW_MENU_BAR_HIT_NONE

#define EGUI_VIEW_MENU_BAR_TRAILING_NONE    0
#define EGUI_VIEW_MENU_BAR_TRAILING_SUBMENU 1

typedef struct egui_view_menu_bar_menu egui_view_menu_bar_menu_t;
struct egui_view_menu_bar_menu
{
    const char *title;
    uint8_t emphasized;
    uint8_t enabled;
};

typedef struct egui_view_menu_bar_panel_item egui_view_menu_bar_panel_item_t;
struct egui_view_menu_bar_panel_item
{
    const char *title;
    const char *meta;
    uint8_t tone;
    uint8_t emphasized;
    uint8_t enabled;
    uint8_t separator_before;
    uint8_t trailing_kind;
};

typedef struct egui_view_menu_bar_snapshot egui_view_menu_bar_snapshot_t;
struct egui_view_menu_bar_snapshot
{
    const egui_view_menu_bar_menu_t *menus;
    uint8_t menu_count;
    uint8_t current_menu;
    const egui_view_menu_bar_panel_item_t *panel_items;
    uint8_t panel_item_count;
    uint8_t focus_item;
    uint8_t show_panel;
};

typedef void (*egui_view_on_menu_bar_selection_changed_listener_t)(egui_view_t *self, uint8_t snapshot_index, uint8_t item_index);
typedef void (*egui_view_on_menu_bar_item_activated_listener_t)(egui_view_t *self, uint8_t snapshot_index, uint8_t item_index);

typedef struct egui_view_menu_bar egui_view_menu_bar_t;
struct egui_view_menu_bar
{
    egui_view_t base;
    const egui_view_menu_bar_snapshot_t *snapshots;
    const egui_font_t *font;
    const egui_font_t *meta_font;
    egui_view_on_menu_bar_selection_changed_listener_t on_selection_changed;
    egui_view_on_menu_bar_item_activated_listener_t on_item_activated;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t accent_color;
    egui_color_t success_color;
    egui_color_t warning_color;
    egui_color_t danger_color;
    egui_color_t shadow_color;
    uint8_t snapshot_count;
    uint8_t current_snapshot;
    uint8_t current_item;
    uint8_t compact_mode;
    uint8_t locked_mode;
    uint8_t pressed_item;
    uint8_t pressed_menu;
};

void egui_view_menu_bar_init(egui_view_t *self);
void egui_view_menu_bar_set_snapshots(egui_view_t *self, const egui_view_menu_bar_snapshot_t *snapshots, uint8_t snapshot_count);
void egui_view_menu_bar_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index);
uint8_t egui_view_menu_bar_get_current_snapshot(egui_view_t *self);
void egui_view_menu_bar_set_current_item(egui_view_t *self, uint8_t item_index);
uint8_t egui_view_menu_bar_get_current_item(egui_view_t *self);
void egui_view_menu_bar_activate_current_item(egui_view_t *self);
uint8_t egui_view_menu_bar_hit_menu(egui_view_t *self, egui_dim_t x, egui_dim_t y);
void egui_view_menu_bar_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_menu_bar_set_meta_font(egui_view_t *self, const egui_font_t *font);
void egui_view_menu_bar_set_on_selection_changed_listener(egui_view_t *self, egui_view_on_menu_bar_selection_changed_listener_t listener);
void egui_view_menu_bar_set_on_item_activated_listener(egui_view_t *self, egui_view_on_menu_bar_item_activated_listener_t listener);
void egui_view_menu_bar_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_menu_bar_set_locked_mode(egui_view_t *self, uint8_t locked_mode);
void egui_view_menu_bar_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                    egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t success_color, egui_color_t warning_color,
                                    egui_color_t danger_color, egui_color_t shadow_color);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_MENU_BAR_H_ */

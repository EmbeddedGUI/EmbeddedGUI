#ifndef _EGUI_VIEW_TAB_VIEW_H_
#define _EGUI_VIEW_TAB_VIEW_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_TAB_VIEW_MAX_TABS      5
#define EGUI_VIEW_TAB_VIEW_MAX_SNAPSHOTS 4
#define EGUI_VIEW_TAB_VIEW_TAB_NONE      0xFF

#define EGUI_VIEW_TAB_VIEW_PART_TAB   0
#define EGUI_VIEW_TAB_VIEW_PART_CLOSE 1
#define EGUI_VIEW_TAB_VIEW_PART_ADD   2

#define EGUI_VIEW_TAB_VIEW_ACTION_CLOSE 0
#define EGUI_VIEW_TAB_VIEW_ACTION_ADD   1

#define EGUI_VIEW_TAB_VIEW_TONE_ACCENT  0
#define EGUI_VIEW_TAB_VIEW_TONE_SUCCESS 1
#define EGUI_VIEW_TAB_VIEW_TONE_WARNING 2
#define EGUI_VIEW_TAB_VIEW_TONE_NEUTRAL 3

typedef struct egui_view_tab_view_tab egui_view_tab_view_tab_t;
struct egui_view_tab_view_tab
{
    const char *title;
    const char *subtitle;
    const char *eyebrow;
    const char *body_primary;
    const char *body_secondary;
    const char *footer;
    const char *badge;
    uint8_t tone;
    uint8_t dirty;
    uint8_t closable;
};

typedef struct egui_view_tab_view_snapshot egui_view_tab_view_snapshot_t;
struct egui_view_tab_view_snapshot
{
    const char *window_title;
    const char *window_meta;
    const egui_view_tab_view_tab_t *tabs;
    uint8_t tab_count;
    uint8_t current_index;
    uint8_t add_enabled;
};

typedef void (*egui_view_on_tab_view_changed_listener_t)(egui_view_t *self, uint8_t snapshot_index, uint8_t tab_index, uint8_t part);
typedef void (*egui_view_on_tab_view_action_listener_t)(egui_view_t *self, uint8_t action, uint8_t tab_index);

typedef struct egui_view_tab_view egui_view_tab_view_t;
struct egui_view_tab_view
{
    egui_view_t base;
    const egui_view_tab_view_snapshot_t *snapshots;
    const egui_font_t *font;
    const egui_font_t *meta_font;
    egui_view_on_tab_view_changed_listener_t on_changed;
    egui_view_on_tab_view_action_listener_t on_action;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t section_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t accent_color;
    egui_color_t success_color;
    egui_color_t warning_color;
    egui_color_t neutral_color;
    uint8_t snapshot_count;
    uint8_t current_snapshot;
    uint8_t current_tab;
    uint8_t current_part;
    uint8_t compact_mode;
    uint8_t read_only_mode;
    uint8_t pressed_tab;
    uint8_t pressed_part;
    uint8_t closed_mask;
};

void egui_view_tab_view_init(egui_view_t *self);
void egui_view_tab_view_set_snapshots(egui_view_t *self, const egui_view_tab_view_snapshot_t *snapshots, uint8_t snapshot_count);
void egui_view_tab_view_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index);
uint8_t egui_view_tab_view_get_current_snapshot(egui_view_t *self);
void egui_view_tab_view_set_current_tab(egui_view_t *self, uint8_t tab_index);
uint8_t egui_view_tab_view_get_current_tab(egui_view_t *self);
void egui_view_tab_view_set_current_part(egui_view_t *self, uint8_t part);
uint8_t egui_view_tab_view_get_current_part(egui_view_t *self);
uint8_t egui_view_tab_view_close_current_tab(egui_view_t *self);
uint8_t egui_view_tab_view_restore_tabs(egui_view_t *self);
uint8_t egui_view_tab_view_get_visible_tab_count(egui_view_t *self);
uint8_t egui_view_tab_view_is_tab_closed(egui_view_t *self, uint8_t tab_index);
uint8_t egui_view_tab_view_get_tab_region(egui_view_t *self, uint8_t tab_index, egui_region_t *region);
uint8_t egui_view_tab_view_get_part_region(egui_view_t *self, uint8_t part, egui_region_t *region);
void egui_view_tab_view_set_on_changed_listener(egui_view_t *self, egui_view_on_tab_view_changed_listener_t listener);
void egui_view_tab_view_set_on_action_listener(egui_view_t *self, egui_view_on_tab_view_action_listener_t listener);
void egui_view_tab_view_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_tab_view_set_meta_font(egui_view_t *self, const egui_font_t *font);
void egui_view_tab_view_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_tab_view_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode);
void egui_view_tab_view_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t section_color,
                                    egui_color_t text_color, egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t success_color,
                                    egui_color_t warning_color, egui_color_t neutral_color);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_TAB_VIEW_H_ */

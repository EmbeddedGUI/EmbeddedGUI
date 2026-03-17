#ifndef _EGUI_VIEW_SPLIT_BUTTON_H_
#define _EGUI_VIEW_SPLIT_BUTTON_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_SPLIT_BUTTON_MAX_SNAPSHOTS 6

#define EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY 0
#define EGUI_VIEW_SPLIT_BUTTON_PART_MENU    1
#define EGUI_VIEW_SPLIT_BUTTON_PART_NONE    0xFF

#define EGUI_VIEW_SPLIT_BUTTON_TONE_ACCENT  0
#define EGUI_VIEW_SPLIT_BUTTON_TONE_SUCCESS 1
#define EGUI_VIEW_SPLIT_BUTTON_TONE_WARNING 2
#define EGUI_VIEW_SPLIT_BUTTON_TONE_DANGER  3
#define EGUI_VIEW_SPLIT_BUTTON_TONE_NEUTRAL 4

typedef struct egui_view_split_button_snapshot egui_view_split_button_snapshot_t;
struct egui_view_split_button_snapshot
{
    const char *title;
    const char *glyph;
    const char *label;
    const char *helper;
    uint8_t tone;
    uint8_t emphasized;
    uint8_t primary_enabled;
    uint8_t menu_enabled;
    uint8_t focus_part;
};

typedef void (*egui_view_on_split_button_part_changed_listener_t)(egui_view_t *self, uint8_t part);

typedef struct egui_view_split_button egui_view_split_button_t;
struct egui_view_split_button
{
    egui_view_t base;
    const egui_view_split_button_snapshot_t *snapshots;
    const egui_font_t *font;
    const egui_font_t *meta_font;
    egui_view_on_split_button_part_changed_listener_t on_part_changed;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t accent_color;
    egui_color_t success_color;
    egui_color_t warning_color;
    egui_color_t danger_color;
    egui_color_t neutral_color;
    uint8_t snapshot_count;
    uint8_t current_snapshot;
    uint8_t current_part;
    uint8_t compact_mode;
    uint8_t disabled_mode;
    uint8_t pressed_part;
};

void egui_view_split_button_init(egui_view_t *self);
void egui_view_split_button_set_snapshots(egui_view_t *self, const egui_view_split_button_snapshot_t *snapshots, uint8_t snapshot_count);
void egui_view_split_button_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index);
uint8_t egui_view_split_button_get_current_snapshot(egui_view_t *self);
void egui_view_split_button_set_current_part(egui_view_t *self, uint8_t part);
uint8_t egui_view_split_button_get_current_part(egui_view_t *self);
void egui_view_split_button_set_on_part_changed_listener(egui_view_t *self, egui_view_on_split_button_part_changed_listener_t listener);
void egui_view_split_button_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_split_button_set_meta_font(egui_view_t *self, const egui_font_t *font);
void egui_view_split_button_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_split_button_set_disabled_mode(egui_view_t *self, uint8_t disabled_mode);
void egui_view_split_button_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                        egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t success_color, egui_color_t warning_color,
                                        egui_color_t danger_color, egui_color_t neutral_color);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_SPLIT_BUTTON_H_ */

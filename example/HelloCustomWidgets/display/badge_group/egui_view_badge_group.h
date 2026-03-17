#ifndef _EGUI_VIEW_BADGE_GROUP_H_
#define _EGUI_VIEW_BADGE_GROUP_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_BADGE_GROUP_MAX_SNAPSHOTS 6
#define EGUI_VIEW_BADGE_GROUP_MAX_ITEMS     6

typedef struct egui_view_badge_group_item egui_view_badge_group_item_t;
struct egui_view_badge_group_item
{
    const char *label;
    const char *meta;
    uint8_t tone;
    uint8_t emphasized;
    uint8_t outlined;
};

typedef struct egui_view_badge_group_snapshot egui_view_badge_group_snapshot_t;
struct egui_view_badge_group_snapshot
{
    const char *eyebrow;
    const char *title;
    const char *body;
    const char *footer;
    const egui_view_badge_group_item_t *items;
    uint8_t item_count;
    uint8_t focus_index;
};

typedef struct egui_view_badge_group egui_view_badge_group_t;
struct egui_view_badge_group
{
    egui_view_t base;
    const egui_view_badge_group_snapshot_t *snapshots;
    const egui_font_t *font;
    const egui_font_t *meta_font;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t accent_color;
    egui_color_t success_color;
    egui_color_t warning_color;
    egui_color_t neutral_color;
    uint8_t snapshot_count;
    uint8_t current_snapshot;
    uint8_t compact_mode;
    uint8_t locked_mode;
};

void egui_view_badge_group_init(egui_view_t *self);
void egui_view_badge_group_set_snapshots(egui_view_t *self, const egui_view_badge_group_snapshot_t *snapshots, uint8_t snapshot_count);
void egui_view_badge_group_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index);
uint8_t egui_view_badge_group_get_current_snapshot(egui_view_t *self);
void egui_view_badge_group_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_badge_group_set_meta_font(egui_view_t *self, const egui_font_t *font);
void egui_view_badge_group_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_badge_group_set_locked_mode(egui_view_t *self, uint8_t locked_mode);
void egui_view_badge_group_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                       egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t success_color, egui_color_t warning_color,
                                       egui_color_t neutral_color);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_BADGE_GROUP_H_ */

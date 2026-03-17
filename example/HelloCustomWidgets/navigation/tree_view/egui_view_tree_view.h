#ifndef _EGUI_VIEW_TREE_VIEW_H_
#define _EGUI_VIEW_TREE_VIEW_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_TREE_VIEW_MAX_SNAPSHOTS 6
#define EGUI_VIEW_TREE_VIEW_MAX_ITEMS     8

#define EGUI_VIEW_TREE_VIEW_TONE_ACCENT  0
#define EGUI_VIEW_TREE_VIEW_TONE_SUCCESS 1
#define EGUI_VIEW_TREE_VIEW_TONE_WARNING 2
#define EGUI_VIEW_TREE_VIEW_TONE_NEUTRAL 3

#define EGUI_VIEW_TREE_VIEW_KIND_FOLDER 0
#define EGUI_VIEW_TREE_VIEW_KIND_LEAF   1

#define EGUI_VIEW_TREE_VIEW_INDEX_NONE 0xFF

typedef struct egui_view_tree_view_item egui_view_tree_view_item_t;
struct egui_view_tree_view_item
{
    const char *title;
    const char *meta;
    uint8_t depth;
    uint8_t tone;
    uint8_t kind;
    uint8_t has_children;
    uint8_t expanded;
};

typedef struct egui_view_tree_view_snapshot egui_view_tree_view_snapshot_t;
struct egui_view_tree_view_snapshot
{
    const char *title;
    const char *caption;
    const char *footer;
    const egui_view_tree_view_item_t *items;
    uint8_t item_count;
    uint8_t focus_index;
};

typedef void (*egui_view_on_tree_view_selection_changed_listener_t)(egui_view_t *self, uint8_t index);

typedef struct egui_view_tree_view egui_view_tree_view_t;
struct egui_view_tree_view
{
    egui_view_t base;
    const egui_view_tree_view_snapshot_t *snapshots;
    const egui_font_t *font;
    const egui_font_t *meta_font;
    egui_view_on_tree_view_selection_changed_listener_t on_selection_changed;
    egui_color_t surface_color;
    egui_color_t section_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t accent_color;
    egui_color_t success_color;
    egui_color_t warning_color;
    egui_color_t neutral_color;
    uint8_t snapshot_count;
    uint8_t current_snapshot;
    uint8_t current_index;
    uint8_t compact_mode;
    uint8_t locked_mode;
    uint8_t pressed_index;
};

void egui_view_tree_view_init(egui_view_t *self);
void egui_view_tree_view_set_snapshots(egui_view_t *self, const egui_view_tree_view_snapshot_t *snapshots, uint8_t snapshot_count);
void egui_view_tree_view_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index);
uint8_t egui_view_tree_view_get_current_snapshot(egui_view_t *self);
void egui_view_tree_view_set_current_index(egui_view_t *self, uint8_t index);
uint8_t egui_view_tree_view_get_current_index(egui_view_t *self);
void egui_view_tree_view_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_tree_view_set_meta_font(egui_view_t *self, const egui_font_t *font);
void egui_view_tree_view_set_on_selection_changed_listener(egui_view_t *self, egui_view_on_tree_view_selection_changed_listener_t listener);
void egui_view_tree_view_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_tree_view_set_locked_mode(egui_view_t *self, uint8_t locked_mode);
void egui_view_tree_view_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t section_color, egui_color_t border_color,
                                     egui_color_t text_color, egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t success_color,
                                     egui_color_t warning_color, egui_color_t neutral_color);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_TREE_VIEW_H_ */

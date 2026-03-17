#ifndef _EGUI_VIEW_COMMAND_BAR_H_
#define _EGUI_VIEW_COMMAND_BAR_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_COMMAND_BAR_MAX_SNAPSHOTS 6
#define EGUI_VIEW_COMMAND_BAR_MAX_ITEMS     6

#define EGUI_VIEW_COMMAND_BAR_INDEX_NONE 0xFF

#define EGUI_VIEW_COMMAND_BAR_TONE_ACCENT  0
#define EGUI_VIEW_COMMAND_BAR_TONE_SUCCESS 1
#define EGUI_VIEW_COMMAND_BAR_TONE_WARNING 2
#define EGUI_VIEW_COMMAND_BAR_TONE_DANGER  3
#define EGUI_VIEW_COMMAND_BAR_TONE_NEUTRAL 4

#define EGUI_VIEW_COMMAND_BAR_ITEM_KIND_NORMAL   0
#define EGUI_VIEW_COMMAND_BAR_ITEM_KIND_OVERFLOW 1

typedef struct egui_view_command_bar_item egui_view_command_bar_item_t;
struct egui_view_command_bar_item
{
    const char *glyph;
    const char *label;
    uint8_t tone;
    uint8_t emphasized;
    uint8_t enabled;
    uint8_t kind;
};

typedef struct egui_view_command_bar_snapshot egui_view_command_bar_snapshot_t;
struct egui_view_command_bar_snapshot
{
    const char *eyebrow;
    const char *title;
    const char *scope;
    const char *footer;
    const egui_view_command_bar_item_t *items;
    uint8_t item_count;
    uint8_t focus_index;
};

typedef void (*egui_view_on_command_bar_selection_changed_listener_t)(egui_view_t *self, uint8_t index);

typedef struct egui_view_command_bar egui_view_command_bar_t;
struct egui_view_command_bar
{
    egui_view_t base;
    const egui_view_command_bar_snapshot_t *snapshots;
    const egui_font_t *font;
    const egui_font_t *meta_font;
    egui_view_on_command_bar_selection_changed_listener_t on_selection_changed;
    egui_color_t surface_color;
    egui_color_t section_color;
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
    uint8_t current_index;
    uint8_t compact_mode;
    uint8_t disabled_mode;
    uint8_t pressed_index;
};

void egui_view_command_bar_init(egui_view_t *self);
void egui_view_command_bar_set_snapshots(egui_view_t *self, const egui_view_command_bar_snapshot_t *snapshots, uint8_t snapshot_count);
void egui_view_command_bar_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index);
uint8_t egui_view_command_bar_get_current_snapshot(egui_view_t *self);
void egui_view_command_bar_set_current_index(egui_view_t *self, uint8_t index);
uint8_t egui_view_command_bar_get_current_index(egui_view_t *self);
void egui_view_command_bar_set_on_selection_changed_listener(egui_view_t *self, egui_view_on_command_bar_selection_changed_listener_t listener);
void egui_view_command_bar_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_command_bar_set_meta_font(egui_view_t *self, const egui_font_t *font);
void egui_view_command_bar_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_command_bar_set_disabled_mode(egui_view_t *self, uint8_t disabled_mode);
void egui_view_command_bar_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t section_color, egui_color_t border_color,
                                       egui_color_t text_color, egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t success_color,
                                       egui_color_t warning_color, egui_color_t danger_color, egui_color_t neutral_color);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_COMMAND_BAR_H_ */

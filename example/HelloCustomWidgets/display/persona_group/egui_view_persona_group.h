#ifndef _EGUI_VIEW_PERSONA_GROUP_H_
#define _EGUI_VIEW_PERSONA_GROUP_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_PERSONA_GROUP_MAX_ITEMS     4
#define EGUI_VIEW_PERSONA_GROUP_MAX_SNAPSHOTS 4

#define EGUI_VIEW_PERSONA_GROUP_TONE_ACCENT  0
#define EGUI_VIEW_PERSONA_GROUP_TONE_SUCCESS 1
#define EGUI_VIEW_PERSONA_GROUP_TONE_WARNING 2
#define EGUI_VIEW_PERSONA_GROUP_TONE_NEUTRAL 3

#define EGUI_VIEW_PERSONA_GROUP_PRESENCE_LIVE 0
#define EGUI_VIEW_PERSONA_GROUP_PRESENCE_BUSY 1
#define EGUI_VIEW_PERSONA_GROUP_PRESENCE_AWAY 2
#define EGUI_VIEW_PERSONA_GROUP_PRESENCE_IDLE 3

typedef struct egui_view_persona_group_item egui_view_persona_group_item_t;
struct egui_view_persona_group_item
{
    const char *initials;
    const char *name;
    const char *role;
    uint8_t tone;
    uint8_t presence;
    uint8_t emphasized;
};

typedef struct egui_view_persona_group_snapshot egui_view_persona_group_snapshot_t;
struct egui_view_persona_group_snapshot
{
    const char *eyebrow;
    const char *title;
    const char *summary;
    const egui_view_persona_group_item_t *items;
    uint8_t item_count;
    uint8_t focus_index;
    uint8_t overflow_count;
};

typedef void (*egui_view_on_persona_group_focus_changed_listener_t)(egui_view_t *self, uint8_t snapshot_index, uint8_t item_index);

typedef struct egui_view_persona_group egui_view_persona_group_t;
struct egui_view_persona_group
{
    egui_view_t base;
    const egui_view_persona_group_snapshot_t *snapshots;
    const egui_font_t *font;
    const egui_font_t *meta_font;
    egui_view_on_persona_group_focus_changed_listener_t on_focus_changed;
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
    uint8_t current_index;
    uint8_t compact_mode;
    uint8_t read_only_mode;
    uint8_t pressed_index;
};

void egui_view_persona_group_init(egui_view_t *self);
void egui_view_persona_group_set_snapshots(egui_view_t *self, const egui_view_persona_group_snapshot_t *snapshots, uint8_t snapshot_count);
void egui_view_persona_group_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index);
uint8_t egui_view_persona_group_get_current_snapshot(egui_view_t *self);
void egui_view_persona_group_set_current_index(egui_view_t *self, uint8_t item_index);
uint8_t egui_view_persona_group_get_current_index(egui_view_t *self);
void egui_view_persona_group_set_on_focus_changed_listener(egui_view_t *self, egui_view_on_persona_group_focus_changed_listener_t listener);
void egui_view_persona_group_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_persona_group_set_meta_font(egui_view_t *self, const egui_font_t *font);
void egui_view_persona_group_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_persona_group_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode);
void egui_view_persona_group_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t section_color,
                                         egui_color_t text_color, egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t success_color,
                                         egui_color_t warning_color, egui_color_t neutral_color);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_PERSONA_GROUP_H_ */

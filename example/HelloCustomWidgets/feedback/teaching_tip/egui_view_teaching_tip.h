#ifndef _EGUI_VIEW_TEACHING_TIP_H_
#define _EGUI_VIEW_TEACHING_TIP_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_TEACHING_TIP_MAX_SNAPSHOTS 6

#define EGUI_VIEW_TEACHING_TIP_PART_TARGET    0
#define EGUI_VIEW_TEACHING_TIP_PART_PRIMARY   1
#define EGUI_VIEW_TEACHING_TIP_PART_SECONDARY 2
#define EGUI_VIEW_TEACHING_TIP_PART_CLOSE     3
#define EGUI_VIEW_TEACHING_TIP_PART_NONE      0xFF

#define EGUI_VIEW_TEACHING_TIP_TONE_ACCENT  0
#define EGUI_VIEW_TEACHING_TIP_TONE_SUCCESS 1
#define EGUI_VIEW_TEACHING_TIP_TONE_WARNING 2
#define EGUI_VIEW_TEACHING_TIP_TONE_NEUTRAL 3

#define EGUI_VIEW_TEACHING_TIP_PLACEMENT_BOTTOM 0
#define EGUI_VIEW_TEACHING_TIP_PLACEMENT_TOP    1

typedef struct egui_view_teaching_tip_snapshot egui_view_teaching_tip_snapshot_t;
struct egui_view_teaching_tip_snapshot
{
    const char *target_label;
    const char *eyebrow;
    const char *title;
    const char *body;
    const char *primary_action;
    const char *secondary_action;
    const char *footer;
    uint8_t tone;
    uint8_t placement;
    uint8_t open_mode;
    uint8_t primary_enabled;
    uint8_t secondary_enabled;
    uint8_t show_close;
    uint8_t focus_part;
    int16_t target_offset_x;
};

typedef void (*egui_view_on_teaching_tip_part_changed_listener_t)(egui_view_t *self, uint8_t part);

typedef struct egui_view_teaching_tip egui_view_teaching_tip_t;
struct egui_view_teaching_tip
{
    egui_view_t base;
    const egui_view_teaching_tip_snapshot_t *snapshots;
    const egui_font_t *font;
    const egui_font_t *meta_font;
    egui_view_on_teaching_tip_part_changed_listener_t on_part_changed;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t accent_color;
    egui_color_t success_color;
    egui_color_t warning_color;
    egui_color_t neutral_color;
    egui_color_t shadow_color;
    uint8_t snapshot_count;
    uint8_t current_snapshot;
    uint8_t current_part;
    uint8_t compact_mode;
    uint8_t read_only_mode;
    uint8_t pressed_part;
};

void egui_view_teaching_tip_init(egui_view_t *self);
void egui_view_teaching_tip_set_snapshots(egui_view_t *self, const egui_view_teaching_tip_snapshot_t *snapshots, uint8_t snapshot_count);
void egui_view_teaching_tip_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index);
uint8_t egui_view_teaching_tip_get_current_snapshot(egui_view_t *self);
void egui_view_teaching_tip_set_current_part(egui_view_t *self, uint8_t part);
uint8_t egui_view_teaching_tip_get_current_part(egui_view_t *self);
void egui_view_teaching_tip_set_on_part_changed_listener(egui_view_t *self, egui_view_on_teaching_tip_part_changed_listener_t listener);
void egui_view_teaching_tip_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_teaching_tip_set_meta_font(egui_view_t *self, const egui_font_t *font);
void egui_view_teaching_tip_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_teaching_tip_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode);
void egui_view_teaching_tip_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                        egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t success_color, egui_color_t warning_color,
                                        egui_color_t neutral_color, egui_color_t shadow_color);
uint8_t egui_view_teaching_tip_get_part_region(egui_view_t *self, uint8_t part, egui_region_t *region);
uint8_t egui_view_teaching_tip_handle_navigation_key(egui_view_t *self, uint8_t key_code);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_TEACHING_TIP_H_ */

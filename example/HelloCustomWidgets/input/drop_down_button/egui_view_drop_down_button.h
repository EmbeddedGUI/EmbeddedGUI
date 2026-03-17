#ifndef _EGUI_VIEW_DROP_DOWN_BUTTON_H_
#define _EGUI_VIEW_DROP_DOWN_BUTTON_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_DROP_DOWN_BUTTON_MAX_SNAPSHOTS 6

#define EGUI_VIEW_DROP_DOWN_BUTTON_TONE_ACCENT  0
#define EGUI_VIEW_DROP_DOWN_BUTTON_TONE_SUCCESS 1
#define EGUI_VIEW_DROP_DOWN_BUTTON_TONE_WARNING 2
#define EGUI_VIEW_DROP_DOWN_BUTTON_TONE_DANGER  3
#define EGUI_VIEW_DROP_DOWN_BUTTON_TONE_NEUTRAL 4

typedef struct egui_view_drop_down_button_snapshot egui_view_drop_down_button_snapshot_t;
struct egui_view_drop_down_button_snapshot
{
    const char *title;
    const char *glyph;
    const char *label;
    const char *helper;
    const char *hint;
    uint8_t tone;
    uint8_t emphasized;
};

typedef struct egui_view_drop_down_button egui_view_drop_down_button_t;
struct egui_view_drop_down_button
{
    egui_view_t base;
    const egui_view_drop_down_button_snapshot_t *snapshots;
    const egui_font_t *font;
    const egui_font_t *meta_font;
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
    uint8_t compact_mode;
    uint8_t read_only_mode;
};

void egui_view_drop_down_button_init(egui_view_t *self);
void egui_view_drop_down_button_set_snapshots(egui_view_t *self, const egui_view_drop_down_button_snapshot_t *snapshots, uint8_t snapshot_count);
void egui_view_drop_down_button_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index);
uint8_t egui_view_drop_down_button_get_current_snapshot(egui_view_t *self);
void egui_view_drop_down_button_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_drop_down_button_set_meta_font(egui_view_t *self, const egui_font_t *font);
void egui_view_drop_down_button_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_drop_down_button_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode);
void egui_view_drop_down_button_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                            egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t success_color, egui_color_t warning_color,
                                            egui_color_t danger_color, egui_color_t neutral_color);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_DROP_DOWN_BUTTON_H_ */

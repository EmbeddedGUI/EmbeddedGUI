#ifndef _EGUI_VIEW_DIALOG_SHEET_H_
#define _EGUI_VIEW_DIALOG_SHEET_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_DIALOG_SHEET_MAX_SNAPSHOTS 6

#define EGUI_VIEW_DIALOG_SHEET_TONE_ACCENT  0
#define EGUI_VIEW_DIALOG_SHEET_TONE_SUCCESS 1
#define EGUI_VIEW_DIALOG_SHEET_TONE_WARNING 2
#define EGUI_VIEW_DIALOG_SHEET_TONE_ERROR   3
#define EGUI_VIEW_DIALOG_SHEET_TONE_NEUTRAL 4

#define EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY   0
#define EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY 1
#define EGUI_VIEW_DIALOG_SHEET_ACTION_NONE      0xFF

typedef struct egui_view_dialog_sheet_snapshot egui_view_dialog_sheet_snapshot_t;
struct egui_view_dialog_sheet_snapshot
{
    const char *eyebrow;
    const char *title;
    const char *body;
    const char *primary_action;
    const char *secondary_action;
    const char *tag;
    const char *footer;
    uint8_t tone;
    uint8_t show_secondary;
    uint8_t show_close;
    uint8_t focus_action;
};

typedef void (*egui_view_on_dialog_sheet_action_changed_listener_t)(egui_view_t *self, uint8_t action_index);

typedef struct egui_view_dialog_sheet egui_view_dialog_sheet_t;
struct egui_view_dialog_sheet
{
    egui_view_t base;
    const egui_view_dialog_sheet_snapshot_t *snapshots;
    const egui_font_t *font;
    const egui_font_t *meta_font;
    egui_view_on_dialog_sheet_action_changed_listener_t on_action_changed;
    egui_color_t surface_color;
    egui_color_t overlay_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t accent_color;
    egui_color_t success_color;
    egui_color_t warning_color;
    egui_color_t error_color;
    egui_color_t neutral_color;
    uint8_t snapshot_count;
    uint8_t current_snapshot;
    uint8_t current_action;
    uint8_t compact_mode;
    uint8_t locked_mode;
    uint8_t pressed_action;
};

void egui_view_dialog_sheet_init(egui_view_t *self);
void egui_view_dialog_sheet_set_snapshots(egui_view_t *self, const egui_view_dialog_sheet_snapshot_t *snapshots, uint8_t snapshot_count);
void egui_view_dialog_sheet_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index);
uint8_t egui_view_dialog_sheet_get_current_snapshot(egui_view_t *self);
void egui_view_dialog_sheet_set_current_action(egui_view_t *self, uint8_t action_index);
uint8_t egui_view_dialog_sheet_get_current_action(egui_view_t *self);
void egui_view_dialog_sheet_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_dialog_sheet_set_meta_font(egui_view_t *self, const egui_font_t *font);
void egui_view_dialog_sheet_set_on_action_changed_listener(egui_view_t *self, egui_view_on_dialog_sheet_action_changed_listener_t listener);
void egui_view_dialog_sheet_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_dialog_sheet_set_locked_mode(egui_view_t *self, uint8_t locked_mode);
void egui_view_dialog_sheet_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t overlay_color, egui_color_t border_color,
                                        egui_color_t text_color, egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t success_color,
                                        egui_color_t warning_color, egui_color_t error_color, egui_color_t neutral_color);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_DIALOG_SHEET_H_ */

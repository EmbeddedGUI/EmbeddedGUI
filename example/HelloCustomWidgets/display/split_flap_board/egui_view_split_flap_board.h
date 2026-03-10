#ifndef _EGUI_VIEW_SPLIT_FLAP_BOARD_H_
#define _EGUI_VIEW_SPLIT_FLAP_BOARD_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_SPLIT_FLAP_BOARD_MAX_SNAPSHOTS 4

typedef struct egui_view_split_flap_board_snapshot egui_view_split_flap_board_snapshot_t;
struct egui_view_split_flap_board_snapshot
{
    const char *title;
    const char *status;
    const char *value_text;
    const char *route_text;
    const char *time_text;
    uint8_t focus_slot;
    uint8_t is_alert;
};

typedef struct egui_view_split_flap_board egui_view_split_flap_board_t;
struct egui_view_split_flap_board
{
    egui_view_t base;
    const egui_view_split_flap_board_snapshot_t *snapshots;
    const egui_font_t *font;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t accent_color;
    egui_color_t alert_color;
    egui_color_t split_color;
    uint8_t snapshot_count;
    uint8_t current_snapshot;
    uint8_t compact_mode;
};

void egui_view_split_flap_board_init(egui_view_t *self);
void egui_view_split_flap_board_set_snapshots(egui_view_t *self, const egui_view_split_flap_board_snapshot_t *snapshots, uint8_t snapshot_count);
void egui_view_split_flap_board_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index);
uint8_t egui_view_split_flap_board_get_current_snapshot(egui_view_t *self);
void egui_view_split_flap_board_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_split_flap_board_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_split_flap_board_set_palette(
        egui_view_t *self,
        egui_color_t surface_color,
        egui_color_t border_color,
        egui_color_t text_color,
        egui_color_t muted_text_color,
        egui_color_t accent_color,
        egui_color_t alert_color,
        egui_color_t split_color);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_SPLIT_FLAP_BOARD_H_ */

#ifndef _EGUI_VIEW_SEAT_MAP_H_
#define _EGUI_VIEW_SEAT_MAP_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_SEAT_MAP_MAX_SEATS 24
#define EGUI_VIEW_SEAT_MAP_MAX_SNAPSHOTS 3

typedef struct egui_view_seat_map_snapshot egui_view_seat_map_snapshot_t;
struct egui_view_seat_map_snapshot
{
    const char *title;
    const uint8_t *seat_states;
    uint8_t row_count;
    uint8_t col_count;
    uint8_t aisle_after_col;
    uint8_t focus_row;
    uint8_t focus_seat;
};

typedef struct egui_view_seat_map egui_view_seat_map_t;
struct egui_view_seat_map
{
    egui_view_t base;
    const egui_view_seat_map_snapshot_t *snapshots;
    const egui_font_t *font;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t active_color;
    uint8_t snapshot_count;
    uint8_t current_snapshot;
    uint8_t focus_row;
    uint8_t focus_seat;
    uint8_t show_header;
    uint8_t compact_mode;
};

void egui_view_seat_map_init(egui_view_t *self);
void egui_view_seat_map_set_snapshots(egui_view_t *self, const egui_view_seat_map_snapshot_t *snapshots, uint8_t snapshot_count);
void egui_view_seat_map_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index);
uint8_t egui_view_seat_map_get_current_snapshot(egui_view_t *self);
void egui_view_seat_map_set_focus_row(egui_view_t *self, uint8_t row_index);
void egui_view_seat_map_set_focus_seat(egui_view_t *self, uint8_t seat_index);
void egui_view_seat_map_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_seat_map_set_show_header(egui_view_t *self, uint8_t show_header);
void egui_view_seat_map_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_seat_map_set_palette(
        egui_view_t *self,
        egui_color_t surface_color,
        egui_color_t border_color,
        egui_color_t text_color,
        egui_color_t muted_text_color,
        egui_color_t active_color);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_SEAT_MAP_H_ */

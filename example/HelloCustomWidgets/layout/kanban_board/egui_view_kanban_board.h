#ifndef _EGUI_VIEW_KANBAN_BOARD_H_
#define _EGUI_VIEW_KANBAN_BOARD_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_KANBAN_BOARD_MAX_LANES     4
#define EGUI_VIEW_KANBAN_BOARD_MAX_SNAPSHOTS 3

typedef struct egui_view_kanban_lane egui_view_kanban_lane_t;
struct egui_view_kanban_lane
{
    const char *title;
    const char **card_titles;
    egui_color_t accent_color;
    uint8_t visible_card_count;
    uint8_t total_card_count;
    uint8_t wip_count;
};

typedef struct egui_view_kanban_snapshot egui_view_kanban_snapshot_t;
struct egui_view_kanban_snapshot
{
    const char *title;
    const egui_view_kanban_lane_t *lanes;
    uint8_t lane_count;
};

typedef struct egui_view_kanban_board egui_view_kanban_board_t;
struct egui_view_kanban_board
{
    egui_view_t base;
    const egui_view_kanban_snapshot_t *snapshots;
    const egui_font_t *font;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t focus_color;
    uint8_t snapshot_count;
    uint8_t current_snapshot;
    uint8_t focus_lane;
    uint8_t show_card_text;
    uint8_t show_header;
};

void egui_view_kanban_board_init(egui_view_t *self);
void egui_view_kanban_board_set_snapshots(egui_view_t *self, const egui_view_kanban_snapshot_t *snapshots, uint8_t snapshot_count);
void egui_view_kanban_board_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index);
uint8_t egui_view_kanban_board_get_current_snapshot(egui_view_t *self);
void egui_view_kanban_board_set_focus_lane(egui_view_t *self, uint8_t lane_index);
void egui_view_kanban_board_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_kanban_board_set_show_card_text(egui_view_t *self, uint8_t show_card_text);
void egui_view_kanban_board_set_show_header(egui_view_t *self, uint8_t show_header);
void egui_view_kanban_board_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                        egui_color_t muted_text_color, egui_color_t focus_color);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_KANBAN_BOARD_H_ */

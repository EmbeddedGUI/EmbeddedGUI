#ifndef _EGUI_VIEW_WINDOW_SNAP_GRID_H_
#define _EGUI_VIEW_WINDOW_SNAP_GRID_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_WINDOW_SNAP_GRID_MAX_SNAPSHOTS 6

typedef struct egui_view_window_snap_grid_snapshot egui_view_window_snap_grid_snapshot_t;
struct egui_view_window_snap_grid_snapshot
{
    const char *title;
    const char *status;
    const char *summary;
    const char *footer;
    uint8_t layout_mode;
    uint8_t highlight_mask;
    uint8_t accent_mode;
    uint8_t focus_slot;
};

typedef struct egui_view_window_snap_grid egui_view_window_snap_grid_t;
struct egui_view_window_snap_grid
{
    egui_view_t base;
    const egui_view_window_snap_grid_snapshot_t *snapshots;
    const egui_font_t *font;
    egui_color_t surface_color;
    egui_color_t panel_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t accent_color;
    egui_color_t warn_color;
    egui_color_t lock_color;
    egui_color_t focus_color;
    uint8_t snapshot_count;
    uint8_t current_snapshot;
    uint8_t compact_mode;
    uint8_t locked_mode;
};

void egui_view_window_snap_grid_set_snapshots(egui_view_t *self, const egui_view_window_snap_grid_snapshot_t *snapshots, uint8_t snapshot_count);
void egui_view_window_snap_grid_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index);
uint8_t egui_view_window_snap_grid_get_current_snapshot(egui_view_t *self);
void egui_view_window_snap_grid_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_window_snap_grid_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_window_snap_grid_set_locked_mode(egui_view_t *self, uint8_t locked_mode);
void egui_view_window_snap_grid_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t panel_color, egui_color_t border_color,
                                            egui_color_t text_color, egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t warn_color,
                                            egui_color_t lock_color, egui_color_t focus_color);
void egui_view_window_snap_grid_init(egui_view_t *self);

#ifdef __cplusplus
}
#endif

#endif

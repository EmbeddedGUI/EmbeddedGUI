#ifndef _EGUI_VIEW_SKELETON_LOADER_H_
#define _EGUI_VIEW_SKELETON_LOADER_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_SKELETON_LOADER_MAX_BLOCKS 8
#define EGUI_VIEW_SKELETON_LOADER_MAX_SNAPSHOTS 3

typedef struct egui_view_skeleton_loader_block egui_view_skeleton_loader_block_t;
struct egui_view_skeleton_loader_block
{
    uint8_t x;
    uint8_t y;
    uint8_t width;
    uint8_t height;
    uint8_t radius;
};

typedef struct egui_view_skeleton_loader_snapshot egui_view_skeleton_loader_snapshot_t;
struct egui_view_skeleton_loader_snapshot
{
    const char *title;
    const egui_view_skeleton_loader_block_t *blocks;
    uint8_t block_count;
    uint8_t highlight_block;
};

typedef struct egui_view_skeleton_loader egui_view_skeleton_loader_t;
struct egui_view_skeleton_loader
{
    egui_view_t base;
    const egui_view_skeleton_loader_snapshot_t *snapshots;
    const egui_font_t *font;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t active_color;
    uint8_t snapshot_count;
    uint8_t current_snapshot;
    uint8_t focus_block;
    uint8_t show_header;
    uint8_t compact_mode;
};

void egui_view_skeleton_loader_init(egui_view_t *self);
void egui_view_skeleton_loader_set_snapshots(egui_view_t *self, const egui_view_skeleton_loader_snapshot_t *snapshots, uint8_t snapshot_count);
void egui_view_skeleton_loader_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index);
uint8_t egui_view_skeleton_loader_get_current_snapshot(egui_view_t *self);
void egui_view_skeleton_loader_set_focus_block(egui_view_t *self, uint8_t block_index);
void egui_view_skeleton_loader_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_skeleton_loader_set_show_header(egui_view_t *self, uint8_t show_header);
void egui_view_skeleton_loader_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_skeleton_loader_set_palette(
        egui_view_t *self,
        egui_color_t surface_color,
        egui_color_t border_color,
        egui_color_t text_color,
        egui_color_t muted_text_color,
        egui_color_t active_color);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_SKELETON_LOADER_H_ */

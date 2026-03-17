#ifndef _EGUI_VIEW_SKELETON_H_
#define _EGUI_VIEW_SKELETON_H_

#include "core/egui_timer.h"
#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_SKELETON_MAX_BLOCKS    12
#define EGUI_VIEW_SKELETON_MAX_SNAPSHOTS 4

#define EGUI_VIEW_SKELETON_ANIM_NONE  0
#define EGUI_VIEW_SKELETON_ANIM_WAVE  1
#define EGUI_VIEW_SKELETON_ANIM_PULSE 2

typedef struct egui_view_skeleton_block egui_view_skeleton_block_t;
struct egui_view_skeleton_block
{
    uint8_t x;
    uint8_t y;
    uint8_t width;
    uint8_t height;
    uint8_t radius;
};

typedef struct egui_view_skeleton_snapshot egui_view_skeleton_snapshot_t;
struct egui_view_skeleton_snapshot
{
    const char *title;
    const char *footer;
    const egui_view_skeleton_block_t *blocks;
    uint8_t block_count;
    uint8_t emphasis_block;
};

typedef struct egui_view_skeleton egui_view_skeleton_t;
struct egui_view_skeleton
{
    egui_view_t base;
    egui_timer_t anim_timer;
    const egui_view_skeleton_snapshot_t *snapshots;
    const egui_font_t *font;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t block_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t accent_color;
    uint8_t snapshot_count;
    uint8_t current_snapshot;
    uint8_t emphasis_block;
    uint8_t show_footer;
    uint8_t compact_mode;
    uint8_t locked_mode;
    uint8_t animation_mode;
    uint8_t anim_phase;
    uint8_t timer_started;
};

void egui_view_skeleton_init(egui_view_t *self);
void egui_view_skeleton_set_snapshots(egui_view_t *self, const egui_view_skeleton_snapshot_t *snapshots, uint8_t snapshot_count);
void egui_view_skeleton_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index);
uint8_t egui_view_skeleton_get_current_snapshot(egui_view_t *self);
void egui_view_skeleton_set_emphasis_block(egui_view_t *self, uint8_t block_index);
void egui_view_skeleton_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_skeleton_set_show_footer(egui_view_t *self, uint8_t show_footer);
void egui_view_skeleton_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_skeleton_set_locked_mode(egui_view_t *self, uint8_t locked_mode);
void egui_view_skeleton_set_animation_mode(egui_view_t *self, uint8_t animation_mode);
void egui_view_skeleton_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t block_color, egui_color_t text_color,
                                    egui_color_t muted_text_color, egui_color_t accent_color);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_SKELETON_H_ */

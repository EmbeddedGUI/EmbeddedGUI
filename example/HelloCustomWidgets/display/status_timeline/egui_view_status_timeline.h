#ifndef _EGUI_VIEW_STATUS_TIMELINE_H_
#define _EGUI_VIEW_STATUS_TIMELINE_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_STATUS_TIMELINE_MAX_STEPS     5
#define EGUI_VIEW_STATUS_TIMELINE_MAX_SNAPSHOTS 3

typedef struct egui_view_status_timeline_step egui_view_status_timeline_step_t;
struct egui_view_status_timeline_step
{
    const char *title;
    const char *tag;
};

typedef struct egui_view_status_timeline_snapshot egui_view_status_timeline_snapshot_t;
struct egui_view_status_timeline_snapshot
{
    const char *title;
    const egui_view_status_timeline_step_t *steps;
    uint8_t step_count;
    uint8_t current_step;
};

typedef struct egui_view_status_timeline egui_view_status_timeline_t;
struct egui_view_status_timeline
{
    egui_view_t base;
    const egui_view_status_timeline_snapshot_t *snapshots;
    const egui_font_t *font;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t active_color;
    egui_color_t done_color;
    uint8_t snapshot_count;
    uint8_t current_snapshot;
    uint8_t focus_step;
    uint8_t show_header;
    uint8_t compact_mode;
};

void egui_view_status_timeline_init(egui_view_t *self);
void egui_view_status_timeline_set_snapshots(egui_view_t *self, const egui_view_status_timeline_snapshot_t *snapshots, uint8_t snapshot_count);
void egui_view_status_timeline_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index);
uint8_t egui_view_status_timeline_get_current_snapshot(egui_view_t *self);
void egui_view_status_timeline_set_focus_step(egui_view_t *self, uint8_t step_index);
void egui_view_status_timeline_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_status_timeline_set_show_header(egui_view_t *self, uint8_t show_header);
void egui_view_status_timeline_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_status_timeline_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                           egui_color_t muted_text_color, egui_color_t active_color, egui_color_t done_color);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_STATUS_TIMELINE_H_ */

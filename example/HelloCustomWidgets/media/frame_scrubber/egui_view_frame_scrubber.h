#ifndef _EGUI_VIEW_FRAME_SCRUBBER_H_
#define _EGUI_VIEW_FRAME_SCRUBBER_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_FRAME_SCRUBBER_MAX_SNAPSHOTS 8

typedef struct egui_view_frame_scrubber_snapshot egui_view_frame_scrubber_snapshot_t;
struct egui_view_frame_scrubber_snapshot
{
    const char *title;
    const char *status;
    const char *summary;
    const char *footer;
    uint8_t playhead_index;
    uint8_t marker_index;
    uint8_t accent_mode;
};

typedef struct egui_view_frame_scrubber egui_view_frame_scrubber_t;
struct egui_view_frame_scrubber
{
    egui_view_t base;
    const egui_view_frame_scrubber_snapshot_t *snapshots;
    const egui_font_t *font;
    uint8_t snapshot_count;
    uint8_t current_index;
    uint8_t last_zone;
};

void egui_view_frame_scrubber_set_snapshots(egui_view_t *self, const egui_view_frame_scrubber_snapshot_t *snapshots, uint8_t snapshot_count);
void egui_view_frame_scrubber_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_frame_scrubber_init(egui_view_t *self);

#ifdef __cplusplus
}
#endif

#endif

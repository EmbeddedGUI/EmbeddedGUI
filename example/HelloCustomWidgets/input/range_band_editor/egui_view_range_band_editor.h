#ifndef _EGUI_VIEW_RANGE_BAND_EDITOR_H_
#define _EGUI_VIEW_RANGE_BAND_EDITOR_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_RANGE_BAND_EDITOR_MAX_SNAPSHOTS 6

typedef struct egui_view_range_band_editor_snapshot egui_view_range_band_editor_snapshot_t;
struct egui_view_range_band_editor_snapshot
{
    const char *title;
    const char *status;
    const char *summary;
    const char *footer;
    uint8_t range_start;
    uint8_t range_end;
    uint8_t focus_tick;
    uint8_t accent_mode;
};

typedef struct egui_view_range_band_editor egui_view_range_band_editor_t;
struct egui_view_range_band_editor
{
    egui_view_t base;
    const egui_view_range_band_editor_snapshot_t *primary_snapshots;
    const egui_view_range_band_editor_snapshot_t *compact_snapshots;
    const egui_view_range_band_editor_snapshot_t *locked_snapshots;
    const egui_font_t *font;
    uint8_t primary_snapshot_count;
    uint8_t compact_snapshot_count;
    uint8_t locked_snapshot_count;
    uint8_t current_primary;
    uint8_t current_compact;
    uint8_t current_locked;
    uint8_t last_zone;
};

void egui_view_range_band_editor_set_primary_snapshots(
        egui_view_t *self,
        const egui_view_range_band_editor_snapshot_t *snapshots,
        uint8_t snapshot_count);
void egui_view_range_band_editor_set_compact_snapshots(
        egui_view_t *self,
        const egui_view_range_band_editor_snapshot_t *snapshots,
        uint8_t snapshot_count);
void egui_view_range_band_editor_set_locked_snapshots(
        egui_view_t *self,
        const egui_view_range_band_editor_snapshot_t *snapshots,
        uint8_t snapshot_count);
void egui_view_range_band_editor_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_range_band_editor_init(egui_view_t *self);

#ifdef __cplusplus
}
#endif

#endif

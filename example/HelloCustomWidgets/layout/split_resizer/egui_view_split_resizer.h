#ifndef _EGUI_VIEW_SPLIT_RESIZER_H_
#define _EGUI_VIEW_SPLIT_RESIZER_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_SPLIT_RESIZER_MAX_SNAPSHOTS 6

typedef struct egui_view_split_resizer_snapshot egui_view_split_resizer_snapshot_t;
struct egui_view_split_resizer_snapshot
{
    const char *title;
    const char *status;
    const char *summary;
    const char *footer;
    uint8_t primary_ratio;
    uint8_t secondary_ratio;
    uint8_t accent_mode;
    uint8_t vertical_mode;
};

typedef struct egui_view_split_resizer egui_view_split_resizer_t;
struct egui_view_split_resizer
{
    egui_view_t base;
    const egui_view_split_resizer_snapshot_t *primary_snapshots;
    const egui_view_split_resizer_snapshot_t *column_snapshots;
    const egui_view_split_resizer_snapshot_t *locked_snapshots;
    const egui_font_t *font;
    uint8_t primary_snapshot_count;
    uint8_t column_snapshot_count;
    uint8_t locked_snapshot_count;
    uint8_t current_primary;
    uint8_t current_column;
    uint8_t current_locked;
    uint8_t last_zone;
};

void egui_view_split_resizer_set_primary_snapshots(egui_view_t *self, const egui_view_split_resizer_snapshot_t *snapshots, uint8_t snapshot_count);
void egui_view_split_resizer_set_column_snapshots(egui_view_t *self, const egui_view_split_resizer_snapshot_t *snapshots, uint8_t snapshot_count);
void egui_view_split_resizer_set_locked_snapshots(egui_view_t *self, const egui_view_split_resizer_snapshot_t *snapshots, uint8_t snapshot_count);
void egui_view_split_resizer_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_split_resizer_init(egui_view_t *self);

#ifdef __cplusplus
}
#endif

#endif

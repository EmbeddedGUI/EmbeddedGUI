#ifndef _EGUI_VIEW_CONVERSION_FUNNEL_H_
#define _EGUI_VIEW_CONVERSION_FUNNEL_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_CONVERSION_FUNNEL_MAX_SNAPSHOTS 6

typedef struct egui_view_conversion_funnel_snapshot egui_view_conversion_funnel_snapshot_t;
struct egui_view_conversion_funnel_snapshot
{
    const char *title;
    const char *status;
    const char *summary;
    const char *footer;
    uint8_t profile;
    uint8_t focus_stage;
    uint8_t accent_mode;
    uint8_t compact_mode;
};

typedef struct egui_view_conversion_funnel egui_view_conversion_funnel_t;
struct egui_view_conversion_funnel
{
    egui_view_t base;
    const egui_view_conversion_funnel_snapshot_t *primary_snapshots;
    const egui_view_conversion_funnel_snapshot_t *compact_snapshots;
    const egui_view_conversion_funnel_snapshot_t *locked_snapshots;
    const egui_font_t *font;
    uint8_t primary_snapshot_count;
    uint8_t compact_snapshot_count;
    uint8_t locked_snapshot_count;
    uint8_t current_primary;
    uint8_t current_compact;
    uint8_t current_locked;
    uint8_t last_zone;
};

void egui_view_conversion_funnel_set_primary_snapshots(egui_view_t *self, const egui_view_conversion_funnel_snapshot_t *snapshots, uint8_t snapshot_count);
void egui_view_conversion_funnel_set_compact_snapshots(egui_view_t *self, const egui_view_conversion_funnel_snapshot_t *snapshots, uint8_t snapshot_count);
void egui_view_conversion_funnel_set_locked_snapshots(egui_view_t *self, const egui_view_conversion_funnel_snapshot_t *snapshots, uint8_t snapshot_count);
void egui_view_conversion_funnel_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_conversion_funnel_init(egui_view_t *self);

#ifdef __cplusplus
}
#endif

#endif

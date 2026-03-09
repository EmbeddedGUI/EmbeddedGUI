#ifndef _EGUI_VIEW_SANKEY_FLOW_H_
#define _EGUI_VIEW_SANKEY_FLOW_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_SANKEY_FLOW_MAX_SNAPSHOTS 6

typedef struct egui_view_sankey_flow_snapshot egui_view_sankey_flow_snapshot_t;
struct egui_view_sankey_flow_snapshot
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

typedef struct egui_view_sankey_flow egui_view_sankey_flow_t;
struct egui_view_sankey_flow
{
    egui_view_t base;
    const egui_view_sankey_flow_snapshot_t *primary_snapshots;
    const egui_view_sankey_flow_snapshot_t *compact_snapshots;
    const egui_view_sankey_flow_snapshot_t *locked_snapshots;
    const egui_font_t *font;
    uint8_t primary_snapshot_count;
    uint8_t compact_snapshot_count;
    uint8_t locked_snapshot_count;
    uint8_t current_primary;
    uint8_t current_compact;
    uint8_t current_locked;
    uint8_t last_zone;
};

void egui_view_sankey_flow_set_primary_snapshots(egui_view_t *self, const egui_view_sankey_flow_snapshot_t *snapshots, uint8_t snapshot_count);
void egui_view_sankey_flow_set_compact_snapshots(egui_view_t *self, const egui_view_sankey_flow_snapshot_t *snapshots, uint8_t snapshot_count);
void egui_view_sankey_flow_set_locked_snapshots(egui_view_t *self, const egui_view_sankey_flow_snapshot_t *snapshots, uint8_t snapshot_count);
void egui_view_sankey_flow_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_sankey_flow_init(egui_view_t *self);

#ifdef __cplusplus
}
#endif

#endif

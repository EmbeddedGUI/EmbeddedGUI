#ifndef _EGUI_VIEW_SIGNAL_BEACON_H_
#define _EGUI_VIEW_SIGNAL_BEACON_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_SIGNAL_BEACON_MAX_SNAPSHOTS 6

typedef struct egui_view_signal_beacon_snapshot egui_view_signal_beacon_snapshot_t;
struct egui_view_signal_beacon_snapshot
{
    const char *title;
    const char *status;
    const char *caption;
    const char *footer;
    uint8_t signal_level;
    uint8_t focus_node;
    uint8_t is_alert;
};

typedef struct egui_view_signal_beacon egui_view_signal_beacon_t;
struct egui_view_signal_beacon
{
    egui_view_t base;
    const egui_view_signal_beacon_snapshot_t *snapshots;
    const egui_font_t *font;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t accent_color;
    egui_color_t warn_color;
    egui_color_t critical_color;
    egui_color_t node_fill_color;
    uint8_t snapshot_count;
    uint8_t current_snapshot;
    uint8_t compact_mode;
    uint8_t locked_mode;
};

void egui_view_signal_beacon_set_snapshots(egui_view_t *self, const egui_view_signal_beacon_snapshot_t *snapshots, uint8_t snapshot_count);
void egui_view_signal_beacon_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index);
uint8_t egui_view_signal_beacon_get_current_snapshot(egui_view_t *self);
void egui_view_signal_beacon_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_signal_beacon_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_signal_beacon_set_locked_mode(egui_view_t *self, uint8_t locked_mode);
void egui_view_signal_beacon_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                         egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t warn_color, egui_color_t critical_color,
                                         egui_color_t node_fill_color);
void egui_view_signal_beacon_init(egui_view_t *self);

#ifdef __cplusplus
}
#endif

#endif

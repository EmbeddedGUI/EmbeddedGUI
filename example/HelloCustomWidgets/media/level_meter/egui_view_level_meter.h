#ifndef _EGUI_VIEW_LEVEL_METER_H_
#define _EGUI_VIEW_LEVEL_METER_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_LEVEL_METER_MAX_CHANNELS 6
#define EGUI_VIEW_LEVEL_METER_MAX_SNAPSHOTS 3

typedef struct egui_view_level_meter_channel egui_view_level_meter_channel_t;
struct egui_view_level_meter_channel
{
    const char *label;
    uint8_t level;
    uint8_t peak;
    uint8_t status;
};

typedef struct egui_view_level_meter_snapshot egui_view_level_meter_snapshot_t;
struct egui_view_level_meter_snapshot
{
    const char *title;
    const egui_view_level_meter_channel_t *channels;
    uint8_t channel_count;
    uint8_t focus_channel;
};

typedef struct egui_view_level_meter egui_view_level_meter_t;
struct egui_view_level_meter
{
    egui_view_t base;
    const egui_view_level_meter_snapshot_t *snapshots;
    const egui_font_t *font;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t active_color;
    uint8_t snapshot_count;
    uint8_t current_snapshot;
    uint8_t focus_channel;
    uint8_t show_header;
    uint8_t compact_mode;
};

void egui_view_level_meter_init(egui_view_t *self);
void egui_view_level_meter_set_snapshots(egui_view_t *self, const egui_view_level_meter_snapshot_t *snapshots, uint8_t snapshot_count);
void egui_view_level_meter_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index);
uint8_t egui_view_level_meter_get_current_snapshot(egui_view_t *self);
void egui_view_level_meter_set_focus_channel(egui_view_t *self, uint8_t channel_index);
void egui_view_level_meter_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_level_meter_set_show_header(egui_view_t *self, uint8_t show_header);
void egui_view_level_meter_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_level_meter_set_palette(
        egui_view_t *self,
        egui_color_t surface_color,
        egui_color_t border_color,
        egui_color_t text_color,
        egui_color_t muted_text_color,
        egui_color_t active_color);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_LEVEL_METER_H_ */

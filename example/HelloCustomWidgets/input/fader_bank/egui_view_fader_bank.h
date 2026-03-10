#ifndef _EGUI_VIEW_FADER_BANK_H_
#define _EGUI_VIEW_FADER_BANK_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_FADER_BANK_MAX_CHANNELS 5
#define EGUI_VIEW_FADER_BANK_MAX_SNAPSHOTS 3

typedef struct egui_view_fader_bank_channel egui_view_fader_bank_channel_t;
struct egui_view_fader_bank_channel
{
    const char *label;
    uint8_t level;
    uint8_t mute_on;
    uint8_t solo_on;
    uint8_t accent;
};

typedef struct egui_view_fader_bank_snapshot egui_view_fader_bank_snapshot_t;
struct egui_view_fader_bank_snapshot
{
    const char *title;
    const egui_view_fader_bank_channel_t *channels;
    uint8_t channel_count;
    uint8_t focus_channel;
};

typedef struct egui_view_fader_bank egui_view_fader_bank_t;
struct egui_view_fader_bank
{
    egui_view_t base;
    const egui_view_fader_bank_snapshot_t *snapshots;
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

void egui_view_fader_bank_init(egui_view_t *self);
void egui_view_fader_bank_set_snapshots(egui_view_t *self, const egui_view_fader_bank_snapshot_t *snapshots, uint8_t snapshot_count);
void egui_view_fader_bank_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index);
uint8_t egui_view_fader_bank_get_current_snapshot(egui_view_t *self);
void egui_view_fader_bank_set_focus_channel(egui_view_t *self, uint8_t channel_index);
void egui_view_fader_bank_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_fader_bank_set_show_header(egui_view_t *self, uint8_t show_header);
void egui_view_fader_bank_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_fader_bank_set_palette(
        egui_view_t *self,
        egui_color_t surface_color,
        egui_color_t border_color,
        egui_color_t text_color,
        egui_color_t muted_text_color,
        egui_color_t active_color);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_FADER_BANK_H_ */

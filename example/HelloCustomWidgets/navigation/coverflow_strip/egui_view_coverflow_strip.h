#ifndef _EGUI_VIEW_COVERFLOW_STRIP_H_
#define _EGUI_VIEW_COVERFLOW_STRIP_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_COVERFLOW_STRIP_MAX_SNAPSHOTS 8

typedef struct egui_view_coverflow_strip_snapshot egui_view_coverflow_strip_snapshot_t;
struct egui_view_coverflow_strip_snapshot
{
    const char *title;
    const char *status;
    const char *summary;
    const char *footer;
    const char *left_hint;
    const char *right_hint;
    uint8_t accent_mode;
};

typedef struct egui_view_coverflow_strip egui_view_coverflow_strip_t;
struct egui_view_coverflow_strip
{
    egui_view_t base;
    const egui_view_coverflow_strip_snapshot_t *snapshots;
    const egui_font_t *font;
    uint8_t snapshot_count;
    uint8_t current_index;
    uint8_t last_zone;
};

void egui_view_coverflow_strip_set_snapshots(
        egui_view_t *self,
        const egui_view_coverflow_strip_snapshot_t *snapshots,
        uint8_t snapshot_count);
void egui_view_coverflow_strip_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_coverflow_strip_init(egui_view_t *self);

#ifdef __cplusplus
}
#endif

#endif

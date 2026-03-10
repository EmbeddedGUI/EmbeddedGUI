#ifndef _EGUI_VIEW_TAG_CLOUD_H_
#define _EGUI_VIEW_TAG_CLOUD_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_TAG_CLOUD_MAX_ITEMS 6
#define EGUI_VIEW_TAG_CLOUD_MAX_SNAPSHOTS 3

typedef struct egui_view_tag_cloud_snapshot egui_view_tag_cloud_snapshot_t;
struct egui_view_tag_cloud_snapshot
{
    const char *title;
    const char **words;
    const uint8_t *weights;
    uint8_t item_count;
    uint8_t current_item;
};

typedef struct egui_view_tag_cloud egui_view_tag_cloud_t;
struct egui_view_tag_cloud
{
    egui_view_t base;
    const egui_view_tag_cloud_snapshot_t *snapshots;
    const egui_font_t *font;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t active_color;
    uint8_t snapshot_count;
    uint8_t current_snapshot;
    uint8_t focus_item;
    uint8_t show_header;
    uint8_t compact_mode;
};

void egui_view_tag_cloud_init(egui_view_t *self);
void egui_view_tag_cloud_set_snapshots(egui_view_t *self, const egui_view_tag_cloud_snapshot_t *snapshots, uint8_t snapshot_count);
void egui_view_tag_cloud_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index);
uint8_t egui_view_tag_cloud_get_current_snapshot(egui_view_t *self);
void egui_view_tag_cloud_set_focus_item(egui_view_t *self, uint8_t item_index);
void egui_view_tag_cloud_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_tag_cloud_set_show_header(egui_view_t *self, uint8_t show_header);
void egui_view_tag_cloud_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_tag_cloud_set_palette(
        egui_view_t *self,
        egui_color_t surface_color,
        egui_color_t border_color,
        egui_color_t text_color,
        egui_color_t muted_text_color,
        egui_color_t active_color);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_TAG_CLOUD_H_ */

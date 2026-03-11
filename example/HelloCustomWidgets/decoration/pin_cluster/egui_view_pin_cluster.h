#ifndef _EGUI_VIEW_PIN_CLUSTER_H_
#define _EGUI_VIEW_PIN_CLUSTER_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_PIN_CLUSTER_MAX_PINS      6
#define EGUI_VIEW_PIN_CLUSTER_MAX_SNAPSHOTS 3

typedef struct egui_view_pin_cluster_pin egui_view_pin_cluster_pin_t;
struct egui_view_pin_cluster_pin
{
    uint8_t x;
    uint8_t y;
    uint8_t radius;
    const char *label;
    uint8_t tone;
};

typedef struct egui_view_pin_cluster_snapshot egui_view_pin_cluster_snapshot_t;
struct egui_view_pin_cluster_snapshot
{
    const char *title;
    const egui_view_pin_cluster_pin_t *pins;
    uint8_t pin_count;
    uint8_t focus_pin;
    uint8_t badge_count;
};

typedef struct egui_view_pin_cluster egui_view_pin_cluster_t;
struct egui_view_pin_cluster
{
    egui_view_t base;
    const egui_view_pin_cluster_snapshot_t *snapshots;
    const egui_font_t *font;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t active_color;
    uint8_t snapshot_count;
    uint8_t current_snapshot;
    uint8_t focus_pin;
    uint8_t show_header;
    uint8_t compact_mode;
};

void egui_view_pin_cluster_init(egui_view_t *self);
void egui_view_pin_cluster_set_snapshots(egui_view_t *self, const egui_view_pin_cluster_snapshot_t *snapshots, uint8_t snapshot_count);
void egui_view_pin_cluster_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index);
uint8_t egui_view_pin_cluster_get_current_snapshot(egui_view_t *self);
void egui_view_pin_cluster_set_focus_pin(egui_view_t *self, uint8_t pin_index);
void egui_view_pin_cluster_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_pin_cluster_set_show_header(egui_view_t *self, uint8_t show_header);
void egui_view_pin_cluster_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_pin_cluster_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                       egui_color_t muted_text_color, egui_color_t active_color);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_PIN_CLUSTER_H_ */

#ifndef _EGUI_VIEW_LAYER_STACK_H_
#define _EGUI_VIEW_LAYER_STACK_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_LAYER_STACK_MAX_SNAPSHOTS 6

typedef struct egui_view_layer_stack_snapshot egui_view_layer_stack_snapshot_t;
struct egui_view_layer_stack_snapshot
{
    const char *title;
    const char *status;
    const char *summary;
    const char *footer;
    uint8_t active_layer;
    uint8_t visible_mask;
    uint8_t accent_mode;
};

typedef struct egui_view_layer_stack egui_view_layer_stack_t;
struct egui_view_layer_stack
{
    egui_view_t base;
    const egui_view_layer_stack_snapshot_t *snapshots;
    const egui_font_t *font;
    egui_color_t surface_color;
    egui_color_t panel_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t accent_color;
    egui_color_t warn_color;
    egui_color_t lock_color;
    egui_color_t focus_color;
    uint8_t snapshot_count;
    uint8_t current_snapshot;
    uint8_t compact_mode;
    uint8_t locked_mode;
};

void egui_view_layer_stack_set_snapshots(egui_view_t *self, const egui_view_layer_stack_snapshot_t *snapshots, uint8_t snapshot_count);
void egui_view_layer_stack_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index);
uint8_t egui_view_layer_stack_get_current_snapshot(egui_view_t *self);
void egui_view_layer_stack_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_layer_stack_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_layer_stack_set_locked_mode(egui_view_t *self, uint8_t locked_mode);
void egui_view_layer_stack_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t panel_color, egui_color_t border_color,
                                       egui_color_t text_color, egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t warn_color,
                                       egui_color_t lock_color, egui_color_t focus_color);
void egui_view_layer_stack_init(egui_view_t *self);

#ifdef __cplusplus
}
#endif

#endif

#ifndef _EGUI_VIEW_TOAST_STACK_H_
#define _EGUI_VIEW_TOAST_STACK_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_TOAST_STACK_MAX_SNAPSHOTS 4

typedef struct egui_view_toast_stack_snapshot egui_view_toast_stack_snapshot_t;
struct egui_view_toast_stack_snapshot
{
    const char *title;
    const char *body;
    const char *action;
    const char *meta;
    const char *mid_title;
    const char *back_title;
    uint8_t severity;
    uint8_t closable;
};

typedef struct egui_view_toast_stack egui_view_toast_stack_t;
struct egui_view_toast_stack
{
    egui_view_t base;
    const egui_view_toast_stack_snapshot_t *snapshots;
    const egui_font_t *font;
    const egui_font_t *meta_font;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t accent_color;
    egui_color_t info_color;
    egui_color_t success_color;
    egui_color_t warning_color;
    egui_color_t error_color;
    uint8_t snapshot_count;
    uint8_t current_snapshot;
    uint8_t compact_mode;
    uint8_t locked_mode;
};

void egui_view_toast_stack_init(egui_view_t *self);
void egui_view_toast_stack_set_snapshots(egui_view_t *self, const egui_view_toast_stack_snapshot_t *snapshots, uint8_t snapshot_count);
void egui_view_toast_stack_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index);
uint8_t egui_view_toast_stack_get_current_snapshot(egui_view_t *self);
void egui_view_toast_stack_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_toast_stack_set_meta_font(egui_view_t *self, const egui_font_t *font);
void egui_view_toast_stack_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_toast_stack_set_locked_mode(egui_view_t *self, uint8_t locked_mode);
void egui_view_toast_stack_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                       egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t info_color, egui_color_t success_color,
                                       egui_color_t warning_color, egui_color_t error_color);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_TOAST_STACK_H_ */

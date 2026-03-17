#ifndef _EGUI_VIEW_SCROLL_BAR_H_
#define _EGUI_VIEW_SCROLL_BAR_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_SCROLL_BAR_PART_NONE     0
#define EGUI_VIEW_SCROLL_BAR_PART_DECREASE 1
#define EGUI_VIEW_SCROLL_BAR_PART_THUMB    2
#define EGUI_VIEW_SCROLL_BAR_PART_INCREASE 3
#define EGUI_VIEW_SCROLL_BAR_PART_TRACK    4

typedef void (*egui_view_on_scroll_bar_changed_listener_t)(egui_view_t *self, egui_dim_t offset, egui_dim_t max_offset, uint8_t part);

typedef struct egui_view_scroll_bar egui_view_scroll_bar_t;
struct egui_view_scroll_bar
{
    egui_view_t base;
    egui_view_on_scroll_bar_changed_listener_t on_changed;
    const egui_font_t *font;
    const egui_font_t *meta_font;
    const char *label;
    const char *helper;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t accent_color;
    egui_color_t preview_color;
    egui_dim_t content_length;
    egui_dim_t viewport_length;
    egui_dim_t offset;
    egui_dim_t line_step;
    egui_dim_t page_step;
    egui_dim_t drag_anchor_y;
    egui_dim_t drag_anchor_offset;
    uint8_t compact_mode;
    uint8_t read_only_mode;
    uint8_t current_part;
    uint8_t pressed_part;
    uint8_t pressed_track_direction;
    uint8_t thumb_dragging;
};

void egui_view_scroll_bar_init(egui_view_t *self);
void egui_view_scroll_bar_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_scroll_bar_set_meta_font(egui_view_t *self, const egui_font_t *font);
void egui_view_scroll_bar_set_label(egui_view_t *self, const char *label);
void egui_view_scroll_bar_set_helper(egui_view_t *self, const char *helper);
void egui_view_scroll_bar_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                      egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t preview_color);
void egui_view_scroll_bar_set_content_metrics(egui_view_t *self, egui_dim_t content_length, egui_dim_t viewport_length);
egui_dim_t egui_view_scroll_bar_get_content_length(egui_view_t *self);
egui_dim_t egui_view_scroll_bar_get_viewport_length(egui_view_t *self);
void egui_view_scroll_bar_set_step_size(egui_view_t *self, egui_dim_t line_step, egui_dim_t page_step);
void egui_view_scroll_bar_set_offset(egui_view_t *self, egui_dim_t offset);
egui_dim_t egui_view_scroll_bar_get_offset(egui_view_t *self);
egui_dim_t egui_view_scroll_bar_get_max_offset(egui_view_t *self);
void egui_view_scroll_bar_set_current_part(egui_view_t *self, uint8_t part);
uint8_t egui_view_scroll_bar_get_current_part(egui_view_t *self);
void egui_view_scroll_bar_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_scroll_bar_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode);
void egui_view_scroll_bar_set_on_changed_listener(egui_view_t *self, egui_view_on_scroll_bar_changed_listener_t listener);
uint8_t egui_view_scroll_bar_get_part_region(egui_view_t *self, uint8_t part, egui_region_t *region);
uint8_t egui_view_scroll_bar_handle_navigation_key(egui_view_t *self, uint8_t key_code);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_SCROLL_BAR_H_ */

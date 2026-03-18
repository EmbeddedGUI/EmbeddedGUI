#ifndef _EGUI_VIEW_ANNOTATED_SCROLL_BAR_H_
#define _EGUI_VIEW_ANNOTATED_SCROLL_BAR_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_NONE     0
#define EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_DECREASE 1
#define EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL     2
#define EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_INCREASE 3
#define EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_MARKER   4

#define EGUI_VIEW_ANNOTATED_SCROLL_BAR_MAX_MARKERS 10

typedef struct egui_view_annotated_scroll_bar_marker egui_view_annotated_scroll_bar_marker_t;
struct egui_view_annotated_scroll_bar_marker
{
    const char *label;
    const char *detail;
    egui_dim_t offset;
    egui_color_t accent_color;
};

typedef void (*egui_view_on_annotated_scroll_bar_changed_listener_t)(egui_view_t *self, egui_dim_t offset, egui_dim_t max_offset, uint8_t active_marker,
                                                                     uint8_t part);

typedef struct egui_view_annotated_scroll_bar egui_view_annotated_scroll_bar_t;
struct egui_view_annotated_scroll_bar
{
    egui_view_t base;
    egui_view_on_annotated_scroll_bar_changed_listener_t on_changed;
    const egui_font_t *font;
    const egui_font_t *meta_font;
    const char *title;
    const char *helper;
    const egui_view_annotated_scroll_bar_marker_t *markers;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t accent_color;
    egui_color_t preview_color;
    egui_dim_t content_length;
    egui_dim_t viewport_length;
    egui_dim_t offset;
    egui_dim_t small_change;
    egui_dim_t large_change;
    uint8_t marker_count;
    uint8_t active_marker;
    uint8_t current_part;
    uint8_t pressed_part;
    uint8_t pressed_marker;
    uint8_t rail_dragging;
    uint8_t compact_mode;
    uint8_t read_only_mode;
};

void egui_view_annotated_scroll_bar_init(egui_view_t *self);
void egui_view_annotated_scroll_bar_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_annotated_scroll_bar_set_meta_font(egui_view_t *self, const egui_font_t *font);
void egui_view_annotated_scroll_bar_set_title(egui_view_t *self, const char *title);
void egui_view_annotated_scroll_bar_set_helper(egui_view_t *self, const char *helper);
void egui_view_annotated_scroll_bar_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                                egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t preview_color);
void egui_view_annotated_scroll_bar_set_markers(egui_view_t *self, const egui_view_annotated_scroll_bar_marker_t *markers, uint8_t marker_count);
uint8_t egui_view_annotated_scroll_bar_get_marker_count(egui_view_t *self);
void egui_view_annotated_scroll_bar_set_content_metrics(egui_view_t *self, egui_dim_t content_length, egui_dim_t viewport_length);
egui_dim_t egui_view_annotated_scroll_bar_get_content_length(egui_view_t *self);
egui_dim_t egui_view_annotated_scroll_bar_get_viewport_length(egui_view_t *self);
void egui_view_annotated_scroll_bar_set_step_size(egui_view_t *self, egui_dim_t small_change, egui_dim_t large_change);
void egui_view_annotated_scroll_bar_set_offset(egui_view_t *self, egui_dim_t offset);
egui_dim_t egui_view_annotated_scroll_bar_get_offset(egui_view_t *self);
egui_dim_t egui_view_annotated_scroll_bar_get_max_offset(egui_view_t *self);
uint8_t egui_view_annotated_scroll_bar_get_active_marker(egui_view_t *self);
void egui_view_annotated_scroll_bar_set_current_part(egui_view_t *self, uint8_t part);
uint8_t egui_view_annotated_scroll_bar_get_current_part(egui_view_t *self);
void egui_view_annotated_scroll_bar_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_annotated_scroll_bar_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode);
void egui_view_annotated_scroll_bar_set_on_changed_listener(egui_view_t *self, egui_view_on_annotated_scroll_bar_changed_listener_t listener);
uint8_t egui_view_annotated_scroll_bar_get_part_region(egui_view_t *self, uint8_t part, egui_region_t *region);
uint8_t egui_view_annotated_scroll_bar_get_marker_region(egui_view_t *self, uint8_t index, egui_region_t *region);
uint8_t egui_view_annotated_scroll_bar_handle_navigation_key(egui_view_t *self, uint8_t key_code);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_ANNOTATED_SCROLL_BAR_H_ */

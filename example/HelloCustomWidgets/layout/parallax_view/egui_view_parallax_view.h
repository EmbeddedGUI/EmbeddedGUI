#ifndef _EGUI_VIEW_PARALLAX_VIEW_H_
#define _EGUI_VIEW_PARALLAX_VIEW_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_PARALLAX_VIEW_MAX_ROWS 6

#define EGUI_VIEW_PARALLAX_VIEW_TONE_ACCENT  0
#define EGUI_VIEW_PARALLAX_VIEW_TONE_SUCCESS 1
#define EGUI_VIEW_PARALLAX_VIEW_TONE_WARNING 2
#define EGUI_VIEW_PARALLAX_VIEW_TONE_NEUTRAL 3

#define EGUI_VIEW_PARALLAX_VIEW_INDEX_NONE 0xFF

typedef struct egui_view_parallax_view_row egui_view_parallax_view_row_t;
struct egui_view_parallax_view_row
{
    const char *title;
    const char *meta;
    egui_dim_t anchor_offset;
    uint8_t tone;
};

typedef void (*egui_view_on_parallax_view_changed_listener_t)(egui_view_t *self, egui_dim_t offset, uint8_t active_row);

typedef struct egui_view_parallax_view egui_view_parallax_view_t;
struct egui_view_parallax_view
{
    egui_view_t base;
    const egui_view_parallax_view_row_t *rows;
    const egui_font_t *font;
    const egui_font_t *meta_font;
    egui_view_on_parallax_view_changed_listener_t on_changed;
    const char *title;
    const char *subtitle;
    const char *footer_prefix;
    egui_color_t surface_color;
    egui_color_t panel_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t accent_color;
    egui_color_t success_color;
    egui_color_t warning_color;
    egui_color_t neutral_color;
    egui_dim_t content_length;
    egui_dim_t viewport_length;
    egui_dim_t offset;
    egui_dim_t vertical_shift;
    egui_dim_t line_step;
    egui_dim_t page_step;
    uint8_t row_count;
    uint8_t compact_mode;
    uint8_t locked_mode;
    uint8_t pressed_row;
};

void egui_view_parallax_view_init(egui_view_t *self);
void egui_view_parallax_view_set_rows(egui_view_t *self, const egui_view_parallax_view_row_t *rows, uint8_t row_count);
void egui_view_parallax_view_set_header(egui_view_t *self, const char *title, const char *subtitle, const char *footer_prefix);
void egui_view_parallax_view_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_parallax_view_set_meta_font(egui_view_t *self, const egui_font_t *font);
void egui_view_parallax_view_set_on_changed_listener(egui_view_t *self, egui_view_on_parallax_view_changed_listener_t listener);
void egui_view_parallax_view_set_content_metrics(egui_view_t *self, egui_dim_t content_length, egui_dim_t viewport_length);
egui_dim_t egui_view_parallax_view_get_content_length(egui_view_t *self);
egui_dim_t egui_view_parallax_view_get_viewport_length(egui_view_t *self);
void egui_view_parallax_view_set_offset(egui_view_t *self, egui_dim_t offset);
egui_dim_t egui_view_parallax_view_get_offset(egui_view_t *self);
egui_dim_t egui_view_parallax_view_get_max_offset(egui_view_t *self);
void egui_view_parallax_view_set_vertical_shift(egui_view_t *self, egui_dim_t vertical_shift);
egui_dim_t egui_view_parallax_view_get_vertical_shift(egui_view_t *self);
void egui_view_parallax_view_set_step_size(egui_view_t *self, egui_dim_t line_step, egui_dim_t page_step);
void egui_view_parallax_view_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_parallax_view_set_locked_mode(egui_view_t *self, uint8_t locked_mode);
uint8_t egui_view_parallax_view_get_active_row(egui_view_t *self);
uint8_t egui_view_parallax_view_get_row_region(egui_view_t *self, uint8_t row_index, egui_region_t *region);

#ifdef __cplusplus
}
#endif

#endif

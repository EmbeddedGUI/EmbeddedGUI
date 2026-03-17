#ifndef _EGUI_VIEW_TIME_PICKER_H_
#define _EGUI_VIEW_TIME_PICKER_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_TIME_PICKER_SEGMENT_HOUR   0
#define EGUI_VIEW_TIME_PICKER_SEGMENT_MINUTE 1
#define EGUI_VIEW_TIME_PICKER_SEGMENT_PERIOD 2
#define EGUI_VIEW_TIME_PICKER_SEGMENT_NONE   0xFF

typedef void (*egui_view_on_time_picker_changed_listener_t)(egui_view_t *self, uint8_t hour24, uint8_t minute);
typedef void (*egui_view_on_time_picker_open_changed_listener_t)(egui_view_t *self, uint8_t opened);

typedef struct egui_view_time_picker egui_view_time_picker_t;
struct egui_view_time_picker
{
    egui_view_t base;
    egui_view_on_time_picker_changed_listener_t on_time_changed;
    egui_view_on_time_picker_open_changed_listener_t on_open_changed;
    const egui_font_t *font;
    const egui_font_t *meta_font;
    const char *label;
    const char *helper;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t accent_color;
    uint8_t hour24;
    uint8_t minute;
    uint8_t minute_step;
    uint8_t focused_segment;
    uint8_t compact_mode;
    uint8_t read_only_mode;
    uint8_t use_24h;
    uint8_t open_mode;
    uint8_t pressed_part;
};

void egui_view_time_picker_init(egui_view_t *self);
void egui_view_time_picker_set_time(egui_view_t *self, uint8_t hour24, uint8_t minute);
uint8_t egui_view_time_picker_get_hour24(egui_view_t *self);
uint8_t egui_view_time_picker_get_minute(egui_view_t *self);
void egui_view_time_picker_set_minute_step(egui_view_t *self, uint8_t minute_step);
uint8_t egui_view_time_picker_get_minute_step(egui_view_t *self);
void egui_view_time_picker_set_use_24h(egui_view_t *self, uint8_t use_24h);
uint8_t egui_view_time_picker_get_use_24h(egui_view_t *self);
void egui_view_time_picker_set_opened(egui_view_t *self, uint8_t opened);
uint8_t egui_view_time_picker_get_opened(egui_view_t *self);
void egui_view_time_picker_set_focused_segment(egui_view_t *self, uint8_t focused_segment);
uint8_t egui_view_time_picker_get_focused_segment(egui_view_t *self);
void egui_view_time_picker_set_label(egui_view_t *self, const char *label);
void egui_view_time_picker_set_helper(egui_view_t *self, const char *helper);
void egui_view_time_picker_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_time_picker_set_meta_font(egui_view_t *self, const egui_font_t *font);
void egui_view_time_picker_set_on_time_changed_listener(egui_view_t *self, egui_view_on_time_picker_changed_listener_t listener);
void egui_view_time_picker_set_on_open_changed_listener(egui_view_t *self, egui_view_on_time_picker_open_changed_listener_t listener);
void egui_view_time_picker_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_time_picker_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode);
void egui_view_time_picker_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                       egui_color_t muted_text_color, egui_color_t accent_color);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_TIME_PICKER_H_ */

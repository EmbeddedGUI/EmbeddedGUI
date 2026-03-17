#ifndef _EGUI_VIEW_TRANSPORT_BAR_H_
#define _EGUI_VIEW_TRANSPORT_BAR_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_TRANSPORT_BAR_PART_PREVIOUS   0
#define EGUI_VIEW_TRANSPORT_BAR_PART_PLAY_PAUSE 1
#define EGUI_VIEW_TRANSPORT_BAR_PART_NEXT       2
#define EGUI_VIEW_TRANSPORT_BAR_PART_SEEK       3
#define EGUI_VIEW_TRANSPORT_BAR_PART_NONE       0xFF

typedef struct egui_view_transport_bar_snapshot egui_view_transport_bar_snapshot_t;
struct egui_view_transport_bar_snapshot
{
    const char *eyebrow;
    const char *title;
    const char *subtitle;
    const char *footer;
    egui_color_t surface_color;
    egui_color_t accent_color;
    uint16_t position_seconds;
    uint16_t duration_seconds;
    uint8_t playing;
};

typedef void (*egui_view_on_transport_bar_changed_listener_t)(egui_view_t *self, uint16_t position_seconds, uint16_t duration_seconds, uint8_t playing,
                                                              uint8_t current_part);

typedef struct egui_view_transport_bar egui_view_transport_bar_t;
struct egui_view_transport_bar
{
    egui_view_t base;
    egui_view_on_transport_bar_changed_listener_t on_changed;
    const egui_view_transport_bar_snapshot_t *snapshot;
    const egui_font_t *font;
    const egui_font_t *meta_font;
    const char *title;
    const char *helper;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t inactive_color;
    uint16_t position_seconds;
    uint16_t duration_seconds;
    uint8_t playing;
    uint8_t current_part;
    uint8_t compact_mode;
    uint8_t read_only_mode;
    uint8_t pressed_part;
    uint8_t dragging_seek;
};

void egui_view_transport_bar_init(egui_view_t *self);
void egui_view_transport_bar_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_transport_bar_set_meta_font(egui_view_t *self, const egui_font_t *font);
void egui_view_transport_bar_set_title(egui_view_t *self, const char *title);
void egui_view_transport_bar_set_helper(egui_view_t *self, const char *helper);
void egui_view_transport_bar_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                         egui_color_t muted_text_color, egui_color_t inactive_color);
void egui_view_transport_bar_set_snapshot(egui_view_t *self, const egui_view_transport_bar_snapshot_t *snapshot);
const egui_view_transport_bar_snapshot_t *egui_view_transport_bar_get_snapshot(egui_view_t *self);
void egui_view_transport_bar_set_current_part(egui_view_t *self, uint8_t part);
uint8_t egui_view_transport_bar_get_current_part(egui_view_t *self);
void egui_view_transport_bar_set_position_seconds(egui_view_t *self, uint16_t position_seconds);
uint16_t egui_view_transport_bar_get_position_seconds(egui_view_t *self);
void egui_view_transport_bar_set_duration_seconds(egui_view_t *self, uint16_t duration_seconds);
uint16_t egui_view_transport_bar_get_duration_seconds(egui_view_t *self);
void egui_view_transport_bar_set_playing(egui_view_t *self, uint8_t playing);
uint8_t egui_view_transport_bar_get_playing(egui_view_t *self);
void egui_view_transport_bar_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_transport_bar_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode);
void egui_view_transport_bar_set_on_changed_listener(egui_view_t *self, egui_view_on_transport_bar_changed_listener_t listener);
uint8_t egui_view_transport_bar_get_part_region(egui_view_t *self, uint8_t part, egui_region_t *region);
uint8_t egui_view_transport_bar_handle_navigation_key(egui_view_t *self, uint8_t key_code);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_TRANSPORT_BAR_H_ */

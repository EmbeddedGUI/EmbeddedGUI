#ifndef _EGUI_VIEW_CALENDAR_VIEW_H_
#define _EGUI_VIEW_CALENDAR_VIEW_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_CALENDAR_VIEW_PART_NONE 0
#define EGUI_VIEW_CALENDAR_VIEW_PART_GRID 1
#define EGUI_VIEW_CALENDAR_VIEW_PART_PREV 2
#define EGUI_VIEW_CALENDAR_VIEW_PART_NEXT 3

typedef void (*egui_view_on_calendar_view_changed_listener_t)(egui_view_t *self, uint16_t year, uint8_t month, uint8_t start_day, uint8_t end_day,
                                                              uint8_t focus_day, uint8_t editing_range);
typedef void (*egui_view_on_calendar_view_display_month_changed_listener_t)(egui_view_t *self, uint16_t year, uint8_t month);

typedef struct egui_view_calendar_view egui_view_calendar_view_t;
struct egui_view_calendar_view
{
    egui_view_t base;
    egui_view_on_calendar_view_changed_listener_t on_changed;
    egui_view_on_calendar_view_display_month_changed_listener_t on_display_month_changed;
    const egui_font_t *font;
    const egui_font_t *meta_font;
    const char *label;
    const char *helper;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t accent_color;
    egui_color_t today_color;
    uint16_t display_year;
    uint16_t selection_year;
    uint16_t committed_year;
    uint16_t today_year;
    uint8_t display_month;
    uint8_t selection_month;
    uint8_t committed_month;
    uint8_t start_day;
    uint8_t end_day;
    uint8_t committed_start_day;
    uint8_t committed_end_day;
    uint8_t focus_day;
    uint8_t anchor_day;
    uint8_t today_month;
    uint8_t today_day;
    uint8_t first_day_of_week;
    uint8_t compact_mode;
    uint8_t read_only_mode;
    uint8_t editing_range;
    uint8_t current_part;
    uint8_t pressed_part;
    uint8_t pressed_day;
};

void egui_view_calendar_view_init(egui_view_t *self);
void egui_view_calendar_view_set_range(egui_view_t *self, uint16_t year, uint8_t month, uint8_t start_day, uint8_t end_day);
uint16_t egui_view_calendar_view_get_selection_year(egui_view_t *self);
uint8_t egui_view_calendar_view_get_selection_month(egui_view_t *self);
uint8_t egui_view_calendar_view_get_start_day(egui_view_t *self);
uint8_t egui_view_calendar_view_get_end_day(egui_view_t *self);
uint8_t egui_view_calendar_view_get_focus_day(egui_view_t *self);
uint8_t egui_view_calendar_view_get_editing_range(egui_view_t *self);
void egui_view_calendar_view_set_today(egui_view_t *self, uint16_t year, uint8_t month, uint8_t day);
void egui_view_calendar_view_set_first_day_of_week(egui_view_t *self, uint8_t first_day_of_week);
uint8_t egui_view_calendar_view_get_first_day_of_week(egui_view_t *self);
void egui_view_calendar_view_set_display_month(egui_view_t *self, uint16_t year, uint8_t month);
uint16_t egui_view_calendar_view_get_display_year(egui_view_t *self);
uint8_t egui_view_calendar_view_get_display_month(egui_view_t *self);
void egui_view_calendar_view_set_label(egui_view_t *self, const char *label);
void egui_view_calendar_view_set_helper(egui_view_t *self, const char *helper);
void egui_view_calendar_view_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_calendar_view_set_meta_font(egui_view_t *self, const egui_font_t *font);
void egui_view_calendar_view_set_on_changed_listener(egui_view_t *self, egui_view_on_calendar_view_changed_listener_t listener);
void egui_view_calendar_view_set_on_display_month_changed_listener(egui_view_t *self, egui_view_on_calendar_view_display_month_changed_listener_t listener);
void egui_view_calendar_view_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_calendar_view_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode);
void egui_view_calendar_view_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                         egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t today_color);
void egui_view_calendar_view_set_current_part(egui_view_t *self, uint8_t part);
uint8_t egui_view_calendar_view_get_current_part(egui_view_t *self);
uint8_t egui_view_calendar_view_handle_navigation_key(egui_view_t *self, uint8_t key_code);
uint8_t egui_view_calendar_view_get_part_region(egui_view_t *self, uint8_t part, egui_region_t *region);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_CALENDAR_VIEW_H_ */

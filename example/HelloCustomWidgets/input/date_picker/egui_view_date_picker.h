#ifndef _EGUI_VIEW_DATE_PICKER_H_
#define _EGUI_VIEW_DATE_PICKER_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*egui_view_on_date_picker_changed_listener_t)(egui_view_t *self, uint16_t year, uint8_t month, uint8_t day);
typedef void (*egui_view_on_date_picker_open_changed_listener_t)(egui_view_t *self, uint8_t opened);
typedef void (*egui_view_on_date_picker_display_month_changed_listener_t)(egui_view_t *self, uint16_t year, uint8_t month);

typedef struct egui_view_date_picker egui_view_date_picker_t;
struct egui_view_date_picker
{
    egui_view_t base;
    egui_view_on_date_picker_changed_listener_t on_date_changed;
    egui_view_on_date_picker_open_changed_listener_t on_open_changed;
    egui_view_on_date_picker_display_month_changed_listener_t on_display_month_changed;
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
    uint16_t year;
    uint16_t panel_year;
    uint16_t today_year;
    uint8_t month;
    uint8_t panel_month;
    uint8_t day;
    uint8_t today_month;
    uint8_t today_day;
    uint8_t first_day_of_week;
    uint8_t compact_mode;
    uint8_t read_only_mode;
    uint8_t open_mode;
    uint8_t preserve_display_month_on_open;
    uint8_t pressed_part;
    uint8_t pressed_day;
};

void egui_view_date_picker_init(egui_view_t *self);
void egui_view_date_picker_set_date(egui_view_t *self, uint16_t year, uint8_t month, uint8_t day);
uint16_t egui_view_date_picker_get_year(egui_view_t *self);
uint8_t egui_view_date_picker_get_month(egui_view_t *self);
uint8_t egui_view_date_picker_get_day(egui_view_t *self);
void egui_view_date_picker_set_today(egui_view_t *self, uint16_t year, uint8_t month, uint8_t day);
void egui_view_date_picker_set_first_day_of_week(egui_view_t *self, uint8_t first_day_of_week);
uint8_t egui_view_date_picker_get_first_day_of_week(egui_view_t *self);
void egui_view_date_picker_set_display_month(egui_view_t *self, uint16_t year, uint8_t month);
uint16_t egui_view_date_picker_get_display_year(egui_view_t *self);
uint8_t egui_view_date_picker_get_display_month(egui_view_t *self);
void egui_view_date_picker_set_opened(egui_view_t *self, uint8_t opened);
uint8_t egui_view_date_picker_get_opened(egui_view_t *self);
void egui_view_date_picker_set_label(egui_view_t *self, const char *label);
void egui_view_date_picker_set_helper(egui_view_t *self, const char *helper);
void egui_view_date_picker_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_date_picker_set_meta_font(egui_view_t *self, const egui_font_t *font);
void egui_view_date_picker_set_on_date_changed_listener(egui_view_t *self, egui_view_on_date_picker_changed_listener_t listener);
void egui_view_date_picker_set_on_open_changed_listener(egui_view_t *self, egui_view_on_date_picker_open_changed_listener_t listener);
void egui_view_date_picker_set_on_display_month_changed_listener(egui_view_t *self, egui_view_on_date_picker_display_month_changed_listener_t listener);
void egui_view_date_picker_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_date_picker_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode);
void egui_view_date_picker_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                       egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t today_color);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_DATE_PICKER_H_ */

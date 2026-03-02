#ifndef _EGUI_VIEW_MINI_CALENDAR_H_
#define _EGUI_VIEW_MINI_CALENDAR_H_

#include "egui_view.h"
#include "core/egui_theme.h"
#include "font/egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*egui_view_on_date_selected_listener_t)(egui_view_t *self, uint8_t day);

typedef struct egui_view_mini_calendar egui_view_mini_calendar_t;
struct egui_view_mini_calendar
{
    egui_view_t base;

    uint16_t year;
    uint8_t month;             // 1-12
    uint8_t day;               // 1-31, selected day
    uint8_t today_day;         // today highlight
    uint8_t first_day_of_week; // 0=Sunday, 1=Monday

    egui_color_t header_color;
    egui_color_t text_color;
    egui_color_t today_color;
    egui_color_t selected_color;
    egui_color_t weekend_color;

    const egui_font_t *font;
    egui_view_on_date_selected_listener_t on_date_selected;
};

// ============== MiniCalendar Params ==============
typedef struct egui_view_mini_calendar_params egui_view_mini_calendar_params_t;
struct egui_view_mini_calendar_params
{
    egui_region_t region;
    uint16_t year;
    uint8_t month;
    uint8_t day;
};

#define EGUI_VIEW_MINI_CALENDAR_PARAMS_INIT(_name, _x, _y, _w, _h, _year, _month, _day)                                                                        \
    static const egui_view_mini_calendar_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .year = (_year), .month = (_month), .day = (_day)}

void egui_view_mini_calendar_apply_params(egui_view_t *self, const egui_view_mini_calendar_params_t *params);
void egui_view_mini_calendar_init_with_params(egui_view_t *self, const egui_view_mini_calendar_params_t *params);

void egui_view_mini_calendar_set_date(egui_view_t *self, uint16_t year, uint8_t month, uint8_t day);
void egui_view_mini_calendar_set_today(egui_view_t *self, uint8_t day);
void egui_view_mini_calendar_set_first_day_of_week(egui_view_t *self, uint8_t day);
void egui_view_mini_calendar_set_on_date_selected_listener(egui_view_t *self, egui_view_on_date_selected_listener_t listener);
void egui_view_mini_calendar_on_draw(egui_view_t *self);
void egui_view_mini_calendar_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_MINI_CALENDAR_H_ */

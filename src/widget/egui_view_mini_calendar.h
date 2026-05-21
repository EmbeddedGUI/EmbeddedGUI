#ifndef _EGUI_VIEW_MINI_CALENDAR_H_
#define _EGUI_VIEW_MINI_CALENDAR_H_

#include "egui_view.h"
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
    uint8_t pressed_day;       // current pressed day during touch tracking
    uint8_t focused_day;       // keyboard-focused day before Enter commits selection
    uint8_t first_day_of_week; // 0=Sunday, 1=Monday

    egui_color_t header_color;
    egui_color_t text_color;
    egui_color_t today_color;
    egui_color_t selected_color;
    egui_color_t weekend_color;
    egui_color_t bg_color;

    const egui_font_t *font;
    const char *weekday_labels[7];
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

/** Apply the region and initial year/month/day from one parameter block. */
void egui_view_mini_calendar_apply_params(egui_view_t *self, const egui_view_mini_calendar_params_t *params);
/** Initialize a mini calendar and immediately apply its parameter block. */
void egui_view_mini_calendar_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_mini_calendar_params_t *params);

/** Set the calendar month and the currently selected day. */
void egui_view_mini_calendar_set_date(egui_view_t *self, uint16_t year, uint8_t month, uint8_t day);
/** Return the currently stored calendar year. */
uint16_t egui_view_mini_calendar_get_year(egui_view_t *self);
/** Return the currently stored calendar month. */
uint8_t egui_view_mini_calendar_get_month(egui_view_t *self);
/** Return the currently selected day. */
uint8_t egui_view_mini_calendar_get_day(egui_view_t *self);
/** Set the day number highlighted as "today". Use 0 to clear the today highlight. */
void egui_view_mini_calendar_set_today(egui_view_t *self, uint8_t day);
/** Return the day highlighted as "today", or 0 when no today highlight is active. */
uint8_t egui_view_mini_calendar_get_today(egui_view_t *self);
/** Return the touch-tracked pressed day, or 0 when no day is pressed. */
uint8_t egui_view_mini_calendar_get_pressed_day(egui_view_t *self);
/** Return the keyboard-focused day before selection is committed. */
uint8_t egui_view_mini_calendar_get_focused_day(egui_view_t *self);
/** Set which weekday column appears first. Use the widget numbering (`0 = Sunday`, `1 = Monday`, and so on). */
void egui_view_mini_calendar_set_first_day_of_week(egui_view_t *self, uint8_t day);
/** Return which weekday column appears first. */
uint8_t egui_view_mini_calendar_get_first_day_of_week(egui_view_t *self);
/** Borrow a 7-entry weekday-label array. Pass NULL to restore the built-in labels. */
void egui_view_mini_calendar_set_weekday_labels(egui_view_t *self, const char *const *labels);
/** Return one resolved weekday label, including the built-in fallback labels. */
const char *egui_view_mini_calendar_get_weekday_label(egui_view_t *self, uint8_t index);
/** Register the callback fired after the user selects a different day by touch release. */
void egui_view_mini_calendar_set_on_date_selected_listener(egui_view_t *self, egui_view_on_date_selected_listener_t listener);
/** Return the registered date selection callback. */
egui_view_on_date_selected_listener_t egui_view_mini_calendar_get_on_date_selected_listener(egui_view_t *self);
/** Default draw hook used by the mini-calendar API table. */
void egui_view_mini_calendar_on_draw(egui_view_t *self);
/** Initialize the compact month-view calendar widget. */
void egui_view_mini_calendar_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_MINI_CALENDAR_H_ */

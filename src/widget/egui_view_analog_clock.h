#ifndef _EGUI_VIEW_ANALOG_CLOCK_H_
#define _EGUI_VIEW_ANALOG_CLOCK_H_

#include "egui_view.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_analog_clock egui_view_analog_clock_t;
struct egui_view_analog_clock
{
    egui_view_t base;

    uint8_t hour;        // 0-11
    uint8_t minute;      // 0-59
    uint8_t second;      // 0-59
    uint8_t show_second; // whether to draw second hand
    uint8_t show_ticks;  // whether to draw tick marks
    egui_dim_t hour_hand_width;
    egui_dim_t minute_hand_width;
    egui_dim_t second_hand_width;
    egui_color_t dial_color;
    egui_color_t hour_color;
    egui_color_t minute_color;
    egui_color_t second_color;
    egui_color_t tick_color;
};

// ============== Analog Clock Params ==============
typedef struct egui_view_analog_clock_params egui_view_analog_clock_params_t;
struct egui_view_analog_clock_params
{
    egui_region_t region;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

#define EGUI_VIEW_ANALOG_CLOCK_PARAMS_INIT(_name, _x, _y, _w, _h, _hour, _min, _sec)                                                                           \
    static const egui_view_analog_clock_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .hour = (_hour), .minute = (_min), .second = (_sec)}

/** Apply an analog clock parameter block after initialization. */
void egui_view_analog_clock_apply_params(egui_view_t *self, const egui_view_analog_clock_params_t *params);
/** Initialize an analog clock and immediately apply its parameter block. */
void egui_view_analog_clock_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_analog_clock_params_t *params);

/** Update the displayed time. Hours are normalized to 0-11 and minutes or seconds are clamped to 59. */
void egui_view_analog_clock_set_time(egui_view_t *self, uint8_t h, uint8_t m, uint8_t s);
/** Return the stored hour field in the 0-11 range. */
uint8_t egui_view_analog_clock_get_hour(egui_view_t *self);
/** Return the stored minute field in the 0-59 range. */
uint8_t egui_view_analog_clock_get_minute(egui_view_t *self);
/** Return the stored second field in the 0-59 range. */
uint8_t egui_view_analog_clock_get_second(egui_view_t *self);
/** Show or hide the second hand. */
void egui_view_analog_clock_show_second(egui_view_t *self, uint8_t show);
/** Return whether the second hand is shown. */
uint8_t egui_view_analog_clock_get_show_second(egui_view_t *self);
/** Show or hide the dial tick marks. */
void egui_view_analog_clock_show_ticks(egui_view_t *self, uint8_t show);
/** Return whether dial tick marks are shown. */
uint8_t egui_view_analog_clock_get_show_ticks(egui_view_t *self);
/** Set the hour hand stroke width. */
void egui_view_analog_clock_set_hour_hand_width(egui_view_t *self, egui_dim_t width);
/** Return the hour hand stroke width. */
egui_dim_t egui_view_analog_clock_get_hour_hand_width(egui_view_t *self);
/** Set the minute hand stroke width. */
void egui_view_analog_clock_set_minute_hand_width(egui_view_t *self, egui_dim_t width);
/** Return the minute hand stroke width. */
egui_dim_t egui_view_analog_clock_get_minute_hand_width(egui_view_t *self);
/** Set the second hand stroke width. */
void egui_view_analog_clock_set_second_hand_width(egui_view_t *self, egui_dim_t width);
/** Return the second hand stroke width. */
egui_dim_t egui_view_analog_clock_get_second_hand_width(egui_view_t *self);
/** Set the dial circle color. */
void egui_view_analog_clock_set_dial_color(egui_view_t *self, egui_color_t color);
/** Return the dial circle color. */
egui_color_t egui_view_analog_clock_get_dial_color(egui_view_t *self);
/** Set the hour hand color. */
void egui_view_analog_clock_set_hour_color(egui_view_t *self, egui_color_t color);
/** Return the hour hand color. */
egui_color_t egui_view_analog_clock_get_hour_color(egui_view_t *self);
/** Set the minute hand color. */
void egui_view_analog_clock_set_minute_color(egui_view_t *self, egui_color_t color);
/** Return the minute hand color. */
egui_color_t egui_view_analog_clock_get_minute_color(egui_view_t *self);
/** Set the second hand color. */
void egui_view_analog_clock_set_second_color(egui_view_t *self, egui_color_t color);
/** Return the second hand color. */
egui_color_t egui_view_analog_clock_get_second_color(egui_view_t *self);
/** Set the tick mark color. */
void egui_view_analog_clock_set_tick_color(egui_view_t *self, egui_color_t color);
/** Return the tick mark color. */
egui_color_t egui_view_analog_clock_get_tick_color(egui_view_t *self);
/** Default draw hook used by the analog clock API table. */
void egui_view_analog_clock_on_draw(egui_view_t *self);
/** Initialize an analog clock with the second hand and ticks enabled. */
void egui_view_analog_clock_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_ANALOG_CLOCK_H_ */

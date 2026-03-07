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

void egui_view_analog_clock_apply_params(egui_view_t *self, const egui_view_analog_clock_params_t *params);
void egui_view_analog_clock_init_with_params(egui_view_t *self, const egui_view_analog_clock_params_t *params);

void egui_view_analog_clock_set_time(egui_view_t *self, uint8_t h, uint8_t m, uint8_t s);
void egui_view_analog_clock_show_second(egui_view_t *self, uint8_t show);
void egui_view_analog_clock_show_ticks(egui_view_t *self, uint8_t show);
void egui_view_analog_clock_on_draw(egui_view_t *self);
void egui_view_analog_clock_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_ANALOG_CLOCK_H_ */

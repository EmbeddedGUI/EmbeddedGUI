#ifndef _EGUI_VIEW_LED_H_
#define _EGUI_VIEW_LED_H_

#include "egui_view.h"
#include "core/egui_timer.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_led egui_view_led_t;
/**
 * @brief Small circular status lamp with optional blink timer.
 *
 * This widget keeps only the on or off state, a few colors, and one timer used
 * to flip the state while blinking. It is a good example of a lightweight
 * self-animated indicator.
 */
struct egui_view_led
{
    egui_view_t base;

    uint8_t is_on;
    uint8_t is_blinking;
    uint16_t blink_period;
    egui_color_t on_color;
    egui_color_t off_color;
    egui_color_t border_color;
    egui_dim_t border_width;
    egui_timer_t blink_timer;
};

// ============== LED Params ==============
typedef struct egui_view_led_params egui_view_led_params_t;
/**
 * @brief Construction-time parameter block for one LED widget.
 */
struct egui_view_led_params
{
    egui_region_t region;
    uint8_t is_on;
};

/** Build an LED parameter block with region and initial on or off state. */
#define EGUI_VIEW_LED_PARAMS_INIT(_name, _x, _y, _w, _h, _on)                                                                                                  \
    static const egui_view_led_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .is_on = (_on)}

/** Apply an LED parameter block after initialization. */
void egui_view_led_apply_params(egui_view_t *self, const egui_view_led_params_t *params);
/** Initialize an LED and immediately apply its parameter block. */
void egui_view_led_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_led_params_t *params);

/** Force the LED into the on state. */
void egui_view_led_set_on(egui_view_t *self);
/** Force the LED into the off state. */
void egui_view_led_set_off(egui_view_t *self);
/** Flip the current on or off state immediately. */
void egui_view_led_toggle(egui_view_t *self);
/** Enable blinking with the supplied period in milliseconds. A zero period effectively pauses the timer. */
void egui_view_led_set_blink(egui_view_t *self, uint16_t period_ms);
/** Stop blinking and keep the current on or off state. */
void egui_view_led_stop_blink(egui_view_t *self);
/** Return whether the indicator is currently lit. */
int egui_view_led_get_is_on(egui_view_t *self);
/** Return whether the indicator is currently blinking. */
int egui_view_led_get_is_blinking(egui_view_t *self);
/** Return the blink period in milliseconds (default 500). */
uint16_t egui_view_led_get_blink_period(egui_view_t *self);
/** Return the color used when the LED is on. */
egui_color_t egui_view_led_get_on_color(egui_view_t *self);
/** Return the color used when the LED is off. */
egui_color_t egui_view_led_get_off_color(egui_view_t *self);
/** Return the border color of the LED circle. */
egui_color_t egui_view_led_get_border_color(egui_view_t *self);
/** Return the border width of the LED circle in pixels. */
egui_dim_t egui_view_led_get_border_width(egui_view_t *self);
/** Set the colors used for lit and unlit states. */
void egui_view_led_set_colors(egui_view_t *self, egui_color_t on_color, egui_color_t off_color);
/** Default draw hook used by the LED API table. */
void egui_view_led_on_draw(egui_view_t *self);
/** Initialize an LED indicator that starts off and not blinking. */
void egui_view_led_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_LED_H_ */

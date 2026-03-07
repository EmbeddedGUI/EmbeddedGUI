#ifndef _EGUI_VIEW_LED_H_
#define _EGUI_VIEW_LED_H_

#include "egui_view.h"
#include "core/egui_timer.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_led egui_view_led_t;
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
struct egui_view_led_params
{
    egui_region_t region;
    uint8_t is_on;
};

#define EGUI_VIEW_LED_PARAMS_INIT(_name, _x, _y, _w, _h, _on)                                                                                                  \
    static const egui_view_led_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .is_on = (_on)}

void egui_view_led_apply_params(egui_view_t *self, const egui_view_led_params_t *params);
void egui_view_led_init_with_params(egui_view_t *self, const egui_view_led_params_t *params);

void egui_view_led_set_on(egui_view_t *self);
void egui_view_led_set_off(egui_view_t *self);
void egui_view_led_toggle(egui_view_t *self);
void egui_view_led_set_blink(egui_view_t *self, uint16_t period_ms);
void egui_view_led_stop_blink(egui_view_t *self);
void egui_view_led_set_colors(egui_view_t *self, egui_color_t on_color, egui_color_t off_color);
void egui_view_led_on_draw(egui_view_t *self);
void egui_view_led_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_LED_H_ */

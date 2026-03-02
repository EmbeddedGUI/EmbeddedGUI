#ifndef _EGUI_VIEW_DIGITAL_CLOCK_H_
#define _EGUI_VIEW_DIGITAL_CLOCK_H_

#include "egui.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_digital_clock egui_view_digital_clock_t;
struct egui_view_digital_clock
{
    egui_view_label_t base;

    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t format_24h;
    uint8_t show_second;
    uint8_t colon_visible;
    uint8_t colon_blink;
    char time_buffer[12];
};

// ============== Digital Clock Params (reuse Label) ==============
#define EGUI_VIEW_DIGITAL_CLOCK_PARAMS_INIT        EGUI_VIEW_LABEL_PARAMS_INIT
#define EGUI_VIEW_DIGITAL_CLOCK_PARAMS_INIT_SIMPLE EGUI_VIEW_LABEL_PARAMS_INIT_SIMPLE
#define egui_view_digital_clock_apply_params       egui_view_label_apply_params

void egui_view_digital_clock_set_time(egui_view_t *self, uint8_t hour, uint8_t minute, uint8_t second);
void egui_view_digital_clock_set_format(egui_view_t *self, uint8_t format_24h);
void egui_view_digital_clock_set_colon_blink(egui_view_t *self, uint8_t enable);
void egui_view_digital_clock_set_colon_visible(egui_view_t *self, uint8_t visible);
void egui_view_digital_clock_set_show_second(egui_view_t *self, uint8_t show);
void egui_view_digital_clock_init(egui_view_t *self);
void egui_view_digital_clock_init_with_params(egui_view_t *self, const egui_view_label_params_t *params);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_DIGITAL_CLOCK_H_ */

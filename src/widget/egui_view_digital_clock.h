#ifndef _EGUI_VIEW_DIGITAL_CLOCK_H_
#define _EGUI_VIEW_DIGITAL_CLOCK_H_

#include "egui.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_digital_clock egui_view_digital_clock_t;
/**
 * @brief Label-derived clock widget that keeps a formatted ASCII time buffer.
 *
 * The widget does not own a timer. Callers update the raw time fields and, if
 * desired, toggle `colon_visible` periodically to implement blinking.
 */
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
/** Reuse the label-style parameter macro for clock construction. */
#define EGUI_VIEW_DIGITAL_CLOCK_PARAMS_INIT        EGUI_VIEW_LABEL_PARAMS_INIT
/** Reuse the simplified label-style parameter macro for clock construction. */
#define EGUI_VIEW_DIGITAL_CLOCK_PARAMS_INIT_SIMPLE EGUI_VIEW_LABEL_PARAMS_INIT_SIMPLE
/** Reuse the label parameter application helper directly. */
#define egui_view_digital_clock_apply_params       egui_view_label_apply_params

/** Set the raw time fields and immediately refresh the rendered label text. */
void egui_view_digital_clock_set_time(egui_view_t *self, uint8_t hour, uint8_t minute, uint8_t second);
/** Switch between 24-hour and 12-hour text formatting. */
void egui_view_digital_clock_set_format(egui_view_t *self, uint8_t format_24h);
/** Enable colon replacement when `colon_visible` is false. This API does not create a timer by itself. */
void egui_view_digital_clock_set_colon_blink(egui_view_t *self, uint8_t enable);
/** Control whether colons render as ':' or spaces when colon blink mode is enabled. */
void egui_view_digital_clock_set_colon_visible(egui_view_t *self, uint8_t visible);
/** Show or hide seconds in 24-hour mode. 12-hour mode always renders `HH:MM AM/PM`. */
void egui_view_digital_clock_set_show_second(egui_view_t *self, uint8_t show);
/** Initialize a digital clock label in 24-hour mode with seconds shown. */
void egui_view_digital_clock_init(egui_view_t *self, egui_core_t *core);
/** Initialize a digital clock and apply the same parameter block used by label widgets. */
void egui_view_digital_clock_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_label_params_t *params);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_DIGITAL_CLOCK_H_ */

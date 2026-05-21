#ifndef _EGUI_VIEW_STOPWATCH_H_
#define _EGUI_VIEW_STOPWATCH_H_

#include "egui_view_label.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Stopwatch is idle and not accumulating time. */
#define EGUI_VIEW_STOPWATCH_STATE_STOPPED 0
/** Stopwatch is logically running. External code must still update elapsed time. */
#define EGUI_VIEW_STOPWATCH_STATE_RUNNING 1
/** Stopwatch is paused while keeping the current elapsed time. */
#define EGUI_VIEW_STOPWATCH_STATE_PAUSED  2

typedef struct egui_view_stopwatch egui_view_stopwatch_t;
/**
 * @brief Label wrapper that formats elapsed milliseconds into stopwatch text.
 *
 * This widget does not own a timer. Callers update `elapsed_ms`, while the
 * widget focuses on formatting and on invalidating only the changed text area.
 */
struct egui_view_stopwatch
{
    egui_view_label_t base;

    uint32_t elapsed_ms;
    uint8_t state;
    uint8_t show_ms;
    char time_buffer[16];
};

// ============== Stopwatch Params (reuse Label) ==============
/** Reuse the label-style parameter macro for stopwatch construction. */
#define EGUI_VIEW_STOPWATCH_PARAMS_INIT        EGUI_VIEW_LABEL_PARAMS_INIT
/** Reuse the simplified label-style parameter macro for stopwatch construction. */
#define EGUI_VIEW_STOPWATCH_PARAMS_INIT_SIMPLE EGUI_VIEW_LABEL_PARAMS_INIT_SIMPLE
/** Reuse the label parameter application helper directly. */
#define egui_view_stopwatch_apply_params       egui_view_label_apply_params

/** Set the elapsed time in milliseconds and refresh the rendered label text. */
void egui_view_stopwatch_set_elapsed(egui_view_t *self, uint32_t elapsed_ms);
/** Read the stored elapsed time in milliseconds. */
uint32_t egui_view_stopwatch_get_elapsed(egui_view_t *self);
/** Return the current formatted stopwatch text buffer. */
const char *egui_view_stopwatch_get_time_text(egui_view_t *self);
/** Store the logical stopwatch state. This API does not start or stop a timer by itself. */
void egui_view_stopwatch_set_state(egui_view_t *self, uint8_t state);
/** Read the logical stopwatch state flag. */
uint8_t egui_view_stopwatch_get_state(egui_view_t *self);
/** Show or hide the millisecond field in the formatted label text. */
void egui_view_stopwatch_set_show_ms(egui_view_t *self, uint8_t show);
/** Return whether the millisecond field is shown in the formatted label text. */
uint8_t egui_view_stopwatch_get_show_ms(egui_view_t *self);
/** Initialize a stopwatch label with millisecond output enabled. */
void egui_view_stopwatch_init(egui_view_t *self, egui_core_t *core);
/** Initialize a stopwatch and apply the same parameter block used by label widgets. */
void egui_view_stopwatch_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_label_params_t *params);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_STOPWATCH_H_ */

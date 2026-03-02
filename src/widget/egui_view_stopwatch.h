#ifndef _EGUI_VIEW_STOPWATCH_H_
#define _EGUI_VIEW_STOPWATCH_H_

#include "egui_view_label.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_STOPWATCH_STATE_STOPPED 0
#define EGUI_VIEW_STOPWATCH_STATE_RUNNING 1
#define EGUI_VIEW_STOPWATCH_STATE_PAUSED  2

typedef struct egui_view_stopwatch egui_view_stopwatch_t;
struct egui_view_stopwatch
{
    egui_view_label_t base;

    uint32_t elapsed_ms;
    uint8_t state;
    uint8_t show_ms;
    char time_buffer[16];
};

// ============== Stopwatch Params (reuse Label) ==============
#define EGUI_VIEW_STOPWATCH_PARAMS_INIT        EGUI_VIEW_LABEL_PARAMS_INIT
#define EGUI_VIEW_STOPWATCH_PARAMS_INIT_SIMPLE EGUI_VIEW_LABEL_PARAMS_INIT_SIMPLE
#define egui_view_stopwatch_apply_params       egui_view_label_apply_params

void egui_view_stopwatch_set_elapsed(egui_view_t *self, uint32_t elapsed_ms);
uint32_t egui_view_stopwatch_get_elapsed(egui_view_t *self);
void egui_view_stopwatch_set_state(egui_view_t *self, uint8_t state);
uint8_t egui_view_stopwatch_get_state(egui_view_t *self);
void egui_view_stopwatch_set_show_ms(egui_view_t *self, uint8_t show);
void egui_view_stopwatch_init(egui_view_t *self);
void egui_view_stopwatch_init_with_params(egui_view_t *self, const egui_view_label_params_t *params);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_STOPWATCH_H_ */

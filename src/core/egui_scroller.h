#ifndef _EGUI_SCROLLER_H_
#define _EGUI_SCROLLER_H_

#include "egui_common.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

enum egui_scroller_mode
{
    EGUI_SCROLLER_MODE_NORMAL,
    EGUI_SCROLLER_MODE_FLING,
};

typedef struct egui_scroller egui_scroller_t;
struct egui_scroller
{
    uint8_t mode;
    uint8_t finished;
    uint16_t duration; // duration of the scrolling
    union
    {
        egui_float_t velocity;            // velocity of the scrolling, unit is pixel per millisecond.
        egui_float_t duration_reciprocal; // get the duration reciprocal, avoid division operation in the loop.
    };
    egui_dim_t delta;        // delta of the scrolling
    egui_dim_t delta_offset; // current delta offset of the scrolling
    uint32_t start_time;     // time when the scrolling starts
};

void egui_scroller_about_animation(egui_scroller_t *self);
void egui_scroller_start_scroll(egui_scroller_t *self, egui_dim_t delta, uint16_t duration);
void egui_scroller_start_filing(egui_scroller_t *self, egui_dim_t delta, egui_float_t velocity);
int egui_scroller_compute_scroll_offset(egui_scroller_t *self);
void egui_scroller_init(egui_scroller_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_SCROLLER_H_ */

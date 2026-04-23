#ifndef _EGUI_SCROLLER_H_
#define _EGUI_SCROLLER_H_

/**
 * @file egui_scroller.h
 * @brief Small helper that turns a target scroll distance or fling velocity into per-frame deltas.
 */

#include "egui_common.h"
#include "egui_typedef.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Scroll-animation modes supported by the reusable scroller helper. */
enum egui_scroller_mode
{
    /** Fixed-distance scrolling that completes after `duration` milliseconds. */
    EGUI_SCROLLER_MODE_NORMAL,
    /** Inertial fling animation driven by an initial velocity and deceleration. */
    EGUI_SCROLLER_MODE_FLING,
};

typedef struct egui_scroller egui_scroller_t;
/** Mutable scroll-animation state typically embedded inside scrollable widgets. */
struct egui_scroller
{
    uint8_t mode;      // enum egui_scroller_mode
    uint8_t finished;  // non-zero once no more offset remains to be produced
    uint16_t duration; // total animation time in milliseconds
    union
    {
        egui_float_t velocity;            // fling velocity in pixels per millisecond
        egui_float_t duration_reciprocal; // cached `delta / duration` used by normal scrolling
    };
    egui_dim_t delta;        // target total scroll delta, positive usually means content moves up/left
    egui_dim_t delta_offset; // amount of `delta` that has already been emitted to the caller
    uint32_t start_time;     // animation start timestamp in milliseconds
};

/** Abort any active scroll animation and mark the scroller as finished. */
void egui_scroller_about_animation(egui_scroller_t *self);
/** Start a fixed-distance scroll animation. Each `compute_scroll_offset` call returns only the next incremental delta. */
void egui_scroller_start_scroll(egui_scroller_t *self, egui_core_t *core, egui_dim_t delta, uint16_t duration);
/** Start a fling-style scroll using an initial velocity and built-in deceleration, clamped by `delta`. */
void egui_scroller_start_filing(egui_scroller_t *self, egui_core_t *core, egui_dim_t delta, egui_float_t velocity);
/** Advance the active animation and return only the newly produced delta since the previous call. */
int egui_scroller_compute_scroll_offset(egui_scroller_t *self, egui_core_t *core);
/** Initialize the scroller in the finished state. */
void egui_scroller_init(egui_scroller_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_SCROLLER_H_ */

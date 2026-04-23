#ifndef _EGUI_VELOCITY_TRACKER_H_
#define _EGUI_VELOCITY_TRACKER_H_

#include "egui_common.h"
#include "egui_motion_event.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Maximum number of recent motion points kept for velocity estimation. */
#define EGUI_VELOCITY_TRACKER_MAX_POINTS        (EGUI_CONFIG_INPUT_MOTION_CACHE_COUNT)
/** Oldest motion age, in milliseconds, that still contributes to velocity estimation. */
#define EGUI_VELOCITY_TRACKER_LONGEST_PAST_TIME (200)

typedef struct egui_velocity_tracker_point egui_velocity_tracker_point_t;
struct egui_velocity_tracker_point
{
    egui_dim_t x;  // sampled x position in screen coordinates
    egui_dim_t y;  // sampled y position in screen coordinates
    uint32_t time; // sample timestamp in milliseconds, `0` means unused slot
};

typedef struct egui_velocity_tracker egui_velocity_tracker_t;
struct egui_velocity_tracker
{
    egui_velocity_tracker_point_t points[EGUI_VELOCITY_TRACKER_MAX_POINTS]; // oldest-to-newest motion history
    egui_float_t velocity_x;                                                // most recently computed horizontal velocity in pixels per millisecond
    egui_float_t velocity_y;                                                // most recently computed vertical velocity in pixels per millisecond
};

/** Append one sampled point, dropping entries that are too old or exceed the fixed history size. */
void egui_velocity_tracker_add_point(egui_velocity_tracker_t *self, egui_dim_t x, egui_dim_t y, uint32_t time);
/** Clear all stored history points. The last computed velocities remain unchanged until recomputed or reinitialized. */
void egui_velocity_tracker_clear(egui_velocity_tracker_t *self);
/** Recompute `velocity_x` and `velocity_y` from the currently stored motion history. */
void egui_velocity_tracker_compute_velocity(egui_velocity_tracker_t *self);
/** Feed one motion event into the tracker. `DOWN` clears history and `UP` triggers velocity calculation. */
void egui_velocity_tracker_add_motion(egui_velocity_tracker_t *self, egui_motion_event_t *event);
/** Initialize the tracker with empty history and zero velocities. */
void egui_velocity_tracker_init(egui_velocity_tracker_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VELOCITY_TRACKER_H_ */

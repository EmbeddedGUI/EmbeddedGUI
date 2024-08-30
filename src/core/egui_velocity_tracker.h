#ifndef _EGUI_VELOCITY_TRACKER_H_
#define _EGUI_VELOCITY_TRACKER_H_

#include "egui_common.h"
#include "egui_motion_event.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VELOCITY_TRACKER_MAX_POINTS        (EGUI_CONFIG_INPUT_MOTION_CACHE_COUNT)
#define EGUI_VELOCITY_TRACKER_LONGEST_PAST_TIME (200)

typedef struct egui_velocity_tracker_point egui_velocity_tracker_point_t;
struct egui_velocity_tracker_point
{
    egui_dim_t x;
    egui_dim_t y;
    uint32_t time;
};

typedef struct egui_velocity_tracker egui_velocity_tracker_t;
struct egui_velocity_tracker
{
    egui_velocity_tracker_point_t points[EGUI_VELOCITY_TRACKER_MAX_POINTS];
    egui_float_t velocity_x; // unit is pixel per millisecond.
    egui_float_t velocity_y; // unit is pixel per millisecond.
};

void egui_velocity_tracker_add_point(egui_velocity_tracker_t *self, egui_dim_t x, egui_dim_t y, uint32_t time);
void egui_velocity_tracker_clear(egui_velocity_tracker_t *self);
void egui_velocity_tracker_compute_velocity(egui_velocity_tracker_t *self);
void egui_velocity_tracker_add_motion(egui_velocity_tracker_t *self, egui_motion_event_t *event);
void egui_velocity_tracker_init(egui_velocity_tracker_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VELOCITY_TRACKER_H_ */

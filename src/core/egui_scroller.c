#include <stdio.h>
#include <assert.h>

#include "egui_scroller.h"
#include "egui_api.h"
#include "egui_velocity_tracker.h"

/**
 * @file egui_scroller.c
 * @brief Small scroll-animation engine shared by widgets that need smooth scrolling.
 */

#define EGUI_SCROLLER_DECCELERATION (EGUI_FLOAT_VALUE(0.008f))

/** Mark the current animation as finished so future polling returns zero offset. */
void egui_scroller_about_animation(egui_scroller_t *self)
{
    self->finished = 1;
}

/**
 * Start a fixed-distance scroll animation.
 * The scroller stores `delta / duration` up front so each frame can compute the current theoretical position with one multiplication instead of a repeated
 * division.
 *
 * @param delta Distance to travel. Positive numbers will scroll the
 *        content
 * up/left.
 * @param duration Duration of the scroll in milliseconds.
 */
void egui_scroller_start_scroll(egui_scroller_t *self, egui_core_t *core, egui_dim_t delta, uint16_t duration)
{
    self->mode = EGUI_SCROLLER_MODE_NORMAL;
    self->finished = 0;

    if (duration == 0)
    {
        duration = 1;
    }

    self->duration = duration;
    self->delta = delta;
    self->delta_offset = 0;
    self->start_time = egui_api_timer_get_current_core(core);

    // Cache the scroll speed in pixels per millisecond.
    self->duration_reciprocal = EGUI_FLOAT_DIV(delta, duration);
    if (self->duration_reciprocal == EGUI_FLOAT_VALUE(0.0f))
    {
        self->duration_reciprocal = EGUI_FLOAT_VALUE(1.0f);
    }

    // EGUI_LOG_DBG("egui_scroller_start_scroll, delta:%d, duration:%d, duration_reciprocal: 0x%x\n", delta, duration, self->duration_reciprocal);
}
/**
 * Start an inertial fling animation.
 * The final travel distance is derived from the initial velocity and a fixed deceleration, then clamped so the fling never exceeds the caller's maximum allowed
 * scroll range.
 */
void egui_scroller_start_filing(egui_scroller_t *self, egui_core_t *core, egui_dim_t delta, egui_float_t velocity)
{
    self->mode = EGUI_SCROLLER_MODE_FLING;
    self->finished = 0;

    // Empirical scaling that makes tracked pointer velocity feel closer to UI scroll distance.
    velocity = velocity * 2;

    self->start_time = egui_api_timer_get_current_core(core);

    self->velocity = velocity;
    self->duration = EGUI_ABS(velocity) / EGUI_SCROLLER_DECCELERATION; // unit is milliseconds

    uint32_t total_distance = (uint32_t)(EGUI_FLOAT_MULT(velocity, velocity) / (EGUI_SCROLLER_DECCELERATION * 2));
    int delta_limit = EGUI_ABS(delta);

    if (delta_limit > EGUI_DIM_MAX)
    {
        delta_limit = EGUI_DIM_MAX;
    }
    if (total_distance > (uint32_t)EGUI_DIM_MAX)
    {
        total_distance = (uint32_t)EGUI_DIM_MAX;
    }

    self->delta = total_distance < (uint32_t)delta_limit ? (egui_dim_t)total_distance : (egui_dim_t)delta_limit;
    self->delta_offset = 0;

    if (velocity < 0)
    {
        self->delta = -self->delta;
    }

    // EGUI_LOG_DBG("egui_scroller_compute_scroll_offset, delta: %d, velocity: %d, duration: %d, total_distance: %d\n", self->delta, velocity, self->duration,
    // total_distance);
}

/** Advance the animation and return only the newly generated incremental offset. */
int egui_scroller_compute_scroll_offset(egui_scroller_t *self, egui_core_t *core)
{
    if (self->finished)
    {
        return 0;
    }
    egui_dim_t offset = 0;
    egui_dim_t cur_delta;
    uint32_t time_elapsed = egui_api_timer_get_current_core(core) - self->start_time;
    // uint16_t old_offset = self->delta_offset;
    // EGUI_LOG_DBG("egui_scroller_compute_scroll_offset, time_elapsed: %d, old_offset: %d\n", time_elapsed, old_offset);
    if (time_elapsed >= self->duration)
    {
        // Emit the remaining tail exactly once so rounding errors do not leave residual distance behind.
        egui_scroller_about_animation(self);
        offset = self->delta - self->delta_offset;
    }
    else
    {
        switch (self->mode)
        {
        case EGUI_SCROLLER_MODE_NORMAL:
            // Linear interpolation from 0 to `delta`.
            cur_delta = EGUI_FLOAT_MULT(time_elapsed, self->duration_reciprocal);
            offset = cur_delta - self->delta_offset;
            break;
        case EGUI_SCROLLER_MODE_FLING:
            // Distance = v*t - 1/2*a*t^2 with the sign restored after the magnitude computation.
            cur_delta =
                    (EGUI_FLOAT_MULT(time_elapsed, EGUI_ABS(self->velocity))) - (EGUI_FLOAT_MULT(time_elapsed * time_elapsed, EGUI_SCROLLER_DECCELERATION / 2));
            if (self->velocity < 0)
            {
                cur_delta = -cur_delta;
            }
            offset = cur_delta - self->delta_offset;
            break;
        }
    }

    // Keep track of the total emitted distance so the next call can return only the delta since this frame.
    self->delta_offset += offset;
    // EGUI_LOG_DBG("egui_scroller_compute_scroll_offset, offset:%d, delta_offset:%d\n", offset, self->delta_offset);

    return offset;
}

/** Initialize the helper in the idle/finished state. */
void egui_scroller_init(egui_scroller_t *self, egui_core_t *core)
{
    EGUI_UNUSED(core);
    self->finished = 1;
}

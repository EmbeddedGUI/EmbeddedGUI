#include <stdio.h>
#include <assert.h>

#include "egui_scroller.h"
#include "egui_api.h"
#include "egui_velocity_tracker.h"

// TODO: select a better acceleration value. pixel per ms.
#define EGUI_SCROLLER_DECCELERATION (EGUI_FLOAT_VALUE(0.001f))

void egui_scroller_about_animation(egui_scroller_t *self)
{
    self->finished = 1;
}
/**
 * Start scrolling by providing a starting point and the distance to travel.
 *
 * @param delta Distance to travel. Positive numbers will scroll the
 *        content up/left.
 * @param duration Duration of the scroll in milliseconds.
 */
void egui_scroller_start_scroll(egui_scroller_t *self, egui_dim_t delta, uint16_t duration)
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
    self->start_time = egui_api_timer_get_current();

    // point per ms.
    self->duration_reciprocal = EGUI_FLOAT_DIV(delta, duration);
    if (self->duration_reciprocal == EGUI_FLOAT_VALUE(0.0f))
    {
        self->duration_reciprocal = EGUI_FLOAT_VALUE(1.0f);
    }

    // EGUI_LOG_DBG("egui_scroller_start_scroll, delta:%d, duration:%d, duration_reciprocal: 0x%x\n", delta, duration, self->duration_reciprocal);
}
/**
 * Start scrolling based on a fling gesture. The distance travelled will
 * depend on the initial velocity of the fling.
 *
 * @param delta Maximum delta value. The scroller will not scroll past this
 *        point.
 * @param velocity Initial velocity of the fling measured in pixels per
 *        millisecond.
 */
void egui_scroller_start_filing(egui_scroller_t *self, egui_dim_t delta, egui_float_t velocity)
{
    self->mode = EGUI_SCROLLER_MODE_FLING;
    self->finished = 0;

    velocity = velocity * 2;

    self->start_time = egui_api_timer_get_current();

    self->velocity = velocity;
    self->duration = EGUI_ABS(velocity) / EGUI_SCROLLER_DECCELERATION; // unit is milliseconds

    uint32_t total_distance = (uint32_t)(EGUI_FLOAT_MULT(velocity, velocity) / (EGUI_SCROLLER_DECCELERATION * 2));

    self->delta = EGUI_MIN(EGUI_ABS(delta), total_distance);
    self->delta_offset = 0;

    if (velocity < 0)
    {
        self->delta = -self->delta;
    }

    // EGUI_LOG_DBG("egui_scroller_compute_scroll_offset, delta: %d, velocity: %d, duration: %d, total_distance: %d\n", self->delta, velocity, self->duration,
    // total_distance);
}

int egui_scroller_compute_scroll_offset(egui_scroller_t *self)
{
    if (self->finished)
    {
        return 0;
    }
    egui_dim_t offset = 0;
    egui_dim_t cur_delta;
    uint32_t time_elapsed = egui_api_timer_get_current() - self->start_time;
    uint16_t old_offset = self->delta_offset;
    // EGUI_LOG_DBG("egui_scroller_compute_scroll_offset, time_elapsed: %d, old_offset: %d\n", time_elapsed, old_offset);
    if (time_elapsed >= self->duration)
    {
        egui_scroller_about_animation(self);
        offset = self->delta - self->delta_offset;
    }
    else
    {
        switch (self->mode)
        {
        case EGUI_SCROLLER_MODE_NORMAL:
            cur_delta = EGUI_FLOAT_MULT(time_elapsed, self->duration_reciprocal);
            offset = cur_delta - self->delta_offset;
            break;
        case EGUI_SCROLLER_MODE_FLING:
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

    // update delta_offset
    self->delta_offset += offset;
    // EGUI_LOG_DBG("egui_scroller_compute_scroll_offset, offset:%d, delta_offset:%d\n", offset, self->delta_offset);

    return offset;
}

void egui_scroller_init(egui_scroller_t *self)
{
    self->finished = 1;
}

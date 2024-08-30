#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_velocity_tracker.h"
#include "egui_api.h"

void egui_velocity_tracker_add_point(egui_velocity_tracker_t *self, egui_dim_t x, egui_dim_t y, uint32_t time)
{
    int drop = -1;
    int i;
    for (i = 0; i < EGUI_VELOCITY_TRACKER_MAX_POINTS; i++)
    {
        if (self->points[i].time == 0)
        {
            break;
        }
        else if (self->points[i].time < time - EGUI_VELOCITY_TRACKER_LONGEST_PAST_TIME)
        {
            // Dropping past too old point.
            drop = i;
        }
    }

    if (i == EGUI_VELOCITY_TRACKER_MAX_POINTS && drop < 0)
    {
        drop = 0;
    }

    if (drop == i)
    {
        drop--;
    }

    // drop oldleast points if necessary.
    if (drop >= 0)
    {
        int start = drop + 1;
        int count = EGUI_VELOCITY_TRACKER_MAX_POINTS - drop - 1;
        memcpy(&self->points[start], &self->points[0], count * sizeof(egui_velocity_tracker_point_t));

        i -= drop + 1;
    }

    self->points[i].x = x;
    self->points[i].y = y;
    self->points[i].time = time;
    i++;
    // clear next points.
    if (i < EGUI_VELOCITY_TRACKER_MAX_POINTS)
    {
        self->points[i].time = 0;
    }
}

// Reset the velocity tracker back to its initial state
void egui_velocity_tracker_clear(egui_velocity_tracker_t *self)
{
    self->points[0].time = 0;
}

void egui_velocity_tracker_compute_velocity(egui_velocity_tracker_t *self)
{
    egui_dim_t oldest_x = self->points[0].x;
    egui_dim_t oldest_y = self->points[0].y;
    uint32_t oldest_time = self->points[0].time;
    egui_float_t accum_x = 0;
    egui_float_t accum_y = 0;
    int count = 0;
    while (count < EGUI_VELOCITY_TRACKER_MAX_POINTS)
    {
        if (self->points[count].time == 0)
        {
            break;
        }

        count++;
    }

    // Skip the last received event, since it is probably pretty noisy.
    if (count > 3)
    {
        count--;
    }

    for (int i = 1; i < count; i++)
    {
        int dur = self->points[i].time - oldest_time;
        if (dur == 0)
        {
            continue;
        }
        egui_dim_t dist = self->points[i].x - oldest_x;
        egui_float_t vel = EGUI_FLOAT_DIV(dist, dur);
        if (accum_x == 0)
        {
            accum_x = vel;
        }
        else
        {
            accum_x = (accum_x + vel) / 2;
        }

        dist = self->points[i].y - oldest_y;
        vel = EGUI_FLOAT_DIV(dist, dur);
        if (accum_y == 0)
        {
            accum_y = vel;
        }
        else
        {
            accum_y = (accum_y + vel) / 2;
        }
    }

    self->velocity_x = accum_x;
    self->velocity_y = accum_y;
}

void egui_velocity_tracker_add_motion(egui_velocity_tracker_t *self, egui_motion_event_t *event)
{
    if (event->type == EGUI_MOTION_EVENT_ACTION_DOWN)
    {
        egui_velocity_tracker_clear(self);
    }
    egui_velocity_tracker_add_point(self, event->location.x, event->location.y, event->timestamp);
    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_UP:
        egui_velocity_tracker_compute_velocity(self);
        break;
    case EGUI_MOTION_EVENT_ACTION_MOVE:
    case EGUI_MOTION_EVENT_ACTION_DOWN:
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        break;
    default:
        break;
    }
}

void egui_velocity_tracker_init(egui_velocity_tracker_t *self)
{
    egui_velocity_tracker_clear(self);
    self->velocity_x = 0;
    self->velocity_y = 0;
}

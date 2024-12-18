#include <stdio.h>
#include <assert.h>

#include "egui_animation.h"
#include "widget/egui_view.h"
#include "core/egui_api.h"

extern void egui_core_animation_append(egui_animation_t *anim);
extern void egui_core_animation_remove(egui_animation_t *anim);

void egui_animation_duration_set(egui_animation_t *self, uint16_t duration)
{
    self->duration = duration;
}

void egui_animation_interpolator_set(egui_animation_t *self, egui_interpolator_t *interpolator)
{
    self->interpolator = interpolator;
}

void egui_animation_repeat_mode_set(egui_animation_t *self, uint8_t repeat_mode)
{
    self->repeat_mode = repeat_mode;
}

void egui_animation_repeat_count_set(egui_animation_t *self, uint8_t repeat_count)
{
    self->repeat_count = repeat_count;
}

void egui_animation_handle_set(egui_animation_t *self, const egui_animation_handle_t *handle)
{
    self->handle = handle;
}

void egui_animation_is_fill_before_set(egui_animation_t *self, int is_fill_before)
{
    self->is_fill_before = is_fill_before;
}

void egui_animation_is_fill_after_set(egui_animation_t *self, int is_fill_after)
{
    self->is_fill_after = is_fill_after;
}

void egui_animation_is_inside_animation_set(egui_animation_t *self, int is_inside_animation)
{
    self->is_inside_animation = is_inside_animation;
}

void egui_animation_notify_start(egui_animation_t *self)
{
    if (self->handle && self->handle->start)
    {
        self->handle->start(self);
    }
}

void egui_animation_notify_end(egui_animation_t *self)
{
    if (self->is_fill_before)
    {
        self->api->on_update(self, EGUI_FLOAT_VALUE(0.0f));
    }
    else if (self->is_fill_after)
    {
        self->api->on_update(self, EGUI_FLOAT_VALUE(1.0f));
    }
    if (self->handle && self->handle->end)
    {
        self->handle->end(self);
    }
}

void egui_animation_notify_repeat(egui_animation_t *self)
{
    if (self->handle && self->handle->repeat)
    {
        self->handle->repeat(self);
    }
}

void egui_animation_start(egui_animation_t *self)
{
    if (self->target_view == NULL)
    {
        return;
    }
    self->start_time = -1;
    self->is_running = true;
    self->is_started = false;
    self->is_ended = false;
    self->is_cycle_flip = false;
    self->repeated = 0;

    if (!self->is_inside_animation)
    {
        egui_core_animation_append(self);
    }

    self->api->on_start(self);
}

void egui_animation_stop(egui_animation_t *self)
{
    if (self->is_running)
    {
        self->is_running = false;
        if (!self->is_inside_animation)
        {
            egui_core_animation_remove(self);
        }
    }
}

void egui_animation_update(egui_animation_t *self, uint32_t current_time)
{
    int done = 0;
    uint32_t duration = self->duration;
    egui_float_t fraction = EGUI_FLOAT_VALUE(1.0f);
    if (!self->is_started)
    {
        self->is_started = true;
        egui_animation_notify_start(self);
    }

    if (self->start_time == (uint32_t)-1)
    {
        self->start_time = current_time;
    }

    if (duration > 0)
    {
        fraction = EGUI_FLOAT_DIV((current_time - self->start_time), duration);
    }

    if (fraction >= EGUI_FLOAT_VALUE(1.0f))
    {
        fraction = EGUI_FLOAT_VALUE(1.0f);
        done = 1;
    }

    if (self->is_cycle_flip)
    {
        fraction = EGUI_FLOAT_VALUE(1.0f) - fraction;
    }
    if (self->interpolator)
    {
#if EGUI_CONFIG_PERFORMANCE_USE_FLOAT
        EGUI_LOG_DBG("old fraction:%f\r\n", fraction);
        fraction = self->interpolator->api->get_interpolation(self->interpolator, fraction);
        EGUI_LOG_DBG("new fraction:%f\r\n", fraction);
#else
        EGUI_LOG_DBG("old fraction:0x%x\r\n", fraction);
        fraction = self->interpolator->api->get_interpolation(self->interpolator, fraction);
        EGUI_LOG_DBG("new fraction:0x%x\r\n", fraction);
#endif
    }

    self->api->on_update(self, fraction);

    if (done)
    {
        if (self->repeat_count == self->repeated)
        {
            egui_animation_stop(self);

            self->is_ended = true;
            egui_animation_notify_end(self);
        }
        else
        {
            if (self->repeat_count > 0)
            {
                self->repeated++;
            }

            if (self->repeat_mode == EGUI_ANIMATION_REPEAT_MODE_REVERSE)
            {
                self->is_cycle_flip = !self->is_cycle_flip;
            }

            // Restart animation
            self->start_time = (uint32_t)-1;
            egui_animation_notify_repeat(self);
        }
    }
}

void egui_animation_target_view_set(egui_animation_t *self, egui_view_t *view)
{
    self->target_view = view;
}

void egui_animation_on_start(egui_animation_t *self)
{
    // implement by subclass
}

void egui_animation_on_update(egui_animation_t *self, egui_float_t fraction)
{
    // implement by subclass
}

const egui_animation_api_t egui_animation_t_api_table = {
        .on_start = egui_animation_on_start,
        .update = egui_animation_update,
        .on_update = egui_animation_on_update,
};

void egui_animation_init(egui_animation_t *self)
{
    // init api
    self->api = &egui_animation_t_api_table;

    self->duration = 0;
    self->interpolator = NULL;
    self->repeat_mode = EGUI_ANIMATION_REPEAT_MODE_RESTART;
    self->repeat_count = 0;
    self->repeated = 0;
    self->handle = NULL;
    self->target_view = NULL;

    self->is_running = false;
    self->is_started = false;
    self->is_ended = false;
    self->is_cycle_flip = false;
    self->is_fill_before = false;
    self->is_fill_after = false;
    self->is_inside_animation = false;
}

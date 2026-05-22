#include <stdio.h>
#include <assert.h>

#include "egui_animation.h"
#include "widget/egui_view.h"
#include "core/egui_core_internal.h"
#include "core/egui_api.h"

/**
 * @file egui_animation.c
 * @brief Base animation state machine that binds one animation to a target view, advances normalized progress, and manages
 * repeats.
 */

/** Resolve the core that owns this animation by following its bound target view. */
static egui_core_t *egui_animation_get_core(egui_animation_t *self)
{
    if (self == NULL || self->target_view == NULL)
    {
        return NULL;
    }

    return egui_view_get_core(self->target_view);
}

/** Store the duration of one animation cycle in milliseconds. */
void egui_animation_duration_set(egui_animation_t *self, uint16_t duration)
{
    self->duration = duration;
}

uint16_t egui_animation_duration_get(egui_animation_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    return self->duration;
}

/** Install the interpolator used to remap the linear 0..1 progress fraction. */
void egui_animation_interpolator_set(egui_animation_t *self, egui_interpolator_t *interpolator)
{
    self->interpolator = interpolator;
}

egui_interpolator_t *egui_animation_interpolator_get(egui_animation_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    return self->interpolator;
}

/** Choose how each repeated cycle restarts once the current cycle finishes. */
void egui_animation_repeat_mode_set(egui_animation_t *self, uint8_t repeat_mode)
{
    self->repeat_mode = repeat_mode;
}

uint8_t egui_animation_repeat_mode_get(egui_animation_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    return self->repeat_mode;
}

/** Set how many extra cycles run after the first play. */
void egui_animation_repeat_count_set(egui_animation_t *self, uint8_t repeat_count)
{
    self->repeat_count = repeat_count;
}

uint8_t egui_animation_repeat_count_get(egui_animation_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    return (uint8_t)self->repeat_count;
}

/** Install optional lifecycle callbacks notified on start, repeat, and end. */
void egui_animation_handle_set(egui_animation_t *self, const egui_animation_handle_t *handle)
{
    self->handle = handle;
}

/** When enabled, finish by forcing the animated property back to fraction 0. */
void egui_animation_is_fill_before_set(egui_animation_t *self, int is_fill_before)
{
    self->is_fill_before = is_fill_before;
}

/** When enabled, finish by forcing the animated property to fraction 1. */
void egui_animation_is_fill_after_set(egui_animation_t *self, int is_fill_after)
{
    self->is_fill_after = is_fill_after;
}

/** Mark whether this animation is owned by an animation set instead of the core queue. */
void egui_animation_is_inside_animation_set(egui_animation_t *self, int is_inside_animation)
{
    self->is_inside_animation = is_inside_animation;
}

/** Notify the optional external start callback once the animation actually begins running. */
void egui_animation_notify_start(egui_animation_t *self)
{
    if (self->handle && self->handle->start)
    {
        self->handle->start(self);
    }
}

/** Apply configured fill behavior, then notify the optional external end callback. */
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
#if EGUI_CONFIG_FUNCTION_ANIM_COMPLETE_CB
    if (self->on_complete_cb)
    {
        self->on_complete_cb(self, self->on_complete_user_data);
    }
#endif
}

/** Notify the optional external repeat callback after one cycle completes. */
void egui_animation_notify_repeat(egui_animation_t *self)
{
    if (self->handle && self->handle->repeat)
    {
        self->handle->repeat(self);
    }
}

/** Reset runtime state and queue the animation on its owning core unless it is nested in an animation set. */
void egui_animation_start(egui_animation_t *self)
{
    egui_core_t *core;
    egui_core_t *queue_core;

    if (self->target_view == NULL)
    {
        return;
    }

    core = egui_animation_get_core(self);
    if (!self->is_inside_animation && core == NULL)
    {
        return;
    }

    queue_core = self->queue_core;
    if (!self->is_inside_animation && queue_core != NULL)
    {
        // Restarting the same animation instance must first remove its old node
        // from the queue, otherwise the singly-linked list can self-link.
        egui_core_animation_remove(queue_core, self);
    }

    self->start_time = (uint32_t)-1;
    self->is_running = true;
    self->is_started = false;
    self->is_ended = false;
    self->is_cycle_flip = false;
    self->repeated = 0;
#if EGUI_CONFIG_FUNCTION_ANIM_DELAY
    self->is_delay_passed = (self->delay_ms == 0) ? 1u : 0u;
#endif

    if (!self->is_inside_animation)
    {
        self->queue_core = core;
        egui_core_animation_append(core, self);
    }
    else
    {
        self->queue_core = NULL;
    }

    self->api->on_start(self);
}

/** Force the animation to its final state for the current direction, then end it immediately. */
void egui_animation_complete(egui_animation_t *self)
{
    egui_float_t fraction = EGUI_FLOAT_VALUE(1.0f);

    if (self == NULL || !self->is_running)
    {
        return;
    }

    if (!self->is_started)
    {
        self->is_started = true;
        egui_animation_notify_start(self);
    }

    if (self->is_cycle_flip)
    {
        fraction = EGUI_FLOAT_VALUE(0.0f);
    }

    if (self->interpolator)
    {
        fraction = self->interpolator->api->get_interpolation(self->interpolator, fraction);
    }

    self->api->on_update(self, fraction);
    egui_animation_stop(self);
    self->is_ended = true;
    egui_animation_notify_end(self);
}

/** Stop the animation and detach it from the owning core queue when needed. */
void egui_animation_stop(egui_animation_t *self)
{
    if (self->is_running)
    {
        egui_core_t *core;

        self->is_running = false;
        if (!self->is_inside_animation)
        {
            core = self->queue_core;
            if (core != NULL)
            {
                egui_core_animation_remove(core, self);
            }
            self->queue_core = NULL;
        }
    }
}

uint8_t egui_animation_is_running(egui_animation_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    return self->is_running;
}

#if EGUI_CONFIG_FUNCTION_ANIM_PAUSE_RESUME
void egui_animation_pause(egui_animation_t *self)
{
    if (self == NULL || !self->is_running || self->is_paused)
    {
        return;
    }
    if (self->start_time == (uint32_t)-1)
    {
        self->pause_elapsed = 0;
    }
    else
    {
        uint32_t now = egui_timer_get_current_time();
        self->pause_elapsed = now - self->start_time;
    }
    self->is_paused = 1;
}

void egui_animation_resume(egui_animation_t *self)
{
    if (self == NULL || !self->is_paused)
    {
        return;
    }
    self->start_time = egui_timer_get_current_time() - self->pause_elapsed;
    self->is_paused = 0;
}

uint8_t egui_animation_is_paused(egui_animation_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    return self->is_paused;
}
#endif /* EGUI_CONFIG_FUNCTION_ANIM_PAUSE_RESUME */

/** Advance one animation tick, apply interpolation/repeat rules, and dispatch the subclass update hook. */
void egui_animation_update(egui_animation_t *self, uint32_t current_time)
{
    int done = 0;
    uint32_t duration = self->duration;
    egui_float_t fraction = EGUI_FLOAT_VALUE(1.0f);

#if EGUI_CONFIG_FUNCTION_ANIM_PAUSE_RESUME
    if (self->is_paused)
    {
        return; /* suspended — do not advance */
    }
#endif

#if EGUI_CONFIG_FUNCTION_ANIM_DELAY
    /* Handle start delay: wait until delay_ms ms have elapsed since start was called. */
    if (!self->is_delay_passed)
    {
        /* start_time is set to -1 when egui_animation_start() is called; use it to
         * record the moment the animation entered the queue (first update call). */
        if (self->start_time == (uint32_t)-1)
        {
            /* Record the real wall-clock entry time for delay counting. */
            self->start_time = current_time;
        }
        if ((current_time - self->start_time) < (uint32_t)self->delay_ms)
        {
            return; /* still waiting */
        }
        /* Delay expired — reset start_time so the animation duration is measured from now. */
        self->is_delay_passed = 1;
        self->start_time = (uint32_t)-1; /* will be set below */
    }
#endif

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

/** Bind the animation to one target view so it can locate the correct core queue. */
void egui_animation_target_view_set(egui_animation_t *self, egui_view_t *view)
{
    self->target_view = view;
}

egui_view_t *egui_animation_target_view_get(egui_animation_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    return self->target_view;
}

uint8_t egui_animation_is_fill_before_get(egui_animation_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    return self->is_fill_before;
}

uint8_t egui_animation_is_fill_after_get(egui_animation_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    return self->is_fill_after;
}

/** Default on_start hook for subclasses that do not need setup work. */
void egui_animation_on_start(egui_animation_t *self)
{
    EGUI_UNUSED(self);
}

/** Default on_update hook for subclasses that only override the base API selectively. */
void egui_animation_on_update(egui_animation_t *self, egui_float_t fraction)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(fraction);
}

const egui_animation_api_t egui_animation_t_api_table = {
        .on_start = egui_animation_on_start,
        .update = egui_animation_update,
        .on_update = egui_animation_on_update,
};

/** Initialize the base animation object with default flags, no target view, and the base API table. */
void egui_animation_init(egui_animation_t *self)
{
    self->api = &egui_animation_t_api_table;

    self->duration = 0;
    self->interpolator = NULL;
    self->repeat_mode = EGUI_ANIMATION_REPEAT_MODE_RESTART;
    self->repeat_count = 0;
    self->repeated = 0;
    self->handle = NULL;
    self->target_view = NULL;
    self->queue_core = NULL;

    self->is_running = false;
    self->is_started = false;
    self->is_ended = false;
    self->is_cycle_flip = false;
    self->is_fill_before = false;
    self->is_fill_after = false;
    self->is_inside_animation = false;

#if EGUI_CONFIG_FUNCTION_ANIM_DELAY
    self->delay_ms = 0;
    self->is_delay_passed = 1;
#endif

#if EGUI_CONFIG_FUNCTION_ANIM_COMPLETE_CB
    self->on_complete_cb = NULL;
    self->on_complete_user_data = NULL;
#endif
}

#if EGUI_CONFIG_FUNCTION_ANIM_DELAY
void egui_animation_set_delay(egui_animation_t *self, uint16_t delay_ms)
{
    if (self == NULL)
        return;
    self->delay_ms = delay_ms;
}

uint16_t egui_animation_get_delay(egui_animation_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    return self->delay_ms;
}
#endif

#if EGUI_CONFIG_FUNCTION_ANIM_COMPLETE_CB
void egui_animation_set_on_complete(egui_animation_t *self, egui_animation_on_complete_cb_t cb, void *user_data)
{
    if (self == NULL)
        return;
    self->on_complete_cb = cb;
    self->on_complete_user_data = user_data;
}
#endif

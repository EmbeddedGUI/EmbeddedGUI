#include <string.h>

#include "egui_timer.h"
#include "egui_core.h"
#include "egui_api.h"

/**
 * @file egui_timer.c
 * @brief Sorted timer-queue management for one core.
 */

#define EGUI_TIMER_MAX_VALUE          (0xFFFFFFFFU)
#define EGUI_TIMER_MAX_VALUE_OVERFLOW (EGUI_TIMER_MAX_VALUE >> 1)

/** Compare two timestamps with wrap-around handling. Returns non-zero when `time1` is not earlier than `time0`. */
static uint8_t _timer_past(uint32_t time0, uint32_t time1)
{
    if (time0 <= time1)
    {
        return time1 - time0 < EGUI_TIMER_MAX_VALUE_OVERFLOW;
    }

    return time0 - time1 > EGUI_TIMER_MAX_VALUE_OVERFLOW;
}

/** Add two timestamps/intervals using the platform timer's natural wrap-around arithmetic. */
static uint32_t _timer_add(uint32_t time0, uint32_t time1)
{
    return time0 + time1;
}

// static int32_t _timer_sub(uint32_t time0, uint32_t time1)
// {
//     return time0 - time1;
// }

/**
 * @brief Set the target time.
 * @param[in] isEnable: Enable or disable the timer.
 * @param[in] timeout: The time to be set.
 */
// static void _timer_target_set(uint8_t isEnable, uint32_t timeout)
// {
// }

/** Reprogram the platform wakeup source using the earliest queued timer, or stop it when the queue is empty. */
static void _timer_refresh_timeout(egui_core_t *core)
{
    egui_timer_t *t = core->system.timer_root;

    if (t)
    {
        egui_api_timer_start(core, t->expiry_time);
    }
    else
    {
        egui_api_timer_stop(core);
    }
}

/** Remove one timer from the sorted queue. Returns non-zero when the queue head changed and the platform timer must be refreshed. */
static int _timer_remove(egui_core_t *core, egui_timer_t *handle)
{
    int is_refresh = 0;
    __egui_disable_isr();
    egui_timer_t *current = core->system.timer_root;
    egui_timer_t *prev = NULL;

    while ((current != NULL) && (current != handle))
    {
        prev = current;
        current = current->next;
    }

    if (current == handle)
    {
        if (prev)
        {
            prev->next = handle->next;
        }
        else
        {
            core->system.timer_root = current->next;

            is_refresh = 1;
        }

        // Clear the intrusive next pointer so the handle looks idle again.
        handle->next = NULL;
    }
    __egui_enable_isr();

    return is_refresh;
}

/** Check whether the given timer handle is currently linked into the active queue. */
static int _timer_check_in_queue(egui_core_t *core, egui_timer_t *handle)
{
    egui_timer_t *current = core->system.timer_root;

    while ((current != NULL) && (current != handle))
    {
        current = current->next;
    }

    if (current == handle)
    {
        return 1;
    }

    return 0;
}

/** Insert one timer into the queue ordered by expiry time. Returns non-zero when it becomes the new queue head. */
static int _timer_insert(egui_core_t *core, egui_timer_t *handle)
{
    int is_refresh;
    __egui_disable_isr();
    egui_timer_t *current;
    egui_timer_t *prev = NULL;

    // Remove any stale queue entry first so restart/update always behaves like a replace.
    is_refresh = _timer_remove(core, handle);

    current = core->system.timer_root;

    while ((current != NULL) && _timer_past(current->expiry_time, handle->expiry_time))
    {
        prev = current;
        current = current->next;
    }

    handle->next = current;

    if (prev == NULL)
    {
        // A new earliest deadline means the platform wakeup source must be updated.
        core->system.timer_root = handle;

        is_refresh = 1;
    }
    else
    {
        prev->next = handle;
    }
    __egui_enable_isr();

    return is_refresh;
}

/** Start or restart one timer handle at an absolute expiry timestamp. */
static int _start_timer(egui_core_t *core, egui_timer_t *handle, uint32_t time)
{
    int is_refresh;

    // Overwrite the target deadline before reinserting the handle.
    handle->expiry_time = time;

    is_refresh = _timer_insert(core, handle);

    return is_refresh;
}

/**
 * Expire every timer that is due right now.
 * Periodic timers are rescheduled before their callback runs so the callback sees the timer as active and can stop/restart it safely if needed.
 */
static void _timer_expire(egui_core_t *core)
{
    int is_refresh = 0;
    egui_timer_t *t;
    while ((t = core->system.timer_root) != NULL && _timer_past(t->expiry_time, egui_api_timer_get_current_core(core)))
    {
        // Periodic timers are moved to their next deadline; one-shots are simply unlinked.
        if (t->period)
        {
            is_refresh |= _start_timer(core, t, _timer_add(egui_api_timer_get_current_core(core), t->period));
        }
        else
        {
            is_refresh |= _timer_remove(core, t);
        }

        // Callbacks run after queue maintenance so they can freely manipulate the timer again.
        if (t->callback)
        {
            t->callback(t);
        }
    }

    if (is_refresh)
    {
        _timer_refresh_timeout(core);
    }
}

/** Force the platform wakeup source to match the current queue head. */
void egui_timer_force_refresh_timer(egui_core_t *core)
{
    _timer_refresh_timeout(core);
}

/** Poll the timer queue and dispatch every timer whose deadline has passed. */
void egui_timer_polling_work(egui_core_t *core)
{
    _timer_expire(core);
}

/** Start or restart one timer after `ms` milliseconds, optionally as a periodic timer. */
int egui_timer_start_timer(egui_core_t *core, egui_timer_t *handle, uint32_t ms, uint32_t period)
{
    if (handle == NULL)
    {
        return 1;
    }

    handle->period = period;
    if (_start_timer(core, handle, _timer_add(egui_api_timer_get_current_core(core), ms)))
    {
        _timer_refresh_timeout(core);
    }
    return 0;
}

/** Stop one timer if it is currently queued. */
void egui_timer_stop_timer(egui_core_t *core, egui_timer_t *handle)
{
    if (_timer_remove(core, handle))
    {
        _timer_refresh_timeout(core);
    }
}

/** Return non-zero when the timer handle is currently active. */
int egui_timer_check_timer_start(egui_core_t *core, egui_timer_t *handle)
{
    return _timer_check_in_queue(core, handle);
}

/** Return the current global platform tick count in milliseconds. */
uint32_t egui_timer_get_current_time(void)
{
    return egui_api_timer_get_current();
}

/** Reset one timer handle to its idle state and attach callback/user data. */
void egui_timer_init_timer(egui_timer_t *handle, void *user_data, egui_timer_callback_func callback)
{
    handle->next = NULL;
    handle->expiry_time = EGUI_TIMER_ZERO;
    handle->period = EGUI_TIMER_ZERO;
    handle->user_data = user_data;
    handle->callback = callback;
}

/** Initialize the per-core timer subsystem with an empty queue. */
void egui_timer_init(egui_core_t *core)
{
    core->system.timer_root = NULL;
}

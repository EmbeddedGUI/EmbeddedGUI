#include <string.h>

#include "egui_timer.h"
#include "egui_core.h"
#include "egui_api.h"

#define EGUI_TIMER_MAX_VALUE          (0xFFFFFFFFU)
#define EGUI_TIMER_MAX_VALUE_OVERFLOW (EGUI_TIMER_MAX_VALUE >> 1)

static uint8_t _timer_past(uint32_t time0, uint32_t time1)
{
    if (time0 <= time1)
    {
        return time1 - time0 < EGUI_TIMER_MAX_VALUE_OVERFLOW;
    }

    return time0 - time1 > EGUI_TIMER_MAX_VALUE_OVERFLOW;
}

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

/**
 * @brief Refresh the timeout.
 */
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

/**
 * @brief Add a timer to the timer list.
 * @param[in] time: The time to be added.
 * @return 1 if the timer is the new root. 0 if the timer is not the new root.
 */
static int _timer_remove(egui_core_t *core, egui_timer_t *handle)
{
    int is_refresh = 0;
    egui_base_t level = egui_hw_interrupt_disable_core(core);
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

        // clear next pointer
        handle->next = NULL;
    }
    egui_hw_interrupt_enable_core(core, level);

    return is_refresh;
}

/**
 * @brief Check the timer in the queue.
 * @param[in] handle: The virtual timer
 * @return 1 if the timer is in the queue. 0 if the timer is not in the queue.
 */
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

/**
 * @brief Insert a timer into the timer list.
 * @param[in] handle: The virtual timer
 * @return 1 if the timer is the new root. 0 if the timer is not the new root.
 */
static int _timer_insert(egui_core_t *core, egui_timer_t *handle)
{
    int is_refresh;
    egui_base_t level = egui_hw_interrupt_disable_core(core);
    egui_timer_t *current;
    egui_timer_t *prev = NULL;

    /* Force stop timer */
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
        /* We are the new root */
        core->system.timer_root = handle;

        is_refresh = 1;
    }
    else
    {
        prev->next = handle;
    }
    egui_hw_interrupt_enable_core(core, level);

    return is_refresh;
}

/**
 * @brief Start a timer.
 * @param[in] handle: The virtual timer
 * @param[in] time: The time to be started.
 */
static int _start_timer(egui_core_t *core, egui_timer_t *handle, uint32_t time)
{
    int is_refresh;

    /* The timer is already started */
    handle->expiry_time = time;

    is_refresh = _timer_insert(core, handle);

    return is_refresh;
}

/**
 * @brief Timer expire.
 */
static void _timer_expire(egui_core_t *core)
{
    int is_refresh = 0;
    egui_timer_t *t;
    while ((t = core->system.timer_root) != NULL && _timer_past(t->expiry_time, egui_api_timer_get_current_core(core)))
    {
        if (t->period)
        {
            is_refresh |= _start_timer(core, t, _timer_add(egui_api_timer_get_current_core(core), t->period));
        }
        else
        {
            is_refresh |= _timer_remove(core, t);
        }

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

void egui_timer_force_refresh_timer(egui_core_t *core)
{
    _timer_refresh_timeout(core);
}

void egui_timer_polling_work(egui_core_t *core)
{
    _timer_expire(core);
}

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

void egui_timer_stop_timer(egui_core_t *core, egui_timer_t *handle)
{
    if (_timer_remove(core, handle))
    {
        _timer_refresh_timeout(core);
    }
}

int egui_timer_check_timer_start(egui_core_t *core, egui_timer_t *handle)
{
    return _timer_check_in_queue(core, handle);
}

void egui_timer_init_timer(egui_timer_t *handle, void *user_data, egui_timer_callback_func callback)
{
    handle->next = NULL;
    handle->expiry_time = EGUI_TIMER_ZERO;
    handle->period = EGUI_TIMER_ZERO;
    handle->user_data = user_data;
    handle->callback = callback;
}

void egui_timer_init(egui_core_t *core)
{
    core->system.timer_root = NULL;
}

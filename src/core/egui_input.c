#include <stdio.h>
#include <assert.h>

#include "egui_input.h"
#include "egui_motion_event.h"
#include "egui_api.h"

#include "egui_core.h"

#include "egui_rotation.h"
#include "egui_display_driver.h"

#include "utils/simple_ringbuffer/simple_pool.h"

/**
 * @file egui_input.c
 * @brief Per-core input queues that bridge touch/key drivers into the core event pipeline.
 */

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
#include "egui_key_event.h"
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
/* Convenience macro: dereference the active input state stored inside the core. */
#define egui_input_info (core->touch.input)

#define input_motion_pool              (egui_input_info.motion_pool)
#define input_motion_pool_fifo_storage (egui_input_info.motion_pool_fifo_storage)
#define input_motion_pool_data_storage (egui_input_info.motion_pool_data_storage)

#include "egui_touch_driver.h"

/** Queue one single-pointer motion event, coalescing consecutive MOVE events to save queue slots. */
int egui_input_add_motion(egui_core_t *core, uint8_t type, egui_dim_t x, egui_dim_t y)
{
    // EGUI_LOG_DBG("egui_input_add_motion type:%d x:%d y:%d\n", type, x, y);
    __egui_disable_isr();

    egui_motion_event_t *motion_event;
    // Merge move events with the same type and location
    egui_motion_event_t *last_motion_event;
    int is_reused = 0;

    last_motion_event = (egui_motion_event_t *)egui_slist_peek_tail(&egui_input_info.motion_list);
    if (last_motion_event != NULL)
    {
        if (last_motion_event->type == type && last_motion_event->type == EGUI_MOTION_EVENT_ACTION_MOVE)
        {
            // Reuse the last queued MOVE node so fast drags do not exhaust the fixed pool.
            motion_event = last_motion_event;
            is_reused = 1;
        }
    }
    if (!is_reused)
    {
        // Get a motion event from the pool
        if (!SIMPLE_POOL_DEQUEUE(&input_motion_pool, motion_event))
        {
            EGUI_LOG_DBG("egui_input_add_motion failed type:%d x:%d y:%d\n", type, x, y);
            __egui_enable_isr();
            return 0;
        }
    }
    // Save info and append to motion list
    motion_event->type = type;
    motion_event->location.x = x;
    motion_event->location.y = y;
    motion_event->timestamp = egui_api_timer_get_current_core(core);
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    motion_event->pointer_count = 1;
    motion_event->location2.x = 0;
    motion_event->location2.y = 0;
    motion_event->scroll_delta = 0;
#endif

#if EGUI_CONFIG_FUNCTION_INPUT_VELOCITY_TRACKER
    // Feed the tracker from the same event stream that will later be dispatched into the core.
    egui_velocity_tracker_add_motion(&egui_input_info.velocity_tracker, motion_event);
#endif
    if (!is_reused)
    {
        egui_slist_append(&egui_input_info.motion_list, &motion_event->node);
    }

    __egui_enable_isr();
    return 1;
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
/** Queue one multi-touch motion event, using the same MOVE coalescing policy as single-touch input. */
int egui_input_add_motion_multi(egui_core_t *core, uint8_t type, uint8_t pointer_count, egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2)
{
    __egui_disable_isr();
    egui_motion_event_t *motion_event;
    egui_motion_event_t *last_motion_event;
    int is_reused = 0;

    // Merge consecutive multi-touch MOVE events
    last_motion_event = (egui_motion_event_t *)egui_slist_peek_tail(&egui_input_info.motion_list);
    if (last_motion_event != NULL)
    {
        if (last_motion_event->type == type && last_motion_event->type == EGUI_MOTION_EVENT_ACTION_MOVE)
        {
            motion_event = last_motion_event;
            is_reused = 1;
        }
    }
    if (!is_reused)
    {
        if (!SIMPLE_POOL_DEQUEUE(&input_motion_pool, motion_event))
        {
            __egui_enable_isr();
            return 0;
        }
    }

    motion_event->type = type;
    motion_event->location.x = x1;
    motion_event->location.y = y1;
    motion_event->timestamp = egui_api_timer_get_current_core(core);
    motion_event->pointer_count = pointer_count;
    motion_event->location2.x = x2;
    motion_event->location2.y = y2;
    motion_event->scroll_delta = 0;

#if EGUI_CONFIG_FUNCTION_INPUT_VELOCITY_TRACKER
    egui_velocity_tracker_add_motion(&egui_input_info.velocity_tracker, motion_event);
#endif
    if (!is_reused)
    {
        egui_slist_append(&egui_input_info.motion_list, &motion_event->node);
    }
    __egui_enable_isr();
    return 1;
}

/** Queue one scroll-wheel style motion event that carries a delta instead of a pressed state. */
int egui_input_add_scroll(egui_core_t *core, egui_dim_t x, egui_dim_t y, int16_t delta)
{
    __egui_disable_isr();
    egui_motion_event_t *motion_event;

    if (!SIMPLE_POOL_DEQUEUE(&input_motion_pool, motion_event))
    {
        __egui_enable_isr();
        return 0;
    }

    motion_event->type = EGUI_MOTION_EVENT_ACTION_SCROLL;
    motion_event->location.x = x;
    motion_event->location.y = y;
    motion_event->timestamp = egui_api_timer_get_current_core(core);
    motion_event->pointer_count = 0;
    motion_event->location2.x = 0;
    motion_event->location2.y = 0;
    motion_event->scroll_delta = delta;

    egui_slist_append(&egui_input_info.motion_list, &motion_event->node);
    __egui_enable_isr();
    return 1;
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH

/** Return the last measured horizontal fling velocity, or zero when tracking is disabled. */
egui_float_t egui_input_get_velocity_x(egui_core_t *core)
{
#if !EGUI_CONFIG_FUNCTION_INPUT_VELOCITY_TRACKER
    return 0;
#else
    return egui_input_info.velocity_tracker.velocity_x;
#endif
}

/** Return the last measured vertical fling velocity, or zero when tracking is disabled. */
egui_float_t egui_input_get_velocity_y(egui_core_t *core)
{
#if !EGUI_CONFIG_FUNCTION_INPUT_VELOCITY_TRACKER
    return 0;
#else
    return egui_input_info.velocity_tracker.velocity_y;
#endif
}

/** Report whether the queued motion-event list is currently empty. */
int egui_input_check_idle(egui_core_t *core)
{
    return egui_slist_is_empty(&egui_input_info.motion_list);
}

/**
 * Poll the registered touch driver once, convert its state changes into queued
 * motion events, then drain the queue and dispatch each event into the core.
 */
void egui_input_polling_work(egui_core_t *core)
{
    egui_motion_event_t *motion_event;

    // Sample the touch driver first so polling-mode ports still feed the same queue-based pipeline.
    egui_touch_driver_t *tdrv = egui_touch_driver_get(core);
    if (tdrv != NULL && tdrv->ops->read != NULL)
    {
        uint8_t pressed = 0;
        uint8_t has_position = 0;
        int16_t tx = 0, ty = 0;
        if (tdrv->ops->read_ex != NULL)
        {
            tdrv->ops->read_ex(core, &pressed, &tx, &ty, &has_position);
        }
        else
        {
            tdrv->ops->read(core, &pressed, &tx, &ty);
            has_position = pressed;
        }

        // Translate the current raw pressed/not-pressed state into DOWN/MOVE/UP edge events.
        if (pressed && !core->touch.prev_pressed)
        {
            egui_input_add_motion(core, EGUI_MOTION_EVENT_ACTION_DOWN, tx, ty);
        }
        else if (pressed && core->touch.prev_pressed)
        {
            if (tx != core->touch.prev_x || ty != core->touch.prev_y)
            {
                egui_input_add_motion(core, EGUI_MOTION_EVENT_ACTION_MOVE, tx, ty);
            }
        }
        else if (!pressed && core->touch.prev_pressed)
        {
            // Prefer a driver-reported release coordinate when available.
            // Fallback to the last pressed coordinate for ports that cannot report it.
            egui_input_add_motion(core, EGUI_MOTION_EVENT_ACTION_UP, has_position ? tx : core->touch.prev_x, has_position ? ty : core->touch.prev_y);
        }
        core->touch.prev_pressed = pressed;
        if (pressed || has_position)
        {
            core->touch.prev_x = tx;
            core->touch.prev_y = ty;
        }
    }

    // Drain the motion queue in FIFO order so producer-side interrupt handlers stay short.
    while (1)
    {
        {
            __egui_disable_isr();
            motion_event = (egui_motion_event_t *)egui_slist_get(&egui_input_info.motion_list);
            __egui_enable_isr();
        }

        if (motion_event == NULL)
        {
            break;
        }

        // EGUI_LOG_DBG("egui_input_polling_work type:%d x:%d y:%d\n", motion_event->type, motion_event->location.x, motion_event->location.y);

        // Convert native panel coordinates back into logical GUI coordinates when software rotation owns the panel transform.
        if (core->render.software_rotation)
        {
            egui_display_driver_t *drv = egui_display_driver_get(core);
            if (drv != NULL && drv->rotation != EGUI_DISPLAY_ROTATION_0 && drv->ops->set_rotation == NULL)
            {
                egui_rotation_transform_touch(drv->rotation, drv->physical_width, drv->physical_height, &motion_event->location.x, &motion_event->location.y);
            }
        }

        // Hand the normalized event to the rest of the input pipeline.
        egui_core_process_input_motion(core, motion_event);

        // Return the node to the fixed pool so the next producer can reuse it.
        {
            __egui_disable_isr();
            SIMPLE_POOL_ENQUEUE(&input_motion_pool, motion_event);
            __egui_enable_isr();
        }
    }
}

/** Initialize the touch-side queues, pools, velocity tracker, and previous sampled state. */
void egui_input_init(egui_core_t *core)
{
    simple_pool_init(&input_motion_pool, input_motion_pool_fifo_storage, (uint8_t *)input_motion_pool_data_storage, EGUI_CONFIG_INPUT_MOTION_CACHE_COUNT,
                     sizeof(egui_motion_event_t));

    egui_slist_init(&egui_input_info.motion_list);
#if EGUI_CONFIG_FUNCTION_INPUT_VELOCITY_TRACKER
    egui_velocity_tracker_init(&egui_input_info.velocity_tracker);
#endif
    core->touch.prev_pressed = 0;
    core->touch.prev_x = 0;
    core->touch.prev_y = 0;

    // Initialize touch driver if registered
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY

#define input_key_pool              (egui_input_info.key_pool)
#define input_key_pool_fifo_storage (egui_input_info.key_pool_fifo_storage)
#define input_key_pool_data_storage (egui_input_info.key_pool_data_storage)

/** Queue one key event into the fixed-capacity FIFO used by keyboard dispatch. */
int egui_input_add_key(egui_core_t *core, uint8_t type, uint8_t key_code, uint8_t is_shift, uint8_t is_ctrl)
{
    __egui_disable_isr();
    egui_key_event_t *key_event;

    // Get a key event from the pool
    if (!SIMPLE_POOL_DEQUEUE(&input_key_pool, key_event))
    {
        EGUI_LOG_DBG("egui_input_add_key failed type:%d key_code:%d\n", type, key_code);
        __egui_enable_isr();
        return 0;
    }

    key_event->type = type;
    key_event->key_code = key_code;
    key_event->is_shift = is_shift;
    key_event->is_ctrl = is_ctrl;
    key_event->reserved = 0;
    key_event->timestamp = egui_api_timer_get_current_core(core);

    egui_slist_append(&egui_input_info.key_list, &key_event->node);
    __egui_enable_isr();

    return 1;
}

/** Report whether the queued key-event list is currently empty. */
int egui_input_check_key_idle(egui_core_t *core)
{
    return egui_slist_is_empty(&egui_input_info.key_list);
}

/** Drain the queued key events and forward them into the core in FIFO order. */
void egui_input_key_dispatch_work(egui_core_t *core)
{
    egui_key_event_t *key_event;

    // Process queued key events
    while ((key_event = (egui_key_event_t *)egui_slist_get(&egui_input_info.key_list)) != NULL)
    {
        egui_core_process_input_key(core, key_event);

        // Put the key event back to the pool
        SIMPLE_POOL_ENQUEUE(&input_key_pool, key_event);
    }
}

/** Initialize the key-event queue and its fixed pool storage. */
void egui_input_key_init(egui_core_t *core)
{
    simple_pool_init(&input_key_pool, input_key_pool_fifo_storage, (uint8_t *)input_key_pool_data_storage, EGUI_CONFIG_INPUT_KEY_CACHE_COUNT,
                     sizeof(egui_key_event_t));

    egui_slist_init(&egui_input_info.key_list);
}

#endif // EGUI_CONFIG_FUNCTION_SUPPORT_KEY

#if !EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH && !EGUI_CONFIG_FUNCTION_SUPPORT_KEY

/** No-op stub used when both touch and key support are compiled out. */
int egui_input_add_motion(egui_core_t *core, uint8_t type, egui_dim_t x, egui_dim_t y)
{
    EGUI_UNUSED(type);
    EGUI_UNUSED(x);
    EGUI_UNUSED(y);
    return 0;
}

/** No-op stub used when both touch and key support are compiled out. */
egui_float_t egui_input_get_velocity_x(egui_core_t *core)
{
    return 0;
}

/** No-op stub used when both touch and key support are compiled out. */
egui_float_t egui_input_get_velocity_y(egui_core_t *core)
{
    return 0;
}

/** No-op stub used when both touch and key support are compiled out. */
int egui_input_check_idle(egui_core_t *core)
{
    return 1;
}

/** No-op stub used when both touch and key support are compiled out. */
void egui_input_polling_work(egui_core_t *core)
{
}

/** No-op stub used when both touch and key support are compiled out. */
void egui_input_init(egui_core_t *core)
{
}

#endif

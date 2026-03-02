#include <stdio.h>
#include <assert.h>

#include "egui_input.h"
#include "egui_motion_event.h"
#include "egui_api.h"

#include "egui_core.h"

#include "utils/simple_ringbuffer/simple_pool.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
#include "egui_key_event.h"
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
egui_input_t egui_input_info;
SIMPLE_POOL_DEFINE(input_motion_pool, EGUI_CONFIG_INPUT_MOTION_CACHE_COUNT, sizeof(egui_motion_event_t));

#include "egui_touch_driver.h"

/** Previous touch state for edge detection */
static uint8_t egui_touch_prev_pressed = 0;

int egui_input_add_motion(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    // EGUI_LOG_DBG("egui_input_add_motion type:%d x:%d y:%d\n", type, x, y);
    egui_base_t level = egui_hw_interrupt_disable();

    egui_motion_event_t *motion_event;
    // Merge move events with the same type and location
    egui_motion_event_t *last_motion_event;
    int is_reused = 0;

    last_motion_event = (egui_motion_event_t *)egui_slist_peek_tail(&egui_input_info.motion_list);
    if (last_motion_event != NULL)
    {
        if (last_motion_event->type == type && last_motion_event->type == EGUI_MOTION_EVENT_ACTION_MOVE)
        {
            // reuse the last motion event
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
            egui_hw_interrupt_enable(level);
            return 0;
        }
    }
    // Save info and append to motion list
    motion_event->type = type;
    motion_event->location.x = x;
    motion_event->location.y = y;
    motion_event->timestamp = egui_api_timer_get_current();
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    motion_event->pointer_count = 1;
    motion_event->location2.x = 0;
    motion_event->location2.y = 0;
    motion_event->scroll_delta = 0;
#endif

    egui_velocity_tracker_add_motion(&egui_input_info.velocity_tracker, motion_event);
    if (!is_reused)
    {
        egui_slist_append(&egui_input_info.motion_list, &motion_event->node);
    }

    egui_hw_interrupt_enable(level);
    return 1;
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
int egui_input_add_motion_multi(uint8_t type, uint8_t pointer_count, egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2)
{
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
            return 0;
        }
    }

    motion_event->type = type;
    motion_event->location.x = x1;
    motion_event->location.y = y1;
    motion_event->timestamp = egui_api_timer_get_current();
    motion_event->pointer_count = pointer_count;
    motion_event->location2.x = x2;
    motion_event->location2.y = y2;
    motion_event->scroll_delta = 0;

    egui_velocity_tracker_add_motion(&egui_input_info.velocity_tracker, motion_event);
    if (!is_reused)
    {
        egui_slist_append(&egui_input_info.motion_list, &motion_event->node);
    }
    return 1;
}

int egui_input_add_scroll(egui_dim_t x, egui_dim_t y, int16_t delta)
{
    egui_motion_event_t *motion_event;

    if (!SIMPLE_POOL_DEQUEUE(&input_motion_pool, motion_event))
    {
        return 0;
    }

    motion_event->type = EGUI_MOTION_EVENT_ACTION_SCROLL;
    motion_event->location.x = x;
    motion_event->location.y = y;
    motion_event->timestamp = egui_api_timer_get_current();
    motion_event->pointer_count = 0;
    motion_event->location2.x = 0;
    motion_event->location2.y = 0;
    motion_event->scroll_delta = delta;

    egui_slist_append(&egui_input_info.motion_list, &motion_event->node);
    return 1;
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH

egui_float_t egui_input_get_velocity_x(void)
{
    return egui_input_info.velocity_tracker.velocity_x;
}

egui_float_t egui_input_get_velocity_y(void)
{
    return egui_input_info.velocity_tracker.velocity_y;
}

int egui_input_check_idle(void)
{
    return egui_slist_is_empty(&egui_input_info.motion_list);
}

void egui_input_polling_work(void)
{
    egui_motion_event_t *motion_event;

    // Poll touch driver if registered
    egui_touch_driver_t *tdrv = egui_touch_driver_get();
    if (tdrv != NULL && tdrv->ops->read != NULL)
    {
        uint8_t pressed = 0;
        int16_t tx = 0, ty = 0;
        tdrv->ops->read(&pressed, &tx, &ty);

        if (pressed && !egui_touch_prev_pressed)
        {
            egui_input_add_motion(EGUI_MOTION_EVENT_ACTION_DOWN, tx, ty);
        }
        else if (pressed && egui_touch_prev_pressed)
        {
            egui_input_add_motion(EGUI_MOTION_EVENT_ACTION_MOVE, tx, ty);
        }
        else if (!pressed && egui_touch_prev_pressed)
        {
            egui_input_add_motion(EGUI_MOTION_EVENT_ACTION_UP, tx, ty);
        }
        egui_touch_prev_pressed = pressed;
    }

    while (1)
    {
        egui_base_t level = egui_hw_interrupt_disable();
        motion_event = (egui_motion_event_t *)egui_slist_get(&egui_input_info.motion_list);
        egui_hw_interrupt_enable(level);

        if (motion_event == NULL)
        {
            break;
        }

        // EGUI_LOG_DBG("egui_input_polling_work type:%d x:%d y:%d\n", motion_event->type, motion_event->location.x, motion_event->location.y);
        // handle motion event
        egui_core_process_input_motion(motion_event);

        // Put the motion event back to the pool
        level = egui_hw_interrupt_disable();
        SIMPLE_POOL_ENQUEUE(&input_motion_pool, motion_event);
        egui_hw_interrupt_enable(level);
    }
}

void egui_input_init(void)
{
    SIMPLE_POOL_INIT(input_motion_pool, EGUI_CONFIG_INPUT_MOTION_CACHE_COUNT, sizeof(egui_motion_event_t));

    egui_slist_init(&egui_input_info.motion_list);
    egui_velocity_tracker_init(&egui_input_info.velocity_tracker);
    egui_touch_prev_pressed = 0;

    // Initialize touch driver if registered
    egui_touch_driver_t *tdrv = egui_touch_driver_get();
    if (tdrv != NULL && tdrv->ops->init != NULL)
    {
        tdrv->ops->init();
    }
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY

#if !EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
egui_input_t egui_input_info;
#endif

SIMPLE_POOL_DEFINE(input_key_pool, EGUI_CONFIG_INPUT_KEY_CACHE_COUNT, sizeof(egui_key_event_t));

int egui_input_add_key(uint8_t type, uint8_t key_code, uint8_t is_shift, uint8_t is_ctrl)
{
    egui_key_event_t *key_event;

    // Get a key event from the pool
    if (!SIMPLE_POOL_DEQUEUE(&input_key_pool, key_event))
    {
        EGUI_LOG_DBG("egui_input_add_key failed type:%d key_code:%d\n", type, key_code);
        return 0;
    }

    key_event->type = type;
    key_event->key_code = key_code;
    key_event->is_shift = is_shift;
    key_event->is_ctrl = is_ctrl;
    key_event->reserved = 0;
    key_event->timestamp = egui_api_timer_get_current();

    egui_slist_append(&egui_input_info.key_list, &key_event->node);

    return 1;
}

int egui_input_check_key_idle(void)
{
    return egui_slist_is_empty(&egui_input_info.key_list);
}

void egui_input_key_dispatch_work(void)
{
    egui_key_event_t *key_event;

    // Process queued key events
    while ((key_event = (egui_key_event_t *)egui_slist_get(&egui_input_info.key_list)) != NULL)
    {
        egui_core_process_input_key(key_event);

        // Put the key event back to the pool
        SIMPLE_POOL_ENQUEUE(&input_key_pool, key_event);
    }
}

void egui_input_key_init(void)
{
    SIMPLE_POOL_INIT(input_key_pool, EGUI_CONFIG_INPUT_KEY_CACHE_COUNT, sizeof(egui_key_event_t));

    egui_slist_init(&egui_input_info.key_list);
}

#endif // EGUI_CONFIG_FUNCTION_SUPPORT_KEY

#include <stdio.h>
#include <assert.h>

#include "egui_input.h"
#include "egui_motion_event.h"
#include "egui_api.h"

#include "egui_core.h"

#include "egui_rotation.h"
#include "egui_display_driver.h"

#include "utils/simple_ringbuffer/simple_pool.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
#include "egui_key_event.h"
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
/* Convenience macro: dereference the active input pointer */
#define egui_input_info (core->touch.input)

#define input_motion_pool              (egui_input_info.motion_pool)
#define input_motion_pool_fifo_storage (egui_input_info.motion_pool_fifo_storage)
#define input_motion_pool_data_storage (egui_input_info.motion_pool_data_storage)

#include "egui_touch_driver.h"

static egui_base_t egui_input_interrupt_disable_if_available(egui_core_t *core, int *locked)
{
    if (locked != NULL)
    {
        *locked = 0;
    }

    if (core == NULL || core->render.platform == NULL || core->render.platform->ops == NULL || core->render.platform->ops->interrupt_disable == NULL)
    {
        return 0;
    }

    if (locked != NULL)
    {
        *locked = 1;
    }

    return core->render.platform->ops->interrupt_disable();
}

static void egui_input_interrupt_enable_if_locked(egui_core_t *core, egui_base_t level, int locked)
{
    if (!locked || core == NULL || core->render.platform == NULL || core->render.platform->ops == NULL || core->render.platform->ops->interrupt_enable == NULL)
    {
        return;
    }

    core->render.platform->ops->interrupt_enable(level);
}

int egui_input_add_motion(egui_core_t *core, uint8_t type, egui_dim_t x, egui_dim_t y)
{
    // EGUI_LOG_DBG("egui_input_add_motion type:%d x:%d y:%d\n", type, x, y);
    int locked;
    egui_base_t level = egui_input_interrupt_disable_if_available(core, &locked);

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
            egui_input_interrupt_enable_if_locked(core, level, locked);
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

#if EGUI_CONFIG_INPUT_VELOCITY_TRACKER_ENABLE
    egui_velocity_tracker_add_motion(&egui_input_info.velocity_tracker, motion_event);
#endif
    if (!is_reused)
    {
        egui_slist_append(&egui_input_info.motion_list, &motion_event->node);
    }

    egui_input_interrupt_enable_if_locked(core, level, locked);
    return 1;
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
int egui_input_add_motion_multi(egui_core_t *core, uint8_t type, uint8_t pointer_count, egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2)
{
    int locked;
    egui_base_t level = egui_input_interrupt_disable_if_available(core, &locked);
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
            egui_input_interrupt_enable_if_locked(core, level, locked);
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

#if EGUI_CONFIG_INPUT_VELOCITY_TRACKER_ENABLE
    egui_velocity_tracker_add_motion(&egui_input_info.velocity_tracker, motion_event);
#endif
    if (!is_reused)
    {
        egui_slist_append(&egui_input_info.motion_list, &motion_event->node);
    }
    egui_input_interrupt_enable_if_locked(core, level, locked);
    return 1;
}

int egui_input_add_scroll(egui_core_t *core, egui_dim_t x, egui_dim_t y, int16_t delta)
{
    int locked;
    egui_base_t level = egui_input_interrupt_disable_if_available(core, &locked);
    egui_motion_event_t *motion_event;

    if (!SIMPLE_POOL_DEQUEUE(&input_motion_pool, motion_event))
    {
        egui_input_interrupt_enable_if_locked(core, level, locked);
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
    egui_input_interrupt_enable_if_locked(core, level, locked);
    return 1;
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH

egui_float_t egui_input_get_velocity_x(egui_core_t *core)
{
#if !EGUI_CONFIG_INPUT_VELOCITY_TRACKER_ENABLE
    return 0;
#else
    return egui_input_info.velocity_tracker.velocity_x;
#endif
}

egui_float_t egui_input_get_velocity_y(egui_core_t *core)
{
#if !EGUI_CONFIG_INPUT_VELOCITY_TRACKER_ENABLE
    return 0;
#else
    return egui_input_info.velocity_tracker.velocity_y;
#endif
}

int egui_input_check_idle(egui_core_t *core)
{
    return egui_slist_is_empty(&egui_input_info.motion_list);
}

void egui_input_polling_work(egui_core_t *core)
{
    egui_motion_event_t *motion_event;

    // Poll touch driver if registered
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

    while (1)
    {
        egui_base_t level = egui_hw_interrupt_disable_core(core);
        motion_event = (egui_motion_event_t *)egui_slist_get(&egui_input_info.motion_list);
        egui_hw_interrupt_enable_core(core, level);

        if (motion_event == NULL)
        {
            break;
        }

        // EGUI_LOG_DBG("egui_input_polling_work type:%d x:%d y:%d\n", motion_event->type, motion_event->location.x, motion_event->location.y);

        // Runtime software rotation: transform physical touch coords to logical coords
        if (core->render.software_rotation)
        {
            egui_display_driver_t *drv = egui_display_driver_get(core);
            if (drv != NULL && drv->rotation != EGUI_DISPLAY_ROTATION_0 && drv->ops->set_rotation == NULL)
            {
                egui_rotation_transform_touch(drv->rotation, drv->physical_width, drv->physical_height, &motion_event->location.x, &motion_event->location.y);
            }
        }

        // handle motion event
        egui_core_process_input_motion(core, motion_event);

        // Put the motion event back to the pool
        level = egui_hw_interrupt_disable_core(core);
        SIMPLE_POOL_ENQUEUE(&input_motion_pool, motion_event);
        egui_hw_interrupt_enable_core(core, level);
    }
}

void egui_input_init(egui_core_t *core)
{
    simple_pool_init(&input_motion_pool, input_motion_pool_fifo_storage, (uint8_t *)input_motion_pool_data_storage, EGUI_CONFIG_INPUT_MOTION_CACHE_COUNT,
                     sizeof(egui_motion_event_t));

    egui_slist_init(&egui_input_info.motion_list);
#if EGUI_CONFIG_INPUT_VELOCITY_TRACKER_ENABLE
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

int egui_input_add_key(egui_core_t *core, uint8_t type, uint8_t key_code, uint8_t is_shift, uint8_t is_ctrl)
{
    int locked;
    egui_base_t level = egui_input_interrupt_disable_if_available(core, &locked);
    egui_key_event_t *key_event;

    // Get a key event from the pool
    if (!SIMPLE_POOL_DEQUEUE(&input_key_pool, key_event))
    {
        EGUI_LOG_DBG("egui_input_add_key failed type:%d key_code:%d\n", type, key_code);
        egui_input_interrupt_enable_if_locked(core, level, locked);
        return 0;
    }

    key_event->type = type;
    key_event->key_code = key_code;
    key_event->is_shift = is_shift;
    key_event->is_ctrl = is_ctrl;
    key_event->reserved = 0;
    key_event->timestamp = egui_api_timer_get_current_core(core);

    egui_slist_append(&egui_input_info.key_list, &key_event->node);
    egui_input_interrupt_enable_if_locked(core, level, locked);

    return 1;
}

int egui_input_check_key_idle(egui_core_t *core)
{
    return egui_slist_is_empty(&egui_input_info.key_list);
}

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

void egui_input_key_init(egui_core_t *core)
{
    simple_pool_init(&input_key_pool, input_key_pool_fifo_storage, (uint8_t *)input_key_pool_data_storage, EGUI_CONFIG_INPUT_KEY_CACHE_COUNT,
                     sizeof(egui_key_event_t));

    egui_slist_init(&egui_input_info.key_list);
}

#endif // EGUI_CONFIG_FUNCTION_SUPPORT_KEY

#if !EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH && !EGUI_CONFIG_FUNCTION_SUPPORT_KEY

int egui_input_add_motion(egui_core_t *core, uint8_t type, egui_dim_t x, egui_dim_t y)
{
    EGUI_UNUSED(type);
    EGUI_UNUSED(x);
    EGUI_UNUSED(y);
    return 0;
}

egui_float_t egui_input_get_velocity_x(egui_core_t *core)
{
    return 0;
}

egui_float_t egui_input_get_velocity_y(egui_core_t *core)
{
    return 0;
}

int egui_input_check_idle(egui_core_t *core)
{
    return 1;
}

void egui_input_polling_work(egui_core_t *core)
{
}

void egui_input_init(egui_core_t *core)
{
}

#endif

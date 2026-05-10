#include <stdio.h>
#include <assert.h>

#include "egui_input.h"
#include "egui_motion_event.h"
#include "egui_api.h"

#include "egui_core.h"

#if EGUI_CONFIG_FUNCTION_SOFTWARE_ROTATION_ENABLE
#include "egui_rotation.h"
#endif
#include "egui_display_driver.h"

#include "utils/simple_ringbuffer/simple_pool.h"

/**
 * @file egui_input.c
 * @brief Per-core input queues that bridge touch/key drivers into the core event pipeline.
 */

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
#include "egui_key_event.h"
#endif

/* Convenience macro: dereference the active input state stored inside the core. */
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
#define egui_input_info (core->touch.input)
#elif EGUI_CONFIG_FUNCTION_SUPPORT_KEY
#define egui_input_info (core->input)
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
#define input_motion_pool              (egui_input_info.motion_pool)
#define input_motion_pool_fifo_storage (egui_input_info.motion_pool_fifo_storage)
#define input_motion_pool_data_storage (egui_input_info.motion_pool_data_storage)

#include "egui_touch_driver.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
static uint8_t egui_input_touch_point_count_normalize(uint8_t point_count)
{
    if (point_count > EGUI_TOUCH_DRIVER_MAX_POINTS)
    {
        point_count = EGUI_TOUCH_DRIVER_MAX_POINTS;
    }
    return point_count;
}

static uint8_t egui_input_touch_ids_are_unique(const uint8_t *ids, uint8_t count)
{
    if (ids == NULL)
    {
        return 0;
    }

    for (uint8_t i = 0; i < count; i++)
    {
        for (uint8_t j = (uint8_t)(i + 1U); j < count; j++)
        {
            if (ids[i] == ids[j])
            {
                return 0;
            }
        }
    }
    return 1;
}

static uint8_t egui_input_touch_ids_are_compact_index_order(const uint8_t *ids, uint8_t count)
{
    if (ids == NULL)
    {
        return 0;
    }

    for (uint8_t i = 0; i < count; i++)
    {
        if (ids[i] != i)
        {
            return 0;
        }
    }
    return 1;
}

static uint8_t egui_input_touch_should_match_by_id(const uint8_t *prev_ids, uint8_t prev_count, const uint8_t *current_ids, uint8_t current_count)
{
    if (!egui_input_touch_ids_are_unique(prev_ids, prev_count) || !egui_input_touch_ids_are_unique(current_ids, current_count))
    {
        return 0;
    }

    if (egui_input_touch_ids_are_compact_index_order(prev_ids, prev_count) && egui_input_touch_ids_are_compact_index_order(current_ids, current_count))
    {
        return 0;
    }

    return 1;
}

static uint8_t egui_input_touch_find_active_prev_index(const uint8_t *active_prev_indices, uint8_t active_count, uint8_t prev_index)
{
    for (uint8_t i = 0; i < active_count; i++)
    {
        if (active_prev_indices[i] == prev_index)
        {
            return i;
        }
    }
    return EGUI_TOUCH_DRIVER_MAX_POINTS;
}

static void egui_input_touch_remove_active_point(egui_location_t *active_locations, uint8_t *active_ids, uint8_t *active_prev_indices, uint8_t *active_count,
                                                 uint8_t active_index)
{
    if (active_locations == NULL || active_ids == NULL || active_prev_indices == NULL || active_count == NULL || active_index >= *active_count)
    {
        return;
    }

    for (uint8_t i = active_index; i + 1U < *active_count; i++)
    {
        active_locations[i] = active_locations[i + 1U];
        active_ids[i] = active_ids[i + 1U];
        active_prev_indices[i] = active_prev_indices[i + 1U];
    }
    (*active_count)--;
}

static void egui_input_touch_make_pointer_up_locations(const egui_location_t *active_locations, uint8_t active_count, uint8_t release_index,
                                                       egui_location_t *event_locations)
{
    uint8_t out_index = 0;

    if (active_locations == NULL || event_locations == NULL || release_index >= active_count)
    {
        return;
    }

    for (uint8_t i = 0; i < active_count; i++)
    {
        if (i == release_index)
        {
            continue;
        }
        event_locations[out_index++] = active_locations[i];
    }
    event_locations[active_count - 1U] = active_locations[release_index];
}

static uint8_t egui_input_touch_locations_changed_from_prev(egui_core_t *core, const egui_location_t *active_locations, uint8_t active_count)
{
    if (core == NULL || active_locations == NULL)
    {
        return 0;
    }

    for (uint8_t i = 0; i < active_count; i++)
    {
        if (i >= core->touch.prev_point_count || active_locations[i].x != core->touch.prev_locations[i].x ||
            active_locations[i].y != core->touch.prev_locations[i].y)
        {
            return 1;
        }
    }
    return 0;
}

static void egui_input_motion_set_locations(egui_motion_event_t *motion_event, uint8_t pointer_count, const egui_location_t *locations, egui_dim_t fallback_x,
                                            egui_dim_t fallback_y)
{
    pointer_count = egui_input_touch_point_count_normalize(pointer_count);

    motion_event->location.x = fallback_x;
    motion_event->location.y = fallback_y;
    motion_event->pointer_count = pointer_count;
    motion_event->scroll_delta = 0;

    for (uint8_t i = 0; i < EGUI_TOUCH_DRIVER_MAX_POINTS; i++)
    {
        motion_event->locations[i].x = 0;
        motion_event->locations[i].y = 0;
    }

    if (locations == NULL || pointer_count == 0)
    {
        return;
    }

    for (uint8_t i = 0; i < pointer_count; i++)
    {
        motion_event->locations[i] = locations[i];
    }

    motion_event->location = motion_event->locations[0];
}

int egui_input_add_motion_points(egui_core_t *core, uint8_t type, uint8_t pointer_count, const egui_location_t *locations)
{
    egui_location_t fallback = {0, 0};

    pointer_count = egui_input_touch_point_count_normalize(pointer_count);
    if (locations != NULL && pointer_count > 0)
    {
        fallback = locations[0];
    }

    __egui_disable_isr();
    egui_motion_event_t *motion_event = NULL;
    egui_motion_event_t *last_motion_event;
    int is_reused = 0;

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
    motion_event->timestamp = egui_api_timer_get_current_core(core);
    egui_input_motion_set_locations(motion_event, pointer_count, locations, fallback.x, fallback.y);

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
#endif

/** Queue one single-pointer motion event, coalescing consecutive MOVE events to save queue slots. */
int egui_input_add_motion(egui_core_t *core, uint8_t type, egui_dim_t x, egui_dim_t y)
{
    // EGUI_LOG_DBG("egui_input_add_motion type:%d x:%d y:%d\n", type, x, y);
    __egui_disable_isr();

    egui_motion_event_t *motion_event = NULL;
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
    egui_location_t location = {x, y};
    egui_input_motion_set_locations(motion_event, 1, &location, x, y);
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
    egui_input_motion_set_locations(motion_event, 0, NULL, x, y);
    motion_event->scroll_delta = delta;

    egui_slist_append(&egui_input_info.motion_list, &motion_event->node);
    __egui_enable_isr();
    return 1;
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
static void egui_input_queue_touch_sample(egui_core_t *core, const egui_touch_driver_data_t *data)
{
    uint8_t point_count;
    uint8_t prev_count;
    egui_location_t current_locations[EGUI_TOUCH_DRIVER_MAX_POINTS];
    egui_location_t active_locations[EGUI_TOUCH_DRIVER_MAX_POINTS];
    egui_location_t event_locations[EGUI_TOUCH_DRIVER_MAX_POINTS];
    egui_location_t last_release_location = {0, 0};
    uint8_t current_ids[EGUI_TOUCH_DRIVER_MAX_POINTS];
    uint8_t active_ids[EGUI_TOUCH_DRIVER_MAX_POINTS];
    uint8_t active_prev_indices[EGUI_TOUCH_DRIVER_MAX_POINTS];
    uint8_t prev_to_current[EGUI_TOUCH_DRIVER_MAX_POINTS];
    uint8_t current_used[EGUI_TOUCH_DRIVER_MAX_POINTS];
    uint8_t active_count;
    uint8_t use_id_match;
    uint8_t emitted_pointer_down = 0;
    uint8_t emitted_release = 0;

    if (data == NULL)
    {
        return;
    }

    point_count = egui_input_touch_point_count_normalize(data->point_count);
    prev_count = core->touch.prev_point_count;

    for (uint8_t i = 0; i < EGUI_TOUCH_DRIVER_MAX_POINTS; i++)
    {
        if (i < point_count)
        {
            current_locations[i].x = data->points[i].x;
            current_locations[i].y = data->points[i].y;
            current_ids[i] = data->points[i].id;
        }
        else
        {
            current_locations[i].x = 0;
            current_locations[i].y = 0;
            current_ids[i] = i;
        }
        prev_to_current[i] = EGUI_TOUCH_DRIVER_MAX_POINTS;
        current_used[i] = 0;
    }

    use_id_match = egui_input_touch_should_match_by_id(core->touch.prev_ids, prev_count, current_ids, point_count);

    for (uint8_t prev_index = 0; prev_index < prev_count; prev_index++)
    {
        for (uint8_t current_index = 0; current_index < point_count; current_index++)
        {
            if (current_used[current_index])
            {
                continue;
            }

            if ((use_id_match && core->touch.prev_ids[prev_index] == current_ids[current_index]) || (!use_id_match && prev_index == current_index))
            {
                prev_to_current[prev_index] = current_index;
                current_used[current_index] = 1;
                break;
            }
        }
    }

    active_count = prev_count;
    for (uint8_t i = 0; i < active_count; i++)
    {
        active_locations[i] = core->touch.prev_locations[i];
        active_ids[i] = core->touch.prev_ids[i];
        active_prev_indices[i] = i;
    }

    for (uint8_t prev_index = 0; prev_index < prev_count; prev_index++)
    {
        uint8_t current_index = prev_to_current[prev_index];
        uint8_t active_index;

        if (current_index >= EGUI_TOUCH_DRIVER_MAX_POINTS)
        {
            continue;
        }

        active_index = egui_input_touch_find_active_prev_index(active_prev_indices, active_count, prev_index);
        if (active_index < EGUI_TOUCH_DRIVER_MAX_POINTS)
        {
            active_locations[active_index] = current_locations[current_index];
            active_ids[active_index] = current_ids[current_index];
        }
    }

    for (uint8_t prev_offset = 0; prev_offset < prev_count; prev_offset++)
    {
        uint8_t prev_index = (uint8_t)(prev_count - 1U - prev_offset);
        uint8_t active_index;

        if (prev_to_current[prev_index] < EGUI_TOUCH_DRIVER_MAX_POINTS)
        {
            continue;
        }

        active_index = egui_input_touch_find_active_prev_index(active_prev_indices, active_count, prev_index);
        if (active_index >= EGUI_TOUCH_DRIVER_MAX_POINTS)
        {
            continue;
        }

        last_release_location = active_locations[active_index];
        if (active_count > 1)
        {
            egui_input_touch_make_pointer_up_locations(active_locations, active_count, active_index, event_locations);
            egui_input_add_motion_points(core, EGUI_MOTION_EVENT_ACTION_POINTER_UP, active_count, event_locations);
        }
        else
        {
            egui_input_add_motion_points(core, EGUI_MOTION_EVENT_ACTION_UP, 1, active_locations);
        }
        emitted_release = 1;
        egui_input_touch_remove_active_point(active_locations, active_ids, active_prev_indices, &active_count, active_index);
    }

    for (uint8_t current_index = 0; current_index < point_count; current_index++)
    {
        if (current_used[current_index])
        {
            continue;
        }

        if (active_count >= EGUI_TOUCH_DRIVER_MAX_POINTS)
        {
            continue;
        }

        active_locations[active_count] = current_locations[current_index];
        active_ids[active_count] = current_ids[current_index];
        active_prev_indices[active_count] = EGUI_TOUCH_DRIVER_MAX_POINTS;
        active_count++;

        if (active_count == 1)
        {
            egui_input_add_motion_points(core, EGUI_MOTION_EVENT_ACTION_DOWN, 1, active_locations);
        }
        else
        {
            egui_input_add_motion_points(core, EGUI_MOTION_EVENT_ACTION_POINTER_DOWN, active_count, active_locations);
        }
        emitted_pointer_down = 1;
    }

    if (!emitted_pointer_down && active_count > 0 && egui_input_touch_locations_changed_from_prev(core, active_locations, active_count))
    {
        egui_input_add_motion_points(core, EGUI_MOTION_EVENT_ACTION_MOVE, active_count, active_locations);
    }

    core->touch.prev_point_count = active_count;
    core->touch.prev_pressed = active_count > 0;

    for (uint8_t i = 0; i < EGUI_TOUCH_DRIVER_MAX_POINTS; i++)
    {
        if (i < active_count)
        {
            core->touch.prev_locations[i] = active_locations[i];
            core->touch.prev_ids[i] = active_ids[i];
        }
        else
        {
            core->touch.prev_locations[i].x = 0;
            core->touch.prev_locations[i].y = 0;
            core->touch.prev_ids[i] = i;
        }
    }

    if (active_count > 0)
    {
        core->touch.prev_x = active_locations[0].x;
        core->touch.prev_y = active_locations[0].y;
    }
    else if (emitted_release)
    {
        core->touch.prev_x = last_release_location.x;
        core->touch.prev_y = last_release_location.y;
    }
}
#else
static void egui_input_queue_touch_sample(egui_core_t *core, const egui_touch_driver_data_t *data)
{
    uint8_t pressed;
    int16_t tx;
    int16_t ty;

    if (data == NULL)
    {
        return;
    }

    pressed = data->point_count > 0;
    tx = pressed ? data->points[0].x : core->touch.prev_x;
    ty = pressed ? data->points[0].y : core->touch.prev_y;

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
        egui_input_add_motion(core, EGUI_MOTION_EVENT_ACTION_UP, core->touch.prev_x, core->touch.prev_y);
    }

    core->touch.prev_pressed = pressed;
    core->touch.prev_point_count = pressed ? 1 : 0;
    core->touch.prev_ids[0] = 0;
    if (pressed)
    {
        core->touch.prev_x = tx;
        core->touch.prev_y = ty;
        core->touch.prev_locations[0].x = tx;
        core->touch.prev_locations[0].y = ty;
    }
    else
    {
        core->touch.prev_locations[0].x = 0;
        core->touch.prev_locations[0].y = 0;
    }
}
#endif

/** Return the last measured horizontal fling velocity, or zero when tracking is disabled. */
egui_float_t egui_input_get_velocity_x(egui_core_t *core)
{
    EGUI_UNUSED(core);
#if !EGUI_CONFIG_FUNCTION_INPUT_VELOCITY_TRACKER
    return 0;
#else
    return egui_input_info.velocity_tracker.velocity_x;
#endif
}

/** Return the last measured vertical fling velocity, or zero when tracking is disabled. */
egui_float_t egui_input_get_velocity_y(egui_core_t *core)
{
    EGUI_UNUSED(core);
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
    if (tdrv != NULL && tdrv->ops != NULL && tdrv->ops->read != NULL)
    {
        egui_touch_driver_data_t touch_data;
        egui_api_memset(&touch_data, 0, (int)sizeof(touch_data));
        if (tdrv->ops->read(core, &touch_data) == 0)
        {
            egui_input_queue_touch_sample(core, &touch_data);
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
#if EGUI_CONFIG_FUNCTION_SOFTWARE_ROTATION_ENABLE
        if (core->render.software_rotation)
        {
            egui_display_driver_t *drv = egui_display_driver_get(core);
            if (drv != NULL && drv->rotation != EGUI_DISPLAY_ROTATION_0 && drv->ops->set_rotation == NULL)
            {
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
                if (motion_event->pointer_count > 0)
                {
                    for (uint8_t i = 0; i < motion_event->pointer_count; i++)
                    {
                        egui_rotation_transform_touch(drv->rotation, drv->physical_width, drv->physical_height, &motion_event->locations[i].x,
                                                      &motion_event->locations[i].y);
                    }
                    motion_event->location = motion_event->locations[0];
                }
                else
                {
                    egui_rotation_transform_touch(drv->rotation, drv->physical_width, drv->physical_height, &motion_event->location.x,
                                                  &motion_event->location.y);
                }
#else
                egui_rotation_transform_touch(drv->rotation, drv->physical_width, drv->physical_height, &motion_event->location.x, &motion_event->location.y);
#endif
            }
        }
#endif

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
    core->touch.prev_point_count = 0;
    for (uint8_t i = 0; i < EGUI_TOUCH_DRIVER_MAX_POINTS; i++)
    {
        core->touch.prev_locations[i].x = 0;
        core->touch.prev_locations[i].y = 0;
        core->touch.prev_ids[i] = i;
    }

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

#if !EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

/** No-op stub used when touch support is compiled out. */
int egui_input_add_motion(egui_core_t *core, uint8_t type, egui_dim_t x, egui_dim_t y)
{
    EGUI_UNUSED(core);
    EGUI_UNUSED(type);
    EGUI_UNUSED(x);
    EGUI_UNUSED(y);
    return 0;
}

/** No-op stub used when touch support is compiled out. */
egui_float_t egui_input_get_velocity_x(egui_core_t *core)
{
    EGUI_UNUSED(core);
    return 0;
}

/** No-op stub used when touch support is compiled out. */
egui_float_t egui_input_get_velocity_y(egui_core_t *core)
{
    EGUI_UNUSED(core);
    return 0;
}

/** No-op stub used when touch support is compiled out. */
int egui_input_check_idle(egui_core_t *core)
{
    EGUI_UNUSED(core);
    return 1;
}

/** No-op stub used when touch support is compiled out. */
void egui_input_polling_work(egui_core_t *core)
{
    EGUI_UNUSED(core);
}

/** No-op stub used when touch support is compiled out. */
void egui_input_init(egui_core_t *core)
{
    EGUI_UNUSED(core);
}

#endif // !EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

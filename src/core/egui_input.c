#include <stdio.h>
#include <assert.h>

#include "egui_input.h"
#include "egui_motion_event.h"
#include "egui_api.h"

#include "egui_core.h"

#include "utils/simple_ringbuffer/simple_pool.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
egui_input_t egui_input_info;
SIMPLE_POOL_DEFINE(input_motion_pool, EGUI_CONFIG_INPUT_MOTION_CACHE_COUNT, sizeof(egui_motion_event_t));

int egui_input_add_motion(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    // EGUI_LOG_DBG("egui_input_add_motion type:%d x:%d y:%d\n", type, x, y);
    egui_motion_event_t *motion_event;
    // Merge move events with the same type and location
    egui_motion_event_t *last_motion_event;
    int is_reused = 0;

    last_motion_event  = (egui_motion_event_t *)egui_slist_peek_tail(&egui_input_info.motion_list);
    if(last_motion_event != NULL)
    {
        if(last_motion_event->type == type && last_motion_event->type == EGUI_MOTION_EVENT_ACTION_MOVE)
        {
            // reuse the last motion event
            motion_event = last_motion_event;
            is_reused = 1;
        }
    }
    if(!is_reused)
    {
        // Get a motion event from the pool
        if (!SIMPLE_POOL_DEQUEUE(&input_motion_pool, motion_event))
        {
            EGUI_LOG_DBG("egui_input_add_motion failed type:%d x:%d y:%d\n", type, x, y);
            return 0;
        }
    }
    // Save info and append to motion list
    motion_event->type = type;
    motion_event->location.x = x;
    motion_event->location.y = y;
    motion_event->timestamp = egui_api_timer_get_current();

    egui_velocity_tracker_add_motion(&egui_input_info.velocity_tracker, motion_event);
    if(!is_reused)
    {
        egui_slist_append(&egui_input_info.motion_list, &motion_event->node);
    }
    return 1;
}

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

    while ((motion_event = (egui_motion_event_t *)egui_slist_get(&egui_input_info.motion_list)) != NULL)
    {
        // EGUI_LOG_DBG("egui_input_polling_work type:%d x:%d y:%d\n", motion_event->type, motion_event->location.x, motion_event->location.y);
        // handle motion event
        egui_core_process_input_motion(motion_event);

        // Put the motion event back to the pool
        SIMPLE_POOL_ENQUEUE(&input_motion_pool, motion_event);
    }
}

void egui_input_init(void)
{
    SIMPLE_POOL_INIT(input_motion_pool, EGUI_CONFIG_INPUT_MOTION_CACHE_COUNT, sizeof(egui_motion_event_t));

    egui_slist_init(&egui_input_info.motion_list);
    egui_velocity_tracker_init(&egui_input_info.velocity_tracker);
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

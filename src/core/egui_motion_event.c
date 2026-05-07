#include <stdio.h>
#include <assert.h>

#include "egui_motion_event.h"
#include "egui_api.h"

/**
 * @file egui_motion_event.c
 * @brief Small debug helpers for motion-event inspection.
 */

#define DEFINE_STR(R) #R

/** Convert one motion-event type to a readable debug string. */
const char *egui_motion_event_string(uint8_t type)
{
    const char *str;
    switch (type)
    {
    case EGUI_MOTION_EVENT_NONE:
        str = DEFINE_STR(EGUI_MOTION_EVENT_NONE);
        break;
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        str = DEFINE_STR(EGUI_MOTION_EVENT_ACTION_DOWN);
        break;
    case EGUI_MOTION_EVENT_ACTION_UP:
        str = DEFINE_STR(EGUI_MOTION_EVENT_ACTION_UP);
        break;
    case EGUI_MOTION_EVENT_ACTION_MOVE:
        str = DEFINE_STR(EGUI_MOTION_EVENT_ACTION_MOVE);
        break;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        str = DEFINE_STR(EGUI_MOTION_EVENT_ACTION_CANCEL);
        break;
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    case EGUI_MOTION_EVENT_ACTION_POINTER_DOWN:
        str = DEFINE_STR(EGUI_MOTION_EVENT_ACTION_POINTER_DOWN);
        break;
    case EGUI_MOTION_EVENT_ACTION_POINTER_UP:
        str = DEFINE_STR(EGUI_MOTION_EVENT_ACTION_POINTER_UP);
        break;
    case EGUI_MOTION_EVENT_ACTION_SCROLL:
        str = DEFINE_STR(EGUI_MOTION_EVENT_ACTION_SCROLL);
        break;
#endif

    default:
        str = "unknown";
        break;
    };

    return str;
}

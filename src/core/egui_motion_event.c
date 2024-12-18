#include <stdio.h>
#include <assert.h>

#include "egui_motion_event.h"
#include "egui_api.h"

#define DEFINE_STR(R) #R

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

    default:
        str = "unknown";
        break;
    };

    return str;
}

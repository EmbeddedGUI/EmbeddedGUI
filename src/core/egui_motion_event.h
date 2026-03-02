#ifndef _EGUI_MOTION_EVENT_H_
#define _EGUI_MOTION_EVENT_H_

#include "egui_common.h"
#include "egui_region.h"
#include "utils/egui_slist.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

enum egui_motion_event_type
{
    EGUI_MOTION_EVENT_NONE = 0,
    EGUI_MOTION_EVENT_ACTION_DOWN,
    EGUI_MOTION_EVENT_ACTION_UP,
    EGUI_MOTION_EVENT_ACTION_MOVE,
    EGUI_MOTION_EVENT_ACTION_CANCEL,
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    EGUI_MOTION_EVENT_ACTION_POINTER_DOWN, // 2nd pointer down while 1st still down
    EGUI_MOTION_EVENT_ACTION_POINTER_UP,   // 2nd pointer up while 1st still down
    EGUI_MOTION_EVENT_ACTION_SCROLL,       // scroll wheel event
#endif
};

typedef struct egui_motion_event egui_motion_event_t;
struct egui_motion_event
{
    egui_snode_t node; // the node that the event is related to

    uint8_t type;             // type of the event, enum egui_motion_event_type
    uint32_t timestamp;       // timestamp of the event in milliseconds
    egui_location_t location; // the position in screen coordinates
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    uint8_t pointer_count;     // number of active pointers (1 or 2)
    egui_location_t location2; // 2nd pointer position (valid when pointer_count >= 2)
    int16_t scroll_delta;      // scroll wheel delta (only for SCROLL events)
#endif
};

const char *egui_motion_event_string(uint8_t type);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_MOTION_EVENT_H_ */

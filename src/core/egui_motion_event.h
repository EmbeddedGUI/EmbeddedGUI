#ifndef _EGUI_MOTION_EVENT_H_
#define _EGUI_MOTION_EVENT_H_

/**
 * @file egui_motion_event.h
 * @brief Pointer-style input events queued by the core input subsystem.
 */

#include "egui_common.h"
#include "egui_region.h"
#include "utils/egui_slist.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Motion-event types emitted by touch, multi-touch, and wheel-style inputs. */
enum egui_motion_event_type
{
    EGUI_MOTION_EVENT_NONE = 0,      // unused/default event slot
    EGUI_MOTION_EVENT_ACTION_DOWN,   // first pointer became active
    EGUI_MOTION_EVENT_ACTION_UP,     // final active pointer was released
    EGUI_MOTION_EVENT_ACTION_MOVE,   // active pointer position changed
    EGUI_MOTION_EVENT_ACTION_CANCEL, // current gesture was aborted
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    EGUI_MOTION_EVENT_ACTION_POINTER_DOWN, // an additional pointer became active while the first is still down
    EGUI_MOTION_EVENT_ACTION_POINTER_UP,   // an additional pointer was released while the first is still down
    EGUI_MOTION_EVENT_ACTION_SCROLL,       // wheel-like event carrying a signed scroll delta
#endif
};

typedef struct egui_motion_event egui_motion_event_t;
/** One queued motion event stored in the per-core motion FIFO. */
struct egui_motion_event
{
    egui_snode_t node; // intrusive list node used by the core's queued motion list

    uint8_t type;             // enum egui_motion_event_type
    uint32_t timestamp;       // event timestamp in milliseconds
    egui_location_t location; // primary pointer position in screen coordinates
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    uint8_t pointer_count;                                   // number of valid touch points described by this event
    egui_location_t locations[EGUI_CONFIG_TOUCH_MAX_POINTS]; // all active/relevant pointer positions
    int16_t scroll_delta;                                    // signed wheel delta used only by `ACTION_SCROLL`
#endif
};

/** Convert one motion-event type to a short debug string for logs and assertions. */
const char *egui_motion_event_string(uint8_t type);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_MOTION_EVENT_H_ */

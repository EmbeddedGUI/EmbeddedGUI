#ifndef _EGUI_INPUT_SIMULATOR_H_
#define _EGUI_INPUT_SIMULATOR_H_

/**
 * @file egui_input_simulator.h
 * @brief Input simulation helpers for GIF recording
 *
 * Provides helper functions and types for simulating user interactions
 * during GIF recording. Used with EGUI_CONFIG_RECORDING_TEST macro.
 */

#include "egui_config.h"
#include "egui_region.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Simulation action types
 */
typedef enum
{
    EGUI_SIM_ACTION_NONE = 0, // No action / end marker
    EGUI_SIM_ACTION_CLICK,    // Single click at position
    EGUI_SIM_ACTION_DRAG,     // Drag from one point to another
    EGUI_SIM_ACTION_SWIPE,    // Fast swipe gesture
    EGUI_SIM_ACTION_WAIT,     // Wait without action
} egui_sim_action_type_t;

/**
 * @brief Simulation action description
 */
typedef struct egui_sim_action
{
    egui_sim_action_type_t type;
    int x1, y1;      // Start position (or click position)
    int x2, y2;      // End position (for drag/swipe)
    int steps;       // Number of steps for drag (0 = auto)
    int interval_ms; // Time before this action (ms)
} egui_sim_action_t;

/**
 * @brief Helper macro to define a click action
 * @param _x Click x position
 * @param _y Click y position
 * @param _interval_ms Interval before click (ms)
 */
#define EGUI_SIM_CLICK(_x, _y, _interval_ms) {EGUI_SIM_ACTION_CLICK, (_x), (_y), 0, 0, 0, (_interval_ms)}

/**
 * @brief Helper macro to define a drag action
 * @param _x1, _y1 Start position
 * @param _x2, _y2 End position
 * @param _steps Number of intermediate steps (0 = auto calculate)
 * @param _interval_ms Interval before drag starts (ms)
 */
#define EGUI_SIM_DRAG(_x1, _y1, _x2, _y2, _steps, _interval_ms) {EGUI_SIM_ACTION_DRAG, (_x1), (_y1), (_x2), (_y2), (_steps), (_interval_ms)}

/**
 * @brief Helper macro to define a swipe action (fast drag)
 * @param _x1, _y1 Start position
 * @param _x2, _y2 End position
 * @param _interval_ms Interval before swipe (ms)
 */
#define EGUI_SIM_SWIPE(_x1, _y1, _x2, _y2, _interval_ms) {EGUI_SIM_ACTION_SWIPE, (_x1), (_y1), (_x2), (_y2), 5, (_interval_ms)}

/**
 * @brief Helper macro to define a wait action
 * @param _interval_ms Wait duration (ms)
 */
#define EGUI_SIM_WAIT(_interval_ms) {EGUI_SIM_ACTION_WAIT, 0, 0, 0, 0, 0, (_interval_ms)}

/**
 * @brief End marker for action array
 */
#define EGUI_SIM_END() {EGUI_SIM_ACTION_NONE, 0, 0, 0, 0, 0, 0}

/**
 * @brief Get center position of a view
 * @param view Pointer to egui_view_t (or derived type, will be cast)
 * @param p_x Pointer to store center x
 * @param p_y Pointer to store center y
 */
__EGUI_STATIC_INLINE__ void egui_sim_get_view_center(void *view, int *p_x, int *p_y)
{
    egui_view_t *v = (egui_view_t *)view;
    *p_x = v->region_screen.location.x + v->region_screen.size.width / 2;
    *p_y = v->region_screen.location.y + v->region_screen.size.height / 2;
}

/**
 * @brief Get position within a view (relative to view's top-left)
 * @param view Pointer to egui_view_t
 * @param rel_x Relative x (0.0 = left, 0.5 = center, 1.0 = right)
 * @param rel_y Relative y (0.0 = top, 0.5 = center, 1.0 = bottom)
 * @param p_x Pointer to store absolute x
 * @param p_y Pointer to store absolute y
 */
__EGUI_STATIC_INLINE__ void egui_sim_get_view_pos(void *view, float rel_x, float rel_y, int *p_x, int *p_y)
{
    egui_view_t *v = (egui_view_t *)view;
    *p_x = v->region_screen.location.x + (int)(v->region_screen.size.width * rel_x);
    *p_y = v->region_screen.location.y + (int)(v->region_screen.size.height * rel_y);
}

/**
 * @brief Get the visible intersection region of a view under an optional clip region.
 * @param view Pointer to egui_view_t
 * @param clip_region Optional clip region in screen coordinates, NULL to use full view region
 * @param p_visible_region Pointer to store the visible region
 * @return 1 if the view has a non-empty visible region, 0 otherwise
 */
__EGUI_STATIC_INLINE__ int egui_sim_get_view_visible_region(void *view, const egui_region_t *clip_region, egui_region_t *p_visible_region)
{
    egui_view_t *v = (egui_view_t *)view;

    if (p_visible_region == NULL)
    {
        return 0;
    }
    egui_region_init_empty(p_visible_region);
    if (v == NULL || v->region_screen.size.width <= 0 || v->region_screen.size.height <= 0)
    {
        return 0;
    }

    *p_visible_region = v->region_screen;
    if (clip_region != NULL)
    {
        egui_region_intersect(p_visible_region, clip_region, p_visible_region);
    }
    return !egui_region_is_empty(p_visible_region);
}

/**
 * @brief Get the center position of a view after clipping to an optional region.
 * @param view Pointer to egui_view_t
 * @param clip_region Optional clip region in screen coordinates
 * @param p_x Pointer to store center x
 * @param p_y Pointer to store center y
 * @return 1 if a visible clipped center could be resolved, 0 otherwise
 */
__EGUI_STATIC_INLINE__ int egui_sim_get_view_clipped_center(void *view, const egui_region_t *clip_region, int *p_x, int *p_y)
{
    egui_region_t visible_region;

    if (p_x == NULL || p_y == NULL || !egui_sim_get_view_visible_region(view, clip_region, &visible_region))
    {
        return 0;
    }

    *p_x = visible_region.location.x + visible_region.size.width / 2;
    *p_y = visible_region.location.y + visible_region.size.height / 2;
    return 1;
}

/**
 * @brief Set a click action using the clipped visible center of a view.
 * @param p_action Pointer to egui_sim_action_t
 * @param view Pointer to egui_view_t
 * @param clip_region Optional clip region in screen coordinates
 * @param interval_ms Interval before click
 * @return 1 if the action was set, 0 otherwise
 */
__EGUI_STATIC_INLINE__ int egui_sim_set_click_view_clipped(egui_sim_action_t *p_action, void *view, const egui_region_t *clip_region, int interval_ms)
{
    if (p_action == NULL || !egui_sim_get_view_clipped_center(view, clip_region, &p_action->x1, &p_action->y1))
    {
        return 0;
    }

    p_action->type = EGUI_SIM_ACTION_CLICK;
    p_action->interval_ms = interval_ms;
    return 1;
}

/**
 * @brief Macro to get view center coordinates as expressions
 * Can be used directly in macro arguments
 */
#define EGUI_SIM_VIEW_CENTER_X(_view) (((egui_view_t *)(_view))->region_screen.location.x + ((egui_view_t *)(_view))->region_screen.size.width / 2)

#define EGUI_SIM_VIEW_CENTER_Y(_view) (((egui_view_t *)(_view))->region_screen.location.y + ((egui_view_t *)(_view))->region_screen.size.height / 2)

/**
 * @brief Set action to click at view center
 * @param _p_action Pointer to egui_sim_action_t
 * @param _view Pointer to view
 * @param _interval_ms Interval before click (ms)
 */
#define EGUI_SIM_SET_CLICK_VIEW(_p_action, _view, _interval_ms)                                                                                                \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        (_p_action)->type = EGUI_SIM_ACTION_CLICK;                                                                                                             \
        (_p_action)->x1 = EGUI_SIM_VIEW_CENTER_X(_view);                                                                                                       \
        (_p_action)->y1 = EGUI_SIM_VIEW_CENTER_Y(_view);                                                                                                       \
        (_p_action)->interval_ms = (_interval_ms);                                                                                                             \
    } while (0)

/**
 * @brief Set action to drag from one view to another
 * @param _p_action Pointer to egui_sim_action_t
 * @param _view_from Start view
 * @param _view_to End view
 * @param _steps Number of steps
 * @param _interval_ms Interval before drag (ms)
 */
#define EGUI_SIM_SET_DRAG_VIEW(_p_action, _view_from, _view_to, _steps, _interval_ms)                                                                          \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        (_p_action)->type = EGUI_SIM_ACTION_DRAG;                                                                                                              \
        (_p_action)->x1 = EGUI_SIM_VIEW_CENTER_X(_view_from);                                                                                                  \
        (_p_action)->y1 = EGUI_SIM_VIEW_CENTER_Y(_view_from);                                                                                                  \
        (_p_action)->x2 = EGUI_SIM_VIEW_CENTER_X(_view_to);                                                                                                    \
        (_p_action)->y2 = EGUI_SIM_VIEW_CENTER_Y(_view_to);                                                                                                    \
        (_p_action)->steps = (_steps);                                                                                                                         \
        (_p_action)->interval_ms = (_interval_ms);                                                                                                             \
    } while (0)

/**
 * @brief Set action to swipe from one view to another
 */
#define EGUI_SIM_SET_SWIPE_VIEW(_p_action, _view_from, _view_to, _interval_ms)                                                                                 \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        (_p_action)->type = EGUI_SIM_ACTION_SWIPE;                                                                                                             \
        (_p_action)->x1 = EGUI_SIM_VIEW_CENTER_X(_view_from);                                                                                                  \
        (_p_action)->y1 = EGUI_SIM_VIEW_CENTER_Y(_view_from);                                                                                                  \
        (_p_action)->x2 = EGUI_SIM_VIEW_CENTER_X(_view_to);                                                                                                    \
        (_p_action)->y2 = EGUI_SIM_VIEW_CENTER_Y(_view_to);                                                                                                    \
        (_p_action)->steps = 5;                                                                                                                                \
        (_p_action)->interval_ms = (_interval_ms);                                                                                                             \
    } while (0)

/**
 * @brief Request a screenshot on the next rendered frame.
 * Call this in egui_port_get_recording_action() to precisely capture a frame.
 * If not called, the framework auto-captures after each action completes (fallback).
 * Implemented in porting layer (e.g., sdl_port.c).
 */
extern void recording_request_snapshot(void);

/**
 * @brief Set action to wait
 */
#define EGUI_SIM_SET_WAIT(_p_action, _interval_ms)                                                                                                             \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        (_p_action)->type = EGUI_SIM_ACTION_WAIT;                                                                                                              \
        (_p_action)->interval_ms = (_interval_ms);                                                                                                             \
    } while (0)

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_INPUT_SIMULATOR_H_ */
